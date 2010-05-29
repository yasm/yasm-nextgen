//
// Optimizer implementation.
//
//  Copyright (C) 2001-2007  Peter Johnson
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#define DEBUG_TYPE "Optimize"

#include "yasmx/Object.h"

#include "util.h"

#include <algorithm>
#include <deque>
#include <list>
#include <memory>
#include <vector>

#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "YAML/emitter.h"
#include "yasmx/Config/functional.h"
#include "yasmx/Support/IntervalTree.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Diagnostic.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/Location_util.h"
#include "yasmx/Section.h"
#include "yasmx/Value.h"


STATISTIC(num_span_terms, "Number of span terms created");
STATISTIC(num_spans, "Number of spans created");
STATISTIC(num_step1d, "Number of spans after step 1b");
STATISTIC(num_itree, "Number of span terms added to interval tree");
STATISTIC(num_offset_setters, "Number of offset setters");
STATISTIC(num_recalc, "Number of span recalculations performed");
STATISTIC(num_expansions, "Number of expansions performed");
STATISTIC(num_initial_qb, "Number of spans on initial QB");

using namespace yasm;

//
// Robertson (1977) optimizer
// Based (somewhat loosely) on the algorithm given in:
//   MRC Technical Summary Report # 1779
//   CODE GENERATION FOR SHORT/LONG ADDRESS MACHINES
//   Edward L. Robertson
//   Mathematics Research Center
//   University of Wisconsin-Madison
//   610 Walnut Street
//   Madison, Wisconsin 53706
//   August 1977
//
// Key components of algorithm:
//  - start assuming all short forms
//  - build spans for short->long transition dependencies
//  - if a long form is needed, walk the dependencies and update
// Major differences from Robertson's algorithm:
//  - detection of cycles
//  - any difference of two locations is allowed
//  - handling of alignment/org gaps (offset setting)
//  - handling of multiples
//
// Data structures:
//  - Interval tree to store spans and associated data
//  - Queues QA and QB
//
// Each span keeps track of:
//  - Associated bytecode (bytecode that depends on the span length)
//  - Active/inactive state (starts out active)
//  - Sign (negative/positive; negative being "backwards" in address)
//  - Current length in bytes
//  - New length in bytes
//  - Negative/Positive thresholds
//  - Span ID (unique within each bytecode)
//
// How org and align and any other offset-based bytecodes are handled:
//
// Some portions are critical values that must not depend on any bytecode
// offset (either relative or absolute).
//
// All offset-setters (ORG and ALIGN) are put into a linked list in section
// order (e.g. increasing offset order).  Each span keeps track of the next
// offset-setter following the span's associated bytecode.
//
// When a bytecode is expanded, the next offset-setter is examined.  The
// offset-setter may be able to absorb the expansion (e.g. any offset
// following it would not change), or it may have to move forward (in the
// case of align) or error (in the case of org).  If it has to move forward,
// following offset-setters must also be examined for absorption or moving
// forward.  In either case, the ongoing offset is updated as well as the
// lengths of any spans dependent on the offset-setter.
//
// Alignment/ORG value is critical value.
// Cannot be combined with TIMES.
//
// How times is handled:
//
// TIMES: Handled separately from bytecode "raw" size.  If not span-dependent,
//      trivial (just multiplied in at any bytecode size increase).  Span
//      dependent times update on any change (span ID 0).  If the resultant
//      next bytecode offset would be less than the old next bytecode offset,
//      error.  Otherwise increase offset and update dependent spans.
//
// To reduce interval tree size, a first expansion pass is performed
// before the spans are added to the tree.
//
// Basic algorithm outline:
//
// 1. Initialization:
//  a. Number bytecodes sequentially (via bc_index) and calculate offsets
//     of all bytecodes assuming minimum length, building a list of all
//     dependent spans as we go.
//     "minimum" here means absolute minimum:
//      - align/org (offset-based) bumps offset as normal
//      - times values (with span-dependent values) assumed to be 0
//  b. Iterate over spans.  Set span length based on bytecode offsets
//     determined in 1a.  If span is "certainly" long because the span
//     is an absolute reference to another section (or external) or the
//     distance calculated based on the minimum length is greater than the
//     span's threshold, expand the span's bytecode, and if no further
//     expansion can result, mark span as inactive.
//  c. Iterate over bytecodes to update all bytecode offsets based on new
//     (expanded) lengths calculated in 1b.
//  d. Iterate over active spans.  Add span to interval tree.  Update span's
//     length based on new bytecode offsets determined in 1c.  If span's
//     length exceeds long threshold, add that span to Q.
// 2. Main loop:
//   While Q not empty:
//     Expand BC dependent on span at head of Q (and remove span from Q).
//     Update span:
//       If BC no longer dependent on span, mark span as inactive.
//       If BC has new thresholds for span, update span.
//     If BC increased in size, for each active span that contains BC:
//       Increase span length by difference between short and long BC length.
//       If span exceeds long threshold (or is flagged to recalculate on any
//       change), add it to tail of Q.
// 3. Final pass over bytecodes to generate final offsets.
//
namespace {
class OffsetSetter
{
public:
    OffsetSetter();
    ~OffsetSetter() {}
    void Write(YAML::Emitter& out) const;
    void Dump() const;

    Bytecode* m_bc;
    unsigned long m_cur_val;
    unsigned long m_new_val;
    unsigned long m_thres;
};
} // anonymous namespace

OffsetSetter::OffsetSetter()
    : m_bc(0),
      m_cur_val(0),
      m_new_val(0),
      m_thres(0)
{
}

void
OffsetSetter::Write(YAML::Emitter& out) const
{
    out << YAML::Flow << YAML::BeginMap;
    out << YAML::Key << "bc" << YAML::Value
        << YAML::Alias("BC@" + llvm::Twine::utohexstr((uint64_t)m_bc));
    out << YAML::Key << "curval" << YAML::Value << m_cur_val;
    out << YAML::Key << "newval" << YAML::Value << m_new_val;
    out << YAML::Key << "thres" << YAML::Value << m_thres;
    out << YAML::EndMap;
}

void
OffsetSetter::Dump() const
{
    YAML::Emitter out;
    Write(out);
    llvm::errs() << out.c_str() << '\n';
}

namespace {
class Optimizer;

class Span
{
    friend class Optimizer;
public:
    class Term
    {
    public:
        Term();
        Term(unsigned int subst,
             Location loc,
             Location loc2,
             Span* span,
             long new_val);
        ~Term() {}
        void Write(YAML::Emitter& out) const;
        void Dump() const;

        Location m_loc;
        Location m_loc2;
        Span* m_span;       // span this term is a member of
        long m_cur_val;
        long m_new_val;
        unsigned int m_subst;
    };

    Span(Bytecode& bc,
         int id,
         const Value& value,
         long neg_thres,
         long pos_thres,
         size_t os_index);
    ~Span();

    bool CreateTerms(Optimizer* optimize, Diagnostic& diags);
    bool RecalcNormal();

    void Write(YAML::Emitter& out) const;
    void Dump() const;

private:
    Span(const Span&);                  // not implemented
    const Span& operator=(const Span&); // not implemented

    void AddTerm(unsigned int subst, Location loc, Location loc2);

    Bytecode& m_bc;

    Value m_depval;

    // span terms in absolute portion of value
    typedef std::vector<Term> Terms;
    Terms m_span_terms;
    ExprTerms m_expr_terms;

    long m_cur_val;
    long m_new_val;

    long m_neg_thres;
    long m_pos_thres;

    int m_id;

    enum { INACTIVE = 0, ACTIVE, ON_Q } m_active;

    // Spans that led to this span.  Used only for
    // checking for circular references (cycles) with id=0 spans.
    typedef llvm::SmallPtrSet<Span*, 4> BacktraceSpans;
    BacktraceSpans m_backtrace;

    // Index of first offset setter following this span's bytecode
    size_t m_os_index;
};

class Optimizer
{
public:
    Optimizer();
    ~Optimizer();
    void AddSpan(Bytecode& bc,
                 int id,
                 const Value& value,
                 long neg_thres,
                 long pos_thres);
    void AddOffsetSetter(Bytecode& bc);

    void Step1b(Diagnostic& diags);
    bool Step1d();
    void Step1e(Diagnostic& diags);
    void Step2(Diagnostic& diags);

    void Write(YAML::Emitter& out) const;
    void Dump() const;

private:
    void ITreeAdd(Span& span, Span::Term& term);
    void CheckCycle(IntervalTreeNode<Span::Term*> * node,
                    Span& span,
                    Diagnostic& diags);
    void ExpandTerm(IntervalTreeNode<Span::Term*> * node, long len_diff);

    typedef std::list<Span*> Spans;
    Spans m_spans;      // ownership list

    typedef std::deque<Span*> SpanQueue;
    SpanQueue m_QA, m_QB;

    IntervalTree<Span::Term*> m_itree;
    std::vector<OffsetSetter> m_offset_setters;
};
} // anonymous namespace

Span::Term::Term()
    : m_span(0),
      m_cur_val(0),
      m_new_val(0),
      m_subst(0)
{
}

Span::Term::Term(unsigned int subst,
                 Location loc,
                 Location loc2,
                 Span* span,
                 long new_val)
    : m_loc(loc),
      m_loc2(loc2),
      m_span(span),
      m_cur_val(0),
      m_new_val(new_val),
      m_subst(subst)
{
    ++num_span_terms;
}

void
Span::Term::Write(YAML::Emitter& out) const
{
    out << YAML::Flow << YAML::BeginMap;
    out << YAML::Key << "loc" << YAML::Value << m_loc;
    out << YAML::Key << "loc2" << YAML::Value << m_loc2;
    out << YAML::Key << "curval" << YAML::Value << m_cur_val;
    out << YAML::Key << "newval" << YAML::Value << m_new_val;
    out << YAML::Key << "subst" << YAML::Value << m_subst;
    out << YAML::EndMap;
}

void
Span::Term::Dump() const
{
    YAML::Emitter out;
    Write(out);
    llvm::errs() << out.c_str() << '\n';
}

Span::Span(Bytecode& bc,
           int id,
           /*@null@*/ const Value& value,
           long neg_thres,
           long pos_thres,
           size_t os_index)
    : m_bc(bc),
      m_depval(value),
      m_cur_val(0),
      m_new_val(0),
      m_neg_thres(neg_thres),
      m_pos_thres(pos_thres),
      m_id(id),
      m_active(ACTIVE),
      m_os_index(os_index)
{
    ++num_spans;
}

void
Optimizer::AddSpan(Bytecode& bc,
                   int id,
                   const Value& value,
                   long neg_thres,
                   long pos_thres)
{
    m_spans.push_back(new Span(bc, id, value, neg_thres, pos_thres,
                               m_offset_setters.size()-1));
}

void
Span::AddTerm(unsigned int subst, Location loc, Location loc2)
{
    IntNum intn;
    bool ok = CalcDist(loc, loc2, &intn);
    ok = ok;    // avoid warning due to assert usage
    assert(ok && "could not calculate bc distance");

    if (subst >= m_span_terms.size())
        m_span_terms.resize(subst+1);
    m_span_terms[subst] = Term(subst, loc, loc2, this, intn.getInt());
}

bool
Span::CreateTerms(Optimizer* optimize, Diagnostic& diags)
{
    // Split out sym-sym terms in absolute portion of dependent value
    if (m_depval.hasAbs())
    {
        SubstDist(*m_depval.getAbs(),
                  BIND::bind(&Span::AddTerm, this, _1, _2, _3));
        if (m_span_terms.size() > 0)
        {
            for (Terms::iterator i=m_span_terms.begin(),
                 end=m_span_terms.end(); i != end; ++i)
            {
                // Create expression terms with dummy value
                m_expr_terms.push_back(ExprTerm(0));

                // Check for circular references
                if (m_id <= 0 &&
                    ((m_bc.getIndex() > i->m_loc.bc->getIndex()-1 &&
                      m_bc.getIndex() <= i->m_loc2.bc->getIndex()-1) ||
                     (m_bc.getIndex() > i->m_loc2.bc->getIndex()-1 &&
                      m_bc.getIndex() <= i->m_loc.bc->getIndex()-1)))
                {
                    diags.Report(m_bc.getSource(),
                                 diag::err_optimizer_circular_reference);
                    return false;
                }
            }
        }
    }
    return true;
}

// Recalculate span value based on current span replacement values.
// Returns True if span needs expansion (e.g. exceeded thresholds).
bool
Span::RecalcNormal()
{
    ++num_recalc;
    m_new_val = 0;

    if (m_depval.isRelative())
        m_new_val = LONG_MAX;       // too complex; force to longest form
    else if (m_depval.hasAbs())
    {
        ExprTerm result;

        // Update sym-sym terms and substitute back into expr
        for (Terms::iterator i=m_span_terms.begin(), end=m_span_terms.end();
             i != end; ++i)
            *m_expr_terms[i->m_subst].getIntNum() = i->m_new_val;
        if (!Evaluate(*m_depval.getAbs(), &result, &m_expr_terms[0],
                      m_expr_terms.size(), false, false)
            || !result.isType(ExprTerm::INT))
            m_new_val = LONG_MAX;   // too complex; force to longest form
        else
            m_new_val = result.getIntNum()->getInt();
    }

    if (m_new_val == LONG_MAX)
        m_active = INACTIVE;

    DEBUG(llvm::errs() << "updated SPAN@" << this << " newval to "
          << m_new_val << '\n');

    // If id<=0, flag update on any change
    if (m_id <= 0)
        return (m_new_val != m_cur_val);

    return (m_new_val < m_neg_thres || m_new_val > m_pos_thres);
}

Span::~Span()
{
}

void
Span::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "bc" << YAML::Value
        << YAML::Alias("BC@" + llvm::Twine::utohexstr((uint64_t)(&m_bc)));
    if (!m_depval.hasAbs() || m_depval.isRelative())
        out << YAML::Key << "depval" << YAML::Value << m_depval;
    else
    {
        llvm::SmallString<256> ss;
        llvm::raw_svector_ostream oss(ss);
        oss << *m_depval.getAbs();
        out << YAML::Key << "depval" << YAML::Value << oss.str();
    }

    out << YAML::Key << "span terms" << YAML::Value << YAML::BeginSeq;
    for (Terms::const_iterator i=m_span_terms.begin(), end=m_span_terms.end();
         i != end; ++i)
    {
        i->Write(out);
    }
    out << YAML::EndSeq;

    out << YAML::Key << "curval" << YAML::Value << m_cur_val;
    out << YAML::Key << "newval" << YAML::Value << m_new_val;
    out << YAML::Key << "negthres" << YAML::Value << m_neg_thres;
    out << YAML::Key << "posthres" << YAML::Value << m_pos_thres;
    out << YAML::Key << "id" << YAML::Value << m_id;

    out << YAML::Key << "active" << YAML::Value;
    switch (m_active)
    {
        case INACTIVE: out << "inactive"; break;
        case ACTIVE: out << "active"; break;
        case ON_Q: out << "on queue"; break;
    }

    out << YAML::Key << "backtrace" << YAML::Value;
    out << YAML::Flow << YAML::BeginSeq;
    for (BacktraceSpans::const_iterator i=m_backtrace.begin(),
         end=m_backtrace.end(); i != end; ++i)
    {
        out << YAML::Alias("SPAN@" + llvm::Twine::utohexstr((uint64_t)(*i)));
    }
    out << YAML::EndSeq;

    out << YAML::Key << "offset setter index";
	out << YAML::Value << static_cast<unsigned long>(m_os_index);
    out << YAML::EndMap;
}

void
Span::Dump() const
{
    YAML::Emitter out;
    Write(out);
    llvm::errs() << out.c_str() << '\n';
}

Optimizer::Optimizer()
{
    // Create an placeholder offset setter for spans to point to; this will
    // get updated if/when we actually run into one.
    m_offset_setters.push_back(OffsetSetter());
}

Optimizer::~Optimizer()
{
    while (!m_spans.empty())
    {
        delete m_spans.back();
        m_spans.pop_back();
    }
}

void
Optimizer::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;

    // spans
    out << YAML::Key << "spans" << YAML::Value;
    if (m_spans.empty())
        out << YAML::Flow;
    out << YAML::BeginSeq;
    for (Spans::const_iterator i=m_spans.begin(), end=m_spans.end();
         i != end; ++i)
    {
        out << YAML::Alias("SPAN@" + llvm::Twine::utohexstr((uint64_t)(*i)));
        (*i)->Write(out);
    }
    out << YAML::EndSeq;

    // queue A
    out << YAML::Key << "QA" << YAML::Value << YAML::Flow << YAML::BeginSeq;
    for (SpanQueue::const_iterator j=m_QA.begin(), end=m_QA.end();
         j != end; ++j)
    {
        out << YAML::Alias("SPAN@" + llvm::Twine::utohexstr((uint64_t)(*j)));
    }
    out << YAML::EndSeq;

    // queue B
    out << YAML::Key << "QB" << YAML::Value << YAML::Flow << YAML::BeginSeq;
    for (SpanQueue::const_iterator k=m_QB.begin(), end=m_QB.end();
         k != end; ++k)
    {
        out << YAML::Alias("SPAN@" + llvm::Twine::utohexstr((uint64_t)(*k)));
    }
    out << YAML::EndSeq;

    // offset setters
    out << YAML::Key << "offset setters" << YAML::Value;
    if (m_offset_setters.empty())
        out << YAML::Flow;
    out << YAML::BeginSeq;
    for (std::vector<OffsetSetter>::const_iterator m=m_offset_setters.begin(),
         end=m_offset_setters.end(); m != end; ++m)
    {
        m->Write(out);
    }
    out << YAML::EndSeq;

    out << YAML::EndMap;
}

void
Optimizer::Dump() const
{
    YAML::Emitter out;
    Write(out);
    llvm::errs() << out.c_str() << '\n';
}

void
Optimizer::AddOffsetSetter(Bytecode& bc)
{
    // Remember it
    OffsetSetter& os = m_offset_setters.back();
    os.m_bc = &bc;
    os.m_thres = bc.getNextOffset();

    // Create new placeholder
    m_offset_setters.push_back(OffsetSetter());
}

void
Optimizer::ITreeAdd(Span& span, Span::Term& term)
{
    long precbc_index, precbc2_index;
    unsigned long low, high;

    // Update term length
    if (term.m_loc.bc)
        precbc_index = term.m_loc.bc->getIndex();
    else
        precbc_index = span.m_bc.getIndex()-1;

    if (term.m_loc2.bc)
        precbc2_index = term.m_loc2.bc->getIndex();
    else
        precbc2_index = span.m_bc.getIndex()-1;

    if (precbc_index < precbc2_index)
    {
        low = precbc_index+1;
        high = precbc2_index;
    }
    else if (precbc_index > precbc2_index)
    {
        low = precbc2_index+1;
        high = precbc_index;
    }
    else
        return;     // difference is same bc - always 0!

    m_itree.Insert(static_cast<long>(low), static_cast<long>(high), &term);
    ++num_itree;
}

void
Optimizer::CheckCycle(IntervalTreeNode<Span::Term*> * node,
                      Span& span,
                      Diagnostic& diags)
{
    Span::Term* term = node->getData();
    Span* depspan = term->m_span;

    // Only check for cycles in id=0 spans
    if (depspan->m_id > 0)
        return;

    // Check for a circular reference by looking to see if this dependent
    // span is in our backtrace.
    if (span.m_backtrace.count(depspan))
    {
        diags.Report(span.m_bc.getSource(),
                     diag::err_optimizer_circular_reference);
        return;
    }

    // Add our complete backtrace and ourselves to backtrace of dependent
    // span.
    depspan->m_backtrace.insert(span.m_backtrace.begin(),
                                span.m_backtrace.end());
    depspan->m_backtrace.insert(&span);
}

void
Optimizer::ExpandTerm(IntervalTreeNode<Span::Term*> * node, long len_diff)
{
    Span::Term* term = node->getData();
    Span* span = term->m_span;
    long precbc_index, precbc2_index;

    // Don't expand inactive spans
    if (span->m_active == Span::INACTIVE)
        return;

    DEBUG(llvm::errs() << "expand SPAN@" << span << " by " << len_diff << '\n');

    // Update term length
    if (term->m_loc.bc)
        precbc_index = term->m_loc.bc->getIndex();
    else
        precbc_index = span->m_bc.getIndex()-1;

    if (term->m_loc2.bc)
        precbc2_index = term->m_loc2.bc->getIndex();
    else
        precbc2_index = span->m_bc.getIndex()-1;

    if (precbc_index < precbc2_index)
        term->m_new_val += len_diff;
    else
        term->m_new_val -= len_diff;
    DEBUG(llvm::errs() << "updated SPAN@" << span << " term "
          << (term-&span->m_span_terms.front())
          << " newval to " << term->m_new_val << '\n');

    // If already on Q, don't re-add
    if (span->m_active == Span::ON_Q)
    {
        DEBUG(llvm::errs() << "SPAN@" << span << " already on queue\n");
        return;
    }

    // Update term and check against thresholds
    if (!span->RecalcNormal())
    {
        DEBUG(llvm::errs() << "SPAN@" << span
              << " didn't change, not readded\n");
        return; // didn't exceed thresholds, we're done
    }

    // Exceeded thresholds, need to add to Q for expansion
    DEBUG(llvm::errs() << "SPAN@" << span << " added back on queue\n");
    if (span->m_id <= 0)
        m_QA.push_back(span);
    else
        m_QB.push_back(span);
    span->m_active = Span::ON_Q;    // Mark as being in Q
}

void
Optimizer::Step1b(Diagnostic& diags)
{
    Spans::iterator spani = m_spans.begin();
    while (spani != m_spans.end())
    {
        Span* span = *spani;
        if (span->CreateTerms(this, diags) && span->RecalcNormal())
        {
            bool still_depend = false;
            if (!span->m_bc.Expand(span->m_id, span->m_cur_val, span->m_new_val,
                                   &still_depend, &span->m_neg_thres,
                                   &span->m_pos_thres, diags))
            {
                continue; // error
            }
            else if (still_depend)
            {
                if (span->m_active == Span::INACTIVE)
                {
                    diags.Report(span->m_bc.getSource(),
                                 diag::err_optimizer_secondary_expansion);
                }
            }
            else
            {
                delete *spani;
                spani = m_spans.erase(spani);
                continue;
            }
        }
        DEBUG(llvm::errs() << "updated SPAN@" << span << " curval from "
              << span->m_cur_val << " to " << span->m_new_val << '\n');
        span->m_cur_val = span->m_new_val;
        ++spani;
    }
}

bool
Optimizer::Step1d()
{
    for (Spans::iterator spani=m_spans.begin(), endspan=m_spans.end();
         spani != endspan; ++spani)
    {
        ++num_step1d;
        Span* span = *spani;

        // Update span terms based on new bc offsets
        for (Span::Terms::iterator term=span->m_span_terms.begin(),
             endterm=span->m_span_terms.end(); term != endterm; ++term)
        {
            IntNum intn;
            bool ok = CalcDist(term->m_loc, term->m_loc2, &intn);
            ok = ok;    // avoid warning due to assert usage
            assert(ok && "could not calculate bc distance");
            term->m_cur_val = term->m_new_val;
            term->m_new_val = intn.getInt();
            DEBUG(llvm::errs() << "updated SPAN@" << span << " term "
                  << (term-span->m_span_terms.begin())
                  << " newval to " << term->m_new_val << '\n');
        }

        if (span->RecalcNormal())
        {
            // Exceeded threshold, add span to QB
            m_QB.push_back(&(*span));
            span->m_active = Span::ON_Q;
            ++num_initial_qb;
        }
    }

    // Do we need step 2?  If not, go ahead and exit.
    return m_QB.empty();
}

void
Optimizer::Step1e(Diagnostic& diags)
{
    // Update offset-setters values
    for (std::vector<OffsetSetter>::iterator os=m_offset_setters.begin(),
         osend=m_offset_setters.end(); os != osend; ++os)
    {
        if (!os->m_bc)
            continue;
        os->m_thres = os->m_bc->getNextOffset();
        os->m_new_val = os->m_bc->getOffset();
        os->m_cur_val = os->m_new_val;
        ++num_offset_setters;
    }

    // Build up interval tree
    for (Spans::iterator spani=m_spans.begin(), endspan=m_spans.end();
         spani != endspan; ++spani)
    {
        Span* span = *spani;
        for (Span::Terms::iterator term=span->m_span_terms.begin(),
             endterm=span->m_span_terms.end(); term != endterm; ++term)
            ITreeAdd(*span, *term);
    }

    // Look for cycles in times expansion (span.id==0)
    for (Spans::iterator spani=m_spans.begin(), endspan=m_spans.end();
         spani != endspan; ++spani)
    {
        Span* span = *spani;
        if (span->m_id > 0)
            continue;
        m_itree.Enumerate(static_cast<long>(span->m_bc.getIndex()),
                          static_cast<long>(span->m_bc.getIndex()),
                          BIND::bind(&Optimizer::CheckCycle, this, _1,
                                     REF::ref(*span), REF::ref(diags)));
    }
}

void
Optimizer::Step2(Diagnostic& diags)
{
    DEBUG(Dump());

    while (!m_QA.empty() || !m_QB.empty())
    {
        Span* span;

        // QA is for TIMES, update those first, then update non-TIMES.
        // This is so that TIMES can absorb increases before we look at
        // expanding non-TIMES BCs.
        if (!m_QA.empty())
        {
            span = m_QA.front();
            m_QA.pop_front();
        }
        else
        {
            span = m_QB.front();
            m_QB.pop_front();
        }

        if (span->m_active == Span::INACTIVE)
            continue;
        span->m_active = Span::ACTIVE;  // no longer in Q

        // Make sure we ended up ultimately exceeding thresholds; due to
        // offset BCs we may have been placed on Q and then reduced in size
        // again.
        if (!span->RecalcNormal())
            continue;

        ++num_expansions;

        unsigned long orig_len = span->m_bc.getTotalLen();

        bool still_depend = false;
        if (!span->m_bc.Expand(span->m_id, span->m_cur_val, span->m_new_val,
                               &still_depend, &span->m_neg_thres,
                               &span->m_pos_thres, diags))
        {
            // error
            continue;
        }
        else if (still_depend)
        {
            // another threshold, keep active
            for (Span::Terms::iterator term=span->m_span_terms.begin(),
                 endterm=span->m_span_terms.end(); term != endterm; ++term)
                term->m_cur_val = term->m_new_val;
            DEBUG(llvm::errs() << "updated SPAN@" << span << " curval from "
                  << span->m_cur_val << " to " << span->m_new_val << '\n');
            span->m_cur_val = span->m_new_val;
        }
        else
            span->m_active = Span::INACTIVE;    // we're done with this span

        long len_diff = span->m_bc.getTotalLen() - orig_len;
        if (len_diff == 0)
            continue;   // didn't increase in size

        DEBUG(llvm::errs() << "BC@" << &span->m_bc << " expansion by "
              << len_diff << ":\n");
        // Iterate over all spans dependent across the bc just expanded
        m_itree.Enumerate(static_cast<long>(span->m_bc.getIndex()),
                          static_cast<long>(span->m_bc.getIndex()),
                          BIND::bind(&Optimizer::ExpandTerm, this, _1,
                                     len_diff));

        // Iterate over offset-setters that follow the bc just expanded.
        // Stop iteration if:
        //  - no more offset-setters in this section
        //  - offset-setter didn't move its following offset
        std::vector<OffsetSetter>::iterator os =
            m_offset_setters.begin() + span->m_os_index;
        long offset_diff = len_diff;
        while (os != m_offset_setters.end()
               && os->m_bc
               && os->m_bc->getContainer() == span->m_bc.getContainer()
               && offset_diff != 0)
        {
            unsigned long old_next_offset =
                os->m_cur_val + os->m_bc->getTotalLen();

            assert((offset_diff >= 0 ||
                    static_cast<unsigned long>(-offset_diff) <= os->m_new_val)
                   && "org/align went to negative offset");
            os->m_new_val += offset_diff;

            orig_len = os->m_bc->getTailLen();
            bool still_depend_temp;
            long neg_thres_temp, pos_thres_temp;
            os->m_bc->Expand(1, static_cast<long>(os->m_cur_val),
                             static_cast<long>(os->m_new_val),
                             &still_depend_temp, &neg_thres_temp,
                             &pos_thres_temp, diags);
            os->m_thres = static_cast<long>(pos_thres_temp);

            offset_diff =
                os->m_new_val + os->m_bc->getTotalLen() - old_next_offset;
            len_diff = os->m_bc->getTailLen() - orig_len;
            if (len_diff != 0)
                m_itree.Enumerate(static_cast<long>(os->m_bc->getIndex()),
                                  static_cast<long>(os->m_bc->getIndex()),
                                  BIND::bind(&Optimizer::ExpandTerm, this, _1,
                                             len_diff));

            os->m_cur_val = os->m_new_val;
            ++os;
        }
    }
}

void
Object::UpdateBytecodeOffsets(Diagnostic& diags)
{
    for (section_iterator sect=m_sections.begin(), end=m_sections.end();
         sect != end; ++sect)
        sect->UpdateOffsets(diags);
}

void
Object::Optimize(Diagnostic& diags)
{
    Optimizer opt;
    unsigned long bc_index = 0;

    // Step 1a
    for (section_iterator sect=m_sections.begin(), sectend=m_sections.end();
         sect != sectend; ++sect)
    {
        unsigned long offset = 0;

        // Set the offset of the first (empty) bytecode.
        sect->bytecodes_front().setIndex(bc_index++);
        sect->bytecodes_front().setOffset(0);

        // Iterate through the remainder, if any.
        for (Section::bc_iterator bc=sect->bytecodes_begin(),
             bcend=sect->bytecodes_end(); bc != bcend; ++bc)
        {
            bc->setIndex(bc_index++);
            bc->setOffset(offset);

            if (bc->CalcLen(BIND::bind(&Optimizer::AddSpan, &opt,
                                       _1, _2, _3, _4, _5),
                            diags))
            {
                if (bc->getSpecial() == Bytecode::Contents::SPECIAL_OFFSET)
                    opt.AddOffsetSetter(*bc);

                offset = bc->getNextOffset();
            }
        }
    }

    if (diags.hasErrorOccurred())
        return;

    // Step 1b
    opt.Step1b(diags);
    if (diags.hasErrorOccurred())
        return;

    // Step 1c
    UpdateBytecodeOffsets(diags);
    if (diags.hasErrorOccurred())
        return;

    // Step 1d
    if (opt.Step1d())
        return;

    // Step 1e
    opt.Step1e(diags);
    if (diags.hasErrorOccurred())
        return;

    // Step 2
    opt.Step2(diags);
    if (diags.hasErrorOccurred())
        return;

    // Step 3
    UpdateBytecodeOffsets(diags);
}

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

#include "yasmx/Optimizer.h"

#include <algorithm>
#include <deque>
#include <list>
#include <memory>
#include <vector>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Config/functional.h"
#include "yasmx/Support/IntervalTree.h"
#include "yasmx/Bytecode.h"
#include "yasmx/DebugDumper.h"
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
#ifdef WITH_XML
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

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

#ifdef WITH_XML
pugi::xml_node
OffsetSetter::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("OffsetSetter");
    root.append_attribute("bc") =
        Twine::utohexstr((uint64_t)m_bc).str().c_str();
    root.append_attribute("curval") = m_cur_val;
    root.append_attribute("newval") = m_new_val;
    root.append_attribute("thres") = m_thres;
    return root;
}
#endif // WITH_XML

namespace {
class Span
{
    friend class Optimizer;
    friend class Optimizer::Impl;
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
#ifdef WITH_XML
        pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

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

    bool CreateTerms(Optimizer::Impl* optimize, DiagnosticsEngine& diags);
    bool RecalcNormal(DiagnosticsEngine& diags);

    std::string getName() const;
#ifdef WITH_XML
    pugi::xml_node Write(pugi::xml_node out) const;
    pugi::xml_node WriteRef(pugi::xml_node out) const;
#endif // WITH_XML

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
} // anonymous namespace

namespace yasm {
class Optimizer::Impl
{
public:
    Impl(DiagnosticsEngine& diags);
    ~Impl();

    void Step1b();
    bool Step1d();
    void Step1e();
    void Step2();

#ifdef WITH_XML
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    void ITreeAdd(Span& span, Span::Term& term);
    void CheckCycle(IntervalTreeNode<Span::Term*> * node,
                    Span& span);
    void ExpandTerm(IntervalTreeNode<Span::Term*> * node, long len_diff);

    DiagnosticsEngine& m_diags;

    typedef std::list<Span*> Spans;
    Spans m_spans;      // ownership list

    typedef std::deque<Span*> SpanQueue;
    SpanQueue m_QA, m_QB;

    IntervalTree<Span::Term*> m_itree;
    std::vector<OffsetSetter> m_offset_setters;
};
} // namespace yasm

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

#ifdef WITH_XML
pugi::xml_node
Span::Term::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("Term");
    append_child(root, "Loc", m_loc);
    append_child(root, "Loc2", m_loc2);
    root.append_attribute("curval") = m_cur_val;
    root.append_attribute("newval") = m_new_val;
    root.append_attribute("subst") = m_subst;
    return root;
}
#endif // WITH_XML

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
    m_impl->m_spans.push_back(new Span(bc, id, value, neg_thres, pos_thres,
                                       m_impl->m_offset_setters.size()-1));
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
Span::CreateTerms(Optimizer::Impl* optimize, DiagnosticsEngine& diags)
{
    // Split out sym-sym terms in absolute portion of dependent value
    if (m_depval.hasAbs())
    {
        SubstDist(*m_depval.getAbs(), diags,
                  TR1::bind(&Span::AddTerm, this, _1, _2, _3));
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
Span::RecalcNormal(DiagnosticsEngine& diags)
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
        if (!Evaluate(*m_depval.getAbs(), diags, &result, m_expr_terms, false,
                      false)
            || !result.isType(ExprTerm::INT))
            m_new_val = LONG_MAX;   // too complex; force to longest form
        else
            m_new_val = result.getIntNum()->getInt();
    }

    if (m_new_val == LONG_MAX)
        m_active = INACTIVE;

    DEBUG(llvm::errs() << "updated " << getName() << " newval to "
          << m_new_val << '\n');

    // If id<=0, flag update on any change
    if (m_id <= 0)
        return (m_new_val != m_cur_val);

    return (m_new_val < m_neg_thres || m_new_val > m_pos_thres);
}

Span::~Span()
{
}

std::string
Span::getName() const
{
    SmallString<32> ss;
    llvm::raw_svector_ostream oss(ss);
    oss << "SPAN{" << m_bc.getIndex() << ',' << m_id << '}';
    return oss.str();
}

#ifdef WITH_XML
pugi::xml_node
Span::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("Span");
    root.append_attribute("bc") =
        Twine::utohexstr((uint64_t)(&m_bc)).str().c_str();
    root.append_attribute("id") = m_id;

    if (!m_depval.hasAbs() || m_depval.isRelative())
        append_child(root, "DepVal", m_depval);
    else
    {
        SmallString<256> ss;
        llvm::raw_svector_ostream oss(ss);
        oss << *m_depval.getAbs() << '\0';
        append_child(root, "DepVal", oss.str().data());
    }

    for (Terms::const_iterator i=m_span_terms.begin(), end=m_span_terms.end();
         i != end; ++i)
        append_data(root, *i);

    root.append_attribute("curval") = m_cur_val;
    root.append_attribute("newval") = m_new_val;
    root.append_attribute("negthres") = m_neg_thres;
    root.append_attribute("posthres") = m_pos_thres;

    switch (m_active)
    {
        case INACTIVE:  root.append_attribute("active") = "inactive"; break;
        case ACTIVE:    root.append_attribute("active") = "active"; break;
        case ON_Q:      root.append_attribute("active") = "queued"; break;
    }

    pugi::xml_node backtrace = root.append_child("Backtrace");
    for (BacktraceSpans::const_iterator i=m_backtrace.begin(),
         end=m_backtrace.end(); i != end; ++i)
        (*i)->WriteRef(backtrace);

    root.append_attribute("os_index") = static_cast<unsigned long>(m_os_index);
    return root;
}

pugi::xml_node
Span::WriteRef(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("Span");
    root.append_attribute("bc") = m_bc.getIndex();
    root.append_attribute("id") = m_id;
    return root;
}
#endif // WITH_XML

Optimizer::Impl::Impl(DiagnosticsEngine& diags)
    : m_diags(diags)
{
    // Create an placeholder offset setter for spans to point to; this will
    // get updated if/when we actually run into one.
    m_offset_setters.push_back(OffsetSetter());
}

Optimizer::Impl::~Impl()
{
    while (!m_spans.empty())
    {
        delete m_spans.back();
        m_spans.pop_back();
    }
}

#ifdef WITH_XML
pugi::xml_node
Optimizer::Impl::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("Optimizer");

    // spans
    pugi::xml_node spans = root.append_child("Spans");
    for (Spans::const_iterator i=m_spans.begin(), end=m_spans.end();
         i != end; ++i)
        append_data(spans, **i);

    // queue A
    pugi::xml_node qa = root.append_child("QueueA");
    for (SpanQueue::const_iterator j=m_QA.begin(), end=m_QA.end();
         j != end; ++j)
        (*j)->WriteRef(qa);

    // queue B
    pugi::xml_node qb = root.append_child("QueueB");
    for (SpanQueue::const_iterator k=m_QB.begin(), end=m_QB.end();
         k != end; ++k)
        (*k)->WriteRef(qb);

    // offset setters
    pugi::xml_node osetters = root.append_child("OffsetSetters");
    for (std::vector<OffsetSetter>::const_iterator m=m_offset_setters.begin(),
         end=m_offset_setters.end(); m != end; ++m)
        append_data(osetters, *m);

    return root;
}
#endif // WITH_XML

void
Optimizer::AddOffsetSetter(Bytecode& bc)
{
    // Remember it
    OffsetSetter& os = m_impl->m_offset_setters.back();
    os.m_bc = &bc;
    os.m_thres = bc.getNextOffset();

    // Create new placeholder
    m_impl->m_offset_setters.push_back(OffsetSetter());
}

void
Optimizer::Impl::ITreeAdd(Span& span, Span::Term& term)
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
        low = precbc_index;
        high = precbc2_index-1;
    }
    else if (precbc_index > precbc2_index)
    {
        low = precbc2_index;
        high = precbc_index-1;
    }
    else
        return;     // difference is same bc - always 0!

    m_itree.Insert(static_cast<long>(low), static_cast<long>(high), &term);
    ++num_itree;
}

void
Optimizer::Impl::CheckCycle(IntervalTreeNode<Span::Term*> * node, Span& span)
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
        m_diags.Report(span.m_bc.getSource(),
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
Optimizer::Impl::ExpandTerm(IntervalTreeNode<Span::Term*> * node, long len_diff)
{
    Span::Term* term = node->getData();
    Span* span = term->m_span;
    long precbc_index, precbc2_index;

    // Don't expand inactive spans
    if (span->m_active == Span::INACTIVE)
        return;

    DEBUG(llvm::errs() << "expand " << span->getName() << " by " << len_diff
          << '\n');

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
    DEBUG(llvm::errs() << "updated " << span->getName() << " term "
          << (term-&span->m_span_terms.front())
          << " newval to " << term->m_new_val << '\n');

    // If already on Q, don't re-add
    if (span->m_active == Span::ON_Q)
    {
        DEBUG(llvm::errs() << span->getName() << " already on queue\n");
        return;
    }

    // Update term and check against thresholds
    if (!span->RecalcNormal(m_diags))
    {
        DEBUG(llvm::errs() << span->getName()
              << " didn't change, not readded\n");
        return; // didn't exceed thresholds, we're done
    }

    // Exceeded thresholds, need to add to Q for expansion
    DEBUG(llvm::errs() << span->getName() << " added back on queue\n");
    if (span->m_id <= 0)
        m_QA.push_back(span);
    else
        m_QB.push_back(span);
    span->m_active = Span::ON_Q;    // Mark as being in Q
}

void
Optimizer::Impl::Step1b()
{
    Spans::iterator spani = m_spans.begin();
    while (spani != m_spans.end())
    {
        Span* span = *spani;
        if (span->CreateTerms(this, m_diags) && span->RecalcNormal(m_diags))
        {
            bool still_depend = false;
            if (!span->m_bc.Expand(span->m_id, span->m_cur_val, span->m_new_val,
                                   &still_depend, &span->m_neg_thres,
                                   &span->m_pos_thres, m_diags))
            {
                continue; // error
            }
            else if (still_depend)
            {
                if (span->m_active == Span::INACTIVE)
                {
                    m_diags.Report(span->m_bc.getSource(),
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
        DEBUG(llvm::errs() << "updated " << span->getName() << " curval from "
              << span->m_cur_val << " to " << span->m_new_val << '\n');
        span->m_cur_val = span->m_new_val;
        ++spani;
    }
}

bool
Optimizer::Impl::Step1d()
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
            DEBUG(llvm::errs() << "updated " << span->getName() << " term "
                  << (term-span->m_span_terms.begin())
                  << " newval to " << term->m_new_val << '\n');
        }

        if (span->RecalcNormal(m_diags))
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
Optimizer::Impl::Step1e()
{
    // Update offset-setters values
    for (std::vector<OffsetSetter>::iterator os=m_offset_setters.begin(),
         osend=m_offset_setters.end(); os != osend; ++os)
    {
        if (!os->m_bc)
            continue;
        os->m_thres = os->m_bc->getNextOffset();
        os->m_new_val = os->m_bc->getOffset() + os->m_bc->getFixedLen();
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
                          TR1::bind(&Optimizer::Impl::CheckCycle, this, _1,
                                    TR1::ref(*span)));
    }
}

void
Optimizer::Impl::Step2()
{
    DEBUG(DumpXml(*this));

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
        if (!span->RecalcNormal(m_diags))
            continue;

        ++num_expansions;

        unsigned long orig_len = span->m_bc.getTotalLen();

        bool still_depend = false;
        if (!span->m_bc.Expand(span->m_id, span->m_cur_val, span->m_new_val,
                               &still_depend, &span->m_neg_thres,
                               &span->m_pos_thres, m_diags))
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
            DEBUG(llvm::errs() << "updated " << span->getName()
                  << " curval from " << span->m_cur_val << " to "
                  << span->m_new_val << '\n');
            span->m_cur_val = span->m_new_val;
        }
        else
            span->m_active = Span::INACTIVE;    // we're done with this span

        long len_diff = span->m_bc.getTotalLen() - orig_len;
        if (len_diff == 0)
            continue;   // didn't increase in size

        DEBUG(llvm::errs() << "BC@" << &span->m_bc << " ("
              << span->m_bc.getIndex() << ") expansion by "
              << len_diff << ":\n");
        // Iterate over all spans dependent across the bc just expanded
        m_itree.Enumerate(static_cast<long>(span->m_bc.getIndex()),
                          static_cast<long>(span->m_bc.getIndex()),
                          TR1::bind(&Optimizer::Impl::ExpandTerm, this, _1,
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
                             &pos_thres_temp, m_diags);
            os->m_thres = static_cast<long>(pos_thres_temp);

            offset_diff =
                os->m_new_val + os->m_bc->getTotalLen() - old_next_offset;
            len_diff = os->m_bc->getTailLen() - orig_len;
            if (len_diff != 0)
            {
                DEBUG(llvm::errs() << "BC@" << os->m_bc << " ("
                      << os->m_bc->getIndex() << ") offset setter change by "
                      << len_diff << ":\n");
                m_itree.Enumerate(static_cast<long>(os->m_bc->getIndex()),
                                  static_cast<long>(os->m_bc->getIndex()),
                                  TR1::bind(&Optimizer::Impl::ExpandTerm, this,
                                            _1, len_diff));
            }

            os->m_cur_val = os->m_new_val;
            ++os;
        }
    }
}

Optimizer::Optimizer(DiagnosticsEngine& diags)
    : m_impl(new Impl(diags))
{
}

Optimizer::~Optimizer()
{
}

void
Optimizer::Step1b()
{
    m_impl->Step1b();
}

bool
Optimizer::Step1d()
{
    return m_impl->Step1d();
}

void
Optimizer::Step1e()
{
    m_impl->Step1e();
}

void
Optimizer::Step2()
{
    m_impl->Step2();
}

#ifdef WITH_XML
pugi::xml_node
Optimizer::Write(pugi::xml_node out) const
{
    return m_impl->Write(out);
}
#endif // WITH_XML

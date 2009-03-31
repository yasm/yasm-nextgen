//
// Object implementation.
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
#include "Object.h"

#include "util.h"

#include <algorithm>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <vector>

#include <boost/pool/pool.hpp>

#include "Arch.h"
#include "Bytecode.h"
#include "Bytecode_util.h"
#include "DebugFormat.h"
#include "errwarn.h"
#include "Errwarns.h"
#include "Expr.h"
#include "hamt.h"
#include "IntervalTree.h"
#include "IntNum.h"
#include "Location_util.h"
#include "marg_ostream.h"
#include "ObjectFormat.h"
#include "Section.h"
#include "Symbol.h"
#include "Value.h"


namespace
{

/// Get name helper for symbol table HAMT.
class SymGetName
{
public:
    const std::string& operator() (const yasm::Symbol* sym) const
    { return sym->get_name(); }
};

} // anonymous namespace

namespace yasm
{

class Object::Impl
{
public:
    Impl(bool nocase)
        : sym_map(nocase)
        , special_sym_map(false)
        , m_sym_pool(sizeof(Symbol))
    {}
    ~Impl() {}

    Symbol* new_symbol(const std::string& name)
    {
        Symbol* sym = static_cast<Symbol*>(m_sym_pool.malloc());
        new (sym) Symbol(name);
        return sym;
    }

    void delete_symbol(Symbol* sym)
    {
        if (sym)
        {
            sym->~Symbol();
            m_sym_pool.free(sym);
        }
    }

    typedef hamt<std::string, Symbol, SymGetName> SymbolTable;

    /// Symbol table symbols, indexed by name.
    SymbolTable sym_map;

    /// Special symbols, indexed by name.
    SymbolTable special_sym_map;

private:
    /// Pool for symbols not in the symbol table.
    boost::pool<> m_sym_pool;
};

Object::Object(const std::string& src_filename,
               const std::string& obj_filename,
               Arch* arch)
    : m_src_filename(src_filename),
      m_arch(arch),
      m_cur_section(0),
      m_sections_owner(m_sections),
      m_symbols_owner(m_symbols),
      m_impl(new Impl(false))
{
}

void
Object::set_source_fn(const std::string& src_filename)
{
    m_src_filename = src_filename;
}

void
Object::set_object_fn(const std::string& obj_filename)
{
    m_obj_filename = obj_filename;
}

Object::~Object()
{
}

marg_ostream&
operator<< (marg_ostream& os, const Object& object)
{
    // Print symbol table
    os << "Symbol Table:\n";
    for (Object::const_symbol_iterator i=object.m_symbols.begin(),
         end=object.m_symbols.end(); i != end; ++i)
    {
        os << "Symbol `" << i->get_name() << "'\n";
        ++os;
        os << *i;
        --os;
    }

    // Print sections and bytecodes
    for (Object::const_section_iterator i=object.m_sections.begin(),
         end=object.m_sections.end(); i != end; ++i)
    {
        os << "Section:\n";
        os << *i;
    }
    return os;
}

void
Object::finalize(Errwarns& errwarns)
{
    std::for_each(m_sections.begin(), m_sections.end(),
                  BIND::bind(&Section::finalize, _1, REF::ref(errwarns)));
}

void
Object::append_section(std::auto_ptr<Section> sect)
{
    sect->m_object = this;
    m_sections.push_back(sect.release());
}

Section*
Object::find_section(const std::string& name)
{
    section_iterator i =
        std::find_if(m_sections.begin(), m_sections.end(),
                     BIND::bind(&Section::is_name, _1, REF::ref(name)));
    if (i == m_sections.end())
        return 0;
    return &(*i);
}

SymbolRef
Object::get_absolute_symbol()
{
    SymbolRef sym = get_symbol("");

    // If we already defined it, we're done.
    if (sym->get_status() & Symbol::DEFINED)
        return sym;

    // Define it
    std::auto_ptr<Expr> v(new Expr(0));
    sym->define_equ(v, 0);
    sym->use(0);
    return sym;
}

SymbolRef
Object::find_symbol(const std::string& name)
{
    return SymbolRef(m_impl->sym_map.find(name));
}

SymbolRef
Object::get_symbol(const std::string& name)
{
    // Don't use pool allocator for symbols in the symbol table.
    // We have to maintain an ordered link list of all symbols in the symbol
    // table, so it's easy enough to reuse that for deleting the symbols.
    // The memory impact of keeping a second linked list (internal to the pool)
    // seems to outweigh the moderate time savings of pool deletion.
    std::auto_ptr<Symbol> sym(new Symbol(name));
    Symbol* sym2 = m_impl->sym_map.insert(sym.get());
    if (sym2)
        return SymbolRef(sym2);

    sym2 = sym.get();
    m_symbols.push_back(sym.release());
    return SymbolRef(sym2);
}

SymbolRef
Object::append_symbol(const std::string& name)
{
    Symbol* sym = new Symbol(name);
    m_symbols.push_back(sym);
    return SymbolRef(sym);
}

SymbolRef
Object::add_non_table_symbol(const std::string& name)
{
    Symbol* sym = m_impl->new_symbol(name);
    return SymbolRef(sym);
}

void
Object::symbols_finalize(Errwarns& errwarns, bool undef_extern)
{
    unsigned long firstundef_line = ULONG_MAX;

    for (symbol_iterator i=m_symbols.begin(), end=m_symbols.end();
         i != end; ++i)
    {
        try
        {
            i->finalize(undef_extern);
        }
        catch (Error& err)
        {
            unsigned long use_line = i->get_use_line();
            errwarns.propagate(use_line, err);
            if (use_line < firstundef_line)
                firstundef_line = use_line;
        }
    }
    if (firstundef_line < ULONG_MAX)
        errwarns.propagate(firstundef_line,
            Error(N_(" (Each undefined symbol is reported only once.)")));
}

SymbolRef
Object::add_special_symbol(const std::string& name)
{
    Symbol* sym = m_impl->new_symbol(name);
    m_impl->special_sym_map.insert(sym);
    return SymbolRef(sym);
}

SymbolRef
Object::find_special_symbol(const std::string& name)
{
    return SymbolRef(m_impl->special_sym_map.find(name));
}

} // namespace yasm

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
namespace
{

using namespace yasm;

class OffsetSetter
{
public:
    OffsetSetter();
    ~OffsetSetter() {}

    Bytecode* m_bc;
    unsigned long m_cur_val;
    unsigned long m_new_val;
    unsigned long m_thres;
};

OffsetSetter::OffsetSetter()
    : m_bc(0),
      m_cur_val(0),
      m_new_val(0),
      m_thres(0)
{
}

class Optimize;

class Span
{
    friend class Optimize;
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

    void create_terms(Optimize* optimize);
    bool recalc_normal();

private:
    Span(const Span&);                  // not implemented
    const Span& operator=(const Span&); // not implemented

    void add_term(unsigned int subst, Location loc, Location loc2);

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
    std::vector<Span*> m_backtrace;

    // Index of first offset setter following this span's bytecode
    size_t m_os_index;
};

class Optimize
{
public:
    Optimize();
    ~Optimize();
    void add_span(Bytecode& bc,
                  int id,
                  const Value& value,
                  long neg_thres,
                  long pos_thres);
    void add_offset_setter(Bytecode& bc);

    bool step_1b(Errwarns& errwarns);
    bool step_1d();
    bool step_1e(Errwarns& errwarns);
    bool step_2(Errwarns& errwarns);

private:
    void itree_add(Span& span, Span::Term& term);
    void check_cycle(IntervalTreeNode<Span::Term*> * node, Span& span);
    void term_expand(IntervalTreeNode<Span::Term*> * node, long len_diff);

    std::list<Span*> m_spans;   // ownership list
    std::queue<Span*> m_QA, m_QB;
    IntervalTree<Span::Term*> m_itree;
    std::vector<OffsetSetter> m_offset_setters;
};

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
}

void
Optimize::add_span(Bytecode& bc,
                   int id,
                   const Value& value,
                   long neg_thres,
                   long pos_thres)
{
    m_spans.push_back(new Span(bc, id, value, neg_thres, pos_thres,
                               m_offset_setters.size()-1));
}

void
Span::add_term(unsigned int subst, Location loc, Location loc2)
{
    IntNum intn;
    bool ok = calc_dist(loc, loc2, &intn);
    ok = ok;    // avoid warning due to assert usage
    assert(ok && "could not calculate bc distance");

    if (subst >= m_span_terms.size())
        m_span_terms.resize(subst+1);
    m_span_terms[subst] = Term(subst, loc, loc2, this, intn.get_int());
}

void
Span::create_terms(Optimize* optimize)
{
    // Split out sym-sym terms in absolute portion of dependent value
    if (m_depval.has_abs())
    {
        subst_dist(*m_depval.get_abs(),
                   BIND::bind(&Span::add_term, this, _1, _2, _3));
        if (m_span_terms.size() > 0)
        {
            for (Terms::iterator i=m_span_terms.begin(),
                 end=m_span_terms.end(); i != end; ++i)
            {
                // Create expression terms with dummy value
                m_expr_terms.push_back(ExprTerm(0));

                // Check for circular references
                if (m_id <= 0 &&
                    ((m_bc.get_index() > i->m_loc.bc->get_index()-1 &&
                      m_bc.get_index() <= i->m_loc2.bc->get_index()-1) ||
                     (m_bc.get_index() > i->m_loc2.bc->get_index()-1 &&
                      m_bc.get_index() <= i->m_loc.bc->get_index()-1)))
                    throw ValueError(N_("circular reference detected"));
            }
        }
    }
}

// Recalculate span value based on current span replacement values.
// Returns True if span needs expansion (e.g. exceeded thresholds).
bool
Span::recalc_normal()
{
    m_new_val = 0;

    if (m_depval.has_abs())
    {
        std::auto_ptr<Expr> abs_copy(m_depval.get_abs()->clone());

        // Update sym-sym terms and substitute back into expr
        for (Terms::iterator i=m_span_terms.begin(), end=m_span_terms.end();
             i != end; ++i)
            m_expr_terms[i->m_subst].get_int()->set(i->m_new_val);
        abs_copy->substitute(m_expr_terms);
        abs_copy->simplify();
        if (const IntNum* num = abs_copy->get_intnum())
            m_new_val = num->get_int();
        else
            m_new_val = LONG_MAX;   // too complex; force to longest form
    }

    if (m_depval.is_relative())
        m_new_val = LONG_MAX;       // too complex; force to longest form

    if (m_new_val == LONG_MAX)
        m_active = INACTIVE;

    // If id<=0, flag update on any change
    if (m_id <= 0)
        return (m_new_val != m_cur_val);

    return (m_new_val < m_neg_thres || m_new_val > m_pos_thres);
}

Span::~Span()
{
}

Optimize::Optimize()
{
    // Create an placeholder offset setter for spans to point to; this will
    // get updated if/when we actually run into one.
    m_offset_setters.push_back(OffsetSetter());
}

Optimize::~Optimize()
{
    while (!m_spans.empty())
    {
        delete m_spans.back();
        m_spans.pop_back();
    }
}

void
Optimize::add_offset_setter(Bytecode& bc)
{
    // Remember it
    OffsetSetter& os = m_offset_setters.back();
    os.m_bc = &bc;
    os.m_thres = bc.next_offset();

    // Create new placeholder
    m_offset_setters.push_back(OffsetSetter());
}

void
Optimize::itree_add(Span& span, Span::Term& term)
{
    long precbc_index, precbc2_index;
    unsigned long low, high;

    /* Update term length */
    if (term.m_loc.bc)
        precbc_index = term.m_loc.bc->get_index();
    else
        precbc_index = span.m_bc.get_index()-1;

    if (term.m_loc2.bc)
        precbc2_index = term.m_loc2.bc->get_index();
    else
        precbc2_index = span.m_bc.get_index()-1;

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
        return;     /* difference is same bc - always 0! */

    m_itree.insert(static_cast<long>(low), static_cast<long>(high), &term);
}

void
Optimize::check_cycle(IntervalTreeNode<Span::Term*> * node, Span& span)
{
    Span::Term* term = node->get_data();
    Span* depspan = term->m_span;

    // Only check for cycles in id=0 spans
    if (depspan->m_id > 0)
        return;

    // Check for a circular reference by looking to see if this dependent
    // span is in our backtrace.
    std::vector<Span*>::iterator iter;
    iter = std::find(span.m_backtrace.begin(), span.m_backtrace.end(),
                     depspan);
    if (iter != span.m_backtrace.end())
        throw ValueError(N_("circular reference detected"));

    // Add our complete backtrace and ourselves to backtrace of dependent
    // span.
    std::copy(span.m_backtrace.begin(), span.m_backtrace.end(),
              depspan->m_backtrace.end());
    depspan->m_backtrace.push_back(&span);
}

void
Optimize::term_expand(IntervalTreeNode<Span::Term*> * node, long len_diff)
{
    Span::Term* term = node->get_data();
    Span* span = term->m_span;
    long precbc_index, precbc2_index;

    // Don't expand inactive spans
    if (span->m_active == Span::INACTIVE)
        return;

    // Update term length
    if (term->m_loc.bc)
        precbc_index = term->m_loc.bc->get_index();
    else
        precbc_index = span->m_bc.get_index()-1;

    if (term->m_loc2.bc)
        precbc2_index = term->m_loc2.bc->get_index();
    else
        precbc2_index = span->m_bc.get_index()-1;

    if (precbc_index < precbc2_index)
        term->m_new_val += len_diff;
    else
        term->m_new_val -= len_diff;

    // If already on Q, don't re-add
    if (span->m_active == Span::ON_Q)
        return;

    // Update term and check against thresholds
    if (!span->recalc_normal())
        return; // didn't exceed thresholds, we're done

    // Exceeded thresholds, need to add to Q for expansion
    if (span->m_id <= 0)
        m_QA.push(span);
    else
        m_QB.push(span);
    span->m_active = Span::ON_Q;    // Mark as being in Q
}

bool
Optimize::step_1b(Errwarns& errwarns)
{
    bool saw_error = false;

    std::list<Span*>::iterator spani = m_spans.begin();
    while (spani != m_spans.end())
    {
        Span* span = *spani;
        bool terms_okay = true;

        try
        {
            span->create_terms(this);
        }
        catch (Error& err)
        {
            errwarns.propagate(span->m_bc.get_line(), err);
            saw_error = true;
            terms_okay = false;
        }

        if (terms_okay && span->recalc_normal())
        {
            bool still_depend =
                expand(span->m_bc, span->m_id, span->m_cur_val,
                       span->m_new_val, &span->m_neg_thres,
                       &span->m_pos_thres, errwarns);
            if (errwarns.num_errors() > 0)
                saw_error = true;
            else if (still_depend)
            {
                if (span->m_active == Span::INACTIVE)
                {
                    errwarns.propagate(span->m_bc.get_line(),
                        ValueError(N_("secondary expansion of an external/complex value")));
                    saw_error = true;
                }
            }
            else
            {
                delete *spani;
                spani = m_spans.erase(spani);
                continue;
            }
        }
        span->m_cur_val = span->m_new_val;
        ++spani;
    }

    return saw_error;
}

bool
Optimize::step_1d()
{
    for (std::list<Span*>::iterator spani=m_spans.begin(),
         endspan=m_spans.end(); spani != endspan; ++spani)
    {
        Span* span = *spani;

        // Update span terms based on new bc offsets
        for (Span::Terms::iterator term=span->m_span_terms.begin(),
             endterm=span->m_span_terms.end(); term != endterm; ++term)
        {
            IntNum intn;
            bool ok = calc_dist(term->m_loc, term->m_loc2, &intn);
            ok = ok;    // avoid warning due to assert usage
            assert(ok && "could not calculate bc distance");
            term->m_cur_val = term->m_new_val;
            term->m_new_val = intn.get_int();
        }

        if (span->recalc_normal())
        {
            // Exceeded threshold, add span to QB
            m_QB.push(&(*span));
            span->m_active = Span::ON_Q;
        }
    }

    // Do we need step 2?  If not, go ahead and exit.
    return m_QB.empty();
}

bool
Optimize::step_1e(Errwarns& errwarns)
{
    bool saw_error = false;

    // Update offset-setters values
    for (std::vector<OffsetSetter>::iterator os=m_offset_setters.begin(),
         osend=m_offset_setters.end(); os != osend; ++os)
    {
        if (!os->m_bc)
            continue;
        os->m_thres = os->m_bc->next_offset();
        os->m_new_val = os->m_bc->get_offset();
        os->m_cur_val = os->m_new_val;
    }

    // Build up interval tree
    for (std::list<Span*>::iterator spani=m_spans.begin(),
         endspan=m_spans.end(); spani != endspan; ++spani)
    {
        Span* span = *spani;
        for (Span::Terms::iterator term=span->m_span_terms.begin(),
             endterm=span->m_span_terms.end(); term != endterm; ++term)
            itree_add(*span, *term);
    }

    // Look for cycles in times expansion (span.id==0)
    for (std::list<Span*>::iterator spani=m_spans.begin(),
         endspan=m_spans.end(); spani != endspan; ++spani)
    {
        Span* span = *spani;
        if (span->m_id > 0)
            continue;
        try
        {
            m_itree.enumerate(static_cast<long>(span->m_bc.get_index()),
                              static_cast<long>(span->m_bc.get_index()),
                              BIND::bind(&Optimize::check_cycle, this, _1,
                                         REF::ref(*span)));
        }
        catch (Error& err)
        {
            errwarns.propagate(span->m_bc.get_line(), err);
            saw_error = true;
        }
    }

    return saw_error;
}

bool
Optimize::step_2(Errwarns& errwarns)
{
    bool saw_error = false;

    while (!m_QA.empty() || !m_QB.empty())
    {
        Span* span;

        // QA is for TIMES, update those first, then update non-TIMES.
        // This is so that TIMES can absorb increases before we look at
        // expanding non-TIMES BCs.
        if (!m_QA.empty())
        {
            span = m_QA.front();
            m_QA.pop();
        }
        else
        {
            span = m_QB.front();
            m_QB.pop();
        }

        if (span->m_active == Span::INACTIVE)
            continue;
        span->m_active = Span::ACTIVE;  // no longer in Q

        // Make sure we ended up ultimately exceeding thresholds; due to
        // offset BCs we may have been placed on Q and then reduced in size
        // again.
        if (!span->recalc_normal())
            continue;

        unsigned long orig_len = span->m_bc.get_total_len();

        bool still_depend =
            expand(span->m_bc, span->m_id, span->m_cur_val, span->m_new_val,
                   &span->m_neg_thres, &span->m_pos_thres, errwarns);

        if (errwarns.num_errors() > 0)
        {
            // error
            saw_error = true;
            continue;
        }
        else if (still_depend)
        {
            // another threshold, keep active
            for (Span::Terms::iterator term=span->m_span_terms.begin(),
                 endterm=span->m_span_terms.end(); term != endterm; ++term)
                term->m_cur_val = term->m_new_val;
            span->m_cur_val = span->m_new_val;
        }
        else
            span->m_active = Span::INACTIVE;    // we're done with this span

        long len_diff = span->m_bc.get_total_len() - orig_len;
        if (len_diff == 0)
            continue;   // didn't increase in size

        // Iterate over all spans dependent across the bc just expanded
        m_itree.enumerate(static_cast<long>(span->m_bc.get_index()),
                          static_cast<long>(span->m_bc.get_index()),
                          BIND::bind(&Optimize::term_expand, this, _1,
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
               && os->m_bc->get_container() == span->m_bc.get_container()
               && offset_diff != 0)
        {
            unsigned long old_next_offset =
                os->m_cur_val + os->m_bc->get_total_len();

            assert((offset_diff >= 0 ||
                    static_cast<unsigned long>(-offset_diff) <= os->m_new_val)
                   && "org/align went to negative offset");
            os->m_new_val += offset_diff;

            orig_len = os->m_bc->get_tail_len();
            long neg_thres_temp, pos_thres_temp;
            expand(*os->m_bc, 1, static_cast<long>(os->m_cur_val),
                   static_cast<long>(os->m_new_val), &neg_thres_temp,
                   &pos_thres_temp, errwarns);
            os->m_thres = static_cast<long>(pos_thres_temp);

            offset_diff =
                os->m_new_val + os->m_bc->get_total_len() - old_next_offset;
            len_diff = os->m_bc->get_tail_len() - orig_len;
            if (len_diff != 0)
                m_itree.enumerate(static_cast<long>(os->m_bc->get_index()),
                                  static_cast<long>(os->m_bc->get_index()),
                                  BIND::bind(&Optimize::term_expand, this, _1,
                                             len_diff));

            os->m_cur_val = os->m_new_val;
            ++os;
        }
    }

    return saw_error;
}

} // anonymous namespace

namespace yasm {

void
Object::update_bc_offsets(Errwarns& errwarns)
{
    for (section_iterator sect=m_sections.begin(), end=m_sections.end();
         sect != end; ++sect)
        sect->update_offsets(errwarns);
}

void
Object::optimize(Errwarns& errwarns)
{
    Optimize opt;
    unsigned long bc_index = 0;
    bool saw_error = false;

    // Step 1a
    for (section_iterator sect=m_sections.begin(), sectend=m_sections.end();
         sect != sectend; ++sect)
    {
        unsigned long offset = 0;

        // Set the offset of the first (empty) bytecode.
        sect->bcs_first().set_index(bc_index++);
        sect->bcs_first().set_offset(0);

        // Iterate through the remainder, if any.
        for (Section::bc_iterator bc=sect->bcs_begin(), bcend=sect->bcs_end();
             bc != bcend; ++bc)
        {
            bc->set_index(bc_index++);
            bc->set_offset(offset);

            calc_len(*bc, BIND::bind(&Optimize::add_span, &opt,
                                     _1, _2, _3, _4, _5),
                     errwarns);
            if (errwarns.num_errors() > 0)
                saw_error = true;
            else
            {
                if (bc->get_special() == Bytecode::Contents::SPECIAL_OFFSET)
                    opt.add_offset_setter(*bc);

                offset = bc->next_offset();
            }
        }
    }

    if (saw_error)
        return;

    // Step 1b
    if (opt.step_1b(errwarns))
        return;

    // Step 1c
    update_bc_offsets(errwarns);
    if (errwarns.num_errors() > 0)
        return;

    // Step 1d
    if (opt.step_1d())
        return;

    // Step 1e
    if (opt.step_1e(errwarns))
        return;

    // Step 2
    if (opt.step_2(errwarns))
        return;

    // Step 3
    update_bc_offsets(errwarns);
}

} // namespace yasm

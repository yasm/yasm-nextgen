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
#include "util.h"

#include <vector>
#include <list>
#include <iomanip>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/ptr_container/ptr_list.hpp>

#include "bytecode.h"
#include "errwarn.h"
#include "expr.h"
#include "intnum.h"
#include "section.h"
#include "object.h"
#include "symbol.h"
#include "value.h"

#include "interval_tree.h"


namespace yasm {
#if 0
/* Wrapper around directive for HAMT insertion */
typedef struct yasm_directive_wrap {
    const yasm_directive *directive;
} yasm_directive_wrap;

/*
 * Standard "builtin" object directives.
 */

static void
dir_extern(yasm_object *object, yasm_valparamhead *valparams,
           yasm_valparamhead *objext_valparams, unsigned long line)
{
    yasm_valparam *vp = yasm_vps_first(valparams);
    yasm_symrec *sym;
    sym = yasm_symtab_declare(object->symtab, yasm_vp_id(vp), YASM_SYM_EXTERN,
                              line);
    if (objext_valparams) {
        yasm_valparamhead *vps = yasm_vps_create();
        *vps = *objext_valparams;   /* structure copy */
        yasm_vps_initialize(objext_valparams);  /* don't double-free */
        yasm_symrec_set_objext_valparams(sym, vps);
    }
}

static void
dir_global(yasm_object *object, yasm_valparamhead *valparams,
           yasm_valparamhead *objext_valparams, unsigned long line)
{
    yasm_valparam *vp = yasm_vps_first(valparams);
    yasm_symrec *sym;
    sym = yasm_symtab_declare(object->symtab, yasm_vp_id(vp), YASM_SYM_GLOBAL,
                              line);
    if (objext_valparams) {
        yasm_valparamhead *vps = yasm_vps_create();
        *vps = *objext_valparams;   /* structure copy */
        yasm_vps_initialize(objext_valparams);  /* don't double-free */
        yasm_symrec_set_objext_valparams(sym, vps);
    }
}

static void
dir_common(yasm_object *object, yasm_valparamhead *valparams,
           yasm_valparamhead *objext_valparams, unsigned long line)
{
    yasm_valparam *vp = yasm_vps_first(valparams);
    yasm_valparam *vp2 = yasm_vps_next(vp);
    yasm_expr *size = yasm_vp_expr(vp2, object->symtab, line);
    yasm_symrec *sym;

    if (!size) {
        yasm_error_set(YASM_ERROR_SYNTAX,
                       N_("no size specified in %s declaration"), "COMMON");
        return;
    }
    sym = yasm_symtab_declare(object->symtab, yasm_vp_id(vp), YASM_SYM_COMMON,
                              line);
    yasm_symrec_set_common_size(sym, size);
    if (objext_valparams) {
        yasm_valparamhead *vps = yasm_vps_create();
        *vps = *objext_valparams;   /* structure copy */
        yasm_vps_initialize(objext_valparams);  /* don't double-free */
        yasm_symrec_set_objext_valparams(sym, vps);
    }
}

static void
dir_section(yasm_object *object, yasm_valparamhead *valparams,
            yasm_valparamhead *objext_valparams, unsigned long line)
{
    yasm_section *new_section =
        yasm_objfmt_section_switch(object, valparams, objext_valparams, line);
    if (new_section)
        object->cur_section = new_section;
    else
        yasm_error_set(YASM_ERROR_SYNTAX,
                       N_("invalid argument to directive `%s'"), "SECTION");
}

static const yasm_directive object_directives[] = {
    { ".extern",        "gas",  dir_extern,     YASM_DIR_ID_REQUIRED },
    { ".global",        "gas",  dir_global,     YASM_DIR_ID_REQUIRED },
    { ".globl",         "gas",  dir_global,     YASM_DIR_ID_REQUIRED },
    { "extern",         "nasm", dir_extern,     YASM_DIR_ID_REQUIRED },
    { "global",         "nasm", dir_global,     YASM_DIR_ID_REQUIRED },
    { "common",         "nasm", dir_common,     YASM_DIR_ID_REQUIRED },
    { "section",        "nasm", dir_section,    YASM_DIR_ARG_REQUIRED },
    { "segment",        "nasm", dir_section,    YASM_DIR_ARG_REQUIRED },
    { NULL, NULL, NULL, 0 }
};

static void
directive_level2_delete(/*@only@*/ void *data)
{
    yasm_xfree(data);
}

static void
directive_level1_delete(/*@only@*/ void *data)
{
    HAMT_destroy(data, directive_level2_delete);
}

static void
directives_add(yasm_object *object, /*@null@*/ const yasm_directive *dir)
{
    if (!dir)
        return;

    while (dir->name) {
        HAMT *level2 = HAMT_search(object->directives, dir->parser);
        int replace;
        yasm_directive_wrap *wrap = yasm_xmalloc(sizeof(yasm_directive_wrap));

        if (!level2) {
            replace = 0;
            level2 = HAMT_insert(object->directives, dir->parser,
                                 HAMT_create(1, yasm_internal_error_),
                                 &replace, directive_level1_delete);
        }
        replace = 0;
        wrap->directive = dir;
        HAMT_insert(level2, dir->name, wrap, &replace,
                    directive_level2_delete);
        dir++;
    }
}
#endif

#if 0
Object::Object(const std::string& src_filename,
               const std::string& obj_filename,
               std::auto_ptr<Arch> arch,
               const ObjectFormatModule* objfmt_module,
               const DebugFormatModule* dbgfmt_module)
    : m_src_filename(src_filename),
      m_obj_filename(obj_filename),
      m_arch(arch.release()),
      m_objfmt(0),
      m_dbgfmt(0),
      m_cur_section(0),
      m_sections(1)         // reserve space for first section
{
#if 0
    // Create empty symbol table
    object->symtab = yasm_symtab_create();

    // Create directives HAMT
    object->directives = HAMT_create(1, yasm_internal_error_);
#endif
#if 0
    // Initialize the object format
    m_objfmt.reset(yasm_objfmt_create(objfmt_module, object));
    if (!object->objfmt) {
        yasm_error_set(YASM_ERROR_GENERAL,
            N_("object format `%s' does not support architecture `%s' machine `%s'"),
            objfmt_module->keyword, ((yasm_arch_base *)arch)->module->keyword,
            yasm_arch_get_machine(arch));
        goto error;
    }

    /* Get a fresh copy of objfmt_module as it may have changed. */
    objfmt_module = ((yasm_objfmt_base *)object->objfmt)->module;

    // Add an initial "default" section to object
    m_cur_section = m_objfmt->add_default_section(this);
#endif
#if 0
    /* Check to see if the requested debug format is in the allowed list
     * for the active object format.
     */
    bool matched = false;
    for (int i=0; !matched && objfmt_module->dbgfmt_keywords[i]; i++)
        if (yasm__strcasecmp(objfmt_module->dbgfmt_keywords[i],
                             dbgfmt_module->keyword) == 0)
            matched = true;
    if (!matched) {
        yasm_error_set(YASM_ERROR_GENERAL,
            N_("`%s' is not a valid debug format for object format `%s'"),
            dbgfmt_module->keyword, objfmt_module->keyword);
    }

    /* Initialize the debug format */
    object->dbgfmt = yasm_dbgfmt_create(dbgfmt_module, object);
    if (!object->dbgfmt) {
        yasm_error_set(YASM_ERROR_GENERAL,
            N_("debug format `%s' does not work with object format `%s'"),
            dbgfmt_module->keyword, objfmt_module->keyword);
    }
#endif
#if 0
    /* Add directives to HAMT.  Note ordering here determines priority. */
    directives_add(object,
                   ((yasm_objfmt_base *)object->objfmt)->module->directives);
    directives_add(object,
                   ((yasm_dbgfmt_base *)object->dbgfmt)->module->directives);
    directives_add(object,
                   ((yasm_arch_base *)object->arch)->module->directives);
    directives_add(object, object_directives);
#endif
}
#endif
void
Object::append_section(std::auto_ptr<Section> sect)
{
    sect->m_object = this;
    m_sections.push_back(sect.release());
}
#if 0
int
yasm_object_directive(yasm_object *object, const char *name,
                      const char *parser, yasm_valparamhead *valparams,
                      yasm_valparamhead *objext_valparams,
                      unsigned long line)
{
    HAMT *level2;
    yasm_directive_wrap *wrap;

    level2 = HAMT_search(object->directives, parser);
    if (!level2)
        return 1;

    wrap = HAMT_search(level2, name);
    if (!wrap)
        return 1;

    yasm_call_directive(wrap->directive, object, valparams, objext_valparams,
                        line);
    return 0;
}
#endif
void
Object::set_source_fn(const std::string& src_filename)
{
    m_src_filename = src_filename;
}
#if 0
Object::~Object()
{
}
#endif
void
Object::put(std::ostream& os, int indent_level) const
{
#if 0
    /* Print symbol table */
    fprintf(f, "%*sSymbol Table:\n", indent_level, "");
    yasm_symtab_print(object->symtab, f, indent_level+1);
#endif
    // Print sections and bytecodes
    for (const_section_iterator i=m_sections.begin(), end=m_sections.end();
         i != end; ++i) {
        os << std::setw(indent_level) << "" << "Section:" << std::endl;
        i->put(os, indent_level+1, true);
    }
}

void
Object::finalize(Errwarns& errwarns)
{
    std::for_each(m_sections.begin(), m_sections.end(),
                  boost::bind(&Section::finalize, _1, boost::ref(errwarns)));
}

Section*
Object::get_section_by_name(const std::string& name)
{
    section_iterator i =
        std::find_if(m_sections.begin(), m_sections.end(),
                     boost::bind(&Section::is_name, _1, boost::ref(name)));
    if (i == m_sections.end())
        return 0;
    return &(*i);
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
namespace {

using namespace yasm;

class OffsetSetter {
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

class Span : private boost::noncopyable {
    friend class Optimize;
public:
    class Term {
    public:
        Term();
        Term(unsigned int subst, Bytecode* precbc, Bytecode* precbc2,
             Span* span, long new_val);
        ~Term() {}

        Bytecode* m_precbc;
        Bytecode* m_precbc2;
        Span* m_span;       // span this term is a member of
        long m_cur_val;
        long m_new_val;
        unsigned int m_subst;
    };

    Span(Bytecode& bc, int id, const Value& value, 
         long neg_thres, long pos_thres, size_t os_index);
    ~Span();

    void create_terms();
    bool recalc_normal();

private:
    void add_term(unsigned int subst, Bytecode& precbc, Bytecode& precbc2);

    Bytecode& m_bc;

    Value m_depval;

    // span term for relative portion of value
    boost::scoped_ptr<Term> m_rel_term;
    // span terms in absolute portion of value
    typedef std::vector<Term> Terms;
    Terms m_span_terms;
    Expr::Terms m_expr_terms;

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

class Optimize {
public:
    Optimize();
    ~Optimize();
    void add_span(Bytecode& bc, int id, const Value& value,
                  long neg_thres, long pos_thres);
    void add_offset_setter(Bytecode& bc);

    bool step_1b(Errwarns& errwarns);
    bool step_1d();
    bool step_1e(Errwarns& errwarns);
    bool step_2(Errwarns& errwarns);

private:
    void itree_add(Span& span, Span::Term& term);
    void check_cycle(IntervalTreeNode<Span::Term*> * node, Span& span);
    void term_expand(IntervalTreeNode<Span::Term*> * node, long len_diff);

    boost::ptr_list<Span> m_spans;
    std::list<Span*> m_QA, m_QB;
    IntervalTree<Span::Term*> m_itree;
    std::vector<OffsetSetter> m_offset_setters;
};

Span::Term::Term()
    : m_precbc(0),
      m_precbc2(0),
      m_span(0),
      m_cur_val(0),
      m_new_val(0),
      m_subst(0)
{
}

Span::Term::Term(unsigned int subst, Bytecode* precbc, Bytecode* precbc2,
                 Span* span, long new_val)
    : m_precbc(precbc),
      m_precbc2(precbc2),
      m_span(span),
      m_cur_val(0),
      m_new_val(new_val),
      m_subst(subst)
{
}

Span::Span(Bytecode& bc, int id, /*@null@*/ const Value& value, 
           long neg_thres, long pos_thres, size_t os_index)
    : m_bc(bc),
      m_depval(value),
      m_rel_term(0),
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
Optimize::add_span(Bytecode& bc, int id, const Value& value,
                   long neg_thres, long pos_thres)
{
    m_spans.push_back(new Span(bc, id, value, neg_thres, pos_thres,
                               m_offset_setters.size()-1));
}

void
Span::add_term(unsigned int subst, Bytecode& precbc, Bytecode& precbc2)
{
    IntNum intn;
    if (!calc_bc_dist(precbc, precbc2, intn))
        throw InternalError(N_("could not calculate bc distance"));

    if (subst >= m_span_terms.size())
        m_span_terms.resize(subst+1);
    m_span_terms[subst] = Term(subst, &precbc, &precbc2, this, intn.get_int());
}

void
Span::create_terms()
{
    // Split out sym-sym terms in absolute portion of dependent value
    if (m_depval.has_abs()) {
        subst_bc_dist(m_depval.get_abs(),
                      boost::bind(&Span::add_term, this, _1, _2, _3));
        if (m_span_terms.size() > 0) {
            for (Terms::iterator i=m_span_terms.begin(),
                 end=m_span_terms.end(); i != end; ++i) {
                // Create expression terms with dummy value
                m_expr_terms.push_back(new IntNum(0));

                // Check for circular references
                if (m_id <= 0 &&
                    ((m_bc.get_index() > i->m_precbc->get_index() &&
                      m_bc.get_index() <= i->m_precbc2->get_index()) ||
                     (m_bc.get_index() > i->m_precbc2->get_index() &&
                      m_bc.get_index() <= i->m_precbc->get_index())))
                    throw ValueError(N_("circular reference detected"));
            }
        }
    }

    // Create term for relative portion of dependent value
    if (m_depval.m_rel) {
        Bytecode* rel_precbc;
        bool sym_local = m_depval.m_rel->get_label(rel_precbc);

        if (m_depval.is_wrt() || m_depval.m_seg_of || m_depval.m_section_rel
            || !sym_local)
            return;     // we can't handle SEG, WRT, or external symbols
        if (rel_precbc->get_section() != m_bc.get_section())
            return;     // not in this section
        if (!m_depval.m_curpos_rel)
            return;     // not PC-relative

        m_rel_term.reset(new Term(~0U, 0, rel_precbc, this,
                                  rel_precbc->next_offset() -
                                  m_bc.get_offset()));
    }
}

// Recalculate span value based on current span replacement values.
// Returns True if span needs expansion (e.g. exceeded thresholds).
bool
Span::recalc_normal()
{
    m_new_val = 0;

    if (m_depval.has_abs()) {
        boost::scoped_ptr<Expr> abs_copy(m_depval.get_abs()->clone());

        // Update sym-sym terms and substitute back into expr
        for (Terms::iterator i=m_span_terms.begin(), end=m_span_terms.end();
             i != end; ++i)
            m_expr_terms[i->m_subst].get_int()->set(i->m_new_val);
        abs_copy->substitute(m_expr_terms);
        if (const IntNum* num = abs_copy->get_intnum())
            m_new_val = num->get_int();
        else
            m_new_val = LONG_MAX;   // too complex; force to longest form
    }

    if (m_rel_term) {
        if (m_new_val != LONG_MAX && m_rel_term->m_new_val != LONG_MAX)
            m_new_val += m_rel_term->m_new_val >> m_depval.m_rshift;
        else
            m_new_val = LONG_MAX;   // too complex; force to longest form
    } else if (m_depval.is_relative())
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
    if (term.m_precbc)
        precbc_index = term.m_precbc->get_index();
    else
        precbc_index = span.m_bc.get_index()-1;

    if (term.m_precbc2)
        precbc2_index = term.m_precbc2->get_index();
    else
        precbc2_index = span.m_bc.get_index()-1;

    if (precbc_index < precbc2_index) {
        low = precbc_index+1;
        high = precbc2_index;
    } else if (precbc_index > precbc2_index) {
        low = precbc2_index+1;
        high = precbc_index;
    } else
        return;     /* difference is same bc - always 0! */

    m_itree.insert((long)low, (long)high, &term);
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
    if (term->m_precbc)
        precbc_index = term->m_precbc->get_index();
    else
        precbc_index = span->m_bc.get_index()-1;

    if (term->m_precbc2)
        precbc2_index = term->m_precbc2->get_index();
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
        m_QA.push_back(span);
    else
        m_QB.push_back(span);
    span->m_active = Span::ON_Q;    // Mark as being in Q
}

bool
Optimize::step_1b(Errwarns& errwarns)
{
    bool saw_error = false;

    boost::ptr_list<Span>::iterator span = m_spans.begin();
    while (span != m_spans.end()) {
        bool terms_okay = true;
        try {
            span->create_terms();
        } catch (Error& err) {
            errwarns.propagate(span->m_bc.get_line());
            saw_error = true;
            terms_okay = false;
        }
        if (terms_okay && span->recalc_normal()) {
            bool still_depend =
                span->m_bc.expand(span->m_id, span->m_cur_val, span->m_new_val,
                                  span->m_neg_thres, span->m_pos_thres,
                                  errwarns);
            if (errwarns.num_errors() > 0)
                saw_error = true;
            else if (still_depend) {
                if (span->m_active == Span::INACTIVE) {
                    errwarns.propagate(span->m_bc.get_line(),
                        ValueError(N_("secondary expansion of an external/complex value")));
                    saw_error = true;
                }
            } else {
                span = m_spans.erase(span);
                continue;
            }
        }
        span->m_cur_val = span->m_new_val;
        ++span;
    }

    return saw_error;
}

bool
Optimize::step_1d()
{
    for (boost::ptr_list<Span>::iterator span=m_spans.begin(),
         endspan=m_spans.end(); span != endspan; ++span) {
        // Update span terms based on new bc offsets
        for (Span::Terms::iterator term=span->m_span_terms.begin(),
             endterm=span->m_span_terms.end(); term != endterm; ++term) {
            IntNum intn;
            if (!calc_bc_dist(*(term->m_precbc), *(term->m_precbc2), intn))
                throw InternalError(N_("could not calculate bc distance"));
            term->m_cur_val = term->m_new_val;
            term->m_new_val = intn.get_int();
        }
        if (span->m_rel_term) {
            span->m_rel_term->m_cur_val = span->m_rel_term->m_new_val;
            if (span->m_rel_term->m_precbc2)
                span->m_rel_term->m_new_val =
                    span->m_rel_term->m_precbc2->next_offset() -
                    span->m_bc.get_offset();
            else
                span->m_rel_term->m_new_val = span->m_bc.get_offset() -
                    span->m_rel_term->m_precbc->next_offset();
        }

        if (span->recalc_normal()) {
            // Exceeded threshold, add span to QB
            m_QB.push_back(&(*span));
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
         osend=m_offset_setters.end(); os != osend; ++os) {
        if (!os->m_bc)
            continue;
        os->m_thres = os->m_bc->next_offset();
        os->m_new_val = os->m_bc->get_offset();
        os->m_cur_val = os->m_new_val;
    }

    // Build up interval tree
    for (boost::ptr_list<Span>::iterator span=m_spans.begin(),
         endspan=m_spans.end(); span != endspan; ++span) {
        for (Span::Terms::iterator term=span->m_span_terms.begin(),
             endterm=span->m_span_terms.end(); term != endterm; ++term)
            itree_add(*span, *term);
        if (span->m_rel_term)
            itree_add(*span, *(span->m_rel_term.get()));
    }

    // Look for cycles in times expansion (span.id==0)
    for (boost::ptr_list<Span>::iterator span=m_spans.begin(),
         endspan=m_spans.end(); span != endspan; ++span) {
        if (span->m_id > 0)
            continue;
        try {
            m_itree.enumerate((long)span->m_bc.get_index(),
                              (long)span->m_bc.get_index(),
                              boost::bind(&Optimize::check_cycle, this, _1,
                                          boost::ref(*span)));
        } catch (Error& err) {
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

    while (!m_QA.empty() || !m_QB.empty()) {
        Span* span;

        // QA is for TIMES, update those first, then update non-TIMES.
        // This is so that TIMES can absorb increases before we look at
        // expanding non-TIMES BCs.
        if (!m_QA.empty()) {
            span = m_QA.front();
            m_QA.pop_front();
        } else {
            span = m_QB.front();
            m_QB.pop_front();
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
            span->m_bc.expand(span->m_id, span->m_cur_val, span->m_new_val,
                              span->m_neg_thres, span->m_pos_thres, errwarns);

        if (errwarns.num_errors() > 0) {
            // error
            saw_error = true;
            continue;
        } else if (still_depend) {
            // another threshold, keep active
            for (Span::Terms::iterator term=span->m_span_terms.begin(),
                 endterm=span->m_span_terms.end(); term != endterm; ++term)
                term->m_cur_val = term->m_new_val;
            if (span->m_rel_term)
                span->m_rel_term->m_cur_val = span->m_rel_term->m_new_val;
            span->m_cur_val = span->m_new_val;
        } else
            span->m_active = Span::INACTIVE;    // we're done with this span

        long len_diff = span->m_bc.get_total_len() - orig_len;
        if (len_diff == 0)
            continue;   // didn't increase in size

        // Iterate over all spans dependent across the bc just expanded
        m_itree.enumerate((long)span->m_bc.get_index(),
                          (long)span->m_bc.get_index(),
                          boost::bind(&Optimize::term_expand, this, _1,
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
               && os->m_bc->get_section() == span->m_bc.get_section()
               && offset_diff != 0) {
            unsigned long old_next_offset =
                os->m_cur_val + os->m_bc->get_len();

            if (offset_diff < 0
                && (unsigned long)(-offset_diff) > os->m_new_val)
                throw InternalError(N_("org/align went to negative offset"));
            os->m_new_val += offset_diff;

            orig_len = os->m_bc->get_len();
            long neg_thres_temp, pos_thres_temp;
            os->m_bc->expand(1, (long)os->m_cur_val, (long)os->m_new_val,
                             neg_thres_temp, pos_thres_temp, errwarns);
            os->m_thres = (long)pos_thres_temp;

            offset_diff = os->m_new_val + os->m_bc->get_len() - old_next_offset;
            len_diff = os->m_bc->get_len() - orig_len;
            if (len_diff != 0)
                m_itree.enumerate((long)os->m_bc->get_index(),
                                  (long)os->m_bc->get_index(),
                                  boost::bind(&Optimize::term_expand, this, _1,
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
        sect->update_bc_offsets(errwarns);
}

void
Object::optimize(Errwarns& errwarns)
{
    Optimize opt;
    unsigned long bc_index = 0;
    bool saw_error = false;

    // Step 1a
    for (section_iterator sect=m_sections.begin(), sectend=m_sections.end();
         sect != sectend; ++sect) {
        unsigned long offset = 0;

        // Set the offset of the first (empty) bytecode.
        sect->bcs_first().set_index(bc_index++);

        // Iterate through the remainder, if any.
        for (Section::bc_iterator bc=sect->bcs_begin(), bcend=sect->bcs_end();
             bc != bcend; ++bc) {
            bc->set_index(bc_index++);
            bc->set_offset(offset);

            bc->calc_len(boost::bind(&Optimize::add_span, &opt,
                                     _1, _2, _3, _4, _5),
                         errwarns);
            if (errwarns.num_errors() > 0)
                saw_error = true;
            else {
                if (bc->get_special() == Bytecode::Contents::SPECIAL_OFFSET) {
                    opt.add_offset_setter(*bc);

                    if (bc->get_multiple_expr()) {
                        errwarns.propagate(bc->get_line(),
                            ValueError(N_("cannot combine multiples and setting assembly position")));
                        saw_error = true;
                    }
                }

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

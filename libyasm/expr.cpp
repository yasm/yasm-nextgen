//
// Expression handling
//
//  Copyright (C) 2001-2007  Michael Urman, Peter Johnson
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

#include "errwarn.h"
#include "intnum.h"
#include "floatnum.h"
#include "expr.h"
//#include "symrec.h"

//#include "bytecode.h"
//#include "section.h"

//#include "arch.h"

#include <cstring>
#include <algorithm>
#include <boost/bind.hpp>
#include <boost/ref.hpp>

namespace {

using namespace yasm;

/// Look for simple identities that make the entire result constant:
/// 0*&x, -1|x, etc.
bool
is_constant(Expr::Op op, const IntNum* intn)
{
    bool iszero = intn->is_zero();
    return ((iszero && op == Expr::MUL) ||
            (iszero && op == Expr::AND) ||
            (iszero && op == Expr::LAND) ||
            (intn->is_neg1() && op == Expr::OR));
}

/// Look for simple "left" identities like 0+x, 1*x, etc.
bool
can_destroy_int_left(Expr::Op op, const IntNum* intn)
{
    bool iszero = intn->is_zero();
    return ((intn->is_pos1() && op == Expr::MUL) ||
            (iszero && op == Expr::ADD) ||
            (intn->is_neg1() && op == Expr::AND) ||
            (!iszero && op == Expr::LAND) ||
            (iszero && op == Expr::OR) ||
            (iszero && op == Expr::LOR));
}

/// Look for simple "right" identities like x+|-0, x*&/1
bool
can_destroy_int_right(Expr::Op op, const IntNum* intn)
{
    int iszero = intn->is_zero();
    int ispos1 = intn->is_pos1();
    return ((ispos1 && op == Expr::MUL) ||
            (ispos1 && op == Expr::DIV) ||
            (iszero && op == Expr::ADD) ||
            (iszero && op == Expr::SUB) ||
            (intn->is_neg1() && op == Expr::AND) ||
            (!iszero && op == Expr::LAND) ||
            (iszero && op == Expr::OR) ||
            (iszero && op == Expr::LOR) ||
            (iszero && op == Expr::SHL) ||
            (iszero && op == Expr::SHR));
}

} // anonymous namespace

namespace yasm {

Expr::Term
Expr::Term::clone() const
{
    switch (m_type) {
        case INT:   return m_data.intn->clone();
        case FLOAT: return m_data.flt->clone();
        case EXPR:  return m_data.expr->clone();
        default:    return *this;
    }
}

void
Expr::Term::destroy()
{
    switch (m_type) {
        case INT:
            delete m_data.intn;
            m_data.intn = 0;
            break;
        case FLOAT:
            delete m_data.flt;
            m_data.flt = 0;
            break;
        case EXPR:
            delete m_data.expr;
            m_data.expr = 0;
            break;
        default:
            break;
    }
}

void
Expr::add_term(const Term& term)
{
    Expr* base_e = term.get_expr();
    if (!base_e) {
        m_terms.push_back(term);
        return;
    }

    Expr* e = base_e;
    Expr* copyfrom = 0;

    // Search downward until we find something *other* than an
    // IDENT, then bring it up to the current level.
    for (;;) {
        if (e->m_op != IDENT)
            break;
        copyfrom = e;

        if (e->m_terms.size() != 1)
            break;

        Expr* sube = e->m_terms.front().get_expr();
        if (!sube)
            break;

        e = sube;
    }

    if (!copyfrom) {
        m_terms.push_back(base_e);
    } else {
        // Transfer the terms up
        m_terms.insert(m_terms.end(), copyfrom->m_terms.begin(),
                       copyfrom->m_terms.end());
        copyfrom->m_terms.clear();
        // Delete the rest
        delete base_e;
    }
}

Expr::Expr(const Term& a, Op op, const Term& b, unsigned long line)
    : m_op(op), m_line(line)
{
    add_term(a);
    add_term(b);
}

Expr::Expr(Op op, const Term& a, unsigned long line)
    : m_op(op), m_line(line)
{
    add_term(a);
}

Expr::Expr(const Term& a, unsigned long line)
    : m_op(IDENT), m_line(line)
{
    add_term(a);
}

Expr&
Expr::operator= (const Expr& rhs)
{
    m_op = rhs.m_op;
    m_line = rhs.m_line;
    std::transform(rhs.m_terms.begin(), rhs.m_terms.end(), m_terms.begin(),
                   boost::mem_fn(&Term::clone));
    return *this;
}

Expr::Expr(const Expr& e)
    : m_op(e.m_op), m_line(e.m_line)
{
    std::transform(e.m_terms.begin(), e.m_terms.end(), m_terms.begin(),
                   boost::mem_fn(&Term::clone));
}

Expr::Expr(Op op, unsigned long line)
    : m_op(op), m_line(line)
{
}

Expr::~Expr()
{
    std::for_each(m_terms.begin(), m_terms.end(), boost::mem_fn(&Term::destroy));
}
#if 0
// Transforms instances of symrec-symrec [symrec+(-1*symrec)] into single
// exprterms if possible.  Uses a simple n^2 algorithm because n is usually
// quite small.  Also works for precbc-precbc (or symrec-precbc,
// precbc-symrec).
static /*@only@*/ yasm_expr *
expr_xform_bc_dist_base(/*@returned@*/ /*@only@*/ yasm_expr *e,
                        /*@null@*/ void *cbd,
                        int (*callback) (yasm_expr__term *ei,
                                         yasm_bytecode *precbc,
                                         yasm_bytecode *precbc2,
                                         void *cbd))
{
    int i;
    /*@dependent@*/ yasm_section *sect;
    /*@dependent@*/ /*@null@*/ yasm_bytecode *precbc;
    int numterms;

    // Handle symrec-symrec in ADD exprs by looking for (-1*symrec) and
    // symrec term pairs (where both symrecs are in the same segment).
    if (e->op != ADD)
        return e;

    for (i=0; i<e->numterms; i++) {
        int j;
        yasm_expr *sube;
        yasm_intnum *intn;
        yasm_symrec *sym = NULL;
        /*@dependent@*/ yasm_section *sect2;
        /*@dependent@*/ /*@null@*/ yasm_bytecode *precbc2;

        // First look for an (-1*symrec) term
        if (e->terms[i].type != EXPR)
            continue;
        sube = e->terms[i].data.expn;
        if (sube->op != MUL || sube->numterms != 2)
            continue;

        if (sube->terms[0].type == INT &&
            (sube->terms[1].type == SYM ||
             sube->terms[1].type == PRECBC)) {
            intn = sube->terms[0].data.intn;
            if (sube->terms[1].type == PRECBC)
                precbc = sube->terms[1].data.precbc;
            else
                sym = sube->terms[1].data.sym;
        } else if ((sube->terms[0].type == SYM ||
                    sube->terms[0].type == PRECBC) &&
                   sube->terms[1].type == INT) {
            if (sube->terms[0].type == PRECBC)
                precbc = sube->terms[0].data.precbc;
            else
                sym = sube->terms[0].data.sym;
            intn = sube->terms[1].data.intn;
        } else
            continue;

        if (!yasm_intnum_is_neg1(intn))
            continue;

        if (sym && !yasm_symrec_get_label(sym, &precbc))
            continue;
        sect2 = yasm_bc_get_section(precbc);

        // Now look for a symrec term in the same segment
        for (j=0; j<e->numterms; j++) {
            if (((e->terms[j].type == SYM &&
                  yasm_symrec_get_label(e->terms[j].data.sym, &precbc2)) ||
                 (e->terms[j].type == PRECBC &&
                  (precbc2 = e->terms[j].data.precbc))) &&
                (sect = yasm_bc_get_section(precbc2)) &&
                sect == sect2 &&
                callback(&e->terms[j], precbc, precbc2, cbd)) {
                // Delete the matching (-1*symrec) term
                yasm_expr_destroy(sube);
                e->terms[i].type = NONE;
                break;  // stop looking for matching symrec term
            }
        }
    }

    // Clean up any deleted (NONE) terms
    numterms = 0;
    for (i=0; i<e->numterms; i++) {
        if (e->terms[i].type != NONE)
            e->terms[numterms++] = e->terms[i]; // structure copy
    }
    if (e->numterms != numterms) {
        e->numterms = numterms;
        e = yasm_xrealloc(e, sizeof(yasm_expr)+((numterms<2) ? 0 :
                          sizeof(yasm_expr__term)*(numterms-2)));
        if (numterms == 1)
            e->op = IDENT;
    }

    return e;
}

static int
expr_xform_bc_dist_cb(yasm_expr__term *ei, yasm_bytecode *precbc,
                      yasm_bytecode *precbc2, /*@null@*/ void *d)
{
    yasm_intnum *dist = yasm_calc_bc_dist(precbc, precbc2);
    if (!dist)
        return 0;
    // Change the term to an integer
    ei->type = INT;
    ei->data.intn = dist;
    return 1;
}
#endif
/// Transforms instances of symrec-symrec [symrec+(-1*symrec)] into integers
/// if possible.
inline void
Expr::xform_bc_dist()
{
#if 0
    xform_bc_dist_base(expr_xform_bc_dist_cb);
#endif
}
#if 0
typedef struct bc_dist_subst_cbd {
    void (*callback) (unsigned int subst, yasm_bytecode *precbc,
                      yasm_bytecode *precbc2, void *cbd);
    void *cbd;
    unsigned int subst;
} bc_dist_subst_cbd;

static int
expr_bc_dist_subst_cb(yasm_expr__term *ei, yasm_bytecode *precbc,
                      yasm_bytecode *precbc2, /*@null@*/ void *d)
{
    bc_dist_subst_cbd *my_cbd = d;
    assert(my_cbd != NULL);
    // Call higher-level callback
    my_cbd->callback(my_cbd->subst, precbc, precbc2, my_cbd->cbd);
    // Change the term to an subst
    ei->type = SUBST;
    ei->data.subst = my_cbd->subst;
    my_cbd->subst++;
    return 1;
}

static yasm_expr *
expr_xform_bc_dist_subst(yasm_expr *e, void *d)
{
    return expr_xform_bc_dist_base(e, d, expr_bc_dist_subst_cb);
}

int
yasm_expr__bc_dist_subst(yasm_expr **ep, void *cbd,
                         void (*callback) (unsigned int subst,
                                           yasm_bytecode *precbc,
                                           yasm_bytecode *precbc2,
                                           void *cbd))
{
    bc_dist_subst_cbd my_cbd;   // callback info for low-level callback
    my_cbd.callback = callback;
    my_cbd.cbd = cbd;
    my_cbd.subst = 0;
    *ep = yasm_expr__level_tree(*ep, 1, 1, 1, 0, &expr_xform_bc_dist_subst,
                                &my_cbd);
    return my_cbd.subst;
}
#endif
/// Negate just a single ExprTerm by building a -1*ei subexpression.
void
Expr::xform_neg_term(Terms::iterator term)
{
    Expr *sube = new Expr(MUL, m_line);
    sube->m_terms.push_back(new IntNum(-1));
    sube->m_terms.push_back(*term);
    *term = sube;
}

/// Negates e by multiplying by -1, with distribution over lower-precedence
/// operators (eg ADD) and special handling to simplify result w/ADD, NEG,
/// and others.
void
Expr::xform_neg_helper()
{
    switch (m_op) {
        case ADD:
            // distribute (recursively if expr) over terms
            for (Terms::iterator i=m_terms.begin(), end=m_terms.end();
                 i != end; ++i) {
                if (Expr* sube = i->get_expr())
                    sube->xform_neg_helper();
                else
                    xform_neg_term(i);
            }
            break;
        case SUB:
            // change op to ADD, and recursively negate left side (if expr)
            m_op = ADD;
            if (Expr* sube = m_terms.front().get_expr())
                sube->xform_neg_helper();
            else
                xform_neg_term(m_terms.begin());
            break;
        case NEG:
            // Negating a negated value?  Make it an IDENT.
            m_op = IDENT;
            break;
        case IDENT:
        {
            // Negating an ident?  Change it into a MUL w/ -1 if there's no
            // floatnums present below; if there ARE floatnums, recurse.
            Term& first = m_terms.front();
            Expr* e;
            if (FloatNum* flt = first.get_float())
                flt->calc(NEG);
            else if (IntNum* intn = first.get_int())
                intn->calc(NEG);
            else if ((e = first.get_expr()) && e->contains(FLOAT))
                    e->xform_neg_helper();
            else {
                m_op = MUL;
                m_terms.push_back(new IntNum(-1));
            }
            break;
        }
        default:
            // Everything else.  MUL will be combined when it's leveled.
            // Replace ourselves with -1*e.
            Expr *ne = new Expr(m_op, m_line);
            m_op = MUL;
            m_terms.swap(ne->m_terms);
            m_terms.push_back(new IntNum(-1));
            m_terms.push_back(ne);
            break;
    }
}

/// Transforms negatives into expressions that are easier to combine:
/// -x -> -1*x
/// a-b -> a+(-1*b)
///
/// Call post-order on an expression tree to transform the entire tree.
void
Expr::xform_neg()
{
    switch (m_op) {
        case NEG:
            // Turn -x into -1*x
            m_op = IDENT;
            xform_neg_helper();
            break;
        case SUB:
        {
            // Turn a-b into a+(-1*b)
            // change op to ADD, and recursively negate right side (if expr)
            m_op = ADD;
            Terms::iterator rhs = m_terms.begin()+1;
            if (Expr* sube = rhs->get_expr())
                sube->xform_neg_helper();
            else
                xform_neg_term(rhs);
            break;
        }
        default:
            break;
    }
}

/// Check for and simplify identities.  Returns new number of expr terms.
/// Sets e->op = IDENT if numterms ends up being 1.
/// Uses numterms parameter instead of e->numterms for basis of "new" number
/// of terms.
/// Assumes int_term is *only* integer term in e.
/// @note Really designed to only be used by level_op().
void
Expr::simplify_identity(IntNum* &intn, bool simplify_reg_mul)
{
    IntNum* first = m_terms.front().get_int();
    bool is_first = (first && intn == first);

    if (m_terms.size() > 1) {
        // Check for simple identities that delete the intnum.
        // Don't do this step if it's 1*REG.
        if ((simplify_reg_mul || m_op != MUL || !intn->is_pos1() ||
             !contains(REG)) &&
            ((is_first && can_destroy_int_left(m_op, intn)) ||
             (!is_first && can_destroy_int_right(m_op, intn)))) {
            // delete int term
            m_terms.erase(std::find_if(m_terms.begin(), m_terms.end(),
                                       boost::bind(&Term::is_type, _1, INT)));
            delete intn;
            intn = 0;
        }
        // Check for simple identites that delete everything BUT the intnum.
        else if (is_constant(m_op, intn)) {
            // Delete everything but the integer term
            Terms terms;
            Terms::iterator i;
            i = std::find_if(m_terms.begin(), m_terms.end(),
                             boost::bind(&Term::is_type, _1, INT));
            terms.push_back(*i);
            i->release(); // don't delete it now we've moved it
            m_terms.swap(terms);
            intn = m_terms.front().get_int();
            // delete old terms
            std::for_each(terms.begin(), terms.end(),
                          boost::mem_fn(&Term::destroy));
        }
    }

    // Compute NOT, NEG, and LNOT on single intnum.
    if (m_terms.size() == 1 && is_first &&
        (m_op == NOT || m_op == NEG || m_op == LNOT))
        intn->calc(m_op);

    // Change expression to IDENT if possible.
    if (m_terms.size() == 1)
        m_op = IDENT;
}

/// Levels the expression tree.  Eg:
/// a+(b+c) -> a+b+c
/// (a+b)+(c+d) -> a+b+c+d
/// Naturally, only levels operators that allow more than two operand terms.
/// @note Only does *one* level of leveling (no recursion).  Should be called
///       post-order on a tree to combine deeper levels.
/// Also brings up any IDENT values into the current level (for ALL operators).
/// Folds (combines by evaluation) *integer* constant values if fold_const.
void
Expr::level_op(bool fold_const, bool simplify_ident, bool simplify_reg_mul)
{
    Terms::iterator first_int_term;
    IntNum* intn = 0;
    bool do_level = false;

    // First, bring up any IDENT'ed values.
    Expr* e = this;
    Expr* sube;
    while (e->m_op == IDENT && (sube = e->m_terms.front().get_expr()))
        e = sube;
    if (e != this) {
        m_op = e->m_op;
        m_terms.swap(e->m_terms);
        delete e;
    }

    // If non-numeric expression, don't fold constants.
    if (m_op > NONNUM)
        fold_const = false;

    for (Terms::iterator i=m_terms.begin(), end=m_terms.end(); i != end; ++i) {
        // Search downward until we find something *other* than an
        // IDENT, then bring it up to the current level.
        if ((e = i->get_expr())) {
            while (e && e->m_op == IDENT) {
                *i = e->m_terms.back();
                e->m_terms.pop_back();
                delete e;
                e = i->get_expr();
            }

            // Shortcut check for possible leveling later
            if (e && e->m_op == m_op)
                do_level = true;
        }

        // Find the first integer term (if one is present) if we're folding
        // constants and combine other integers with it.
        IntNum* intn_temp;
        if (fold_const && (intn_temp = i->get_int())) {
            if (!intn) {
                intn = intn_temp;
                first_int_term = i;
            } else {
                intn->calc(m_op, intn_temp);
                i->destroy();
            }
        }
    }

    if (intn) {
        // Erase folded integer terms; we already deleted their contents above
        Terms::iterator erasefrom =
            std::remove_if(first_int_term+1, m_terms.end(),
                           boost::bind(&Term::is_type, _1, INT));
        m_terms.erase(erasefrom, m_terms.end());

        // Simplify identities and make IDENT if possible.
        if (simplify_ident)
            simplify_identity(intn, simplify_reg_mul);
        else if (m_terms.size() == 1)
            m_op = IDENT;
    }

    // Only level operators that allow more than two operand terms.
    // Also don't bother leveling if it's not necessary to bring up any terms.
    if (!do_level || (m_op != ADD && m_op != MUL &&
                      m_op != OR && m_op != AND &&
                      m_op != LOR && m_op != LAND &&
                      m_op != LXOR && m_op != XOR)) {
        // trim capacity before returning
        Terms(m_terms).swap(m_terms);
        return;
    }

    // Copy up ExprTerms.  Combine integer terms as necessary.
    // This is a two-step process; we do this part in reverse order (to
    // use constant time operations), and then reverse the vector at the end.
    Terms terms;
    for (Terms::reverse_iterator i=m_terms.rbegin(), end=m_terms.rend();
         i != end; ++i) {
        if ((e = i->get_expr()) && e->m_op == m_op) {
            // move up terms, folding constants as we go
            while (!e->m_terms.empty()) {
                Term& last = e->m_terms.back();
                IntNum* intn_temp;
                if (fold_const && (intn_temp = last.get_int())) {
                    // Need to fold it in.. but if there's no int term
                    // already, just move this one up to become it.
                    if (intn) {
                        intn->calc(m_op, intn_temp);
                        last.destroy();
                    } else {
                        intn = intn_temp;
                        terms.push_back(last);
                    }
                } else
                    terms.push_back(last);
                e->m_terms.pop_back();
            }
            i->destroy();
        } else
            terms.push_back(*i);
    }
    std::reverse(terms.begin(), terms.end());
    m_terms.swap(terms);

    // Simplify identities, make IDENT if possible.
    if (simplify_ident && intn)
        simplify_identity(intn, simplify_reg_mul);
    else if (m_terms.size() == 1)
        m_op = IDENT;
}

void
Expr::expand_equ(std::vector<const Expr*>& seen)
{
#if 0
    for (Terms::iterator i=m_terms.begin(), end=m_terms.end(); i != end; ++i) {
        int type = i->type();
        // Expand equ's.
        const Expr *equ;
        if (type == SYM &&
            (equ = ((TermSymbol&) *i).m_data->get_equ())) {
            // Check for circular reference
            if (std::find(seen.begin(), seen.end(), equ) != seen.end()) {
                error_set(ERROR_TOO_COMPLEX,
                          N_("circular reference detected"));
                return;
            }

            Expr *newe = equ->clone();
            m_terms.replace(i, new TermExpr(newe));

            // Remember we saw this equ and recurse
            seen.push_back(equ);
            newe->expand_equ(seen);
            seen.pop_back();
        } else if (type == EXPR) {
            // Recurse
            ((TermExpr&) *i).m_data->expand_equ(seen);
        }
    }
#endif
}

void
Expr::level(bool fold_const,
            bool simplify_ident,
            bool simplify_reg_mul,
            bool calc_bc_dist,
            boost::function<void (Expr*)> xform_extra)
{
    xform_neg();

    // Recurse into all expr terms first
    for (Terms::iterator i=m_terms.begin(), end=m_terms.end(); i != end; ++i) {
        if (Expr* e = i->get_expr())
            e->level(fold_const, simplify_ident, simplify_reg_mul,
                     calc_bc_dist, xform_extra);
    }

    // Check for SEG of SEG:OFF, if we match, simplify to just the segment
    Expr* e;
    if (m_op == SEG && (e = m_terms.front().get_expr()) &&
        e->m_op == SEGOFF) {
        m_op = IDENT;
        e->m_op = IDENT;
        // Destroy the second (offset) term
        e->m_terms.back().destroy();
        e->m_terms.pop_back();
    }

    // Do this level
    level_op(fold_const, simplify_ident, simplify_reg_mul);

    // Do callback
    if (calc_bc_dist || xform_extra) {
        if (calc_bc_dist)
            xform_bc_dist();
        if (xform_extra)
            xform_extra(this);
        // Cleanup recursion pass; zero out callback so we don't
        // infinite loop (come back here again).
        level(fold_const, simplify_ident, simplify_reg_mul, false, 0);
    }
}

void
Expr::level_tree(bool fold_const,
                 bool simplify_ident,
                 bool simplify_reg_mul,
                 bool calc_bc_dist,
                 boost::function<void (Expr*)> xform_extra)
{
    std::vector<const Expr*> seen;
    expand_equ(seen);
    level(fold_const, simplify_ident, simplify_reg_mul, calc_bc_dist,
          xform_extra);
}

void
Expr::order_terms()
{
    // don't bother reordering if only one element
    if (m_terms.size() == 1)
        return;

    // only reorder some types of operations
    switch (m_op) {
        case ADD:
        case MUL:
        case OR:
        case AND:
        case XOR:
        case LOR:
        case LAND:
        case LXOR:
            break;
        default:
            return;
    }

    // Use a stable sort (multiple terms of same type are kept in the same
    // order).
    std::stable_sort(m_terms.begin(), m_terms.end());
}

std::auto_ptr<Expr>
Expr::clone(int except) const
{
    if (except == -1 || m_terms.size() == 1)
        return std::auto_ptr<Expr>(new Expr(*this));

    std::auto_ptr<Expr> e(new Expr(m_op, m_line));
    int j = 0;
    for (Terms::const_iterator i=m_terms.begin(), end=m_terms.end();
         i != end; ++i, ++j) {
        if (j != except)
            e->m_terms.push_back(i->clone());
    }
    return e;
}

bool
Expr::contains(int type) const
{
    return traverse_leaves_in(boost::bind(&Term::is_type, _1, type));
}

bool
Expr::substitute_cb(const Terms& subst_terms)
{
    for (Terms::iterator i=m_terms.begin(), end=m_terms.end(); i != end; ++i) {
        const unsigned int* substp = i->get_subst();
        if (!substp)
            continue;
        if (*substp >= subst_terms.size())
            return true;   // error
        *i = subst_terms[*substp].clone();
    }
    return false;
}

bool
Expr::substitute(const Terms& subst_terms)
{
    return traverse_post(boost::bind(&Expr::substitute_cb, _1,
                                     boost::ref(subst_terms)));
}

bool
Expr::traverse_post(boost::function<bool (Expr*)> func)
{
    for (Terms::iterator i=m_terms.begin(), end=m_terms.end(); i != end; ++i) {
        Expr* e = i->get_expr();
        if (e && e->traverse_post(func))
            return true;
    }
    return func(this);
}

bool
Expr::traverse_leaves_in(boost::function<bool (const Term&)> func) const
{
    for (Terms::const_iterator i=m_terms.begin(), end=m_terms.end();
         i != end; ++i) {
        if (const Expr* e = i->get_expr()) {
            if (e->traverse_leaves_in(func))
                return true;
        } else {
            if (func(*i))
                return true;
        }
    }
    return false;
}

std::auto_ptr<Expr>
Expr::extract_deep_segoff()
{
    // Try to extract at this level
    std::auto_ptr<Expr> retval = extract_segoff();
    if (retval.get() != 0)
        return retval;

    // Not at this level?  Search any expr children.
    for (Terms::iterator i=m_terms.begin(), end=m_terms.end(); i != end; ++i) {
        if (Expr* e = i->get_expr()) {
            retval = e->extract_deep_segoff();
            if (retval.get() != 0)
                return retval;
        }
    }

    // Didn't find one
    return retval;
}

std::auto_ptr<Expr>
Expr::extract_segoff()
{
    // If not SEG:OFF, we can't do this transformation
    if (m_op != SEGOFF || m_terms.size() != 2)
        return std::auto_ptr<Expr>(0);

    Expr* retval;
    Term& left = m_terms.front();

    // Extract the SEG portion out to its own expression
    if (Expr* e = left.get_expr())
        retval = e;
    else {
        // Need to build IDENT expression to hold non-expression contents
        retval = new Expr(IDENT, m_line);
        retval->m_terms.push_back(left);
    }

    // Change the expression into an IDENT
    m_terms.erase(m_terms.begin());
    m_op = IDENT;
    return std::auto_ptr<Expr>(retval);
}

std::auto_ptr<Expr>
Expr::extract_wrt()
{
    // If not WRT, we can't do this transformation
    if (m_op != WRT || m_terms.size() != 2)
        return std::auto_ptr<Expr>(0);

    Expr* retval;
    Term& right = m_terms.back();

    // Extract the right side portion out to its own expression
    if (Expr* e = right.get_expr())
        retval = e;
    else {
        // Need to build IDENT expression to hold non-expression contents
        retval = new Expr(IDENT, m_line);
        retval->m_terms.push_back(right);
    }

    // Change the expr into an IDENT
    m_terms.pop_back();
    m_op = IDENT;
    return std::auto_ptr<Expr>(retval);
}

IntNum*
Expr::get_intnum(bool calc_bc_dist)
{
    simplify(calc_bc_dist);

    if (m_op == IDENT)
        return m_terms.front().get_int();
    else
        return 0;
}

Symbol*
Expr::get_symbol() const
{
    if (m_op == IDENT)
        return m_terms.front().get_sym();
    else
        return 0;
}

const Register*
Expr::get_reg() const
{
    if (m_op == IDENT)
        return m_terms.front().get_reg();
    else
        return 0;
}

std::ostream&
operator<< (std::ostream& os, const Expr::Term& term)
{
    switch (term.m_type) {
        case Expr::NONE:    os << "NONE"; break;
        case Expr::REG:     os << "REG"; break;
        case Expr::INT:     os << *term.m_data.intn; break;
        case Expr::SUBST:   os << "[" << term.m_data.subst << "]"; break;
        case Expr::FLOAT:   os << "FLTN"; break;
        case Expr::SYM:     os << "SYM"; break;
        case Expr::PRECBC:  os << "{PRECBC}"; break;
        case Expr::EXPR:    os << "(" << *term.m_data.expr << ")"; break;
    }
    return os;
}

std::ostream&
operator<< (std::ostream& os, const Expr& e)
{
    const char* opstr = "";

    switch (e.m_op) {
        case Expr::ADD:     opstr = "+"; break;
        case Expr::SUB:     opstr = "-"; break;
        case Expr::MUL:     opstr = "*"; break;
        case Expr::DIV:     opstr = "/"; break;
        case Expr::SIGNDIV: opstr = "//"; break;
        case Expr::MOD:     opstr = "%"; break;
        case Expr::SIGNMOD: opstr = "%%"; break;
        case Expr::NEG:     os << "-"; break;
        case Expr::NOT:     os << "~"; break;
        case Expr::OR:      opstr = "|"; break;
        case Expr::AND:     opstr = "&"; break;
        case Expr::XOR:     opstr = "^"; break;
        case Expr::XNOR:    opstr = "XNOR"; break;
        case Expr::NOR:     opstr = "NOR"; break;
        case Expr::SHL:     opstr = "<<"; break;
        case Expr::SHR:     opstr = ">>"; break;
        case Expr::LOR:     opstr = "||"; break;
        case Expr::LAND:    opstr = "&&"; break;
        case Expr::LNOT:    opstr = "!"; break;
        case Expr::LXOR:    opstr = "^^"; break;
        case Expr::LXNOR:   opstr = "LXNOR"; break;
        case Expr::LNOR:    opstr = "LNOR"; break;
        case Expr::LT:      opstr = "<"; break;
        case Expr::GT:      opstr = ">"; break;
        case Expr::LE:      opstr = "<="; break;
        case Expr::GE:      opstr = ">="; break;
        case Expr::NE:      opstr = "!="; break;
        case Expr::EQ:      opstr = "=="; break;
        case Expr::SEG:     os << "SEG "; break;
        case Expr::WRT:     opstr = " WRT "; break;
        case Expr::SEGOFF:  opstr = ":"; break;
        case Expr::IDENT:   break;
        default:            opstr = " !UNK! "; break;
    }
    for (Expr::Terms::const_iterator i=e.m_terms.begin(), end=e.m_terms.end();
         i != end; ++i) {
        if (i != e.m_terms.begin())
            os << opstr;
        os << *i;
    }
    return os;
}

} // namespace yasm

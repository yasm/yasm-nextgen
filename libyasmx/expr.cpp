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
#include "expr.h"

#include "util.h"

#include <algorithm>
#include <iterator>
#include <ostream>

#include "arch.h"
#include "errwarn.h"
#include "floatnum.h"
#include "intnum.h"


namespace
{

using namespace yasm;

/// Look for simple identities that make the entire result constant:
/// 0*&x, -1|x, etc.
bool
is_constant(Op::Op op, const IntNum* intn)
{
    bool iszero = intn->is_zero();
    return ((iszero && op == Op::MUL) ||
            (iszero && op == Op::AND) ||
            (iszero && op == Op::LAND) ||
            (intn->is_neg1() && op == Op::OR));
}

/// Look for simple "left" identities like 0+x, 1*x, etc.
bool
can_destroy_int_left(Op::Op op, const IntNum* intn)
{
    bool iszero = intn->is_zero();
    return ((intn->is_pos1() && op == Op::MUL) ||
            (iszero && op == Op::ADD) ||
            (intn->is_neg1() && op == Op::AND) ||
            (!iszero && op == Op::LAND) ||
            (iszero && op == Op::OR) ||
            (iszero && op == Op::LOR));
}

/// Look for simple "right" identities like x+|-0, x*&/1
bool
can_destroy_int_right(Op::Op op, const IntNum* intn)
{
    int iszero = intn->is_zero();
    int ispos1 = intn->is_pos1();
    return ((ispos1 && op == Op::MUL) ||
            (ispos1 && op == Op::DIV) ||
            (iszero && op == Op::ADD) ||
            (iszero && op == Op::SUB) ||
            (intn->is_neg1() && op == Op::AND) ||
            (!iszero && op == Op::LAND) ||
            (iszero && op == Op::OR) ||
            (iszero && op == Op::LOR) ||
            (iszero && op == Op::SHL) ||
            (iszero && op == Op::SHR));
}

} // anonymous namespace

namespace yasm
{

Expr::Term::Term(const IntNum& intn)
    : m_type(INT), m_intn(intn.clone())
{
}

Expr::Term::Term(std::auto_ptr<IntNum> intn)
    : m_type(INT), m_intn(intn.release())
{
}

Expr::Term::Term(std::auto_ptr<FloatNum> flt)
    : m_type(FLOAT), m_flt(flt.release())
{
}

Expr::Term::Term(std::auto_ptr<Expr> expr)
    : m_type(EXPR), m_expr(expr.release())
{
}

Expr::Term
Expr::Term::clone() const
{
    switch (m_type)
    {
        case INT:   return m_intn->clone();
        case FLOAT: return m_flt->clone();
        case EXPR:  return m_expr->clone();
        default:    return *this;
    }
}

void
Expr::Term::destroy()
{
    switch (m_type)
    {
        case INT:
            delete m_intn;
            m_intn = 0;
            break;
        case FLOAT:
            delete m_flt;
            m_flt = 0;
            break;
        case EXPR:
            delete m_expr;
            m_expr = 0;
            break;
        default:
            break;
    }
    //m_type = NONE;
}

void
Expr::add_term(const Term& term)
{
    Expr* base_e = term.get_expr();
    if (!base_e)
    {
        m_terms.push_back(term);
        return;
    }

    Expr* e = base_e;
    Expr* copyfrom = 0;

    // Search downward until we find something *other* than an
    // IDENT, then bring it up to the current level.
    for (;;)
    {
        if (e->m_op != Op::IDENT)
            break;
        copyfrom = e;

        if (e->m_terms.size() != 1)
            break;

        Expr* sube = e->m_terms.front().get_expr();
        if (!sube)
            break;

        e = sube;
    }

    if (!copyfrom)
    {
        m_terms.push_back(base_e);
    }
    else
    {
        // Transfer the terms up
        m_terms.insert(m_terms.end(), copyfrom->m_terms.begin(),
                       copyfrom->m_terms.end());
        copyfrom->m_terms.clear();
        // Delete the rest
        delete base_e;
    }
}

Expr::Expr(const Term& a, Op::Op op, const Term& b, unsigned long line)
    : m_op(op), m_line(line)
{
    add_term(a);
    add_term(b);
}

Expr::Expr(Op::Op op, const Term& a, unsigned long line)
    : m_op(op), m_line(line)
{
    if (!is_unary(op))
        throw ValueError(N_("expression with one term must be unary"));
    add_term(a);
}

Expr::Expr(Op::Op op, const Terms& terms, unsigned long line)
    : m_op(op), m_line(line), m_terms(terms)
{
    switch (terms.size())
    {
        case 0:
            throw ValueError(N_("expression must have more than 0 terms"));
        case 1:
            if (!is_unary(op))
                throw ValueError(N_("expression with one term must be unary"));
            return;
        case 2:
            return;
    }
    // more than 2 terms
    if (!is_associative(op))
        throw ValueError(N_("expression with more than two terms must be associative"));
}

Expr::Expr(const Term& a, unsigned long line)
    : m_op(Op::IDENT), m_line(line)
{
    add_term(a);
}

Expr&
Expr::operator= (const Expr& rhs)
{
    if (this != &rhs)
    {
        m_op = rhs.m_op;
        m_line = rhs.m_line;
        std::for_each(m_terms.begin(), m_terms.end(),
                      MEMFN::mem_fn(&Term::destroy));
        m_terms.clear();
        std::transform(rhs.m_terms.begin(), rhs.m_terms.end(),
                       std::back_inserter(m_terms),
                       MEMFN::mem_fn(&Term::clone));
    }
    return *this;
}

Expr::Expr(const Expr& e)
    : m_op(e.m_op), m_line(e.m_line)
{
    std::transform(e.m_terms.begin(), e.m_terms.end(),
                   std::back_inserter(m_terms),
                   MEMFN::mem_fn(&Term::clone));
}

Expr::Expr(unsigned long line, Op::Op op)
    : m_op(op), m_line(line)
{
}

Expr::~Expr()
{
    std::for_each(m_terms.begin(), m_terms.end(),
                  MEMFN::mem_fn(&Term::destroy));
}

/// Negate just a single ExprTerm by building a -1*ei subexpression.
inline void
Expr::xform_neg_term(Terms::iterator term)
{
    Expr *sube = new Expr(m_line, Op::MUL);
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
    switch (m_op)
    {
        case Op::ADD:
            // distribute (recursively if expr) over terms
            for (Terms::iterator i=m_terms.begin(), end=m_terms.end();
                 i != end; ++i)
            {
                if (Expr* sube = i->get_expr())
                    sube->xform_neg_helper();
                else
                    xform_neg_term(i);
            }
            break;
        case Op::SUB:
            // change op to ADD, and recursively negate left side (if expr)
            m_op = Op::ADD;
            if (Expr* sube = m_terms.front().get_expr())
                sube->xform_neg_helper();
            else
                xform_neg_term(m_terms.begin());
            break;
        case Op::NEG:
            // Negating a negated value?  Make it an IDENT.
            m_op = Op::IDENT;
            break;
        case Op::IDENT:
        {
            // Negating an ident?  Change it into a MUL w/ -1 if there's no
            // floatnums present below; if there ARE floatnums, recurse.
            Term& first = m_terms.front();
            Expr* e;
            if (FloatNum* flt = first.get_float())
                flt->calc(Op::NEG);
            else if (IntNum* intn = first.get_int())
                intn->calc(Op::NEG);
            else if ((e = first.get_expr()) && e->contains(FLOAT))
                e->xform_neg_helper();
            else
            {
                m_op = Op::MUL;
                m_terms.push_back(new IntNum(-1));
            }
            break;
        }
        default:
            // Everything else.  MUL will be combined when it's leveled.
            // Replace ourselves with -1*e.
            Expr *ne = new Expr(m_line, m_op);
            m_op = Op::MUL;
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
    switch (m_op)
    {
        case Op::NEG:
            // Turn -x into -1*x
            m_op = Op::IDENT;
            xform_neg_helper();
            break;
        case Op::SUB:
        {
            // Turn a-b into a+(-1*b)
            // change op to ADD, and recursively negate right side (if expr)
            m_op = Op::ADD;
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

    if (m_terms.size() > 1)
    {
        // Check for simple identities that delete the intnum.
        // Don't do this step if it's 1*REG.
        if ((simplify_reg_mul || m_op != Op::MUL || !intn->is_pos1() ||
             !contains(REG)) &&
            ((is_first && can_destroy_int_left(m_op, intn)) ||
             (!is_first && can_destroy_int_right(m_op, intn))))
        {
            // delete int term
            m_terms.erase(std::find_if(m_terms.begin(), m_terms.end(),
                                       BIND::bind(&Term::is_type, _1, INT)));
            delete intn;
            intn = 0;
        }
        // Check for simple identites that delete everything BUT the intnum.
        else if (is_constant(m_op, intn))
        {
            // Delete everything but the integer term
            Terms terms;
            Terms::iterator i;
            i = std::find_if(m_terms.begin(), m_terms.end(),
                             BIND::bind(&Term::is_type, _1, INT));
            terms.push_back(*i);
            i->release(); // don't delete it now we've moved it
            m_terms.swap(terms);
            intn = m_terms.front().get_int();
            // delete old terms
            std::for_each(terms.begin(), terms.end(),
                          MEMFN::mem_fn(&Term::destroy));
        }
    }

    // Compute NOT, NEG, and LNOT on single intnum.
    if (intn && m_terms.size() == 1 && is_first &&
        (m_op == Op::NOT || m_op == Op::NEG || m_op == Op::LNOT))
        intn->calc(m_op);

    // Change expression to IDENT if possible.
    if (m_terms.size() == 1)
        m_op = Op::IDENT;
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
    Expr* e;

    // If non-numeric expression, don't fold constants.
    if (m_op > Op::NONNUM)
        fold_const = false;

    for (Terms::iterator i=m_terms.begin(), end=m_terms.end(); i != end; ++i)
    {
        // Search downward until we find something *other* than an
        // IDENT, then bring it up to the current level.
        if ((e = i->get_expr()))
        {
            while (e && e->m_op == Op::IDENT)
            {
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
        if (fold_const && (intn_temp = i->get_int()))
        {
            if (!intn)
            {
                intn = intn_temp;
                first_int_term = i;
            }
            else
            {
                intn->calc(m_op, intn_temp);
                i->destroy();
            }
        }
    }

    if (intn)
    {
        // Erase folded integer terms; we already deleted their contents above
        Terms::iterator erasefrom =
            std::remove_if(first_int_term+1, m_terms.end(),
                           BIND::bind(&Term::is_type, _1, INT));
        m_terms.erase(erasefrom, m_terms.end());

        // Simplify identities and make IDENT if possible.
        if (simplify_ident)
            simplify_identity(intn, simplify_reg_mul);
        else if (m_terms.size() == 1)
            m_op = Op::IDENT;
    }

    // Only level associative operators.
    // Also don't bother leveling if it's not necessary to bring up any terms.
    if (!do_level || !is_associative(m_op))
    {
        // trim capacity before returning
        Terms(m_terms).swap(m_terms);
        return;
    }

    // Copy up ExprTerms.  Combine integer terms as necessary.
    // This is a two-step process; we do this part in reverse order (to
    // use constant time operations), and then reverse the vector at the end.
    Terms terms;
    for (Terms::reverse_iterator i=m_terms.rbegin(), end=m_terms.rend();
         i != end; ++i)
    {
        if ((e = i->get_expr()) && e->m_op == m_op)
        {
            // move up terms, folding constants as we go
            while (!e->m_terms.empty())
            {
                Term& last = e->m_terms.back();
                IntNum* intn_temp;
                if (fold_const && (intn_temp = last.get_int()))
                {
                    // Need to fold it in.. but if there's no int term
                    // already, just move this one up to become it.
                    if (intn)
                    {
                        intn->calc(m_op, intn_temp);
                        last.destroy();
                    }
                    else
                    {
                        intn = intn_temp;
                        terms.push_back(last);
                    }
                }
                else
                    terms.push_back(last);
                e->m_terms.pop_back();
            }
            i->destroy();
        }
        else
            terms.push_back(*i);
    }
    std::reverse(terms.begin(), terms.end());
    m_terms.swap(terms);

    // Simplify identities, make IDENT if possible.
    if (simplify_ident && intn)
        simplify_identity(intn, simplify_reg_mul);
    else if (m_terms.size() == 1)
        m_op = Op::IDENT;
}

void
Expr::level_tree(bool fold_const,
                 bool simplify_ident,
                 bool simplify_reg_mul,
                 FUNCTION::function<void (Expr*)> xform_extra)
{
    xform_neg();

    // Recurse into all expr terms first
    for (Terms::iterator i=m_terms.begin(), end=m_terms.end(); i != end; ++i)
    {
        if (Expr* e = i->get_expr())
            e->level_tree(fold_const, simplify_ident, simplify_reg_mul,
                          xform_extra);
    }

    // Check for SEG of SEG:OFF, if we match, simplify to just the segment
    Expr* e;
    if (m_op == Op::SEG && (e = m_terms.front().get_expr()) &&
        e->m_op == Op::SEGOFF)
    {
        m_op = Op::IDENT;
        e->m_op = Op::IDENT;
        // Destroy the second (offset) term
        e->m_terms.back().destroy();
        e->m_terms.pop_back();
    }

    // Do this level
    level_op(fold_const, simplify_ident, simplify_reg_mul);

    // Do callback
    if (xform_extra)
    {
        xform_extra(this);
        // Cleanup recursion pass; zero out callback so we don't
        // infinite loop (come back here again).
        level_tree(fold_const, simplify_ident, simplify_reg_mul, 0);
    }
}

void
Expr::order_terms()
{
    // don't bother reordering if only one element
    if (m_terms.size() == 1)
        return;

    // only reorder commutative operators
    if (!is_commutative(m_op))
        return;

    // Use a stable sort (multiple terms of same type are kept in the same
    // order).
    std::stable_sort(m_terms.begin(), m_terms.end());
}

Expr*
Expr::clone(int except) const
{
    if (except == -1 || m_terms.size() == 1)
        return new Expr(*this);

    std::auto_ptr<Expr> e(new Expr(m_line, m_op));
    int j = 0;
    for (Terms::const_iterator i=m_terms.begin(), end=m_terms.end();
         i != end; ++i, ++j)
    {
        if (j != except)
            e->m_terms.push_back(i->clone());
    }
    return e.release();
}

bool
Expr::contains(int type) const
{
    return traverse_leaves_in(BIND::bind(&Term::is_type, _1, type));
}

bool
Expr::substitute_cb(const Terms& subst_terms)
{
    for (Terms::iterator i=m_terms.begin(), end=m_terms.end(); i != end; ++i)
    {
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
    return traverse_post(BIND::bind(&Expr::substitute_cb, _1,
                                    REF::ref(subst_terms)));
}

bool
Expr::traverse_post(FUNCTION::function<bool (Expr*)> func)
{
    for (Terms::iterator i=m_terms.begin(), end=m_terms.end(); i != end; ++i)
    {
        Expr* e = i->get_expr();
        if (e && e->traverse_post(func))
            return true;
    }
    return func(this);
}

bool
Expr::traverse_leaves_in(FUNCTION::function<bool (const Term&)> func) const
{
    for (Terms::const_iterator i=m_terms.begin(), end=m_terms.end();
         i != end; ++i)
    {
        if (const Expr* e = i->get_expr())
        {
            if (e->traverse_leaves_in(func))
                return true;
        }
        else
        {
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
    for (Terms::iterator i=m_terms.begin(), end=m_terms.end(); i != end; ++i)
    {
        if (Expr* e = i->get_expr())
        {
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
    std::auto_ptr<Expr> retval(0);

    // If not SEG:OFF, we can't do this transformation
    if (m_op != Op::SEGOFF || m_terms.size() != 2)
        return retval;

    Term& left = m_terms.front();

    // Extract the SEG portion out to its own expression
    if (Expr* e = left.get_expr())
        retval.reset(e);
    else
    {
        // Need to build IDENT expression to hold non-expression contents
        retval.reset(new Expr(m_line, Op::IDENT));
        retval->m_terms.push_back(left);
    }

    // Change the expression into an IDENT
    m_terms.erase(m_terms.begin());
    m_op = Op::IDENT;
    return retval;
}

std::auto_ptr<Expr>
Expr::extract_wrt()
{
    std::auto_ptr<Expr> retval(0);

    // If not WRT, we can't do this transformation
    if (m_op != Op::WRT || m_terms.size() != 2)
        return retval;

    Term& right = m_terms.back();

    // Extract the right side portion out to its own expression
    if (Expr* e = right.get_expr())
        retval.reset(e);
    else
    {
        // Need to build IDENT expression to hold non-expression contents
        retval.reset(new Expr(m_line, Op::IDENT));
        retval->m_terms.push_back(right);
    }

    // Change the expr into an IDENT
    m_terms.pop_back();
    m_op = Op::IDENT;
    return retval;
}

FloatNum*
Expr::get_float() const
{
    if (m_op == Op::IDENT)
        return m_terms.front().get_float();
    else
        return 0;
}

IntNum*
Expr::get_intnum() const
{
    if (m_op == Op::IDENT)
        return m_terms.front().get_int();
    else
        return 0;
}

Symbol*
Expr::get_symbol() const
{
    if (m_op == Op::IDENT)
        return m_terms.front().get_sym();
    else
        return 0;
}

const Register*
Expr::get_reg() const
{
    if (m_op == Op::IDENT)
        return m_terms.front().get_reg();
    else
        return 0;
}

std::ostream&
operator<< (std::ostream& os, const Expr::Term& term)
{
    switch (term.m_type)
    {
        case Expr::NONE:    os << "NONE"; break;
        case Expr::REG:     os << *term.m_reg; break;
        case Expr::INT:     os << *term.m_intn; break;
        case Expr::SUBST:   os << "[" << term.m_subst << "]"; break;
        case Expr::FLOAT:   os << "FLTN"; break;
        case Expr::SYM:     os << "SYM"; break;
        case Expr::LOC:     os << "{LOC}"; break;
        case Expr::EXPR:    os << "(" << *term.m_expr << ")"; break;
    }
    return os;
}

std::ostream&
operator<< (std::ostream& os, const Expr& e)
{
    const char* opstr = "";

    switch (e.m_op)
    {
        case Op::ADD:       opstr = "+"; break;
        case Op::SUB:       opstr = "-"; break;
        case Op::MUL:       opstr = "*"; break;
        case Op::DIV:       opstr = "/"; break;
        case Op::SIGNDIV:   opstr = "//"; break;
        case Op::MOD:       opstr = "%"; break;
        case Op::SIGNMOD:   opstr = "%%"; break;
        case Op::NEG:       os << "-"; break;
        case Op::NOT:       os << "~"; break;
        case Op::OR:        opstr = "|"; break;
        case Op::AND:       opstr = "&"; break;
        case Op::XOR:       opstr = "^"; break;
        case Op::XNOR:      opstr = "XNOR"; break;
        case Op::NOR:       opstr = "NOR"; break;
        case Op::SHL:       opstr = "<<"; break;
        case Op::SHR:       opstr = ">>"; break;
        case Op::LOR:       opstr = "||"; break;
        case Op::LAND:      opstr = "&&"; break;
        case Op::LNOT:      opstr = "!"; break;
        case Op::LXOR:      opstr = "^^"; break;
        case Op::LXNOR:     opstr = "LXNOR"; break;
        case Op::LNOR:      opstr = "LNOR"; break;
        case Op::LT:        opstr = "<"; break;
        case Op::GT:        opstr = ">"; break;
        case Op::LE:        opstr = "<="; break;
        case Op::GE:        opstr = ">="; break;
        case Op::NE:        opstr = "!="; break;
        case Op::EQ:        opstr = "=="; break;
        case Op::SEG:       os << "SEG "; break;
        case Op::WRT:       opstr = " WRT "; break;
        case Op::SEGOFF:    opstr = ":"; break;
        case Op::IDENT:     break;
        default:            opstr = " !UNK! "; break;
    }

    for (Expr::Terms::const_iterator i=e.m_terms.begin(), end=e.m_terms.end();
         i != end; ++i)
    {
        if (i != e.m_terms.begin())
            os << opstr;
        os << *i;
    }

    return os;
}

} // namespace yasm

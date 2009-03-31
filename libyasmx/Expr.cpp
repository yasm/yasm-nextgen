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
#include "Expr.h"

#include "util.h"

#include <algorithm>
#include <iterator>
#include <ostream>

#include "Config/functional.h"

#include "Arch.h"
#include "errwarn.h"
#include "FloatNum.h"
#include "IntNum.h"
#include "Symbol.h"


namespace yasm
{

/// Look for simple identities that make the entire result constant:
/// 0*&x, -1|x, etc.
static inline bool
is_constant_identity(Op::Op op, const IntNum& intn)
{
    bool iszero = intn.is_zero();
    return ((iszero && op == Op::MUL) ||
            (iszero && op == Op::AND) ||
            (iszero && op == Op::LAND) ||
            (intn.is_neg1() && op == Op::OR));
}

/// Look for simple "left" identities like 0+x, 1*x, etc.
static inline bool
is_left_identity(Op::Op op, const IntNum& intn)
{
    bool iszero = intn.is_zero();
    return ((intn.is_pos1() && op == Op::MUL) ||
            (iszero && op == Op::ADD) ||
            (intn.is_neg1() && op == Op::AND) ||
            (!iszero && op == Op::LAND) ||
            (iszero && op == Op::OR) ||
            (iszero && op == Op::LOR));
}

/// Look for simple "right" identities like x+|-0, x*&/1
static inline bool
is_right_identity(Op::Op op, const IntNum& intn)
{
    bool iszero = intn.is_zero();
    bool ispos1 = intn.is_pos1();
    return ((ispos1 && op == Op::MUL) ||
            (ispos1 && op == Op::DIV) ||
            (iszero && op == Op::ADD) ||
            (iszero && op == Op::SUB) ||
            (intn.is_neg1() && op == Op::AND) ||
            (!iszero && op == Op::LAND) ||
            (iszero && op == Op::OR) ||
            (iszero && op == Op::LOR) ||
            (iszero && op == Op::SHL) ||
            (iszero && op == Op::SHR));
}

const ExprBuilder ADD = {Op::ADD};
const ExprBuilder SUB = {Op::SUB};
const ExprBuilder MUL = {Op::MUL};
const ExprBuilder DIV = {Op::DIV};
const ExprBuilder SIGNDIV = {Op::SIGNDIV};
const ExprBuilder MOD = {Op::MOD};
const ExprBuilder SIGNMOD = {Op::SIGNMOD};
const ExprBuilder NEG = {Op::NEG};
const ExprBuilder NOT = {Op::NOT};
const ExprBuilder OR = {Op::OR};
const ExprBuilder AND = {Op::AND};
const ExprBuilder XOR = {Op::XOR};
const ExprBuilder XNOR = {Op::XNOR};
const ExprBuilder NOR = {Op::NOR};
const ExprBuilder SHL = {Op::SHL};
const ExprBuilder SHR = {Op::SHR};
const ExprBuilder LOR = {Op::LOR};
const ExprBuilder LAND = {Op::LAND};
const ExprBuilder LNOT = {Op::LNOT};
const ExprBuilder LXOR = {Op::LXOR};
const ExprBuilder LXNOR = {Op::LXNOR};
const ExprBuilder LNOR = {Op::LNOR};
const ExprBuilder LT = {Op::LT};
const ExprBuilder GT = {Op::GT};
const ExprBuilder EQ = {Op::EQ};
const ExprBuilder LE = {Op::LE};
const ExprBuilder GE = {Op::GE};
const ExprBuilder NE = {Op::NE};
const ExprBuilder SEG = {Op::SEG};
const ExprBuilder WRT = {Op::WRT};
const ExprBuilder SEGOFF = {Op::SEGOFF};

ExprTerm::ExprTerm(std::auto_ptr<IntNum> intn, int depth)
    : m_type(INT), m_depth(depth)
{
    m_data.intn.m_type = IntNumData::INTNUM_L;
    intn->swap(static_cast<IntNum&>(m_data.intn));
}

ExprTerm::ExprTerm(std::auto_ptr<FloatNum> flt, int depth)
    : m_type(FLOAT), m_depth(depth)
{
    m_data.flt = flt.release();
}

ExprTerm::ExprTerm(const ExprTerm& term)
    : m_data(term.m_data), m_type(term.m_type), m_depth(term.m_depth)
{
    if (m_type == INT)
        m_data.intn = IntNum(static_cast<const IntNum&>(m_data.intn));
    else if (m_type == FLOAT)
        m_data.flt = m_data.flt->clone();
}

void
ExprTerm::swap(ExprTerm& oth)
{
    std::swap(m_type, oth.m_type);
    std::swap(m_depth, oth.m_depth);
    std::swap(m_data, oth.m_data);
}

void
ExprTerm::clear()
{
    if (m_type == INT)
        static_cast<IntNum&>(m_data.intn).~IntNum();
    else if (m_type == FLOAT)
        delete m_data.flt;
    m_type = NONE;
}

void
ExprTerm::zero()
{
    clear();
    m_type = INT;
    m_data.intn = IntNum(0);
}

void
Expr::append_op(Op::Op op, int nchild)
{
    switch (nchild)
    {
        case 0:
            throw ValueError(N_("expression must have more than 0 terms"));
        case 1:
            if (!is_unary(op))
                op = Op::IDENT;
            break;
        case 2:
            if (is_unary(op))
                throw ValueError(N_("unary expression may only have single term"));
            break;
        default:
            // more than 2 terms
            if (!is_associative(op))
                throw ValueError(N_("expression with more than two terms must be associative"));
    }

    for (ExprTerms::iterator i=m_terms.begin(), end=m_terms.end(); i != end;
         ++i)
        i->m_depth++;

    if (op != Op::IDENT)
        m_terms.push_back(ExprTerm(op, nchild, 0));
}

Expr::Expr(const Expr& e)
    : m_terms(e.m_terms)
{
}

Expr::Expr(std::auto_ptr<IntNum> intn)
{
    m_terms.push_back(ExprTerm(intn));
}

Expr::Expr(std::auto_ptr<FloatNum> flt)
{
    m_terms.push_back(ExprTerm(flt));
}

Expr::~Expr()
{
}

void
Expr::swap(Expr& oth)
{
    std::swap(m_terms, oth.m_terms);
}

void
Expr::cleanup()
{
    ExprTerms::iterator erasefrom =
        std::remove_if(m_terms.begin(), m_terms.end(),
                       BIND::bind(&ExprTerm::is_empty, _1));
    m_terms.erase(erasefrom, m_terms.end());
}

void
Expr::reduce_depth(int pos, int delta)
{
    if (pos < 0)
        pos += m_terms.size();
    ExprTerm& parent = m_terms[pos];
    if (parent.is_op())
    {
        for (int n=pos-1; n >= 0; --n)
        {
            ExprTerm& child = m_terms[n];
            if (child.is_empty())
                continue;
            if (child.m_depth <= parent.m_depth)
                break;      // Stop when we're out of children
            child.m_depth -= delta;
        }
    }
    parent.m_depth -= delta;    // Bring up parent
}

void
Expr::make_ident(int pos)
{
    if (pos < 0)
        pos += m_terms.size();

    ExprTerm& root = m_terms[pos];
    if (!root.is_op())
        return;

    // If operator has no children, replace it with a zero.
    if (root.get_nchild() == 0)
    {
        root.zero();
        return;
    }

    // If operator only has one child, may be able to delete operator
    if (root.get_nchild() != 1)
        return;

    Op::Op op = root.get_op();
    bool unary = is_unary(op);
    if (!unary)
    {
        // delete one-term non-unary operators
        reduce_depth(pos);      // bring up child
        root.clear();
    }
    else if (op < Op::NONNUM)
    {
        // find child
        for (int n=pos-1; n >= 0; --n)
        {
            ExprTerm& child = m_terms[n];
            if (child.is_empty())
                continue;
            assert(child.m_depth >= root.m_depth);  // must have one child
            if (child.m_depth != root.m_depth+1)
                continue;

            // if a simple integer, compute it
            if (IntNum* intn = child.get_int())
            {
                intn->calc(op);
                child.m_depth -= 1;
                root.clear();
            }
            break;
        }
    }

    cleanup();
}

void
Expr::clear_except(int pos, int keep)
{
    if (keep > 0)
        assert(!m_terms[keep].is_op());       // unsupported
    if (pos < 0)
        pos += m_terms.size();
    assert(pos >= 0 && pos < static_cast<int>(m_terms.size()));

    ExprTerm& parent = m_terms[pos];
    for (int n=pos-1; n >= 0; --n)
    {
        ExprTerm& child = m_terms[n];
        if (child.is_empty())
            continue;
        if (child.m_depth <= parent.m_depth)
            break;      // Stop when we're out of children
        if (n == keep)
            continue;
        child.clear();
    }
}

int
xform_neg_impl(Expr& e, int pos, int stop_depth, int depth_delta, bool negating)
{
    ExprTerms& terms = e.get_terms();

    int n = pos;
    for (; n >= 0; --n)
    {
        ExprTerm* child = &terms[n];
        if (child->is_empty())
            continue;

        // Update depth as required
        child->m_depth += depth_delta;
        int child_depth = child->m_depth;

        switch (child->get_op())
        {
            case Op::NEG:
            {
                int new_depth = child->m_depth;
                child->clear();
                // Invert current negation state and bring up children
                n = xform_neg_impl(e, n-1, new_depth, depth_delta - 1,
                                   !negating);
                break;
            }
            case Op::SUB:
            {
                child->set_op(Op::ADD);
                int new_depth = child->m_depth+1;
                if (negating)
                {
                    // -(a-b) ==> -a+b, so don't negate right side,
                    // but do negate left side.
                    n = xform_neg_impl(e, n-1, new_depth, depth_delta, false);
                    n = xform_neg_impl(e, n-1, new_depth, depth_delta, true);
                }
                else
                {
                    // a-b ==> a+(-1*b), so negate right side only.
                    n = xform_neg_impl(e, n-1, new_depth, depth_delta, true);
                    n = xform_neg_impl(e, n-1, new_depth, depth_delta, false);
                }
                break;
            }
            case Op::ADD:
            {
                if (!negating)
                    break;

                // Negate all children
                int new_depth = child->m_depth+1;
                for (int x = 0, nchild = child->get_nchild(); x < nchild; ++x)
                    n = xform_neg_impl(e, n-1, new_depth, depth_delta, true);
                break;
            }
            case Op::MUL:
            {
                if (!negating)
                    break;

                // Insert -1 term.  Do this by inserting a new MUL op
                // and changing this term to -1, to avoid having to
                // deal with updating n.
                terms.insert(terms.begin()+n+1,
                             ExprTerm(child->get_op(),
                                      child->get_nchild()+1,
                                      child->m_depth));
                child = &terms[n];      // need to re-get as terms may move
                *child = ExprTerm(-1, child->m_depth+1);
                break;
            }
            default:
            {
                if (!negating)
                    break;

                // Directly negate if possible (integers or floats)
                if (IntNum* intn = child->get_int())
                {
                    intn->calc(Op::NEG);
                    break;
                }

                if (FloatNum* fltn = child->get_float())
                {
                    fltn->calc(Op::NEG);
                    break;
                }

                // Couldn't replace directly; instead replace with -1*e
                // Insert -1 one level down, add operator at this level,
                // and move all subterms one level down.
                terms.insert(terms.begin()+n+1, 2,
                             ExprTerm(Op::MUL, 2, child->m_depth));
                child = &terms[n];      // need to re-get as terms may move
                terms[n+1] = ExprTerm(-1, child->m_depth+1);
                child->m_depth++;
                int new_depth = child->m_depth+1;
                for (int x = 0; x < child->get_nchild(); ++x)
                    n = xform_neg_impl(e, n-1, new_depth, depth_delta+1, false);
            }
        }

        if (child_depth <= stop_depth)
            break;
    }

    return n;
}

void
Expr::xform_neg()
{
    ExprTerm& root = m_terms.back();
    if (!root.is_op())
        return;

    xform_neg_impl(*this, m_terms.size()-1, root.m_depth-1, 0, false);
}

void
Expr::level_op(bool simplify_reg_mul, int pos)
{
    if (pos < 0)
        pos += m_terms.size();
    assert(pos >= 0 && pos < static_cast<int>(m_terms.size()));

    ExprTerm& root = m_terms[pos];
    if (!root.is_op())
        return;
    Op::Op op = root.get_op();
    bool do_level = is_associative(op);

    ExprTerm* intchild = 0;             // first (really last) integer child
    int childnum = root.get_nchild();   // which child this is (0=first)

    for (int n=pos-1; n >= 0; --n)
    {
        ExprTerm& child = m_terms[n];
        if (child.is_empty())
            continue;
        if (child.m_depth <= root.m_depth)
            break;
        if (child.m_depth != root.m_depth+1)
            continue;
        --childnum;

        // Check for SEG of SEG:OFF, if we match, simplify to just the segment
        if (op == Op::SEG && child.is_op(Op::SEGOFF))
        {
            // Find LHS of SEG:OFF, clearing RHS (OFF) as we go.
            int m = n-1;
            for (int cnum=0; m >= 0; --m)
            {
                ExprTerm& child2 = m_terms[m];
                if (child2.is_empty())
                    continue;
                if (child2.m_depth <= child.m_depth)
                    break;
                if (child2.m_depth == child.m_depth+1)
                    cnum++;
                if (cnum == 2)
                    break;
                child2.clear();
            }
            assert(m >= 0);

            // Bring up the SEG portion by two levels
            for (; m >= 0; --m)
            {
                ExprTerm& child2 = m_terms[m];
                if (child2.is_empty())
                    continue;
                if (child2.m_depth <= child.m_depth)
                    break;
                child2.m_depth -= 2;
            }

            // Delete the operators.
            child.clear();
            root.clear();
            return;                 // End immediately since we cleared root.
        }

        if (IntNum* intn = child.get_int())
        {
            // Look for identities that will delete the intnum term.
            // Don't simplify 1*REG if simplify_reg_mul is disabled.
            if ((simplify_reg_mul ||
                 op != Op::MUL ||
                 !intn->is_pos1() ||
                 !contains(ExprTerm::REG, pos))
                &&
                ((childnum != 0 && is_right_identity(op, *intn)) ||
                 (childnum == 0 && is_left_identity(op, *intn))))
            {
                // Delete intnum from expression
                child.clear();
                root.add_nchild(-1);
                continue;
            }
            else if (is_constant_identity(op, *intn))
            {
                // This is special; it deletes all terms except for
                // the integer.  This means we can terminate
                // immediately after deleting all other terms.
                clear_except(pos, n);
                --child.m_depth;            // bring up intnum
                root.clear();               // delete operator
                return;
            }

            if (intchild == 0)
            {
                intchild = &child;
                continue;
            }

            if (op < Op::NONNUM)
            {
                std::swap(*intchild->get_int(), *intn);
                intchild->get_int()->calc(op, *intn);
                child.clear();
                root.add_nchild(-1);
            }
        }
        else if (do_level && child.is_op(op))
        {
            root.add_nchild(child.get_nchild() - 1);
            reduce_depth(n);        // bring up children
            child.clear();          // delete levelled op
        }
    }

    // If operator only has one child, may be able to delete operator
    if (root.get_nchild() == 1)
    {
        bool unary = is_unary(op);
        if (unary && op < Op::NONNUM && intchild != 0)
        {
            // if unary on a simple integer, compute it
            intchild->get_int()->calc(op);
            intchild->m_depth -= 1;
            root.clear();
        }
        else if (!unary)
        {
            // delete one-term non-unary operators
            reduce_depth(pos);      // bring up children
            root.clear();
        }
    }
    else if (root.get_nchild() == 0)
        root.zero();    // If operator has no children, replace it with a zero.
}

void
Expr::simplify(bool simplify_reg_mul)
{
    xform_neg();

    for (int pos=0, size=m_terms.size(); pos<size; ++pos)
    {
        if (!m_terms[pos].is_op())
            continue;
        level_op(simplify_reg_mul, pos);
    }

    cleanup();
}

bool
Expr::contains(int type, int pos) const
{
    if (pos < 0)
        pos += m_terms.size();
    assert(pos >= 0 && pos < static_cast<int>(m_terms.size()));

    const ExprTerm& parent = m_terms[pos];
    if (!parent.is_op())
    {
        if (parent.is_type(type))
            return true;
        return false;
    }
    for (int n=pos-1; n>=0; --n)
    {
        const ExprTerm& child = m_terms[n];
        if (child.is_empty())
            continue;
        if (child.m_depth <= parent.m_depth)
            break;  // Stop when we're out of children
        if (child.is_type(type))
            return true;
    }
    return false;
}

bool
Expr::substitute(const ExprTerms& subst_terms)
{
    for (ExprTerms::iterator i=m_terms.begin(), end=m_terms.end(); i != end;
         ++i)
    {
        const unsigned int* substp = i->get_subst();
        if (!substp)
            continue;
        if (*substp >= subst_terms.size())
            return true;   // error
        int depth = i->m_depth;
        *i = subst_terms[*substp];
        i->m_depth = depth;
    }
    return false;
}

Expr
Expr::extract_lhs(ExprTerms::reverse_iterator op)
{
    Expr retval;

    ExprTerms::reverse_iterator end = m_terms.rend();
    if (op == end)
        return retval;

    // Delete the operator
    int parent_depth = op->m_depth;
    op->clear();

    // Bring up the RHS terms
    ExprTerms::reverse_iterator child = ++op;
    for (; child != end; ++child)
    {
        if (child->is_empty())
            continue;
        if (child->m_depth <= parent_depth)
            break;
        if (child != op && child->m_depth == parent_depth+1)
            break;      // stop when we've reached the second (LHS) child
        child->m_depth--;
    }

    // Extract the LHS terms.
    for (; child != end; ++child)
    {
        if (child->is_empty())
            continue;
        if (child->m_depth <= parent_depth)
            break;
        // Fix up depth for new expression.
        child->m_depth -= parent_depth+1;
        // Add a NONE term to retval, then swap it with the child.
        retval.m_terms.push_back(ExprTerm());
        std::swap(retval.m_terms.back(), *child);
    }

    // We added in reverse order, so fix up.
    std::reverse(retval.m_terms.begin(), retval.m_terms.end());

    // Clean up NONE terms.
    cleanup();

    return retval;
}

Expr
Expr::extract_deep_segoff()
{
    // Look through terms for the first SEG:OFF operator
    for (ExprTerms::reverse_iterator i = m_terms.rbegin(), end = m_terms.rend();
         i != end; ++i)
    {
        if (i->is_op(Op::SEGOFF))
            return extract_lhs(i);
    }

    return Expr();
}

Expr
Expr::extract_segoff()
{
    // If not SEG:OFF, we can't do this transformation
    if (!m_terms.back().is_op(Op::SEGOFF))
        return Expr();

    return extract_lhs(m_terms.rbegin());
}

Expr
Expr::extract_wrt()
{
    // If not WRT, we can't do this transformation
    if (!m_terms.back().is_op(Op::WRT))
        return Expr();

    Expr lhs = extract_lhs(m_terms.rbegin());

    // need to keep LHS, and return RHS, so swap before returning.
    swap(lhs);
    return lhs;
}

FloatNum*
Expr::get_float() const
{
    if (m_terms.size() == 1)
        return m_terms.front().get_float();
    else
        return 0;
}

const IntNum*
Expr::get_intnum() const
{
    if (m_terms.size() == 1)
        return m_terms.front().get_int();
    else
        return 0;
}

IntNum*
Expr::get_intnum()
{
    if (m_terms.size() == 1)
        return m_terms.front().get_int();
    else
        return 0;
}

SymbolRef
Expr::get_symbol() const
{
    if (m_terms.size() == 1)
        return m_terms.front().get_sym();
    else
        return SymbolRef(0);
}

const Register*
Expr::get_reg() const
{
    if (m_terms.size() == 1)
        return m_terms.front().get_reg();
    else
        return 0;
}

std::ostream&
operator<< (std::ostream& os, const ExprTerm& term)
{
    switch (term.get_type())
    {
        case ExprTerm::NONE:    os << "NONE"; break;
        case ExprTerm::REG:     os << *term.get_reg(); break;
        case ExprTerm::INT:     os << *term.get_int(); break;
        case ExprTerm::SUBST:   os << "[" << *term.get_subst() << "]"; break;
        case ExprTerm::FLOAT:   os << "FLTN"; break;
        case ExprTerm::SYM:     os << term.get_sym()->get_name(); break;
        case ExprTerm::LOC:     os << "{LOC}"; break;
        case ExprTerm::OP:
            os << "(" << ((int)term.get_op())
               << ", " << term.get_nchild() << ")";
            break;
    }
    return os;
}

static void
infix(std::ostream& os, const Expr& e, int pos=-1)
{
    const char* opstr = "";
    const ExprTerms& terms = e.get_terms();

    if (terms.size() == 0)
        return;

    if (pos < 0)
        pos += terms.size();
    assert(pos >= 0 && pos < static_cast<int>(terms.size()));

    while (pos >= 0 && terms[pos].is_empty())
        --pos;

    if (pos == -1)
        return;

    if (!terms[pos].is_op())
    {
        os << terms[pos];
        return;
    }

    Op::Op op = terms[pos].get_op();
    switch (op)
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
        case Op::NONNUM:    return;
        default:            opstr = " !UNK! "; break;
    }

    std::vector<int> children;
    const ExprTerm& root = terms[pos];
    --pos;
    for (; pos >= 0; --pos)
    {
        const ExprTerm& child = terms[pos];
        if (child.is_empty())
            continue;
        if (child.m_depth <= root.m_depth)
            break;
        if (child.m_depth != root.m_depth+1)
            continue;
        children.push_back(pos);
    }

    for (std::vector<int>::const_reverse_iterator i=children.rbegin(),
         end=children.rend(); i != end; ++i)
    {
        std::ios_base::fmtflags ff = os.flags();

        if (i != children.rbegin())
        {
            os << opstr;
            // Force RHS of shift operations to decimal
            if (op == Op::SHL || op == Op::SHR)
                ff = os.setf(std::ios::dec, std::ios::basefield);
        }

        if (terms[*i].is_op())
        {
            os << '(';
            infix(os, e, *i);
            os << ')';
        }
        else
            os << terms[*i];

        os.setf(ff);
    }
}

std::ostream&
operator<< (std::ostream& os, const Expr& e)
{
    infix(os, e);
    return os;
}

bool
get_children(Expr& e, /*@out@*/ int* lhs, /*@out@*/ int* rhs, int* pos)
{
    ExprTerms& terms = e.get_terms();
    if (*pos < 0)
        *pos += terms.size();

    ExprTerm& root = terms[*pos];
    if (!root.is_op())
        return false;

    *rhs = -1;
    if (lhs)
    {
        if (root.get_nchild() != 2)
            return false;
        *lhs = -1;
    }
    else if (root.get_nchild() != 1)
        return false;

    int n;
    for (n = *pos-1; n >= 0; --n)
    {
        ExprTerm& child = terms[n];
        if (child.is_empty())
            continue;
        if (child.m_depth <= root.m_depth)
            break;
        if (child.m_depth != root.m_depth+1)
            continue;       // not an immediate child

        if (*rhs < 0)
            *rhs = n;
        else if (lhs && *lhs < 0)
            *lhs = n;
        else
            return false;   // too many children
    }
    *pos = n;
    if (*rhs >= 0 && (!lhs || *lhs >= 0))
        return true;
    return false;   // too few children
}

bool
is_neg1_sym(Expr& e,
            /*@out@*/ int* sym,
            /*@out@*/ int* neg1,
            int* pos,
            bool loc_ok)
{
    ExprTerms& terms = e.get_terms();
    if (*pos < 0)
        *pos += terms.size();
    assert(*pos >= 0 && *pos < static_cast<int>(terms.size()));

    ExprTerm& root = terms[*pos];
    if (!root.is_op(Op::MUL) || root.get_nchild() != 2)
        return false;

    bool have_neg1 = false, have_sym = false;
    int n;
    for (n = *pos-1; n >= 0; --n)
    {
        ExprTerm& child = terms[n];
        if (child.is_empty())
            continue;
        if (child.m_depth <= root.m_depth)
            break;
        if (child.m_depth != root.m_depth+1)
            return false;   // more than one level

        if (IntNum* intn = child.get_int())
        {
            if (!intn->is_neg1())
                return false;
            *neg1 = n;
            have_neg1 = true;
        }
        else if (child.is_type(ExprTerm::SYM) ||
                 (loc_ok && child.is_type(ExprTerm::LOC)))
        {
            *sym = n;
            have_sym = true;
        }
        else
            return false;   // something else
    }

    if (have_neg1 && have_sym)
    {
        *pos = n;
        return true;
    }

    return false;
}

} // namespace yasm

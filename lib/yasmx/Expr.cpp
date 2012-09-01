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
#include "yasmx/Expr.h"

#include <algorithm>
#include <iterator>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Config/functional.h"
#include "yasmx/Arch.h"
#include "yasmx/IntNum.h"
#include "yasmx/Symbol.h"


using namespace yasm;
using llvm::APFloat;

/// Look for simple identities that make the entire result constant:
/// 0*&x, -1|x, etc.
static inline bool
isConstantIdentity(Op::Op op, const IntNum& intn)
{
    bool iszero = intn.isZero();
    return ((iszero && op == Op::MUL) ||
            (iszero && op == Op::AND) ||
            (iszero && op == Op::LAND) ||
            (intn.isNeg1() && op == Op::OR));
}

/// Look for simple "left" identities like 0+x, 1*x, etc.
static inline bool
isLeftIdentity(Op::Op op, const IntNum& intn)
{
    bool iszero = intn.isZero();
    return ((intn.isPos1() && op == Op::MUL) ||
            (iszero && op == Op::ADD) ||
            (intn.isNeg1() && op == Op::AND) ||
            (!iszero && op == Op::LAND) ||
            (iszero && op == Op::OR) ||
            (iszero && op == Op::LOR));
}

/// Look for simple "right" identities like x+|-0, x*&/1
static inline bool
isRightIdentity(Op::Op op, const IntNum& intn)
{
    bool iszero = intn.isZero();
    bool ispos1 = intn.isPos1();
    return ((ispos1 && op == Op::MUL) ||
            (ispos1 && op == Op::DIV) ||
            (iszero && op == Op::ADD) ||
            (iszero && op == Op::SUB) ||
            (intn.isNeg1() && op == Op::AND) ||
            (!iszero && op == Op::LAND) ||
            (iszero && op == Op::OR) ||
            (iszero && op == Op::LOR) ||
            (iszero && op == Op::SHL) ||
            (iszero && op == Op::SHR));
}

bool
yasm::CalcFloat(APFloat* lhs,
                Op::Op op,
                const APFloat& rhs,
                SourceLocation source,
                DiagnosticsEngine& diags)
{
    APFloat::opStatus status;
    switch (op)
    {
        case Op::ADD:
            status = lhs->add(rhs, APFloat::rmNearestTiesToEven);
            break;
        case Op::SUB:
            status = lhs->subtract(rhs, APFloat::rmNearestTiesToEven);
            break;
        case Op::MUL:
            status = lhs->multiply(rhs, APFloat::rmNearestTiesToEven);
            break;
        case Op::DIV:
        case Op::SIGNDIV:
            status = lhs->divide(rhs, APFloat::rmNearestTiesToEven);
            break;
        case Op::MOD:
        case Op::SIGNMOD:
            status = lhs->mod(rhs, APFloat::rmNearestTiesToEven);
            break;
        default:
            status = APFloat::opInvalidOp;
            break;
    }

    if (status & APFloat::opInvalidOp)
    {
        diags.Report(source, diag::err_float_invalid_op);
        return false;
    }
    if (status & APFloat::opDivByZero)
    {
        diags.Report(source, diag::err_divide_by_zero);
        return false;
    }
    if (status & APFloat::opOverflow)
        diags.Report(source, diag::warn_float_overflow);
    else if (status & APFloat::opUnderflow)
        diags.Report(source, diag::warn_float_underflow);
    else if (status & APFloat::opInexact)
        diags.Report(source, diag::warn_float_inexact);
    return true;
}

const ExprBuilder yasm::ADD = {Op::ADD};
const ExprBuilder yasm::SUB = {Op::SUB};
const ExprBuilder yasm::MUL = {Op::MUL};
const ExprBuilder yasm::DIV = {Op::DIV};
const ExprBuilder yasm::SIGNDIV = {Op::SIGNDIV};
const ExprBuilder yasm::MOD = {Op::MOD};
const ExprBuilder yasm::SIGNMOD = {Op::SIGNMOD};
const ExprBuilder yasm::NEG = {Op::NEG};
const ExprBuilder yasm::NOT = {Op::NOT};
const ExprBuilder yasm::OR = {Op::OR};
const ExprBuilder yasm::AND = {Op::AND};
const ExprBuilder yasm::XOR = {Op::XOR};
const ExprBuilder yasm::XNOR = {Op::XNOR};
const ExprBuilder yasm::NOR = {Op::NOR};
const ExprBuilder yasm::SHL = {Op::SHL};
const ExprBuilder yasm::SHR = {Op::SHR};
const ExprBuilder yasm::LOR = {Op::LOR};
const ExprBuilder yasm::LAND = {Op::LAND};
const ExprBuilder yasm::LNOT = {Op::LNOT};
const ExprBuilder yasm::LXOR = {Op::LXOR};
const ExprBuilder yasm::LXNOR = {Op::LXNOR};
const ExprBuilder yasm::LNOR = {Op::LNOR};
const ExprBuilder yasm::LT = {Op::LT};
const ExprBuilder yasm::GT = {Op::GT};
const ExprBuilder yasm::EQ = {Op::EQ};
const ExprBuilder yasm::LE = {Op::LE};
const ExprBuilder yasm::GE = {Op::GE};
const ExprBuilder yasm::NE = {Op::NE};
const ExprBuilder yasm::SEG = {Op::SEG};
const ExprBuilder yasm::WRT = {Op::WRT};
const ExprBuilder yasm::SEGOFF = {Op::SEGOFF};

ExprTerm::ExprTerm(std::auto_ptr<IntNum> intn,
                   SourceLocation source,
                   int depth)
    : m_source(source), m_type(INT), m_depth(depth)
{
    m_data.intn.m_type = IntNumData::INTNUM_SV;
    intn->swap(static_cast<IntNum&>(m_data.intn));
}

ExprTerm::ExprTerm(std::auto_ptr<APFloat> flt,
                   SourceLocation source,
                   int depth)
    : m_source(source), m_type(FLOAT), m_depth(depth)
{
    m_data.flt = flt.release();
}

ExprTerm::ExprTerm(const ExprTerm& term)
    : m_source(term.m_source), m_type(term.m_type), m_depth(term.m_depth)
{
    if (m_type == INT)
    {
        m_data.intn.m_type = IntNumData::INTNUM_SV;
        IntNum tmp(static_cast<const IntNum&>(term.m_data.intn));
        tmp.swap(static_cast<IntNum&>(m_data.intn));
    }
    else if (m_type == FLOAT)
        m_data.flt = new APFloat(*term.m_data.flt);
    else
        m_data = term.m_data;
}

void
ExprTerm::swap(ExprTerm& oth)
{
    std::swap(m_source, oth.m_source);
    std::swap(m_type, oth.m_type);
    std::swap(m_depth, oth.m_depth);
    std::swap(m_data, oth.m_data);
}

void
ExprTerm::PromoteToFloat(const llvm::fltSemantics& semantics)
{
    if (m_type == FLOAT)
        return;
    assert (m_type == INT && "trying to promote non-integer");

    std::auto_ptr<APFloat>
        upconvf(new APFloat(semantics, APFloat::fcZero, false));
    llvm::APInt upconvi(IntNum::BITVECT_NATIVE_SIZE, 0);
    upconvf->convertFromAPInt(*getIntNum()->getBV(&upconvi), true,
                              APFloat::rmNearestTiesToEven);

    Clear();
    m_type = FLOAT;
    m_data.flt = upconvf.release();
}

#ifdef WITH_XML
pugi::xml_node
ExprTerm::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("Term");

    if (m_type == NONE)
        return root;

    pugi::xml_attribute type = root.append_attribute("type");
    switch (m_type)
    {
        case REG:   type = "reg"; append_data(root, *m_data.reg); break;
        case INT:   type = "int"; append_data(root, *getIntNum()); break;
        case SUBST: type = "subst"; append_data(root, m_data.subst); break;
        case FLOAT: type = "float"; break; // TODO
        case SYM:
            type = "sym";
            append_data(root, SymbolRef(m_data.sym));
            break;
        case LOC:   type = "loc"; append_data(root, m_data.loc); break;
        case OP:
        {
            type = "op";
            const char* op = "";
            switch (m_data.op.op)
            {
                case Op::ADD:       op = "ADD"; break;
                case Op::SUB:       op = "SUB"; break;
                case Op::MUL:       op = "MUL"; break;
                case Op::DIV:       op = "DIV"; break;
                case Op::SIGNDIV:   op = "SIGNDIV"; break;
                case Op::MOD:       op = "MOD"; break;
                case Op::SIGNMOD:   op = "SIGNMOD"; break;
                case Op::NEG:       op = "NEG"; break;
                case Op::NOT:       op = "NOT"; break;
                case Op::OR:        op = "OR"; break;
                case Op::AND:       op = "AND"; break;
                case Op::XOR:       op = "XOR"; break;
                case Op::XNOR:      op = "XNOR"; break;
                case Op::NOR:       op = "NOR"; break;
                case Op::SHL:       op = "SHL"; break;
                case Op::SHR:       op = "SHR"; break;
                case Op::LOR:       op = "LOR"; break;
                case Op::LAND:      op = "LAND"; break;
                case Op::LNOT:      op = "LNOT"; break;
                case Op::LXOR:      op = "LXOR"; break;
                case Op::LXNOR:     op = "LXNOR"; break;
                case Op::LNOR:      op = "LNOR"; break;
                case Op::LT:        op = "LT"; break;
                case Op::GT:        op = "GT"; break;
                case Op::LE:        op = "LE"; break;
                case Op::GE:        op = "GE"; break;
                case Op::NE:        op = "NE"; break;
                case Op::EQ:        op = "EQ"; break;
                case Op::SEG:       op = "SEG"; break;
                case Op::WRT:       op = "WRT"; break;
                case Op::SEGOFF:    op = "SEGOFF"; break;
                case Op::IDENT:     op = "IDENT"; break;
                default:            break;
            }
            append_child(root, "Op", op);
            append_child(root, "NChild", m_data.op.nchild);
            break;
        }
        default: break;
    }
    root.append_attribute("depth") = m_depth;
    root.append_attribute("source") = m_source.getRawEncoding();
    return root;
}
#endif // WITH_XML

void
Expr::AppendOp(Op::Op op, int nchild, SourceLocation source)
{
    assert(nchild != 0 && "expression must have more than 0 terms");
    switch (nchild)
    {
        case 1:
            if (!isUnary(op))
                op = Op::IDENT;
            break;
        case 2:
            assert(!isUnary(op) &&
                   "unary expression may only have single term");
            break;
        default:
            // more than 2 terms
            assert(isAssociative(op) &&
                   "expression with more than two terms must be associative");
            break;
    }

    for (ExprTerms::iterator i=m_terms.begin(), end=m_terms.end(); i != end;
         ++i)
        i->m_depth++;

    if (op != Op::IDENT)
        m_terms.push_back(ExprTerm(op, nchild, source, 0));
}

Expr::Expr(const Expr& e)
    : m_terms(e.m_terms)
{
}

Expr::Expr(std::auto_ptr<IntNum> intn, SourceLocation source)
{
    m_terms.push_back(ExprTerm(intn, source));
}

Expr::Expr(std::auto_ptr<APFloat> flt, SourceLocation source)
{
    m_terms.push_back(ExprTerm(flt, source));
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
Expr::Clear()
{
    m_terms.clear();
}

void
Expr::Cleanup()
{
    ExprTerms::iterator erasefrom =
        std::remove_if(m_terms.begin(), m_terms.end(),
                       TR1::bind(&ExprTerm::isEmpty, _1));
    m_terms.erase(erasefrom, m_terms.end());
}

/// Reduce depth of a subexpression.
/// @param terms    expression terms
/// @param pos      term index of subexpression operator
/// @param delta    delta to reduce depth by
static void
ReduceDepth(ExprTerms& terms, int pos, int delta=1)
{
    if (pos < 0)
        pos += terms.size();
    ExprTerm& parent = terms[pos];
    if (parent.isOp())
    {
        for (int n=pos-1; n >= 0; --n)
        {
            ExprTerm& child = terms[n];
            if (child.isEmpty())
                continue;
            if (child.m_depth <= parent.m_depth)
                break;      // Stop when we're out of children
            child.m_depth -= delta;
        }
    }
    parent.m_depth -= delta;    // Bring up parent
}

void
Expr::MakeIdent(DiagnosticsEngine& diags, int pos)
{
    if (pos < 0)
        pos += m_terms.size();

    ExprTerm& root = m_terms[pos];
    if (!root.isOp())
        return;

    // If operator has no children, replace it with a zero.
    if (root.getNumChild() == 0)
    {
        root.Zero();
        return;
    }

    // If operator only has one child, may be able to delete operator
    if (root.getNumChild() != 1)
        return;

    Op::Op op = root.getOp();
    bool unary = isUnary(op);
    if (!unary)
    {
        // delete one-term non-unary operators
        ReduceDepth(m_terms, pos);      // bring up child
        root.Clear();
    }
    else if (op < Op::NONNUM)
    {
        // find child
        for (int n=pos-1; n >= 0; --n)
        {
            ExprTerm& child = m_terms[n];
            if (child.isEmpty())
                continue;
            assert(child.m_depth >= root.m_depth);  // must have one child
            if (child.m_depth != root.m_depth+1)
                continue;

            // if a simple integer, compute it
            if (IntNum* intn = child.getIntNum())
            {
                intn->Calc(op, root.getSource(), diags);
                child.m_depth -= 1;
                root.Clear();
            }
            break;
        }
    }

    Cleanup();
}

/// Clear all terms of a subexpression, possibly keeping a single term.
/// @param terms    expression terms
/// @param pos      term index of subexpression operator
/// @param keep     term index of term to keep; -1 to clear all terms
static void
ClearExcept(ExprTerms& terms, int pos, int keep=-1)
{
    if (keep > 0)
        assert(!terms[keep].isOp());      // unsupported
    if (pos < 0)
        pos += terms.size();
    assert(pos >= 0 && pos < static_cast<int>(terms.size()));

    ExprTerm& parent = terms[pos];
    for (int n=pos-1; n >= 0; --n)
    {
        ExprTerm& child = terms[n];
        if (child.isEmpty())
            continue;
        if (child.m_depth <= parent.m_depth)
            break;      // Stop when we're out of children
        if (n == keep)
            continue;
        child.Clear();
    }
}

static int
TransformNegImpl(Expr& e,
                 int pos,
                 int stop_depth,
                 int depth_delta,
                 bool negating)
{
    ExprTerms& terms = e.getTerms();

    int n = pos;
    for (; n >= 0; --n)
    {
        ExprTerm* child = &terms[n];
        if (child->isEmpty())
            continue;

        // Update depth as required
        child->m_depth += depth_delta;
        int child_depth = child->m_depth;

        switch (child->getOp())
        {
            case Op::NEG:
            {
                int new_depth = child->m_depth;
                child->Clear();
                // Invert current negation state and bring up children
                n = TransformNegImpl(e, n-1, new_depth, depth_delta - 1,
                                     !negating);
                break;
            }
            case Op::SUB:
            {
                child->setOp(Op::ADD);
                int new_depth = child->m_depth+1;
                if (negating)
                {
                    // -(a-b) ==> -a+b, so don't negate right side,
                    // but do negate left side.
                    n = TransformNegImpl(e, n-1, new_depth, depth_delta, false);
                    n = TransformNegImpl(e, n-1, new_depth, depth_delta, true);
                }
                else
                {
                    // a-b ==> a+(-1*b), so negate right side only.
                    n = TransformNegImpl(e, n-1, new_depth, depth_delta, true);
                    n = TransformNegImpl(e, n-1, new_depth, depth_delta, false);
                }
                break;
            }
            case Op::ADD:
            {
                if (!negating)
                    break;

                // Negate all children
                int new_depth = child->m_depth+1;
                for (int x = 0, nchild = child->getNumChild(); x < nchild; ++x)
                    n = TransformNegImpl(e, n-1, new_depth, depth_delta, true);
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
                             ExprTerm(child->getOp(),
                                      child->getNumChild()+1,
                                      child->getSource(),
                                      child->m_depth));
                child = &terms[n];      // need to re-get as terms may move
                *child = ExprTerm(-1, child->getSource(), child->m_depth+1);
                break;
            }
            default:
            {
                if (!negating)
                    break;

                // Directly negate if possible (integers or floats)
                if (IntNum* intn = child->getIntNum())
                {
                    intn->CalcAssert(Op::NEG);
                    break;
                }

                if (APFloat* fltn = child->getFloat())
                {
                    fltn->changeSign();
                    break;
                }

                // Couldn't replace directly; instead replace with -1*e
                // Insert -1 one level down, add operator at this level,
                // and move all subterms one level down.
                terms.insert(terms.begin()+n+1, 2,
                             ExprTerm(Op::MUL, 2, child->getSource(),
                                      child->m_depth));
                child = &terms[n];      // need to re-get as terms may move
                terms[n+1] = ExprTerm(-1, child->getSource(), child->m_depth+1);
                child->m_depth++;
                int new_depth = child->m_depth+1;
                for (int x = 0, nchild = child->getNumChild(); x < nchild; ++x)
                    n = TransformNegImpl(e, n-1, new_depth, depth_delta+1,
                                         false);
            }
        }

        if (child_depth <= stop_depth)
            break;
    }

    return n;
}

void
Expr::TransformNeg()
{
    ExprTerm& root = m_terms.back();
    if (!root.isOp())
        return;

    TransformNegImpl(*this, m_terms.size()-1, root.m_depth-1, 0, false);
}

void
Expr::LevelOp(DiagnosticsEngine& diags, bool simplify_reg_mul, int pos)
{
    if (pos < 0)
        pos += m_terms.size();
    assert(pos >= 0 && pos < static_cast<int>(m_terms.size()));

    ExprTerm& root = m_terms[pos];
    if (!root.isOp())
        return;
    Op::Op op = root.getOp();
    SourceLocation op_source = root.getSource();
    bool do_level = isAssociative(op);

    // Only one of intchild and fltchild is active.  If we run into a float,
    // we force all integers to floats.
    ExprTerm* intchild = 0;             // first (really last) integer child
    ExprTerm* fltchild = 0;             // first (really last) float child
    int childnum = root.getNumChild();  // which child this is (0=first)

    for (int n=pos-1; n >= 0; --n)
    {
        ExprTerm& child = m_terms[n];
        if (child.isEmpty())
            continue;
        if (child.m_depth <= root.m_depth)
            break;
        if (child.m_depth != root.m_depth+1)
            continue;
        --childnum;

        // Check for SEG of SEG:OFF, if we match, simplify to just the segment
        if (op == Op::SEG && child.isOp(Op::SEGOFF))
        {
            // Find LHS of SEG:OFF, clearing RHS (OFF) as we go.
            int m = n-1;
            for (int cnum=0; m >= 0; --m)
            {
                ExprTerm& child2 = m_terms[m];
                if (child2.isEmpty())
                    continue;
                if (child2.m_depth <= child.m_depth)
                    break;
                if (child2.m_depth == child.m_depth+1)
                    cnum++;
                if (cnum == 2)
                    break;
                child2.Clear();
            }
            assert(m >= 0);

            // Bring up the SEG portion by two levels
            for (; m >= 0; --m)
            {
                ExprTerm& child2 = m_terms[m];
                if (child2.isEmpty())
                    continue;
                if (child2.m_depth <= child.m_depth)
                    break;
                child2.m_depth -= 2;
            }

            // Delete the operators.
            child.Clear();
            root.Clear();
            return;                 // End immediately since we cleared root.
        }

again:
        if (IntNum* intn = child.getIntNum())
        {
            // Look for identities that will delete the intnum term.
            // Don't simplify 1*REG if simplify_reg_mul is disabled.
            if ((simplify_reg_mul ||
                 op != Op::MUL ||
                 !intn->isPos1() ||
                 !Contains(ExprTerm::REG, pos))
                && root.getNumChild() > 1
                &&
                ((childnum != 0 && isRightIdentity(op, *intn)) ||
                 (childnum == 0 && isLeftIdentity(op, *intn))))
            {
                // Delete intnum from expression
                child.Clear();
                root.AddNumChild(-1);
                continue;
            }
            else if (isConstantIdentity(op, *intn))
            {
                // This is special; it deletes all terms except for
                // the integer.  This means we can terminate
                // immediately after deleting all other terms.
                ClearExcept(m_terms, pos, n);
                --child.m_depth;            // bring up intnum
                root.Clear();               // delete operator
                return;
            }

            // if there's a float child, upconvert and work against it instead
            if (fltchild != 0 && op < Op::NEG)
            {
                child.PromoteToFloat(fltchild->getFloat()->getSemantics());
                goto again;
            }

            if (intchild == 0)
            {
                intchild = &child;
                continue;
            }

            if (op < Op::NONNUM)
            {
                std::swap(*intchild->getIntNum(), *intn);
                intchild->getIntNum()->Calc(op, *intn, op_source, diags);
                child.Clear();
                root.AddNumChild(-1);
            }
        }
        else if (APFloat* fltn = child.getFloat())
        {
            // currently can only handle 5 basic ops: +, -, *, /, %
            if (op >= Op::NEG)
                continue;

            // if there's an integer child, upconvert it
            if (intchild != 0)
            {
                fltchild = intchild;
                intchild = 0;
                fltchild->PromoteToFloat(fltn->getSemantics());
            }

            if (fltchild == 0)
            {
                fltchild = &child;
                continue;
            }

            std::swap(*fltchild->getFloat(), *fltn);
            CalcFloat(fltchild->getFloat(), op, *fltn, op_source, diags);
            child.Clear();
            root.AddNumChild(-1);
        }
        else if (do_level && child.isOp(op))
        {
            root.AddNumChild(child.getNumChild() - 1);
            ReduceDepth(m_terms, n);    // bring up children
            child.Clear();              // delete levelled op
        }
    }

    // If operator only has one child, may be able to delete operator
    if (root.getNumChild() == 1)
    {
        if ((op == Op::IDENT || op == Op::NEG) && fltchild != 0)
        {
            // if unary on a simple float, compute it
            if (op == Op::NEG)
                fltchild->getFloat()->changeSign();
            fltchild->m_depth -= 1;
            root.Clear();
        }
        bool unary = isUnary(op);
        if (unary && op < Op::NONNUM && intchild != 0)
        {
            // if unary on a simple integer, compute it
            intchild->getIntNum()->Calc(op, op_source, diags);
            intchild->m_depth -= 1;
            root.Clear();
        }
        else if (!unary)
        {
            // delete one-term non-unary operators
            ReduceDepth(m_terms, pos);  // bring up children
            root.Clear();
        }
    }
    else if (root.getNumChild() == 0)
        root.Zero();    // If operator has no children, replace it with a zero.
}

void
Expr::Simplify(DiagnosticsEngine& diags, bool simplify_reg_mul)
{
    TransformNeg();

    for (int pos=0, size=m_terms.size(); pos<size; ++pos)
    {
        if (!m_terms[pos].isOp())
            continue;
        LevelOp(diags, simplify_reg_mul, pos);
    }

    Cleanup();
}

bool
Expr::Contains(int type, int pos) const
{
    if (pos < 0)
        pos += m_terms.size();
    assert(pos >= 0 && pos < static_cast<int>(m_terms.size()));

    const ExprTerm& parent = m_terms[pos];
    if (!parent.isOp())
    {
        if (parent.isType(type))
            return true;
        return false;
    }
    for (int n=pos-1; n>=0; --n)
    {
        const ExprTerm& child = m_terms[n];
        if (child.isEmpty())
            continue;
        if (child.m_depth <= parent.m_depth)
            break;  // Stop when we're out of children
        if (child.isType(type))
            return true;
    }
    return false;
}

bool
Expr::Substitute(const ExprTerm* subst_begin, const ExprTerm* subst_end)
{
    for (ExprTerms::iterator i=m_terms.begin(), end=m_terms.end(); i != end;
         ++i)
    {
        const unsigned int* substp = i->getSubst();
        if (!substp)
            continue;
        if (*substp >= (subst_end-subst_begin))
            return false;   // error
        int depth = i->m_depth;
        *i = subst_begin[*substp];
        i->m_depth = depth;
    }
    return true;
}

/// LHS expression extractor.
/// @param e        expression
/// @param pos      position of operator term to be extracted from
static Expr
ExtractLHS(Expr& e, int pos)
{
    ExprTerms& terms = e.getTerms();
    if (pos < 0)
        pos += terms.size();
    assert(pos >= 0 && pos < static_cast<int>(terms.size()));

    Expr retval;
    if (pos == 0)
        return retval;

    ExprTerms& rvterms = retval.getTerms();

    // Delete the operator
    int parent_depth = terms[pos].m_depth;
    terms[pos].Clear();

    // Bring up the RHS terms
    int n = --pos;
    for (; n >= 0; --n)
    {
        ExprTerm& child = terms[n];
        if (child.isEmpty())
            continue;
        if (child.m_depth <= parent_depth)
            break;
        if (n != pos && child.m_depth == parent_depth+1)
            break;      // stop when we've reached the second (LHS) child
        child.m_depth--;
    }

    // Extract the LHS terms.
    for (; n >= 0; --n)
    {
        ExprTerm& child = terms[n];
        if (child.isEmpty())
            continue;
        if (child.m_depth <= parent_depth)
            break;
        // Fix up depth for new expression.
        child.m_depth -= parent_depth+1;
        // Add a NONE term to retval, then swap it with the child.
        rvterms.push_back(ExprTerm());
        std::swap(rvterms.back(), child);
    }

    // We added in reverse order, so fix up.
    std::reverse(rvterms.begin(), rvterms.end());

    // Clean up NONE terms.
    e.Cleanup();

    return retval;
}

Expr
Expr::ExtractDeepSegOff()
{
    // Look through terms for the first SEG:OFF operator
    for (int i = m_terms.size()-1; i >= 0; --i)
    {
        if (m_terms[i].isOp(Op::SEGOFF))
            return ExtractLHS(*this, i);
    }

    return Expr();
}

Expr
Expr::ExtractSegOff()
{
    // If not SEG:OFF, we can't do this transformation
    if (!m_terms.back().isOp(Op::SEGOFF))
        return Expr();

    return ExtractLHS(*this, -1);
}

Expr
Expr::ExtractWRT()
{
    // If not WRT, we can't do this transformation
    if (!m_terms.back().isOp(Op::WRT))
        return Expr();

    Expr lhs = ExtractLHS(*this, -1);

    // need to keep LHS, and return RHS, so swap before returning.
    swap(lhs);
    return lhs;
}

APFloat*
Expr::getFloat() const
{
    assert(isFloat() && "expression is not float");
    return m_terms.front().getFloat();
}

IntNum
Expr::getIntNum() const
{
    assert(isIntNum() && "expression is not intnum");
    return *m_terms.front().getIntNum();
}

SymbolRef
Expr::getSymbol() const
{
    assert(isSymbol() && "expression is not symbol");
    return m_terms.front().getSymbol();
}

const Register*
Expr::getRegister() const
{
    assert(isRegister() && "expression is not register");
    return m_terms.front().getRegister();
}

#ifdef WITH_XML
pugi::xml_node
Expr::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("Expr");
    for (ExprTerms::const_iterator i=m_terms.begin(), end=m_terms.end();
         i != end; ++i)
        append_data(root, *i);

    // generate an easier to read version as an attribute
    SmallString<128> ss;
    llvm::raw_svector_ostream oss(ss);
    Print(oss);
    oss << '\0';
    root.append_attribute("def") = oss.str().data();

    return root;
}
#endif // WITH_XML

void
ExprTerm::Print(raw_ostream& os, int base) const
{
    switch (m_type)
    {
        case NONE:  os << "NONE"; break;
        case REG:   os << *m_data.reg; break;
        case INT:   getIntNum()->Print(os, base); break;
        case SUBST: os << "[" << m_data.subst << "]"; break;
        case FLOAT: os << "FLTN"; break;
        case SYM:   os << m_data.sym->getName(); break;
        case LOC:   os << "{LOC}"; break;
        case OP:
            os << "(" << ((int)m_data.op.op)
               << ", " << m_data.op.nchild << ")";
            break;
    }
}

static void
Infix(raw_ostream& os, const Expr& e, int base, int pos=-1)
{
    const char* opstr = "";
    const ExprTerms& terms = e.getTerms();

    if (terms.size() == 0)
        return;

    if (pos < 0)
        pos += terms.size();
    assert(pos >= 0 && pos < static_cast<int>(terms.size()));

    while (pos >= 0 && terms[pos].isEmpty())
        --pos;

    if (pos == -1)
        return;

    if (!terms[pos].isOp())
    {
        terms[pos].Print(os, base);
        return;
    }

    Op::Op op = terms[pos].getOp();
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

    typedef SmallVector<int, 32> CVector;
    CVector children;
    const ExprTerm& root = terms[pos];
    --pos;
    for (; pos >= 0; --pos)
    {
        const ExprTerm& child = terms[pos];
        if (child.isEmpty())
            continue;
        if (child.m_depth <= root.m_depth)
            break;
        if (child.m_depth != root.m_depth+1)
            continue;
        children.push_back(pos);
    }

    for (CVector::const_reverse_iterator i=children.rbegin(),
         end=children.rend(); i != end; ++i)
    {
        if (i != children.rbegin())
        {
            os << opstr;
            // Force RHS of shift operations to decimal
            if (op == Op::SHL || op == Op::SHR)
                base = 10;
        }

        if (terms[*i].isOp())
        {
            os << '(';
            Infix(os, e, base, *i);
            os << ')';
        }
        else
            terms[*i].Print(os, base);
    }
}

void
Expr::Print(raw_ostream& os, int base) const
{
    Infix(os, *this, base);
}

bool
yasm::getChildren(Expr& e, /*@out@*/ int* lhs, /*@out@*/ int* rhs, int* pos)
{
    ExprTerms& terms = e.getTerms();
    if (*pos < 0)
        *pos += terms.size();

    ExprTerm& root = terms[*pos];
    if (!root.isOp())
        return false;

    *rhs = -1;
    if (lhs)
    {
        if (root.getNumChild() != 2)
            return false;
        *lhs = -1;
    }
    else if (root.getNumChild() != 1)
        return false;

    int n;
    for (n = *pos-1; n >= 0; --n)
    {
        ExprTerm& child = terms[n];
        if (child.isEmpty())
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
yasm::isNeg1Sym(Expr& e,
                /*@out@*/ int* sym,
                /*@out@*/ int* neg1,
                int* pos,
                bool loc_ok)
{
    ExprTerms& terms = e.getTerms();
    if (*pos < 0)
        *pos += terms.size();
    assert(*pos >= 0 && *pos < static_cast<int>(terms.size()));

    ExprTerm& root = terms[*pos];
    if (!root.isOp(Op::MUL) || root.getNumChild() != 2)
        return false;

    bool have_neg1 = false, have_sym = false;
    int n;
    for (n = *pos-1; n >= 0; --n)
    {
        ExprTerm& child = terms[n];
        if (child.isEmpty())
            continue;
        if (child.m_depth <= root.m_depth)
            break;
        if (child.m_depth != root.m_depth+1)
            return false;   // more than one level

        if (IntNum* intn = child.getIntNum())
        {
            if (!intn->isNeg1())
                return false;
            *neg1 = n;
            have_neg1 = true;
        }
        else if (child.isType(ExprTerm::SYM) ||
                 (loc_ok && child.isType(ExprTerm::LOC)))
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

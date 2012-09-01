//
// YASM location utility functions implementation.
//
//  Copyright (C) 2007  Peter Johnson
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
#include "yasmx/Location_util.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/Symbol.h"


using namespace yasm;

namespace {
struct TransformDistBase
{
    virtual ~TransformDistBase() {}
    void operator() (Expr& e, int pos);
    virtual bool TransformDelta(ExprTerm& term, Location loc, Location loc2)
        = 0;
};

// Transforms instances of Symbol-Symbol [Symbol+(-1*Symbol)] into single
// ExprTerms if possible.  Uses a simple n^2 algorithm because n is usually
// quite small.  Also works for loc-loc (or Symbol-loc, loc-Symbol).
void
TransformDistBase::operator() (Expr& e, int pos)
{
    ExprTerms& terms = e.getTerms();
    if (pos < 0)
        pos += static_cast<int>(terms.size());

    ExprTerm& root = terms[pos];
    if (!root.isOp(Op::ADD))
        return;

    // Handle symrec-symrec by checking for (-1*symrec)
    // and symrec term pairs (where both symrecs are in the same
    // segment).
    SmallVector<int, 3> relpos, subpos, subneg1, subroot;

    // Scan for symrec and (-1*symrec) terms (or location equivalents)
    int n = pos-1;
    while (n >= 0)
    {
        ExprTerm& child = terms[n];
        if (child.isEmpty())
        {
            --n;
            continue;
        }
        if (child.m_depth <= root.m_depth)
            break;
        if (child.m_depth != root.m_depth+1)
        {
            --n;
            continue;
        }

        // Remember symrec terms
        if (child.isType(ExprTerm::SYM | ExprTerm::LOC))
        {
            relpos.push_back(pos-n);
            --n;
            continue;
        }

        int curpos = n;
        int sym, neg1;
        // Remember (-1*symrec) terms
        if (isNeg1Sym(e, &sym, &neg1, &n, true))
        {
            subpos.push_back(pos-sym);
            subneg1.push_back(pos-neg1);
            subroot.push_back(pos-curpos);
            continue;
        }

        --n;
    }

    // Match additive and subtractive symbols.
    for (size_t i=0; i<relpos.size(); ++i)
    {
        ExprTerm& relterm = terms[pos-relpos[i]];
        SymbolRef rel = relterm.getSymbol();

        for (size_t j=0; j<subpos.size(); ++j)
        {
            if (subpos[j] == 0xff)
                continue;   // previously matched
            ExprTerm& subterm = terms[pos-subpos[j]];
            SymbolRef sub = subterm.getSymbol();

            // If it's the same symbol, even if it's external,
            // they should cancel out.
            if (rel && rel == sub)
            {
                relterm.Zero();
                terms[pos-subpos[j]].Clear();
                terms[pos-subneg1[j]].Clear();
                terms[pos-subroot[j]].Zero();
                subpos[j] = 0xff;   // mark as matched
                break;
            }

            Location rel_loc;
            if (rel)
            {
                if (!rel->getLabel(&rel_loc))
                    continue;   // external
            }
            else if (const Location* loc = relterm.getLocation())
                rel_loc = *loc;
            else
                assert(false);

            Location sub_loc;
            if (sub)
            {
                if (!sub->getLabel(&sub_loc))
                    continue;   // external
            }
            else if (const Location* loc = subterm.getLocation())
                sub_loc = *loc;
            else
                assert(false);

            // If both are in the same segment, we leave them in the
            // expression but consider them to "match".
            if (rel_loc.bc->getContainer() !=
                sub_loc.bc->getContainer())
                continue;

            if (TransformDelta(relterm, sub_loc, rel_loc))
            {
                // Set the matching (-1*Symbol) term to 0
                // (will remove from expression during simplify)
                terms[pos-subpos[j]].Clear();
                terms[pos-subneg1[j]].Clear();
                terms[pos-subroot[j]].Zero();
                subpos[j] = 0xff;   // mark as matched
                break;  // stop looking
            }
        }
    }
}

struct CalcDistFunctor : public TransformDistBase
{
    bool TransformDelta(ExprTerm& term, Location loc, Location loc2);
};

struct CalcDistNoBCFunctor : public TransformDistBase
{
    bool TransformDelta(ExprTerm& term, Location loc, Location loc2);
};

struct SubstDistFunctor : public TransformDistBase
{
    const TR1::function<void (unsigned int subst,
                              Location loc,
                              Location loc2)>& m_func;
    unsigned int m_subst;

    SubstDistFunctor(const TR1::function<void (unsigned int subst,
                                               Location loc,
                                               Location loc2)>& func)
        : m_func(func), m_subst(0)
    {}

    bool TransformDelta(ExprTerm& term, Location loc, Location loc2);
};
} // anonymous namespace

bool
CalcDistFunctor::TransformDelta(ExprTerm& term, Location loc, Location loc2)
{
    IntNum dist;
    if (!CalcDist(loc, loc2, &dist))
        return false;
    // Change the term to an integer
    term = ExprTerm(dist, term.getSource(), term.m_depth);
    return true;
}

void
yasm::SimplifyCalcDist(Expr& e, DiagnosticsEngine& diags)
{
    CalcDistFunctor functor;
    e.Simplify(diags, functor);
}

bool
CalcDistNoBCFunctor::TransformDelta(ExprTerm& term, Location loc, Location loc2)
{
    IntNum dist;
    if (!CalcDistNoBC(loc, loc2, &dist))
        return false;
    // Change the term to an integer
    term = ExprTerm(dist, term.getSource(), term.m_depth);
    return true;
}

void
yasm::SimplifyCalcDistNoBC(Expr& e, DiagnosticsEngine& diags)
{
    CalcDistNoBCFunctor functor;
    e.Simplify(diags, functor);
}

bool
SubstDistFunctor::TransformDelta(ExprTerm& term, Location loc, Location loc2)
{
    // Call higher-level callback
    m_func(m_subst, loc, loc2);
    // Change the term to an subst
    term = ExprTerm(ExprTerm::Subst(m_subst), term.getSource(),
                    term.m_depth);
    m_subst++;
    return true;
}

int
yasm::SubstDist(Expr& e, DiagnosticsEngine& diags,
                const TR1::function<void (unsigned int subst,
                                          Location loc,
                                          Location loc2)>& func)
{
    SubstDistFunctor functor(func);
    e.Simplify(diags, functor);
    return functor.m_subst;
}

bool
yasm::Evaluate(const Expr& e,
               DiagnosticsEngine& diags,
               ExprTerm* result,
               ArrayRef<ExprTerm> subst,
               bool valueloc,
               bool zeroreg)
{
    if (e.isEmpty())
        return false;

    const ExprTerms& terms = e.getTerms();

    // Shortcut the most common case: a single integer or float
    if (terms.size() == 1
        && terms.front().isType(ExprTerm::INT|ExprTerm::FLOAT))
    {
        *result = terms.front();
        return true;
    }

    SmallVector<ExprTerm, 8> stack;

    for (ExprTerms::const_iterator i=terms.begin(), end=terms.end();
         i != end; ++i)
    {
        const ExprTerm& term = *i;
        if (term.isOp())
        {
            size_t nchild = term.getNumChild();
            assert(stack.size() >= nchild && "not enough terms to evaluate op");
            Op::Op op = term.getOp();
            SourceLocation op_source = term.getSource();

            // Get first child (will be used as result)
            size_t resultindex = stack.size()-nchild;
            ExprTerm& result = stack[resultindex];

            // For SEG:OFF, throw away the SEG and keep the OFF.
            if (op == Op::SEGOFF)
            {
                assert(nchild == 2 && "SEGOFF without 2 terms");
                stack[resultindex].swap(stack[resultindex+1]);
                stack.pop_back();
                continue;
            }

            // Don't allow any other non-numeric operators (e.g. WRT).
            if (op >= Op::NONNUM)
                return false;

            // Calculate through other children
            for (size_t j = resultindex+1; j<stack.size(); ++j)
            {
                ExprTerm& child = stack[j];

                // Promote to float as needed
                if (result.isType(ExprTerm::FLOAT))
                    child.PromoteToFloat(result.getFloat()->getSemantics());
                else if (child.isType(ExprTerm::FLOAT))
                    result.PromoteToFloat(child.getFloat()->getSemantics());

                // Perform calculation
                if (result.isType(ExprTerm::INT))
                    result.getIntNum()->Calc(op, *child.getIntNum(), op_source,
                                             diags);
                else if (op < Op::NEG)
                    CalcFloat(result.getFloat(), op, *child.getFloat(),
                              op_source, diags);
                else
                    return false;
            }

            // Handle unary op
            if (nchild == 1)
            {
                assert(isUnary(op) && "single-term subexpression is non-unary");
                if (result.isType(ExprTerm::INT))
                    result.getIntNum()->Calc(op, op_source, diags);
                else if (op == Op::NEG)
                    result.getFloat()->changeSign();
                else
                    return false;
            }

            // Pop other children off stack, leaving first child as result
            result.m_depth = term.m_depth;
            stack.erase(stack.begin()+resultindex+1, stack.end());
        }
        else if (!term.isEmpty())
        {
            // Convert term to int or float before pushing onto stack.
            // If cannot convert, return false.
            switch (term.getType())
            {
                case ExprTerm::REG:
                    if (!zeroreg)
                        return false;
                    stack.push_back(ExprTerm(0, term.getSource(),
                                             term.m_depth));
                    break;
                case ExprTerm::SUBST:
                {
                    unsigned int substindex = *term.getSubst();
                    if (substindex >= subst.size())
                        return false;
                    assert((subst[substindex].getType() == ExprTerm::INT ||
                            subst[substindex].getType() == ExprTerm::FLOAT) &&
                           "substitution must be integer or float");
                    stack.push_back(subst[substindex]);
                    break;
                }
                case ExprTerm::INT:
                case ExprTerm::FLOAT:
                    stack.push_back(term);
                    break;
                case ExprTerm::SYM:
                {
                    Location loc;
                    if (!valueloc || !term.getSymbol()->getLabel(&loc) ||
                        !loc.bc)
                        return false;
                    stack.push_back(ExprTerm(loc.getOffset(), term.getSource(),
                                             term.m_depth));
                    break;
                }
                case ExprTerm::LOC:
                {
                    Location loc = *term.getLocation();
                    if (!valueloc || !loc.bc)
                        return false;
                    stack.push_back(ExprTerm(loc.getOffset(), term.getSource(),
                                             term.m_depth));
                    break;
                }
                default:
                    return false;
            }
        }
    }

    assert(stack.size() == 1 && "did not fully evaluate expression");
    result->swap(stack.back());
    return true;
}

bool
yasm::Evaluate(const Expr& e,
               DiagnosticsEngine& diags,
               ExprTerm* result,
               bool valueloc,
               bool zeroreg)
{
    return Evaluate(e, diags, result, ArrayRef<ExprTerm>(), valueloc, zeroreg);
}

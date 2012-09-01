//
// Value handling
//
//  Copyright (C) 2006-2007  Peter Johnson
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
#include "yasmx/Value.h"

#include <algorithm>
#include <bitset>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/SmallVector.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Expr.h"
#include "yasmx/Expr_util.h"
#include "yasmx/IntNum.h"
#include "yasmx/Location_util.h"
#include "yasmx/NumericOutput.h"
#include "yasmx/Object.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"


using namespace yasm;

namespace {
// Predicate used in Value::finalize() to remove matching integers.
class TermIsInt
{
public:
    TermIsInt(const IntNum& intn) : m_intn(intn) {}

    bool operator() (const yasm::ExprTerm& term) const
    {
        const IntNum* intn = term.getIntNum();
        return intn && m_intn == *intn;
    }

private:
    IntNum m_intn;
};
} // anonymous namespace

Value::Value(unsigned int size)
    : m_abs(0),
      m_rel(0),
      m_wrt(0),
      m_sub_sym(false),
      m_sub_loc(false),
      m_insn_start(0),
      m_next_insn(0),
      m_seg_of(false),
      m_rshift(0),
      m_shift(0),
      m_ip_rel(false),
      m_jump_target(false),
      m_section_rel(false),
      m_no_warn(false),
      m_sign(false),
      m_size(size)
{
}

Value::Value(unsigned int size, std::auto_ptr<Expr> e)
    : m_abs(e.release()),
      m_rel(0),
      m_wrt(0),
      m_sub_sym(false),
      m_sub_loc(false),
      m_insn_start(0),
      m_next_insn(0),
      m_seg_of(false),
      m_rshift(0),
      m_shift(0),
      m_ip_rel(false),
      m_jump_target(false),
      m_section_rel(false),
      m_no_warn(false),
      m_sign(false),
      m_size(size)
{
}

Value::Value(unsigned int size, SymbolRef sym)
    : m_abs(0),
      m_rel(sym),
      m_wrt(0),
      m_sub_sym(false),
      m_sub_loc(false),
      m_insn_start(0),
      m_next_insn(0),
      m_seg_of(false),
      m_rshift(0),
      m_shift(0),
      m_ip_rel(false),
      m_jump_target(false),
      m_section_rel(false),
      m_no_warn(false),
      m_sign(false),
      m_size(size)
{
}

Value::Value(const Value& oth)
    : m_abs(0),
      m_rel(oth.m_rel),
      m_wrt(oth.m_wrt),
      m_sub(oth.m_sub),
      m_source(oth.m_source),
      m_sub_sym(oth.m_sub_sym),
      m_sub_loc(oth.m_sub_loc),
      m_insn_start(oth.m_insn_start),
      m_next_insn(oth.m_next_insn),
      m_seg_of(oth.m_seg_of),
      m_rshift(oth.m_rshift),
      m_shift(oth.m_shift),
      m_ip_rel(oth.m_ip_rel),
      m_jump_target(oth.m_jump_target),
      m_section_rel(oth.m_section_rel),
      m_no_warn(oth.m_no_warn),
      m_sign(oth.m_sign),
      m_size(oth.m_size)
{
    if (oth.m_abs)
        m_abs.reset(oth.m_abs->clone());
}

Value::~Value()
{
}

void
Value::swap(Value& oth)
{
    m_abs.swap(oth.m_abs);
    std::swap(m_rel, oth.m_rel);
    std::swap(m_wrt, oth.m_wrt);
    std::swap(m_source, oth.m_source);

    // XXX: Can't std::swap the union, so do it the hard way.
    Symbol* sym = 0;        // avoid warning
    Location loc = {0, 0};  // avoid warning
    if (m_sub_sym)
        sym = m_sub.sym;
    else if (m_sub_loc)
        loc = m_sub.loc;
    if (oth.m_sub_sym)
        m_sub.sym = oth.m_sub.sym;
    else if (oth.m_sub_loc)
        m_sub.loc = oth.m_sub.loc;
    if (m_sub_sym)
        oth.m_sub.sym = sym;
    else if (m_sub_loc)
        oth.m_sub.loc = loc;

    // XXX: Have to do manual copy-construct/assignment "swap", as we can't
    // std::swap individual bitfield members.
    unsigned int sub_sym = m_sub_sym;
    unsigned int sub_loc = m_sub_loc;
    unsigned int insn_start = m_insn_start;
    unsigned int next_insn = m_next_insn;
    unsigned int seg_of = m_seg_of;
    unsigned int rshift = m_rshift;
    unsigned int shift = m_shift;
    unsigned int ip_rel = m_ip_rel;
    unsigned int jump_target = m_jump_target;
    unsigned int section_rel = m_section_rel;
    unsigned int no_warn = m_no_warn;
    unsigned int sign = m_sign;
    unsigned int size = m_size;

    m_sub_sym = oth.m_sub_sym;
    m_sub_loc = oth.m_sub_loc;
    m_insn_start = oth.m_insn_start;
    m_next_insn = oth.m_next_insn;
    m_seg_of = oth.m_seg_of;
    m_rshift = oth.m_rshift;
    m_shift = oth.m_shift;
    m_ip_rel = oth.m_ip_rel;
    m_jump_target = oth.m_jump_target;
    m_section_rel = oth.m_section_rel;
    m_no_warn = oth.m_no_warn;
    m_sign = oth.m_sign;
    m_size = oth.m_size;

    oth.m_sub_sym = sub_sym;
    oth.m_sub_loc = sub_loc;
    oth.m_insn_start = insn_start;
    oth.m_next_insn = next_insn;
    oth.m_seg_of = seg_of;
    oth.m_rshift = rshift;
    oth.m_shift = shift;
    oth.m_ip_rel = ip_rel;
    oth.m_jump_target = jump_target;
    oth.m_section_rel = section_rel;
    oth.m_no_warn = no_warn;
    oth.m_sign = sign;
    oth.m_size = size;
}

void
Value::Clear()
{
    m_abs.reset(0);
    m_rel = SymbolRef(0);
    m_wrt = SymbolRef(0);
    m_source = SourceRange();
    m_sub_sym = false;
    m_sub_loc = false;
    m_insn_start = 0;
    m_next_insn = 0;
    m_seg_of = false;
    m_rshift = 0;
    m_shift = 0;
    m_ip_rel = false;
    m_jump_target = false;
    m_section_rel = false;
    m_no_warn = false;
    m_sign = false;
    m_size = 0;
}

void
Value::ClearRelative()
{
    m_rel = SymbolRef(0);
    m_wrt = SymbolRef(0);
    m_sub_sym = false;
    m_sub_loc = false;
    m_seg_of = false;
    m_rshift = 0;
    m_ip_rel = false;
    m_section_rel = false;
}

Value&
Value::operator= (const Value& rhs)
{
    if (this != &rhs)
    {
        if (rhs.m_abs)
            m_abs.reset(rhs.m_abs->clone());
        else
            m_abs.reset(0);
        m_rel = rhs.m_rel;
        m_wrt = rhs.m_wrt;
        m_sub = rhs.m_sub;
        m_source = rhs.m_source;
        m_sub_sym = rhs.m_sub_sym;
        m_sub_loc = rhs.m_sub_loc;
        m_insn_start = rhs.m_insn_start;
        m_next_insn = rhs.m_next_insn;
        m_seg_of = rhs.m_seg_of;
        m_rshift = rhs.m_rshift;
        m_shift = rhs.m_shift;
        m_ip_rel = rhs.m_ip_rel;
        m_jump_target = rhs.m_jump_target;
        m_section_rel = rhs.m_section_rel;
        m_no_warn = rhs.m_no_warn;
        m_sign = rhs.m_sign;
        m_size = rhs.m_size;
    }
    return *this;
}

bool
Value::SubRelative(Object* object, Location sub)
{
    // In order for us to correctly output subtractive relative values, we must
    // have an additive relative portion of the value.  If one doesn't exist,
    // point to a custom absolute symbol.
    if (!m_rel)
    {
        assert(object != 0);
        m_rel = object->getAbsoluteSymbol();
        if (hasSubRelative())
            return false;
        m_sub.loc = sub;
        m_sub_loc = true;
    }
    else
    {
        Location loc;
        // If same section as m_rel, move both into absolute portion.
        // Can't do this if we're doing something fancier with the relative
        // portion.
        if (!m_wrt && !m_seg_of && !m_rshift && !m_section_rel
            && m_rel->getLabel(&loc)
            && loc.bc->getContainer() == sub.bc->getContainer()
            && (!object->getOptions().DisableGlobalSubRelative ||
                (m_rel->getVisibility() & Symbol::GLOBAL) == 0))
        {
            if (!m_abs)
                m_abs.reset(new Expr());
            *m_abs += SUB(m_rel, sub);
            m_rel = SymbolRef(0);
        }
        else
        {
            if (hasSubRelative())
                return false;
            m_sub.loc = sub;
            m_sub_loc = true;
        }
    }
    return true;
}

bool
Value::FinalizeScan(Expr& e, bool ssym_ok, int* pos)
{
    ExprTerms& terms = e.getTerms();
    if (*pos < 0)
        *pos += static_cast<int>(terms.size());

    ExprTerm& root = terms[*pos];
    if (!root.isOp())
        return true;

    // Thanks to this running after a simplify, we don't need to iterate
    // down through IDENTs or handle SUB.
    //
    // We scan for a single symrec, gathering info along the way.  After
    // we've found the symrec, we keep scanning but error if we find
    // another one.  We pull out the single symrec and any legal operations
    // performed on it.
    //
    // Also, if we find a float anywhere, we don't allow mixing of a single
    // symrec with it.
    switch (root.getOp())
    {
        case Op::ADD:
        {
            // Okay for single symrec anywhere in expr.
            // Check for single symrec anywhere.
            // Handle symrec-symrec by checking for (-1*symrec)
            // and symrec term pairs (where both symrecs are in the same
            // segment).
            typedef SmallVector<int, 4> SymIndexes;
            SymIndexes relpos, subpos;

            // Scan for symrec and (-1*symrec) terms
            int n = *pos-1;
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
                if (SymbolRef sym = child.getSymbol())
                {
                    relpos.push_back(*pos-n);
                    --n;
                    continue;
                }

                int sym, neg1;
                // Remember (-1*symrec) terms
                if (isNeg1Sym(e, &sym, &neg1, &n, false))
                {
                    subpos.push_back(*pos-sym);
                    continue;
                }

                // recurse for all other expr
                if (child.isOp())
                {
                    if (!FinalizeScan(e, ssym_ok, &n))
                        return false;
                    continue;
                }

                --n;
            }

            // Match additive and subtractive symbols.
            for (SymIndexes::iterator i=relpos.begin(), endi=relpos.end();
                 i != endi; ++i)
            {
                ExprTerm& relterm = terms[*pos-*i];
                SymbolRef rel = relterm.getSymbol();
                assert(rel != 0);

                bool matched = false;
                for (SymIndexes::iterator j=subpos.begin(), endj=subpos.end();
                     j != endj; ++j)
                {
                    if (*j == -1)
                        continue;   // previously matched
                    ExprTerm& subterm = terms[*pos-*j];
                    SymbolRef sub = subterm.getSymbol();
                    assert(sub != 0);

                    // If it's the same symrec term, even if it's external,
                    // they should cancel out.
                    if (rel == sub)
                    {
                        relterm.Zero();
                        subterm.Zero();
                        *j = -1;        // mark as matched
                        matched = true;
                        break;
                    }

                    // If both are in the same segment, we leave them in the
                    // expression but consider them to "match".
                    Location rel_loc;
                    if (!rel->getLabel(&rel_loc))
                        continue;   // external

                    Location sub_loc;
                    if (!sub->getLabel(&sub_loc))
                        continue;   // external

                    if (rel_loc.bc->getContainer() ==
                        sub_loc.bc->getContainer())
                    {
                        *j = -1;        // mark as matched
                        matched = true;
                        break;
                    }
                }
                if (matched)
                    continue;

                // Must be relative portion
                if (m_rel || !ssym_ok)
                    return false;   // already have one
                m_rel = rel;

                // Set term to 0 (will remove from expression during simplify)
                relterm.Zero();
            }

            // Handle any remaining subtractive symbols.
            for (SymIndexes::iterator i=subpos.begin(), endi=subpos.end();
                 i != endi; ++i)
            {
                if (*i == -1)
                    continue;   // previously matched
                ExprTerm& subterm = terms[*pos-*i];
                SymbolRef sub = subterm.getSymbol();
                assert(sub != 0);

                // Must be subtractive portion
                if (hasSubRelative())
                    return false;   // already have one
                m_sub.sym = sub;
                m_sub_sym = true;

                // Set term to 0 (will remove from expression during simplify)
                subterm.Zero();
            }

            *pos = n;
            break;
        }
        case Op::SHR:
        {
            // Okay for single symrec in LHS and constant on RHS.
            // Single symrecs are not okay on RHS.
            // If RHS is non-constant, don't allow single symrec on LHS.
            // XXX: should rshift be an expr instead??
            int lhs, rhs;
            if (!getChildren(e, &lhs, &rhs, pos))
                return false;

            // Check for single sym on LHS
            SymbolRef sym = terms[lhs].getSymbol();
            if (!sym)
                break;  // ignore if not shifting symbol directly

            // If we already have a sym, we can't take another one
            if (m_rel || !ssym_ok)
                return false;

            // RHS must be a positive integer.
            IntNum* intn = terms[rhs].getIntNum();
            if (!intn || intn->getSign() < 0)
                return false;
            unsigned long shamt = intn->getUInt();
            if ((shamt + m_rshift) > RSHIFT_MAX)
                return false;   // total shift would be too large

            // Update value parameters
            m_rshift += shamt;
            m_rel = sym;

            // Replace symbol with 0
            terms[lhs].Zero();

            // Just leave SHR in place
            break;
        }
        case Op::SEG:
        {
            // Okay for single symrec (can only be done once).
            // Not okay for anything BUT a single symrec as an immediate
            // child.
            int sympos;
            if (!getChildren(e, 0, &sympos, pos))
                return false;

            SymbolRef sym = terms[sympos].getSymbol();
            if (!sym)
                return false;

            if (m_seg_of)
                return false;   // multiple SEG not legal
            m_seg_of = true;

            if (m_rel || !ssym_ok)
                return false;   // got a relative portion somewhere else?
            m_rel = sym;

            // replace with 0 (at root level)
            terms[sympos].Clear();
            root.Zero();
            break;
        }
        case Op::WRT:
        {
            // Okay for single symrec in LHS and either a register or single
            // symrec (as an immediate child) on RHS.
            // If a single symrec on RHS, can only be done once.
            // WRT reg is left in expr for arch to look at.
            int lhs, rhs;
            if (!getChildren(e, &lhs, &rhs, pos))
                return false;

            // Handle RHS
            if (SymbolRef sym = terms[rhs].getSymbol())
            {
                if (m_wrt)
                    return false;
                m_wrt = sym;
                // change the WRT into a +0 expression
                terms[rhs].Zero();
                root.setOp(Op::ADD);
            }
            else if (terms[rhs].isType(ExprTerm::REG))
                ;  // ignore
            else
                return false;

            // Handle LHS
            if (SymbolRef sym = terms[lhs].getSymbol())
            {
                if (m_rel || !ssym_ok)
                    return false;
                m_rel = sym;
                // replace with 0
                terms[lhs].Zero();
            }
            else if (terms[lhs].isOp())
            {
                // recurse
                if (!FinalizeScan(e, ssym_ok, &lhs))
                    return false;
            }
            break;
        }
        default:
        {
            // Single symrec not allowed anywhere
            int n = *pos-1;
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

                if (child.isType(ExprTerm::SYM))
                    return false;

                // recurse all expr
                if (child.isOp())
                {
                    if (!FinalizeScan(e, false, &n))
                        return false;
                    continue;
                }

                --n;
            }

            *pos = n;
            break;
        }
    }
    return true;
}

bool
Value::Finalize(DiagnosticsEngine& diags, unsigned int err_too_complex)
{
    if (!m_abs)
        return true;

    if (m_abs->isEmpty())
    {
        m_abs.reset(0);
        return true;
    }

    if (!ExpandEqu(*m_abs))
    {
        diags.Report(m_source.getBegin(), diag::err_equ_circular_reference);
        return false;
    }
    m_abs->Simplify(diags, false);

    // Strip top-level AND masking to an all-1s mask the same size
    // of the value size.  This allows forced avoidance of overflow warnings.
    if (m_abs->isOp(Op::AND))
    {
        // Calculate 1<<size - 1 value
        IntNum mask = 1;
        mask <<= m_size;
        mask -= 1;

        ExprTerms& terms = m_abs->getTerms();

        // See if any top-level terms match mask and remove them.
        bool found = false;
        TermIsInt is_mask(mask);
        ExprTerm& root = terms.back();
        for (ExprTerms::reverse_iterator i=terms.rbegin();
             i != terms.rend(); ++i)
        {
            if (i->isEmpty())
                continue;
            if (i->m_depth != root.m_depth+1)
                continue;

            if (is_mask(*i))
            {
                i->Clear();
                root.AddNumChild(-1);
                found = true;
            }
        }

        if (found)
        {
            m_no_warn = true;
            m_abs->MakeIdent(diags);
        }
    }

    // Handle trivial (IDENT) cases immediately
    if (m_abs->isIntNum())
    {
        if (m_abs->getIntNum().isZero())
            m_abs.reset(0);
        return true;
    }
    else if (m_abs->isSymbol())
    {
        m_rel = m_abs->getSymbol();
        m_abs.reset(0);
        return true;
    }

    int pos = -1;
    if (!FinalizeScan(*m_abs, true, &pos))
    {
        diags.Report(m_source.getBegin(), err_too_complex);
        return false;
    }

    m_abs->Simplify(diags, false);

    // Simplify 0 in abs to NULL
    if (m_abs->isIntNum())
    {
        if (m_abs->getIntNum().isZero())
            m_abs.reset(0);
    }

    return true;
}

bool
Value::CalcPCRelSub(/*@out@*/ IntNum* out, Location loc) const
{
    // We can handle this as a PC-relative relocation if the
    // subtractive portion is in the current segment.
    Location sub_loc;
    if (!getSubLocation(&sub_loc)
        || sub_loc.bc->getContainer() != loc.bc->getContainer())
        return false;

    // Need to fix up the value to make it PC-relative.
    // This applies the transformation: rel-sub = (rel-.)+(.-sub)
    // The (rel-.) portion is done by the PC-relative relocation,
    // so we just need to add (.-sub) to the outputted value.
    if (!CalcDist(sub_loc, loc, out))
        assert(false);

    return true;
}

bool
Value::getIntNum(IntNum* out, bool calc_bc_dist, DiagnosticsEngine& diags)
{
    // This code could be written more simply, but this is a very hot method,
    // so we need to shortcut all of the common cases.
    if (m_rel || hasSubRelative() || m_wrt)
        return false;   // can't handle relative values

    if (!m_abs)
        *out = 0;
    else if (m_abs->isIntNum())
        *out = m_abs->getIntNum();
    else if (m_abs->isFloat())
        return false;   // a float
    else
    {
        ExprTerm term;
        if (!Evaluate(*m_abs, diags, &term, calc_bc_dist, false)
            || !term.isType(ExprTerm::INT))
            return false;
        out->swap(*term.getIntNum());
    }

    return true;
}

void
Value::AddAbs(const IntNum& delta)
{
    if (m_abs.get() == 0)
        m_abs.reset(new Expr(delta));
    else
        *m_abs += delta;
}

void
Value::AddAbs(const Expr& delta)
{
    if (m_abs.get() == 0)
        m_abs.reset(delta.clone());
    else
        *m_abs += delta;
}

bool
Value::getSubLocation(Location* loc) const
{
    if (m_sub_loc)
    {
        *loc = m_sub.loc;
        return true;
    }
    else if (m_sub_sym)
        return m_sub.sym->getLabel(loc);
    else
        return false;
}

void
Value::ConfigureOutput(NumericOutput* num_out) const
{
    num_out->setSize(m_size);
    num_out->setShift(m_shift);
    num_out->setRShift(m_rshift);
    num_out->setSign(m_sign);
    num_out->setSource(m_source.getBegin());
    if (m_no_warn)
        num_out->DisableWarnings();
    else
        num_out->EnableWarnings();
}

bool
Value::OutputBasic(NumericOutput& num_out,
                   IntNum* outval,
                   DiagnosticsEngine& diags)
{
    // This code could be written more simply, but this is a very hot method,
    // so we need to shortcut all of the common cases.
    bool rel = m_rel || hasSubRelative() || m_wrt;

    // Try to handle common cases first
    if (!rel)
    {
        if (!m_abs)
        {
            num_out.OutputInteger(0);
            return true;
        }
        else if (m_abs->isIntNum())
        {
            num_out.OutputInteger(*m_abs->getTerms().front().getIntNum());
            return true;
        }
        else if (m_abs->isFloat())
        {
            num_out.OutputFloat(*m_abs->getFloat());
            return true;
        }
    }
    else
    {
        if (!m_abs)
        {
            *outval = 0;
            return false;
        }
        else if (m_abs->isIntNum())
        {
            *outval = m_abs->getIntNum();
            return false;
        }
        else if (m_abs->isFloat())
        {
            diags.Report(m_source.getBegin(), diag::err_reloc_contains_float);
            return true;
        }
    }

    // Not trivial, need to evaluate
    ExprTerm term;
    if (!Evaluate(*m_abs, diags, &term))
    {
        // Check for complex float expressions
        if (m_abs->Contains(ExprTerm::FLOAT))
            diags.Report(m_source.getBegin(), diag::err_reloc_contains_float);
        else
            diags.Report(m_source.getBegin(), diag::err_reloc_too_complex);
        return true;
    }

    // Handle float result
    if (term.isType(ExprTerm::FLOAT))
    {
        if (rel)
            diags.Report(m_source.getBegin(), diag::err_reloc_contains_float);
        num_out.OutputFloat(*term.getFloat());
        return true;
    }

    // Handle integer result
    if (!rel)
    {
        num_out.OutputInteger(*term.getIntNum());
        return true;
    }
    else
    {
        term.getIntNum()->swap(*outval);
        return false;
    }
}

#ifdef WITH_XML
pugi::xml_node
Value::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("Value");

    // abs
    if (m_abs.get() != 0)
        append_data(root, *m_abs);

    // rel
    if (m_rel)
        append_child(root, "Rel", m_rel);

    // wrt
    if (m_wrt)
        append_child(root, "WRT", m_wrt);

    // sub
    if (m_sub_sym)
        append_child(root, "Sub", m_sub.sym->getName());
    else if (m_sub_loc)
        append_child(root, "Sub", m_sub.loc);

    root.append_attribute("source.begin") = m_source.getBegin().getRawEncoding();
    root.append_attribute("source.end") = m_source.getEnd().getRawEncoding();
    append_child(root, "InsnStart", m_insn_start);
    if (m_seg_of)
        root.append_attribute("seg_of") = true;
    if (m_rshift > 0)
        append_child(root, "RShift", m_rshift);
    if (m_shift > 0)
        append_child(root, "Shift", m_shift);
    if (m_ip_rel)
    {
        root.append_attribute("ip_rel") = true;
        append_child(root, "NextInsn", m_next_insn);
    }
    if (m_jump_target)
        root.append_attribute("jump_target") = true;
    if (m_section_rel)
        root.append_attribute("section_rel") = true;
    if (m_no_warn)
        root.append_attribute("no_warn") = true;
    append_child(root, "Sign", static_cast<bool>(m_sign));
    append_child(root, "Size", m_size);
    return root;
}
#endif // WITH_XML

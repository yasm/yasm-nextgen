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
#include "value.h"

#include "util.h"

#include <algorithm>
#include <bitset>

#include "arch.h"
#include "bytecode.h"
#include "bytes.h"
#include "compose.h"
#include "errwarn.h"
#include "expr.h"
#include "expr_util.h"
#include "floatnum.h"
#include "intnum.h"
#include "location_util.h"
#include "marg_ostream.h"
#include "object.h"
#include "section.h"
#include "symbol.h"

namespace {

// Predicate used in Value::finalize() to remove matching integers.
class TermIsInt
{
public:
    TermIsInt(const yasm::IntNum& intn) : m_intn(intn) {}

    bool operator() (const yasm::ExprTerm& term) const
    {
        const yasm::IntNum* intn = term.get_int();
        return intn && m_intn == *intn;
    }

private:
    yasm::IntNum m_intn;
};

} // anonymous namespace

namespace yasm
{

Value::Value(unsigned int size)
    : m_abs(0),
      m_rel(0),
      m_wrt(0),
      m_line(0),
      m_sub_sym(false),
      m_sub_loc(false),
      m_next_insn(0),
      m_seg_of(false),
      m_rshift(0),
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
      m_line(0),
      m_sub_sym(false),
      m_sub_loc(false),
      m_next_insn(0),
      m_seg_of(false),
      m_rshift(0),
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
      m_line(0),
      m_sub_sym(false),
      m_sub_loc(false),
      m_next_insn(0),
      m_seg_of(false),
      m_rshift(0),
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
      m_line(oth.m_line),
      m_sub_sym(oth.m_sub_sym),
      m_sub_loc(oth.m_sub_loc),
      m_next_insn(oth.m_next_insn),
      m_seg_of(oth.m_seg_of),
      m_rshift(oth.m_rshift),
      m_ip_rel(oth.m_ip_rel),
      m_jump_target(oth.m_jump_target),
      m_section_rel(oth.m_section_rel),
      m_no_warn(oth.m_no_warn),
      m_sign(oth.m_sign),
      m_size(oth.m_size)
{
    if (oth.m_abs)
        m_abs = oth.m_abs->clone();
}

Value::~Value()
{
    delete m_abs;
}

void
Value::swap(Value& oth)
{
    // Have to use hybrid approach of zeroing what would otherwise be cloned,
    // and then doing copy-construct/assignment "swap", as we can't std::swap
    // individual bitfield members.
    Expr* othabs = oth.m_abs;
    Expr* myabs = m_abs;
    oth.m_abs = 0;
    m_abs = 0;

    // std::swap(*this, oth);
    Value tmpv = oth;
    oth = *this;
    *this = tmpv;

    oth.m_abs = myabs;
    m_abs = othabs;
}

void
Value::clear()
{
    delete m_abs;
    m_abs = 0;
    m_rel = SymbolRef(0);
    m_wrt = SymbolRef(0);
    m_line = 0;
    m_sub_sym = false;
    m_sub_loc = false;
    m_next_insn = 0;
    m_seg_of = false;
    m_rshift = 0;
    m_ip_rel = false;
    m_jump_target = false;
    m_section_rel = false;
    m_no_warn = false;
    m_sign = false;
    m_size = 0;
}

void
Value::clear_rel()
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
        m_abs = 0;
        if (rhs.m_abs)
            m_abs = rhs.m_abs->clone();
        m_rel = rhs.m_rel;
        m_wrt = rhs.m_wrt;
        m_sub = rhs.m_sub;
        m_line = rhs.m_line;
        m_sub_sym = rhs.m_sub_sym;
        m_sub_loc = rhs.m_sub_loc;
        m_next_insn = rhs.m_next_insn;
        m_seg_of = rhs.m_seg_of;
        m_rshift = rhs.m_rshift;
        m_ip_rel = rhs.m_ip_rel;
        m_jump_target = rhs.m_jump_target;
        m_section_rel = rhs.m_section_rel;
        m_no_warn = rhs.m_no_warn;
        m_sign = rhs.m_sign;
        m_size = rhs.m_size;
    }
    return *this;
}

void
Value::sub_rel(Object* object, Location sub)
{
    // In order for us to correctly output subtractive relative values, we must
    // have an additive relative portion of the value.  If one doesn't exist,
    // point to a custom absolute symbol.
    if (!m_rel)
    {
        assert(object != 0);
        m_rel = object->get_absolute_symbol();
        if (has_sub())
            throw TooComplexError(N_("expression too complex"));
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
            && m_rel->get_label(&loc)
            && loc.bc->get_container() == sub.bc->get_container())
        {
            add_abs(Expr::Ptr(new Expr(m_rel, Op::SUB, sub)));
            m_rel = SymbolRef(0);
        }
        else
        {
            if (has_sub())
                throw TooComplexError(N_("expression too complex"));
            m_sub.loc = sub;
            m_sub_loc = true;
        }
    }
}

bool
Value::finalize_scan(Expr* e, bool ssym_not_ok)
{
    ExprTerms& terms = e->get_terms();

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
    switch (e->get_op())
    {
        case Op::ADD:
        {
            // Okay for single symrec anywhere in expr.
            // Check for single symrec anywhere.
            // Handle symrec-symrec by checking for (-1*symrec)
            // and symrec term pairs (where both symrecs are in the same
            // segment).
            if (terms.size() > 32)
                throw Fatal(String::compose(
                    N_("expression on line %1 has too many add terms; internal limit of 32"),
                    m_line));

            // Yes, this has a maximum upper bound on 32 terms, based on an
            // "insane number of terms" (and ease of implementation) WAG.
            // The right way to do this would be a stack-based alloca, but
            // that's not possible with bitset.  We really don't want to alloc
            // here as this function is hit a lot!
            //
            // This is a bitmask to keep things small, as this is a recursive
            // routine and we don't want to eat up stack space.
            std::bitset<32> used;

            for (ExprTerms::iterator i=terms.begin(), end=terms.end();
                 i != end; ++i)
            {

                // First look for an (-1*symrec) term
                Expr* sube = i->get_expr();
                if (!sube)
                    continue;

                ExprTerms& sube_terms = sube->get_terms();
                if (!sube->is_op(Op::MUL) || sube_terms.size() != 2)
                {
                    // recurse instead
                    if (finalize_scan(sube, ssym_not_ok))
                        return true;
                    continue;
                }

                IntNum* intn;
                Symbol* sym;
                if ((intn = sube_terms[0].get_int()) &&
                    (sym = sube_terms[1].get_sym()))
                {
                    ;
                }
                else if ((sym = sube_terms[0].get_sym()) &&
                         (intn = sube_terms[1].get_int()))
                {
                    ;
                }
                else
                {
                    if (finalize_scan(sube, ssym_not_ok))
                        return true;
                    continue;
                }

                if (!intn->is_neg1())
                {
                    if (finalize_scan(sube, ssym_not_ok))
                        return true;
                    continue;
                }

                // Look for the same symrec term; even if both are external,
                // they should cancel out.
                ExprTerms::iterator j = terms.begin();
                for (; j != end; ++j)
                {
                    if (j->get_sym() == sym && !used[j-terms.begin()])
                    {
                        // Mark as used
                        used[j-terms.begin()] = 1;

                        // Replace both symrec portions with 0
                        i->destroy();
                        *i = IntNum(0);
                        //j->destroy(); // unneeded as it's a symbol
                        *j = IntNum(0);

                        break;  // stop looking
                    }
                }
                if (j != end)
                    continue;

                Location loc;
                if (!sym->get_label(&loc))
                {
                    if (finalize_scan(sube, ssym_not_ok))
                        return true;
                    continue;
                }
                BytecodeContainer* container = loc.bc->get_container();

                // Now look for a unused symrec term in the same segment
                j = terms.begin();
                for (; j != end; ++j)
                {
                    Symbol* sym2;
                    Location loc2;
                    BytecodeContainer* container2;
                    if ((sym2 = j->get_sym())
                        && sym2->get_label(&loc2)
                        && (container2 = loc2.bc->get_container())
                        && container == container2
                        && !used[j-terms.begin()])
                    {
                        // Mark as used
                        used[j-terms.begin()] = 1;
                        break;  // stop looking
                    }
                }

                // We didn't match in the same segment.  Save as subtractive
                // relative value.  If we already have one, don't bother;
                // the unmatched symrec will be caught below.
                if (j == end && !has_sub())
                {
                    m_sub.sym = SymbolRef(sym);
                    m_sub_sym = true;
                    i->destroy();
                    *i = IntNum(0);
                }


                if (j == end)
                    return true;    // We didn't find a match!
            }

            // Look for unmatched symrecs.  If we've already found one or
            // we don't WANT to find one, error out.
            for (ExprTerms::iterator i=terms.begin(), end=terms.end();
                 i != end; ++i)
            {
                SymbolRef sym = i->get_sym();
                if (sym && !used[i-terms.begin()])
                {
                    if (m_rel || ssym_not_ok)
                        return true;
                    m_rel = sym;
                    // and replace with 0
                    //i->destroy(); // unneeded as it's a symbol
                    *i = IntNum(0);
                }
            }
            break;
        }
        case Op::SHR:
        {
            // Okay for single symrec in LHS and constant on RHS.
            // Single symrecs are not okay on RHS.
            // If RHS is non-constant, don't allow single symrec on LHS.
            // XXX: should rshift be an expr instead??

            // Check for not allowed cases on RHS
            switch (terms[1].get_type())
            {
                case ExprTerm::REG:
                case ExprTerm::FLOAT:
                    return true;        // not legal
                case ExprTerm::SYM:
                    return true;
                case ExprTerm::EXPR:
                    if (finalize_scan(terms[1].get_expr(), true))
                        return true;
                    break;
                default:
                    break;
            }

            // Check for single sym and allowed cases on LHS
            switch (terms[0].get_type())
            {
                //case ExprTerm::REG:   ????? should this be illegal ?????
                case ExprTerm::FLOAT:
                    return true;        // not legal
                case ExprTerm::SYM:
                    if (m_rel || ssym_not_ok)
                        return true;
                    m_rel = terms[0].get_sym();
                    // and replace with 0
                    //terms[0].destroy(); // unneeded as it's a symbol
                    terms[0] = IntNum(0);
                    break;
                case ExprTerm::EXPR:
                    // recurse
                    if (finalize_scan(terms[0].get_expr(), ssym_not_ok))
                        return true;
                    break;
                default:
                    break;      // ignore
            }

            // Handle RHS
            if (!m_rel)
                break;          // no handling needed
            IntNum* intn = terms[1].get_int();
            if (!intn)
                return true;    // can't shift sym by non-constant integer
            unsigned long shamt = intn->get_uint();
            if ((shamt + m_rshift) > RSHIFT_MAX)
                return true;    // total shift would be too large
            m_rshift += shamt;

            // Just leave SHR in place
            break;
        }
        case Op::SEG:
        {
            // Okay for single symrec (can only be done once).
            // Not okay for anything BUT a single symrec as an immediate
            // child.
            SymbolRef sym = terms[0].get_sym();
            if (!sym)
                return true;

            if (m_seg_of)
                return true;    // multiple SEG not legal
            m_seg_of = true;

            if (m_rel || ssym_not_ok)
                return true;    // got a relative portion somewhere else?
            m_rel = sym;

            // replace with ident'ed 0
            *e = Expr(IntNum(0));
            break;
        }
        case Op::WRT:
        {
            // Okay for single symrec in LHS and either a register or single
            // symrec (as an immediate child) on RHS.
            // If a single symrec on RHS, can only be done once.
            // WRT reg is left in expr for arch to look at.

            // Handle RHS
            if (SymbolRef sym = terms[1].get_sym())
            {
                if (m_wrt)
                    return true;
                m_wrt = sym;
                // and drop the WRT portion
                //terms[1].destroy(); // unneeded as it's a symbol
                terms.pop_back();
                e->make_ident();
            }
            else if (terms[1].is_type(ExprTerm::REG))
                ;  // ignore
            else
                return true;

            // Handle LHS
            if (SymbolRef sym = terms[0].get_sym())
            {
                if (m_rel || ssym_not_ok)
                    return true;
                m_rel = sym;
                // and replace with 0
                //terms[0].destroy(); // unneeded as it's a symbol
                terms[0] = IntNum(0);
            }
            else if (Expr* sube = terms[0].get_expr())
            {
                // recurse
                return finalize_scan(sube, ssym_not_ok);
            }

            break;
        }
        default:
        {
            // Single symrec not allowed anywhere
            for (ExprTerms::iterator i=terms.begin(), end=terms.end();
                 i != end; ++i)
            {
                if (i->is_type(ExprTerm::SYM))
                    return true;
                else if (Expr* sube = i->get_expr())
                {
                    // recurse
                    return finalize_scan(sube, true);
                }
            }
            break;
        }
    }

    return false;
}

bool
Value::finalize()
{
    if (!m_abs)
        return false;

    expand_equ(m_abs);
    m_abs->level_tree(true, true, false);

    // Strip top-level AND masking to an all-1s mask the same size
    // of the value size.  This allows forced avoidance of overflow warnings.
    if (m_abs->is_op(Op::AND))
    {
        // Calculate 1<<size - 1 value
        IntNum m_mask = 1;
        m_mask <<= m_size;
        m_mask -= 1;

        ExprTerms& terms = m_abs->get_terms();

        // See if any terms match mask
        if (std::count_if(terms.begin(), terms.end(), TermIsInt(m_mask)) > 0)
        {
            // Walk terms and delete all matching masks
            ExprTerms::iterator erasefrom =
                std::remove_if(terms.begin(), terms.end(), TermIsInt(m_mask));
            std::for_each(erasefrom, terms.end(),
                          MEMFN::mem_fn(&ExprTerm::destroy));
            terms.erase(erasefrom, terms.end());
            m_abs->make_ident();
            m_no_warn = true;
        }
    }

    // Handle trivial (IDENT) cases immediately
    if (m_abs->is_op(Op::IDENT))
    {
        if (IntNum* intn = m_abs->get_intnum())
        {
            if (intn->is_zero())
            {
                delete m_abs;
                m_abs = 0;
            }
        }
        else if (SymbolRef sym = m_abs->get_symbol())
        {
            m_rel = sym;
            delete m_abs;
            m_abs = 0;
        }
        return false;
    }

    if (finalize_scan(m_abs, false))
        return true;

    m_abs->level_tree(true, true, false);

    // Simplify 0 in abs to NULL
    IntNum* intn;
    if (m_abs->is_op(Op::IDENT)
        && (intn = m_abs->get_terms()[0].get_int())
        && intn->is_zero())
    {
        delete m_abs;
        m_abs = 0;
    }
    return false;
}

bool
Value::calc_pcrel_sub(/*@out@*/ IntNum* out, Location loc) const
{
    // We can handle this as a PC-relative relocation if the
    // subtractive portion is in the current segment.
    Location sub_loc;
    if (!get_sub_loc(&sub_loc)
        || sub_loc.bc->get_container() != loc.bc->get_container())
        return false;

    // Need to fix up the value to make it PC-relative.
    // This applies the transformation: rel-sub = (rel-.)+(.-sub)
    // The (rel-.) portion is done by the PC-relative relocation,
    // so we just need to add (.-sub) to the outputted value.
    if (!calc_dist(sub_loc, loc, out))
        assert(false);

    return true;
}

bool
Value::get_intnum(IntNum* out, bool calc_bc_dist)
{
    // Output 0 if no absolute or relative
    *out = 0;

    if (m_rel || has_sub() || m_wrt)
        return false;   // can't handle relative values

    if (m_abs != 0)
    {
        // Handle integer expressions, if non-integer or too complex, return
        // NULL.
        if (calc_bc_dist)
            m_abs->simplify(xform_calc_dist);
        IntNum* intn = m_abs->get_intnum();

        if (!intn)
        {
            // Second try before erroring: Expr::get_intnum() doesn't handle
            // SEG:OFF, so try simplifying out any to just the OFF portion,
            // then getting the intnum again.
            m_abs->extract_deep_segoff(); // returns auto_ptr, so ok to drop
            m_abs->simplify(xform_calc_dist);
            intn = m_abs->get_intnum();
        }

        if (!intn)
            return false;

        if (out->is_zero())
            *out = *intn;   // micro-optimization because this is hit a lot
        else
            *out += *intn;
    }

    return true;
}

void
Value::add_abs(const IntNum& delta)
{
    if (!m_abs)
        m_abs = new Expr(delta);
    else
        m_abs = new Expr(m_abs, Op::ADD, delta);
}

void
Value::add_abs(std::auto_ptr<Expr> delta)
{
    if (!m_abs)
        m_abs = delta.release();
    else
        m_abs = new Expr(m_abs, Op::ADD, delta);
}

bool
Value::get_sub_loc(Location* loc) const
{
    if (m_sub_loc)
    {
        *loc = m_sub.loc;
        return true;
    }
    else if (m_sub_sym)
        return m_sub.sym->get_label(loc);
    else
        return false;
}

bool
Value::output_basic(Bytes& bytes, int warn, const Arch& arch)
{
    if (m_no_warn)
        warn = 0;

    if (m_abs != 0)
    {
        // Handle floating point expressions
        FloatNum* flt;
        if (!m_rel && (flt = m_abs->get_float()))
        {
            arch.tobytes(*flt, bytes, m_size, 0, warn);
            return true;
        }

        // Check for complex float expressions
        if (m_abs->contains(ExprTerm::FLOAT))
            throw FloatingPointError(
                N_("floating point expression too complex"));
    }

    IntNum outval;
    if (!get_intnum(&outval, true))
        return false;

    // Adjust warn for signed/unsigned integer warnings
    if (warn != 0)
        warn = m_sign ? -1 : 1;

    // Output!
    arch.tobytes(outval, bytes, m_size, 0, warn);
    return true;
}

marg_ostream&
operator<< (marg_ostream& os, const Value& value)
{
    os << value.get_size() << "-bit, ";
    os << (value.is_signed() ? "" : "un") << "signed\n";
    os << "Absolute portion=";
    if (!value.has_abs())
        os << "0";
    else
        os << *value.get_abs();
    os << '\n';
    if (value.is_relative())
    {
        os << "Relative to=";
        os << (value.is_seg_of() ? "SEG " : "");
        os << value.get_rel()->get_name();
        Location sub_loc;
        if (SymbolRef sub = value.get_sub_sym())
            os << " - " << sub->get_name();
        else if (value.get_sub_loc(&sub_loc))
            os << " - " << sub_loc;
        os << '\n';
        if (value.is_wrt())
        {
            os << "(With respect to=" << value.get_wrt()->get_name() << ")\n";
        }
        if (value.get_rshift() > 0)
        {
            os << "(Right shifted by=" << value.get_rshift() << ")\n";
        }
        if (value.is_ip_rel())
            os << "(IP-relative)\n";
        if (value.is_jump_target())
            os << "(Jump target)\n";
        if (value.is_section_rel())
            os << "(Section-relative)\n";
        if (!value.is_warn_enabled())
            os << "(Overflow warnings disabled)\n";
    }
    return os;
}

} // namespace yasm

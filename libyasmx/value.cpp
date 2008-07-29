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
      m_seg_of(false),
      m_rshift(0),
      m_curpos_rel(false),
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
      m_seg_of(false),
      m_rshift(0),
      m_curpos_rel(false),
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
      m_seg_of(false),
      m_rshift(0),
      m_curpos_rel(false),
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
      m_seg_of(oth.m_seg_of),
      m_rshift(oth.m_rshift),
      m_curpos_rel(oth.m_curpos_rel),
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
    m_seg_of = false;
    m_rshift = 0;
    m_curpos_rel = false;
    m_ip_rel = false;
    m_jump_target = false;
    m_section_rel = false;
    m_no_warn = false;
    m_sign = false;
    m_size = 0;
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
        m_seg_of = rhs.m_seg_of;
        m_rshift = rhs.m_rshift;
        m_curpos_rel = rhs.m_curpos_rel;
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
Value::set_curpos_rel(SymbolRef abs_sym, bool ip_rel)
{
    m_curpos_rel = true;
    m_ip_rel = ip_rel;
    // In order for us to correctly output curpos-relative values, we must
    // have a relative portion of the value.  If one doesn't exist, point
    // to a custom absolute symbol.
    if (!m_rel)
        m_rel = abs_sym;
}

bool
Value::finalize_scan(Expr* e, Location expr_loc, bool ssym_not_ok)
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
                    e->get_line()));

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
                    if (finalize_scan(sube, expr_loc, ssym_not_ok))
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
                    if (finalize_scan(sube, expr_loc, ssym_not_ok))
                        return true;
                    continue;
                }

                if (!intn->is_neg1())
                {
                    if (finalize_scan(sube, expr_loc, ssym_not_ok))
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
                    if (finalize_scan(sube, expr_loc, ssym_not_ok))
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

                // We didn't match in the same segment.  If the
                // -1*symrec is actually -1*curpos, we can match
                // unused symrec terms in other segments and generate
                // a curpos-relative reloc.
                //
                // Similarly, handle -1*symrec in other segment via the
                // following transformation:
                // other-this = (other-.)+(.-this)
                // We can only do this transformation if "this" is in
                // this expr's segment.
                //
                // Don't do this if we've already become curpos-relative.
                // The unmatched symrec will be caught below.
                if (j == end && !m_curpos_rel
                    && (sym->is_curpos()
                        || (container == expr_loc.bc->get_container())))
                {
                    for (j=terms.begin(); j != end; ++j)
                    {
                        Symbol* sym2;
                        if ((sym2 = j->get_sym())
                            && !sym2->get_equ()
                            && !sym2->is_special()
                            && !used[j-terms.begin()])
                        {
                            // Mark as used
                            used[j-terms.begin()] = 1;
                            // Mark value as curpos-relative
                            if (m_rel || ssym_not_ok)
                                return true;
                            m_rel = sym2;
                            m_curpos_rel = true;
                            if (sym->is_curpos())
                            {
                                // Replace both symrec portions with 0
                                i->destroy();
                                *i = IntNum(0);
                                //j->destroy(); // unneeded as it's a symbol
                                *j = IntNum(0);
                            }
                            else
                            {
                                // Replace positive portion with curpos
                                //j->destroy(); // unneeded as it's a symbol
                                Object* object = container->get_object();
                                SymbolRef curpos =
                                    object->add_non_table_symbol(
                                        std::auto_ptr<Symbol>(new Symbol(".")));
                                curpos->define_curpos(expr_loc, e->get_line());
                                *j = curpos;
                            }
                            break;      // stop looking
                        }
                    }
                }


                if (j == end)
                    return true;    // We didn't find a match!
            }

            // Look for unmatched symrecs.  If we've already found one or
            // we don't WANT to find one, error out.
            for (ExprTerms::iterator i=terms.begin(), end=terms.end();
                 i != end; ++i)
            {
                Symbol* sym;
                if ((sym = i->get_sym()) && !used[i-terms.begin()])
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
                    if (finalize_scan(terms[1].get_expr(), expr_loc, true))
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
                    if (finalize_scan(terms[0].get_expr(), expr_loc,
                                      ssym_not_ok))
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
            Symbol* sym = terms[0].get_sym();
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
            if (Symbol* sym = terms[1].get_sym())
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
            if (Symbol* sym = terms[0].get_sym())
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
                return finalize_scan(sube, expr_loc, ssym_not_ok);
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
                    return finalize_scan(sube, expr_loc, true);
                }
            }
            break;
        }
    }

    return false;
}

bool
Value::finalize(Location loc)
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

    if (finalize_scan(m_abs, loc, false))
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

std::auto_ptr<IntNum>
Value::get_intnum(Location loc, bool calc_bc_dist)
{
    /*@dependent@*/ /*@null@*/ IntNum* intn = 0;
    std::auto_ptr<IntNum> outval(0);

    if (m_abs != 0)
    {
        // Handle integer expressions, if non-integer or too complex, return
        // NULL.
        if (calc_bc_dist)
            m_abs->level_tree(true, true, true, xform_calc_dist);
        intn = m_abs->get_intnum();
        if (!intn)
            return outval;
    }

    if (m_rel)
    {
        // If relative portion is not in bc section, return NULL.
        // Otherwise get the relative portion's offset.
        Location rel_loc;
        bool sym_local = m_rel->get_label(&rel_loc);
        if (m_wrt || m_seg_of || m_section_rel || !sym_local)
            return outval;  // we can't handle SEG, WRT, or external symbols
        if (rel_loc.bc->get_container() != loc.bc->get_container())
            return outval;  // not in this section
        if (!m_curpos_rel)
            return outval;  // not PC-relative

        // Calculate value relative to current assembly position
        outval.reset(new IntNum(rel_loc.get_offset()));
        *outval -= loc.get_offset();
        if (m_rshift > 0)
            *outval >>= m_rshift;
        // Add in absolute portion
        if (intn)
            *outval += *intn;
        return outval;
    }

    if (intn)
        return std::auto_ptr<IntNum>(intn->clone());

    // No absolute or relative portions: output 0
    return std::auto_ptr<IntNum>(new IntNum(0));
}

std::auto_ptr<IntNum>
Value::get_intnum(bool calc_bc_dist)
{
    /*@dependent@*/ /*@null@*/ IntNum* intn = 0;
    std::auto_ptr<IntNum> outval(0);

    if (m_abs != 0)
    {
        // Handle integer expressions, if non-integer or too complex, return
        // NULL.
        if (calc_bc_dist)
            m_abs->level_tree(true, true, true, xform_calc_dist);
        intn = m_abs->get_intnum();
        if (!intn)
            return outval;
    }

    if (m_rel)
        return outval;  // Can't calculate relative value

    if (intn)
        return std::auto_ptr<IntNum>(intn->clone());

    // No absolute or relative portions: output 0
    return std::auto_ptr<IntNum>(new IntNum(0));
}

void
Value::add_abs(const IntNum& delta)
{
    if (!m_abs)
        m_abs = new Expr(delta);
    else
        m_abs = new Expr(m_abs, Op::ADD, delta, m_abs->get_line());
}

void
Value::add_abs(std::auto_ptr<Expr> delta)
{
    if (!m_abs)
        m_abs = delta.release();
    else
        m_abs = new Expr(m_abs, Op::ADD, delta, m_abs->get_line());
}

bool
Value::output_basic(Bytes& bytes, Location loc, int warn, const Arch& arch)
{
    /*@dependent@*/ /*@null@*/ IntNum* intn = 0;

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

        // Handle normal integer expressions
        m_abs->level_tree(true, true, true, xform_calc_dist);
        intn = m_abs->get_intnum();

        if (!intn)
        {
            // Second try before erroring: Expr::get_intnum() doesn't handle
            // SEG:OFF, so try simplifying out any to just the OFF portion,
            // then getting the intnum again.
            m_abs->extract_deep_segoff(); // returns auto_ptr, so ok to drop
            m_abs->level_tree(true, true, true, xform_calc_dist);
            intn = m_abs->get_intnum();
        }

        if (!intn)
        {
            // Still don't have an integer!
            throw TooComplexError(N_("expression too complex"));
        }
    }

    // Adjust warn for signed/unsigned integer warnings
    if (warn != 0)
        warn = m_sign ? -1 : 1;

    if (m_rel)
    {
        // If relative portion is not in bc section, don't try to handle it
        // here.  Otherwise get the relative portion's offset.
        Location rel_loc;

        bool sym_local = m_rel->get_label(&rel_loc);
        if (m_wrt || m_seg_of || m_section_rel || !sym_local)
            return false;   // we can't handle SEG, WRT, or external symbols
        if (rel_loc.bc->get_container() != loc.bc->get_container())
            return false;   // not in this section
        if (!m_curpos_rel)
            return false;   // not PC-relative

        // Calculate value relative to current assembly position
        IntNum outval(rel_loc.get_offset());
        outval -= loc.get_offset();
        if (m_rshift > 0)
            outval >>= m_rshift;
        // Add in absolute portion
        if (intn)
            outval += *intn;

        // Output!
        arch.tobytes(outval, bytes, m_size, 0, warn);
        return true;
    }

    if (m_seg_of || m_rshift || m_curpos_rel || m_ip_rel || m_section_rel)
        return false;   // We can't handle this with just an absolute

    if (intn)
    {
        // Output just absolute portion
        arch.tobytes(*intn, bytes, m_size, 0, warn);
    }
    else
    {
        // No absolute or relative portions: output 0
        arch.tobytes(0, bytes, m_size, 0, warn);
    }

    return true;
}

marg_ostream&
operator<< (marg_ostream& os, const Value& value)
{
    os << value.m_size << "-bit, ";
    os << (value.m_sign ? "" : "un") << "signed\n";
    os << "Absolute portion=";
    if (!value.has_abs())
        os << "0";
    else
        os << *value.get_abs();
    os << '\n';
    if (value.m_rel)
    {
        os << "Relative to=";
        os << (value.m_seg_of ? "SEG " : "");
        os << value.m_rel->get_name() << '\n';
        if (value.m_wrt)
        {
            os << "(With respect to=" << value.m_wrt->get_name() << ")\n";
        }
        if (value.m_rshift > 0)
        {
            os << "(Right shifted by=" << value.m_rshift << ")\n";
        }
        if (value.m_curpos_rel)
        {
            os << "(Relative to current position)\n";
        }
        if (value.m_ip_rel)
            os << "(IP-relative)\n";
        if (value.m_jump_target)
            os << "(Jump target)\n";
        if (value.m_section_rel)
            os << "(Section-relative)\n";
        if (value.m_no_warn)
            os << "(Overflow warnings disabled)\n";
    }
    return os;
}

} // namespace yasm

//
// x86 effective address handling
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
#include <util.h>

#include "x86effaddr.h"

#include <iomanip>

#include <boost/bind.hpp>

#include <libyasm/errwarn.h>
#include <libyasm/expr.h>
#include <libyasm/intnum.h>

#include "x86arch.h"
#include "x86regtmod.h"


namespace yasm { namespace arch { namespace x86 {

void
set_rex_from_reg(unsigned char *rex, unsigned char *drex,
                 unsigned char *low3, const X86Register* reg,
                 unsigned int bits, X86RexBitPos rexbit)
{
    *low3 = (unsigned char)(reg->num()&7);

    if (bits == 64) {
        if (reg->type() == X86Register::REG8X || reg->num() >= 8) {
            if (drex) {
                *drex |= ((reg->num() & 8) >> 3) << rexbit;
            } else {
                // Check to make sure we can set it
                if (*rex == 0xff)
                    throw TypeError(
                        N_("cannot use A/B/C/DH with instruction needing REX"));
                *rex |= 0x40 | (((reg->num() & 8) >> 3) << rexbit);
            }
        } else if (reg->type() == X86Register::REG8 && (reg->num() & 7) >= 4) {
            // AH/BH/CH/DH, so no REX allowed
            if (*rex != 0 && *rex != 0xff)
                throw TypeError(
                    N_("cannot use A/B/C/DH with instruction needing REX"));
            *rex = 0xff;    // Flag so we can NEVER set it (see above)
        }
    }
}

void
X86EffAddr::init(unsigned int spare, unsigned char drex,
                 bool need_drex, Bytecode* precbc)
{
    if (m_disp.finalize(precbc))
        throw TooComplexError(N_("effective address too complex"));
    m_modrm &= 0xC7;                  /* zero spare/reg bits */
    m_modrm |= (spare << 3) & 0x38;   /* plug in provided bits */
    m_drex = (unsigned char)drex;
    m_need_drex = need_drex;
}

void
X86EffAddr::set_disponly()
{
    m_valid_modrm = 0;
    m_need_modrm = 0;
    m_valid_sib = 0;
    m_need_sib = 0;
    m_need_drex = 0;
}

X86EffAddr::X86EffAddr()
    : EffAddr(std::auto_ptr<Expr>(0)),
      m_modrm(0),
      m_sib(0),
      m_drex(0),
      m_need_sib(0),
      m_valid_modrm(false),
      m_need_modrm(false),
      m_valid_sib(false),
      m_need_drex(false)
{
}

X86EffAddr::X86EffAddr(const X86Register* reg, unsigned char* rex,
                       unsigned char* drex, unsigned int bits)
    : EffAddr(std::auto_ptr<Expr>(0)),
      m_sib(0),
      m_drex(0),
      m_need_sib(0),
      m_valid_modrm(true),
      m_need_modrm(true),
      m_valid_sib(false),
      m_need_drex(false)
{
    unsigned char rm;
    set_rex_from_reg(rex, drex, &rm, reg, bits, X86_REX_B);
    m_modrm = 0xC0 | rm;    // Mod=11, R/M=Reg, Reg=0
}

X86EffAddr::X86EffAddr(const X86EffAddr& rhs)
    : EffAddr(rhs),
      m_modrm(rhs.m_modrm),
      m_sib(rhs.m_sib),
      m_drex(rhs.m_drex),
      m_need_sib(rhs.m_need_sib),
      m_valid_modrm(rhs.m_valid_modrm),
      m_need_modrm(rhs.m_need_modrm),
      m_valid_sib(rhs.m_valid_sib),
      m_need_drex(rhs.m_need_drex)
{
}

void
X86EffAddr::set_reg(const X86Register* reg, unsigned char* rex,
                    unsigned char* drex, unsigned int bits)
{
    unsigned char rm;

    set_rex_from_reg(rex, drex, &rm, reg, bits, X86_REX_B);

    m_modrm = 0xC0 | rm;    // Mod=11, R/M=Reg, Reg=0
    m_valid_modrm = true;
    m_need_modrm = true;
}

std::auto_ptr<Expr>
X86EffAddr::fixup(const X86Arch& arch, std::auto_ptr<Expr> e)
{
    if (arch.parser() == X86Arch::PARSER_GAS) {
        // Need to change foo+rip into foo wrt rip.
        // Note this assumes a particular ordering coming from the parser
        // to work (it's not very smart)!
        if (e->is_op(Op::ADD) && e->get_terms()[0].get_reg() == X86_RIP) {
            // replace register with 0
            e->get_terms()[0] = new IntNum(0);
            // build new wrt expression
            e.reset(new Expr(e.release(), Op::WRT, X86_RIP, e->get_line()));
        }
    }
    return e;
}

X86EffAddr::X86EffAddr(const X86Arch& arch, std::auto_ptr<Expr> e)
    : EffAddr(fixup(arch, e)),
      m_modrm(0),
      m_sib(0),
      m_drex(0),
      // We won't know whether we need an SIB until we know more about expr
      // and the BITS/address override setting.
      m_need_sib(0xff),
      m_valid_modrm(false),
      m_need_modrm(true),
      m_valid_sib(false),
      m_need_drex(false)
{
    m_need_disp = true;
}

X86EffAddr::X86EffAddr(std::auto_ptr<Expr> imm, unsigned int im_len)
    : EffAddr(imm),
      m_modrm(0),
      m_sib(0),
      m_drex(0),
      m_need_sib(0),
      m_valid_modrm(false),
      m_need_modrm(false),
      m_valid_sib(false),
      m_need_drex(false)
{
    m_disp.m_size = im_len;
    m_need_disp = true;
}

void
X86EffAddr::set_imm(std::auto_ptr<Expr> imm, unsigned int im_len)
{
    m_disp = Value(im_len, imm);
    m_need_disp = true;
}

void
X86EffAddr::put(std::ostream& os, int indent_level) const
{
    EffAddr::put(os, indent_level);
    os << std::setw(indent_level) << "";

    os << "ModRM=";
    std::ios_base::fmtflags origff = os.flags();
    os << std::oct << std::setfill('0') << std::setw(3)
       << ((unsigned int)m_modrm);
    os.flags(origff);

    os << " ValidRM=" << m_valid_modrm;
    os << " NeedRM=" << m_need_modrm;

    os << '\n' << std::setw(indent_level) << "" << "SIB=";
    origff = os.flags();
    os << std::oct << std::setfill('0') << std::setw(3)
       << ((unsigned int)m_sib);
    os.flags(origff);

    os << " ValidSIB=" << m_valid_sib;
    os << " NeedSIB=" << ((unsigned int)m_need_sib);

    os << '\n' << std::setw(indent_level) << "" << "DREX=";
    origff = os.flags();
    os << std::hex << std::setfill('0') << std::setw(2)
       << ((unsigned int)m_drex);
    os.flags(origff);

    os << " NeedDREX=" << m_need_drex;
    os << '\n';
}

X86EffAddr*
X86EffAddr::clone() const
{
    return new X86EffAddr(*this);
}

// Only works if term.type == Expr::REG (doesn't check).
// Overwrites term with intnum of 0 (to eliminate regs from the final expr).
static /*@null@*/ /*@dependent@*/ int*
get_reg3264(Expr::Term& term, int& regnum, int* regs, unsigned char bits,
            unsigned char addrsize)
{
    const X86Register* reg = static_cast<const X86Register*>(term.get_reg());
    assert(reg != 0);
    switch (reg->type()) {
        case X86Register::REG32:
            if (addrsize != 32)
                return 0;
            regnum = reg->num();
            break;
        case X86Register::REG64:
            if (addrsize != 64)
                return 0;
            regnum = reg->num();
            break;
        case X86Register::RIP:
            if (bits != 64)
                return 0;
            regnum = 16;
            break;
        default:
            return 0;
    }

    // overwrite with 0 to eliminate register from displacement expr
    term = new IntNum(0);

    // we're okay
    return &regs[regnum];
}

// Only works if term.type == Expr::REG (doesn't check).
// Overwrites term with intnum of 0 (to eliminate regs from the final expr).
static /*@null@*/ int*
x86_expr_checkea_get_reg16(Expr::Term& term, int& regnum, int& bx, int& si,
                           int& di, int& bp)
{
    // in order: ax,cx,dx,bx,sp,bp,si,di
    /*@-nullassign@*/
    static int* reg16[8] = {0,0,0,0,0,0,0,0};
    /*@=nullassign@*/

    reg16[3] = &bx;
    reg16[5] = &bp;
    reg16[6] = &si;
    reg16[7] = &di;

    const X86Register* reg = static_cast<const X86Register*>(term.get_reg());
    assert(reg != 0);

    // don't allow 32-bit registers
    if (reg->type() != X86Register::REG16)
        return 0;

    // & 7 for sanity check
    regnum = reg->num() & 0x7;

    // only allow BX, SI, DI, BP
    if (!reg16[regnum])
        return 0;

    // overwrite with 0 to eliminate register from displacement expr
    term = new IntNum(0);

    // we're okay
    return reg16[regnum];
}

// Distribute over registers to help bring them to the topmost level of e.
// Also check for illegal operations against registers.
// Returns 0 if something was illegal, 1 if legal and nothing in e changed,
// and 2 if legal and e needs to be simplified.
//
// Only half joking: Someday make this/checkea able to accept crazy things
//  like: (bx+di)*(bx+di)-bx*bx-2*bx*di-di*di+di?  Probably not: NASM never
//  accepted such things, and it's doubtful such an expn is valid anyway
//  (even though the above one is).  But even macros would be hard-pressed
//  to generate something like this.
//
// e must already have been simplified for this function to work properly
// (as it doesn't think things like SUB are valid).
//
// IMPLEMENTATION NOTE: About the only thing this function really needs to
// "distribute" is: (non-float-expn or intnum) * (sum expn of registers).
//
// TODO: Clean up this code, make it easier to understand.
static int
x86_expr_checkea_distcheck_reg(Expr* e, unsigned int bits)
{
    Expr::Terms::iterator end = e->get_terms().end();
    Expr::Terms::iterator havereg = end;
    Expr::Terms::iterator havereg_expr = end;
    int retval = 1;     /* default to legal, no changes */

    for (Expr::Terms::iterator i=e->get_terms().begin(); i != end; ++i) {
        switch (i->get_type()) {
            case Expr::REG:
                /* Check op to make sure it's valid to use w/register. */
                switch (e->get_op()) {
                    case Op::MUL:
                        // Check for reg*reg
                        if (havereg != end)
                            return 0;
                        break;
                    case Op::ADD:
                    case Op::IDENT:
                        break;
                    default:
                        return 0;
                }
                havereg = i;
                break;
            case Expr::FLOAT:
                // Floats not allowed.
                return 0;
            case Expr::EXPR:
            {
                Expr* sube = i->get_expr();
                assert(sube != 0);
                if (sube->contains(Expr::REG)) {
                    int ret2;

                    // Check op to make sure it's valid to use w/register.
                    if (!e->is_op(Op::ADD) && !e->is_op(Op::MUL))
                        return 0;
                    // Check for reg*reg
                    if (e->is_op(Op::MUL) && havereg != end)
                        return 0;
                    havereg = i;
                    havereg_expr = i;
                    // Recurse to check lower levels
                    ret2 = x86_expr_checkea_distcheck_reg(sube, bits);
                    if (ret2 == 0)
                        return 0;
                    if (ret2 == 2)
                        retval = 2;
                } else if (sube->contains(Expr::FLOAT))
                    return 0;   // Disallow floats
                break;
            }
            default:
                break;
        }
    }

    // just exit if no registers were used
    if (havereg == end)
        return retval;

    // Distribute
    if (e->is_op(Op::MUL) && havereg_expr != end) {
        Expr* sube = havereg_expr->get_expr();
        assert(sube != 0);

        retval = 2;     // we're going to change it

        // The reg expn *must* be ADD at this point.  Sanity check.
        assert(sube->is_op(Op::ADD));

        // Iterate over each term in reg expn
        for (Expr::Terms::iterator i=sube->get_terms().begin(),
             end2=sube->get_terms().end(); i != end2; ++i) {
            // Copy everything EXCEPT havereg_expr term into new expression
            std::auto_ptr<Expr>
                ne(e->clone(havereg_expr - e->get_terms().begin()));
            // Add reg expr term to new expn
            ne->get_terms().push_back(*i);
            // Don't destroy it, as we copied directly
            i->release();
            // Overwrite old reg expr term with new expn
            *i = ne;
        }

        // Replace e with expanded reg expn
        havereg_expr->release();    // don't delete it!
        *e = *sube;
    }

    return retval;
}

// Simplify and determine if expression is superficially valid:
// Valid expr should be [(int-equiv expn)]+[reg*(int-equiv expn)+...]
// where the [...] parts are optional.
//
// Don't simplify out constant identities if we're looking for an indexreg: we
// may need the multiplier for determining what the indexreg is!
//
// Returns 1 if invalid register usage, 2 if unable to determine all values,
// and 0 if all values successfully determined and saved in data.
static int
x86_expr_checkea_getregusage(Expr* e, /*@null@*/ int* indexreg,
    bool* pcrel, unsigned int bits,
    boost::function <int* (Expr::Term& term, int& regnum)> get_reg)
{
    int* reg;
    int regnum;
    int indexval = 0;
    int indexmult = 0;

    e->level_tree(true, true, indexreg == 0);

    // Check for WRT rip first
    std::auto_ptr<Expr> wrt = e->extract_wrt();
    if (wrt.get() != 0 && wrt->is_op(Op::IDENT) &&
        wrt->get_terms()[0].is_type(Expr::REG)) {
        if (bits != 64)     // only valid in 64-bit mode
            return 1;
        reg = get_reg(wrt->get_terms()[0], regnum);
        if (!reg || regnum != 16)   // only accept rip
            return 1;
        (*reg)++;

        // Delete WRT.  Set pcrel to 1 to indicate to x86
        // bytecode code to do PC-relative displacement transform.
        *pcrel = true;
    } else if (wrt.get() != 0) {
        return 1;
    }

    switch (x86_expr_checkea_distcheck_reg(e, bits)) {
        case 0:
            return 1;
        case 2:
            // Need to simplify again
            e->level_tree(true, true, indexreg == 0);
            break;
        default:
            break;
    }

    switch (e->get_op()) {
        case Op::ADD:
            // Prescan for non-int multipliers against a reg.
            // This is invalid due to the optimizer structure.
            for (Expr::Terms::iterator i=e->get_terms().begin(),
                 end=e->get_terms().end(); i != end; ++i) {
                if (Expr* sube = i->get_expr()) {
                    sube->order_terms();
                    Expr::Terms& terms = sube->get_terms();
                    if (terms[0].is_type(Expr::REG)) {
                        if (terms.size() > 2)
                            return 1;
                        if (!terms[1].is_type(Expr::INT))
                            return 1;
                    }
                }
            }
            /*@fallthrough@*/
        case Op::IDENT:
            // Check each term for register (and possible multiplier).
            for (Expr::Terms::iterator i=e->get_terms().begin(),
                 end=e->get_terms().end(); i != end; ++i) {
                if (i->is_type(Expr::REG)) {
                    reg = get_reg(*i, regnum);
                    if (!reg)
                        return 1;
                    (*reg)++;
                    // Let last, largest multipler win indexreg
                    if (indexreg && *reg > 0 && indexval <= *reg &&
                        !indexmult) {
                        *indexreg = regnum;
                        indexval = *reg;
                    }
                } else if (Expr* sube = i->get_expr()) {
                    // Already ordered from ADD above, just grab the value.
                    // Sanity check for EXPR_INT.
                    Expr::Terms& terms = sube->get_terms();
                    if (terms[0].is_type(Expr::REG)) {
                        IntNum* intn = terms[1].get_int();
                        if (!intn)
                            throw InternalError(
                                N_("Non-integer value in reg expn"));
                        reg = get_reg(terms[0], regnum);
                        if (!reg)
                            return 1;
                        (*reg) += intn->get_int();
                        // Let last, largest multipler win indexreg
                        if (indexreg && *reg > 0 && indexval <= *reg) {
                            *indexreg = regnum;
                            indexval = *reg;
                            indexmult = 1;
                        }
                    }
                }
            }
            break;
        case Op::MUL:
        {
            // Here, too, check for non-int multipliers against a reg.
            e->order_terms();
            Expr::Terms& terms = e->get_terms();
            if (terms[0].is_type(Expr::REG)) {
                if (terms.size() > 2)
                    return 1;
                IntNum* intn = terms[1].get_int();
                if (!intn)
                    return 1;
                reg = get_reg(terms[0], regnum);
                if (!reg)
                    return 1;
                (*reg) += intn->get_int();
                if (indexreg)
                    *indexreg = regnum;
            }
            break;
        }
        default:
            // Should never get here!
            assert(false); // unexpected expr op
    }

    // Simplify expr, which is now really just the displacement. This
    // should get rid of the 0's we put in for registers in the callback.
    e->simplify();
    return 0;
}

/*@-nullstate@*/
void
X86EffAddr::calc_displen(unsigned int wordsize, bool noreg, bool dispreq)
{
    m_valid_modrm = false;      // default to not yet valid

    switch (m_disp.m_size) {
        case 0:
            break;
        // If not 0, the displacement length was forced; set the Mod bits
        // appropriately and we're done with the ModRM byte.
        case 8:
            // Byte is only a valid override if there are registers in the
            // EA.  With no registers, we must have a 16/32 value.
            if (noreg) {
                warn_set(WARN_GENERAL, N_("invalid displacement size; fixed"));
                m_disp.m_size = wordsize;
            } else
                m_modrm |= 0100;
            m_valid_modrm = true;
            return;
        case 16:
        case 32:
            // Don't allow changing displacement different from BITS setting
            // directly; require an address-size override to change it.
            if (wordsize != m_disp.m_size) {
                throw ValueError(
                    N_("invalid effective address (displacement size)"));
            }
            if (!noreg)
                m_modrm |= 0200;
            m_valid_modrm = true;
            return;
        default:
            // we shouldn't ever get any other size!
            throw InternalError(N_("strange EA displacement size"));
    }

    // The displacement length hasn't been forced (or the forcing wasn't
    // valid), try to determine what it is.
    if (noreg) {
        // No register in ModRM expression, so it must be disp16/32,
        // and as the Mod bits are set to 0 by the caller, we're done
        // with the ModRM byte.
        m_disp.m_size = wordsize;
        m_valid_modrm = true;
        return;
    }

    if (dispreq) {
        // for BP/EBP, there *must* be a displacement value, but we
        // may not know the size (8 or 16/32) for sure right now.
        m_need_nonzero_len = true;
    }

    if (m_disp.m_rel) {
        // Relative displacement; basically all object formats need non-byte
        // for relocation here, so just do that. (TODO: handle this
        // differently?)
        m_disp.m_size = wordsize;
        m_modrm |= 0200;
        m_valid_modrm = true;
        return;
    }

    // At this point there's 3 possibilities for the displacement:
    //  - None (if =0)
    //  - signed 8 bit (if in -128 to 127 range)
    //  - 16/32 bit (word size)
    // For now, check intnum value right now; if it's not 0,
    // assume 8 bit and set up for allowing 16 bit later.
    // FIXME: The complex expression equaling zero is probably a rare case,
    // so we ignore it for now.
    /*@null@*/ std::auto_ptr<IntNum> num = m_disp.get_intnum(0, false);
    if (num.get() == 0) {
        /* Still has unknown values. */
        m_need_nonzero_len = true;
        m_modrm |= 0100;
        m_valid_modrm = true;
        return;
    }

    // Figure out what size displacement we will have.
    if (num->is_zero() && !m_need_nonzero_len) {
        // If we know that the displacement is 0 right now,
        // go ahead and delete the expr and make it so no
        // displacement value is included in the output.
        // The Mod bits of ModRM are set to 0 above, and
        // we're done with the ModRM byte!
        m_disp.clear();
        m_need_disp = false;
    } else if (num->in_range(-128, 127)) {
        // It fits into a signed byte
        m_disp.m_size = 8;
        m_modrm |= 0100;
    } else {
        // It's a 16/32-bit displacement
        m_disp.m_size = wordsize;
        m_modrm |= 0200;
    }
    m_valid_modrm = true;   // We're done with ModRM
}
/*@=nullstate@*/

static bool
getregsize(const Expr::Term& term, unsigned char* addrsize)
{
    if (const X86Register* reg =
        static_cast<const X86Register*>(term.get_reg())) {
        switch (reg->type()) {
            case X86Register::REG16:
                *addrsize = 16;
                break;
            case X86Register::REG32:
                *addrsize = 32;
                break;
            case X86Register::REG64:
            case X86Register::RIP:
                *addrsize = 64;
                break;
            default:
                return false;
        }
        return true;
    } else
        return false;
}

bool
X86EffAddr::check(unsigned char* addrsize, unsigned int bits,
                  bool address16_op, unsigned char* rex, Bytecode& bc)
{
    if (*addrsize == 0) {
        // we need to figure out the address size from what we know about:
        // - the displacement length
        // - what registers are used in the expression
        // - the bits setting
        switch (m_disp.m_size) {
            case 16:
                // must be 16-bit
                *addrsize = 16;
                break;
            case 64:
                // We have to support this for the MemOffs case, but it's
                // otherwise illegal.  It's also illegal in non-64-bit mode.
                if (m_need_modrm || m_need_sib) {
                    throw ValueError(
                        N_("invalid effective address (displacement size)"));
                }
                *addrsize = 64;
                break;
            case 32:
                // Must be 32-bit in 16-bit or 32-bit modes.  In 64-bit mode,
                // we don't know unless we look at the registers, except in the
                // MemOffs case (see the end of this function).
                if (bits != 64 || (!m_need_modrm && !m_need_sib)) {
                    *addrsize = 32;
                    break;
                }
                /*@fallthrough@*/
            default:
                // check for use of 16 or 32-bit registers; if none are used
                // default to bits setting.
                if (!m_disp.has_abs() ||
                    !m_disp.get_abs()->traverse_leaves_in(
                        boost::bind(&getregsize, _1, addrsize)))
                    *addrsize = bits;
                // TODO: Add optional warning here if switched address size
                // from bits setting just by register use.. eg [ax] in
                // 32-bit mode would generate a warning.
        }
    }

    if ((*addrsize == 32 || *addrsize == 64) &&
        ((m_need_modrm && !m_valid_modrm) || (m_need_sib && !m_valid_sib))) {
        int i;
        unsigned char* drex = m_need_drex ? &m_drex : 0;
        unsigned char low3;
        enum Reg3264Type {
            REG3264_NONE = -1,
            REG3264_EAX = 0,
            REG3264_ECX,
            REG3264_EDX,
            REG3264_EBX,
            REG3264_ESP,
            REG3264_EBP,
            REG3264_ESI,
            REG3264_EDI,
            REG64_R8,
            REG64_R9,
            REG64_R10,
            REG64_R11,
            REG64_R12,
            REG64_R13,
            REG64_R14,
            REG64_R15,
            REG64_RIP
        };
        int reg3264mult[17] = {0, 0, 0, 0, 0, 0, 0, 0,
                               0, 0, 0, 0, 0, 0, 0, 0, 0};
        int basereg = REG3264_NONE;     // "base" register (for SIB)
        int indexreg = REG3264_NONE;    // "index" register (for SIB)

        // We can only do 64-bit addresses in 64-bit mode.
        if (*addrsize == 64 && bits != 64) {
            throw TypeError(
                N_("invalid effective address (64-bit in non-64-bit mode)"));
        }

        if (m_pc_rel && bits != 64) {
            warn_set(WARN_GENERAL,
                N_("RIP-relative directive ignored in non-64-bit mode"));
            m_pc_rel = false;
        }

        if (m_disp.has_abs()) {
            bool pcrel = false;
            switch (x86_expr_checkea_getregusage
                    (m_disp.get_abs(), &indexreg, &pcrel, bits,
                     boost::bind(&get_reg3264, _1, _2, reg3264mult, bits,
                                 *addrsize))) {
                case 1:
                    throw ValueError(N_("invalid effective address"));
                case 2:
                    if (pcrel)
                        m_disp.set_curpos_rel(bc, true);
                    return false;
                default:
                    if (pcrel)
                        m_disp.set_curpos_rel(bc, true);
                    break;
            }
        }

        // If indexreg mult is 0, discard it.
        // This is possible because of the way indexreg is found in
        // expr_checkea_getregusage().
        if (indexreg != REG3264_NONE && reg3264mult[indexreg] == 0)
            indexreg = REG3264_NONE;

        // Find a basereg (*1, but not indexreg), if there is one.
        // Also, if an indexreg hasn't been assigned, try to find one.
        // Meanwhile, check to make sure there's no negative register mults.
        for (i=0; i<17; i++) {
            if (reg3264mult[i] < 0)
                throw ValueError(N_("invalid effective address"));
            if (i != indexreg && reg3264mult[i] == 1 &&
                basereg == REG3264_NONE)
                basereg = i;
            else if (indexreg == REG3264_NONE && reg3264mult[i] > 0)
                indexreg = i;
        }

        // Handle certain special cases of indexreg mults when basereg is
        // empty.
        if (indexreg != REG3264_NONE && basereg == REG3264_NONE)
            switch (reg3264mult[indexreg]) {
                case 1:
                    // Only optimize this way if nosplit wasn't specified
                    if (!m_nosplit) {
                        basereg = indexreg;
                        indexreg = -1;
                    }
                    break;
                case 2:
                    // Only split if nosplit wasn't specified
                    if (!m_nosplit) {
                        basereg = indexreg;
                        reg3264mult[indexreg] = 1;
                    }
                    break;
                case 3:
                case 5:
                case 9:
                    basereg = indexreg;
                    reg3264mult[indexreg]--;
                    break;
            }

        // Make sure there's no other registers than the basereg and indexreg
        // we just found.
        for (i=0; i<17; i++)
            if (i != basereg && i != indexreg && reg3264mult[i] != 0)
                throw ValueError(N_("invalid effective address"));

        // Check the index multiplier value for validity if present.
        if (indexreg != REG3264_NONE && reg3264mult[indexreg] != 1 &&
            reg3264mult[indexreg] != 2 && reg3264mult[indexreg] != 4 &&
            reg3264mult[indexreg] != 8)
            throw ValueError(N_("invalid effective address"));

        // ESP is not a legal indexreg.
        if (indexreg == REG3264_ESP) {
            // If mult>1 or basereg is ESP also, there's no way to make it
            // legal.
            if (reg3264mult[REG3264_ESP] > 1 || basereg == REG3264_ESP)
                throw ValueError(N_("invalid effective address"));

            // If mult==1 and basereg is not ESP, swap indexreg w/basereg.
            indexreg = basereg;
            basereg = REG3264_ESP;
        }

        // RIP is only legal if it's the ONLY register used.
        if (indexreg == REG64_RIP ||
            (basereg == REG64_RIP && indexreg != REG3264_NONE))
            throw ValueError(N_("invalid effective address"));

        // At this point, we know the base and index registers and that the
        // memory expression is (essentially) valid.  Now build the ModRM and
        // (optional) SIB bytes.

        // If we're supposed to be RIP-relative and there's no register
        // usage, change to RIP-relative.
        if (basereg == REG3264_NONE && indexreg == REG3264_NONE && m_pc_rel) {
            basereg = REG64_RIP;
            m_disp.set_curpos_rel(bc, true);
        }

        // First determine R/M (Mod is later determined from disp size)
        m_need_modrm = true;    // we always need ModRM
        if (basereg == REG3264_NONE && indexreg == REG3264_NONE) {
            // Just a disp32: in 64-bit mode the RM encoding is used for RIP
            // offset addressing, so we need to use the SIB form instead.
            if (bits == 64) {
                m_modrm |= 4;
                m_need_sib = 1;
            } else {
                m_modrm |= 5;
                m_sib = 0;
                m_valid_sib = false;
                m_need_sib = 0;
            }
        } else if (basereg == REG64_RIP) {
            m_modrm |= 5;
            m_sib = 0;
            m_valid_sib = false;
            m_need_sib = 0;
            // RIP always requires a 32-bit displacement
            m_valid_modrm = true;
            m_disp.m_size = 32;
            return true;
        } else if (indexreg == REG3264_NONE) {
            // basereg only
            // Don't need to go to the full effort of determining what type
            // of register basereg is, as set_rex_from_reg doesn't pay
            // much attention.
            set_rex_from_reg(rex, drex, &low3, X86_REG64[basereg], bits,
                             X86_REX_B);
            m_modrm |= low3;
            // we don't need an SIB *unless* basereg is ESP or R12
            if (basereg == REG3264_ESP || basereg == REG64_R12)
                m_need_sib = 1;
            else {
                m_sib = 0;
                m_valid_sib = false;
                m_need_sib = 0;
            }
        } else {
            // index or both base and index
            m_modrm |= 4;
            m_need_sib = 1;
        }

        // Determine SIB if needed
        if (m_need_sib == 1) {
            m_sib = 0;      // start with 0

            // Special case: no basereg
            if (basereg == REG3264_NONE)
                m_sib |= 5;
            else {
                set_rex_from_reg(rex, drex, &low3, X86_REG64[basereg], bits,
                                 X86_REX_B);
                m_sib |= low3;
            }
            
            // Put in indexreg, checking for none case
            if (indexreg == REG3264_NONE)
                m_sib |= 040;
                // Any scale field is valid, just leave at 0.
            else {
                set_rex_from_reg(rex, drex, &low3, X86_REG64[indexreg], bits,
                                 X86_REX_X);
                m_sib |= low3 << 3;
                // Set scale field, 1 case -> 0, so don't bother.
                switch (reg3264mult[indexreg]) {
                    case 2:
                        m_sib |= 0100;
                        break;
                    case 4:
                        m_sib |= 0200;
                        break;
                    case 8:
                        m_sib |= 0300;
                        break;
                }
            }

            m_valid_sib = true;     // Done with SIB
        }

        // Calculate displacement length (if possible)
        calc_displen(32, basereg == REG3264_NONE,
                     basereg == REG3264_EBP || basereg == REG64_R13);
        return true;
    } else if (*addrsize == 16 && m_need_modrm && !m_valid_modrm) {
        static const unsigned char modrm16[16] = {
            0006 /* disp16  */, 0007 /* [BX]    */, 0004 /* [SI]    */,
            0000 /* [BX+SI] */, 0005 /* [DI]    */, 0001 /* [BX+DI] */,
            0377 /* invalid */, 0377 /* invalid */, 0006 /* [BP]+d  */,
            0377 /* invalid */, 0002 /* [BP+SI] */, 0377 /* invalid */,
            0003 /* [BP+DI] */, 0377 /* invalid */, 0377 /* invalid */,
            0377 /* invalid */
        };
        int bx=0, si=0, di=0, bp=0; // total multiplier for each reg
        enum HaveReg {
            HAVE_NONE = 0,
            HAVE_BX = 1<<0,
            HAVE_SI = 1<<1,
            HAVE_DI = 1<<2,
            HAVE_BP = 1<<3
        };
        int havereg = HAVE_NONE;

        // 64-bit mode does not allow 16-bit addresses
        if (bits == 64 && !address16_op) {
            throw TypeError(
                N_("16-bit addresses not supported in 64-bit mode"));
        }

        // 16-bit cannot have SIB
        m_sib = 0;
        m_valid_sib = false;
        m_need_sib = 0;

        if (m_disp.has_abs()) {
            bool pcrel = false;
            switch (x86_expr_checkea_getregusage
                    (m_disp.get_abs(), (int *)NULL, &pcrel, bits,
                     boost::bind(&x86_expr_checkea_get_reg16, _1, _2, bx, si,
                                 di, bp))) {
                case 1:
                    throw ValueError(N_("invalid effective address"));
                case 2:
                    if (pcrel)
                        m_disp.set_curpos_rel(bc, true);
                    return false;
                default:
                    if (pcrel)
                        m_disp.set_curpos_rel(bc, true);
                    break;
            }
        }

        // reg multipliers not 0 or 1 are illegal.
        if (bx & ~1 || si & ~1 || di & ~1 || bp & ~1)
            throw ValueError(N_("invalid effective address"));

        // Set havereg appropriately
        if (bx > 0)
            havereg |= HAVE_BX;
        if (si > 0)
            havereg |= HAVE_SI;
        if (di > 0)
            havereg |= HAVE_DI;
        if (bp > 0)
            havereg |= HAVE_BP;

        // Check the modrm value for invalid combinations.
        if (modrm16[havereg] & 0070)
            throw ValueError(N_("invalid effective address"));

        // Set ModRM byte for registers
        m_modrm |= modrm16[havereg];

        // Calculate displacement length (if possible)
        calc_displen(16, havereg == HAVE_NONE, havereg == HAVE_BP);
        return true;
    } else if (!m_need_modrm && !m_need_sib) {
        // Special case for MOV MemOffs opcode: displacement but no modrm.
        switch (*addrsize) {
            case 64:
                if (bits != 64) {
                    throw TypeError(
                        N_("invalid effective address (64-bit in non-64-bit mode)"));
                }
                m_disp.m_size = 64;
                break;
            case 32:
                m_disp.m_size = 32;
                break;
            case 16:
                // 64-bit mode does not allow 16-bit addresses
                if (bits == 64 && !address16_op) {
                    throw TypeError(
                        N_("16-bit addresses not supported in 64-bit mode"));
                }
                m_disp.m_size = 16;
                break;
        }
    }
    return true;
}

}}} // namespace yasm::arch::x86

//
// x86 general instruction
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
#include "x86general.h"

#include "util.h"

#include <iomanip>

#include <libyasmx/bc_container.h>
#include <libyasmx/bytecode.h>
#include <libyasmx/bytes.h>
#include <libyasmx/bytes_util.h>
#include <libyasmx/errwarn.h>
#include <libyasmx/expr.h>
#include <libyasmx/intnum.h>
#include <libyasmx/marg_ostream.h>
#include <libyasmx/object.h>
#include <libyasmx/symbol.h>

#include "x86arch.h"
#include "x86common.h"
#include "x86effaddr.h"
#include "x86opcode.h"
#include "x86register.h"


namespace yasm
{
namespace arch
{
namespace x86
{

class X86General : public Bytecode::Contents
{
public:

    X86General(const X86Common& common,
               const X86Opcode& opcode,
               std::auto_ptr<X86EffAddr> ea,
               std::auto_ptr<Value> imm,
               unsigned char special_prefix,
               unsigned char rex,
               GeneralPostOp postop,
               bool default_rel);
    ~X86General();

    void put(marg_ostream& os) const;
    void finalize(Bytecode& bc);
    unsigned long calc_len(Bytecode& bc, Bytecode::AddSpanFunc add_span);
    bool expand(Bytecode& bc, unsigned long& len, int span,
                long old_val, long new_val,
                /*@out@*/ long& neg_thres,
                /*@out@*/ long& pos_thres);
    void output(Bytecode& bc, BytecodeOutput& bc_out);

    X86General* clone() const;

private:
    X86General(const X86General& rhs);

    X86Common m_common;
    X86Opcode m_opcode;

    /*@null@*/ boost::scoped_ptr<X86EffAddr> m_ea;  // effective address

    /*@null@*/ boost::scoped_ptr<Value> m_imm;  // immediate or relative value

    unsigned char m_special_prefix;     // "special" prefix (0=none)

    // REX AMD64 extension, 0 if none,
    // 0xff if not allowed (high 8 bit reg used)
    unsigned char m_rex;

    unsigned char m_default_rel;

    GeneralPostOp m_postop;
};

X86General::X86General(const X86Common& common,
                       const X86Opcode& opcode,
                       std::auto_ptr<X86EffAddr> ea,
                       std::auto_ptr<Value> imm,
                       unsigned char special_prefix,
                       unsigned char rex,
                       GeneralPostOp postop,
                       bool default_rel)
    : m_common(common),
      m_opcode(opcode),
      m_ea(ea.release()),
      m_imm(imm.release()),
      m_special_prefix(special_prefix),
      m_rex(rex),
      m_default_rel(default_rel),
      m_postop(postop)
{
}

X86General::X86General(const X86General& rhs)
    : Bytecode::Contents(),
      m_common(rhs.m_common),
      m_opcode(rhs.m_opcode),
      m_ea(0),
      m_imm(0),
      m_special_prefix(rhs.m_special_prefix),
      m_rex(rhs.m_rex),
      m_postop(rhs.m_postop)
{
    if (rhs.m_ea != 0)
        m_ea.reset(rhs.m_ea->clone());
    if (rhs.m_imm != 0)
        m_imm.reset(new Value(*rhs.m_imm));
}

X86General::~X86General()
{
}

void
X86General::put(marg_ostream& os) const
{
    os << "_Instruction_\n";

    os << "Effective Address:";
    if (m_ea)
    {
        os << '\n';
        ++os;
        os << *m_ea;
        --os;
    }
    else
        os << " (nil)\n";

    os << "Immediate Value:";
    if (m_imm)
    {
        os << '\n';
        ++os;
        os << *m_imm;
        --os;
    }
    else
        os << " (nil)\n";

    os << m_opcode;
    os << m_common;

    std::ios_base::fmtflags origff = os.flags();
    os << "SpPre=" << std::hex << std::setfill('0') << std::setw(2)
       << static_cast<unsigned int>(m_special_prefix);
    os << " REX=" << std::oct << std::setfill('0') << std::setw(3)
       << static_cast<unsigned int>(m_rex);
    os << std::setfill(' ');
    os.flags(origff);
    os << " PostOp=" << static_cast<unsigned int>(m_postop) << '\n';
}

void
X86General::finalize(Bytecode& bc)
{
    Location loc = {&bc, bc.get_fixed_len()};
    if (m_ea)
        m_ea->finalize(loc);
    if (m_imm.get() != 0 && m_imm->finalize(loc))
        throw TooComplexError(N_("immediate expression too complex"));

    if (m_postop == POSTOP_ADDRESS16 && m_common.m_addrsize != 0)
    {
        warn_set(WARN_GENERAL, N_("address size override ignored"));
        m_common.m_addrsize = 0;
    }

    // Handle non-span-dependent post-ops here
    switch (m_postop)
    {
        case POSTOP_SHORT_MOV:
        {
            // Long (modrm+sib) mov instructions in amd64 can be optimized into
            // short mov instructions if a 32-bit address override is applied in
            // 64-bit mode to an EA of just an offset (no registers) and the
            // target register is al/ax/eax/rax.
            //
            // We don't want to do this if we're in default rel mode.
            Expr* abs;
            if (!m_default_rel && m_common.m_mode_bits == 64 &&
                m_common.m_addrsize == 32 &&
                (!(abs = m_ea->m_disp.get_abs()) ||
                 !abs->contains(ExprTerm::REG)))
            {
                m_ea->set_disponly();
                // Make the short form permanent.
                m_opcode.make_alt_1();
            }
            m_postop = POSTOP_NONE;
            break;
        }
        case POSTOP_SIMM32_AVAIL:
        {
            // Used for 64-bit mov immediate, which can take a sign-extended
            // imm32 as well as imm64 values.  The imm32 form is put in the
            // second byte of the opcode and its ModRM byte is put in the third
            // byte of the opcode.
            Expr* abs;
            IntNum* intn;
            if (!(abs = m_imm->get_abs()) ||
                ((intn = m_imm->get_abs()->get_intnum()) &&
                 intn->ok_size(32, 0, 1)))
            {
                // Throwaway REX byte
                unsigned char rex_temp = 0;

                // Build ModRM EA - CAUTION: this depends on
                // opcode 0 being a mov instruction!
                X86Arch* arch = static_cast<X86Arch*>(
                    bc.get_container()->get_object()->get_arch());
                m_ea.reset(new X86EffAddr(arch->get_reg64(m_opcode.get(0)-0xB8),
                                          &rex_temp, 0, 64));

                // Make the imm32s form permanent.
                m_opcode.make_alt_1();
                m_imm->m_size = 32;
            }
            m_postop = POSTOP_NONE;
            break;
        }
        default:
            break;
    }
}

// See if we can optimize a VEX prefix of three byte form into two byte form.
inline void
vex_optimize(X86Opcode& opcode,
             unsigned char& special_prefix,
             unsigned char rex)
{
    // Don't do anything if we don't have a 3-byte VEX prefix
    if (special_prefix != 0xC4)
        return;

    // See if we can shorten the VEX prefix to its two byte form.
    // In order to do this, REX.X, REX.B, and REX.W/VEX.W must all be 0,
    // and the VEX mmmmm field must be 1.
    if ((opcode.get(0) & 0x1F) == 1 &&
        (opcode.get(1) & 0x80) == 0 &&
        (rex == 0xff || (rex & 0x0B) == 0))
    {
        opcode.make_alt_2();
        special_prefix = 0xC5;      // mark as two-byte VEX
    }
}

unsigned long
X86General::calc_len(Bytecode& bc, Bytecode::AddSpanFunc add_span)
{
    unsigned long len = 0;

    if (m_ea != 0)
    {
        // Check validity of effective address and calc R/M bits of
        // Mod/RM byte and SIB byte.  We won't know the Mod field
        // of the Mod/RM byte until we know more about the
        // displacement.
        if (!m_ea->check(&m_common.m_addrsize, m_common.m_mode_bits,
                         m_postop == POSTOP_ADDRESS16, &m_rex,
                         bc.get_container()->get_object()->get_abs_sym()))
            // failed, don't bother checking rest of insn
            throw ValueError(N_("indeterminate effective address during length calculation"));

        if (m_ea->m_disp.m_size == 0 && m_ea->m_need_nonzero_len)
        {
            // Handle unknown case, default to byte-sized and set as
            // critical expression.
            m_ea->m_disp.m_size = 8;
            add_span(bc, 1, m_ea->m_disp, -128, 127);
        }
        len += m_ea->m_disp.m_size/8;

        // Handle address16 postop case
        if (m_postop == POSTOP_ADDRESS16)
            m_common.m_addrsize = 0;

        // Compute length of ea and add to total
        len += m_ea->m_need_modrm + (m_ea->m_need_sib ? 1:0);
        len += m_ea->m_need_drex ? 1:0;
        len += (m_ea->m_segreg != 0) ? 1 : 0;
    }

    if (m_imm != 0)
    {
        unsigned int immlen = m_imm->m_size;

        // TODO: check imm->len vs. sized len from expr?

        // Handle signext_imm8 postop special-casing
        if (m_postop == POSTOP_SIGNEXT_IMM8)
        {
            /*@null@*/ std::auto_ptr<IntNum> num = m_imm->get_intnum(false);

            if (!num.get())
            {
                // Unknown; default to byte form and set as critical
                // expression.
                immlen = 8;
                add_span(bc, 2, *m_imm, -128, 127);
            }
            else
            {
                if (num->in_range(-128, 127))
                {
                    // We can use the sign-extended byte form: shorten
                    // the immediate length to 1 and make the byte form
                    // permanent.
                    m_imm->m_size = 8;
                    m_imm->m_sign = 1;
                    immlen = 8;
                }
                else
                {
                    // We can't.  Copy over the word-sized opcode.
                    m_opcode.make_alt_1();
                }
                m_postop = POSTOP_NONE;
            }
        }

        len += immlen/8;
    }

    // VEX prefixes never have REX.  We can come into this function with the
    // three byte form, so we need to see if we can optimize to the two byte
    // form.  We can't do it earlier, as we don't know all of the REX byte
    // until now.
    vex_optimize(m_opcode, m_special_prefix, m_rex);
    if (m_rex != 0xff && m_rex != 0 &&
        m_special_prefix != 0xC5 && m_special_prefix != 0xC4)
        len++;

    len += m_opcode.get_len();
    len += m_common.get_len();
    len += (m_special_prefix != 0) ? 1:0;
    return len;
}

bool
X86General::expand(Bytecode& bc, unsigned long& len, int span,
                   long old_val, long new_val,
                   /*@out@*/ long& neg_thres, /*@out@*/ long& pos_thres)
{
    if (m_ea != 0 && span == 1)
    {
        // Change displacement length into word-sized
        if (m_ea->m_disp.m_size == 8)
        {
            m_ea->m_disp.m_size = (m_common.m_addrsize == 16) ? 16 : 32;
            m_ea->m_modrm &= ~0300;
            m_ea->m_modrm |= 0200;
            len--;
            len += m_ea->m_disp.m_size/8;
        }
    }

    if (m_imm != 0 && span == 2)
    {
        if (m_postop == POSTOP_SIGNEXT_IMM8)
        {
            // Update len for new opcode and immediate size
            len -= m_opcode.get_len();
            len += m_imm->m_size/8;

            // Change to the word-sized opcode
            m_opcode.make_alt_1();
            m_postop = POSTOP_NONE;
        }
    }

    return false;
}

void
general_tobytes(Bytes& bytes,
                const X86Common& common,
                X86Opcode opcode,
                const X86EffAddr* ea,
                unsigned char special_prefix,
                unsigned char rex)
{
    vex_optimize(opcode, special_prefix, rex);

    // Prefixes
    common.to_bytes(bytes,
        ea != 0 ? static_cast<const X86SegmentRegister*>(ea->m_segreg) : 0);
    if (special_prefix != 0)
        write_8(bytes, special_prefix);
    if (special_prefix == 0xC4)
    {
        // 3-byte VEX; merge in 1s complement of REX.R, REX.X, REX.B
        opcode.mask(0, 0x1F);
        if (rex != 0xff)
            opcode.merge(0, ((~rex) & 0x07) << 5);
        // merge REX.W via ORing; there should never be a case in which REX.W
        // is important when VEX.W is already set by the instruction.
        if (rex != 0xff && (rex & 0x8) != 0)
            opcode.merge(1, 0x80);
    }
    else if (special_prefix == 0xC5)
    {
        // 2-byte VEX; merge in 1s complement of REX.R
        opcode.mask(0, 0x7F);
        if (rex != 0xff && (rex & 0x4) == 0)
            opcode.merge(0, 0x80);
        // No other REX bits should be set
        if (rex != 0xff && (rex & 0xB) != 0)
            throw InternalError(N_("x86: REX.WXB set, but 2-byte VEX"));
    }
    else if (rex != 0xff && rex != 0)
    {
        if (common.m_mode_bits != 64)
            throw InternalError(N_("x86: got a REX prefix in non-64-bit mode"));
        write_8(bytes, rex);
    }

    // Opcode
    opcode.to_bytes(bytes);
}

void
X86General::output(Bytecode& bc, BytecodeOutput& bc_out)
{
    Bytes& bytes = bc_out.get_scratch();

    general_tobytes(bytes, m_common, m_opcode, m_ea.get(), m_special_prefix,
                    m_rex);

    // Effective address: ModR/M (if required), SIB (if required)
    if (m_ea != 0)
    {
        if (m_ea->m_need_modrm)
        {
            if (!m_ea->m_valid_modrm)
                throw InternalError(N_("invalid Mod/RM in x86 tobytes_insn"));
            write_8(bytes, m_ea->m_modrm);
        }

        if (m_ea->m_need_sib)
        {
            if (!m_ea->m_valid_sib)
                throw InternalError(N_("invalid SIB in x86 tobytes_insn"));
            write_8(bytes, m_ea->m_sib);
        }

        if (m_ea->m_need_drex)
            write_8(bytes, m_ea->m_drex);
    }

    bc_out.output(bytes);
    unsigned long pos = bc.get_fixed_len()+bytes.size();

    // Displacement (if required)
    if (m_ea != 0 && m_ea->m_need_disp)
    {
        unsigned int disp_len = m_ea->m_disp.m_size/8;

        if (m_ea->m_disp.m_ip_rel)
        {
            // Adjust relative displacement to end of bytecode
            m_ea->m_disp.add_abs(-static_cast<long>(disp_len));
        }
        Location loc = {&bc, pos};
        pos += disp_len;
        Bytes& dbytes = bc_out.get_scratch();
        dbytes.resize(disp_len);
        bc_out.output(m_ea->m_disp, bytes, loc, 1);
    }

    // Immediate (if required)
    if (m_imm != 0)
    {
        unsigned int imm_len;
        if (m_postop == POSTOP_SIGNEXT_IMM8)
        {
            // If we got here with this postop still set, we need to force
            // imm size to 8 here.
            m_imm->m_size = 8;
            m_imm->m_sign = 1;
            imm_len = 1;
        }
        else
            imm_len = m_imm->m_size/8;
        Location loc = {&bc, pos};
        Bytes& ibytes = bc_out.get_scratch();
        ibytes.resize(imm_len);
        bc_out.output(*m_imm, bytes, loc, 1);
    }
}

X86General*
X86General::clone() const
{
    return new X86General(*this);
}

void
append_general(BytecodeContainer& container,
               const X86Common& common,
               const X86Opcode& opcode,
               std::auto_ptr<X86EffAddr> ea,
               std::auto_ptr<Value> imm,
               unsigned char special_prefix,
               unsigned char rex,
               GeneralPostOp postop,
               bool default_rel)
{
    Bytecode& bc = container.fresh_bytecode();

    // if no postop and no effective address, output the fixed contents
    if (postop == POSTOP_NONE && ea.get() == 0)
    {
        Bytes& bytes = bc.get_fixed();
        general_tobytes(bytes, common, opcode, ea.get(), special_prefix, rex);
        if (imm.get() != 0)
            bc.append_fixed(imm);
        return;
    }

    // TODO: optimize EA case
    bc.transform(Bytecode::Contents::Ptr(new X86General(
        common, opcode, ea, imm, special_prefix, rex, postop, default_rel)));
}

}}} // namespace yasm::arch::x86

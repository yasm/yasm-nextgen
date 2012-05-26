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
#define DEBUG_TYPE "x86"

#include "X86General.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/Twine.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/BytecodeContainer.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/Object.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"

#include "X86Arch.h"
#include "X86EffAddr.h"
#include "X86Register.h"


STATISTIC(num_generic, "Number of generic instructions appended");
STATISTIC(num_generic_bc, "Number of generic bytecodes created");

using namespace yasm;
using namespace yasm::arch;

X86General::X86General(const X86Common& common,
                       const X86Opcode& opcode,
                       std::auto_ptr<X86EffAddr> ea,
                       std::auto_ptr<Value> imm,
                       unsigned char special_prefix,
                       unsigned char rex,
                       X86GeneralPostOp postop,
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

bool
X86General::Finalize(Bytecode& bc, Diagnostic& diags)
{
    if (m_ea)
    {
        if (!m_ea->Finalize(diags))
            return false;
    }

    if (m_imm.get() != 0)
    {
        if (!m_imm->Finalize(diags, diag::err_imm_too_complex))
            return false;
    }

    if (m_postop == X86_POSTOP_ADDRESS16 && m_common.m_addrsize != 0)
    {
        diags.Report(bc.getSource(), diag::warn_address_size_ignored);
        m_common.m_addrsize = 0;
    }

    // Handle non-span-dependent post-ops here
    switch (m_postop)
    {
        case X86_POSTOP_SHORT_MOV:
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
                (!(abs = m_ea->m_disp.getAbs()) ||
                 !abs->Contains(ExprTerm::REG)))
            {
                m_ea->setDispOnly();
                // Make the short form permanent.
                m_opcode.MakeAlt1();
            }
            m_postop = X86_POSTOP_NONE;
            break;
        }
        case X86_POSTOP_SIMM32_AVAIL:
        {
            // Used for 64-bit mov immediate, which can take a sign-extended
            // imm32 as well as imm64 values.  The imm32 form is put in the
            // second byte of the opcode and its ModRM byte is put in the third
            // byte of the opcode.
            Expr* abs;
            if (!(abs = m_imm->getAbs()) ||
                (m_imm->getAbs()->isIntNum() &&
                 m_imm->getAbs()->getIntNum().isOkSize(32, 0, 1)))
            {
                // Throwaway REX byte
                unsigned char rex_temp = 0;

                // Build ModRM EA - CAUTION: this depends on
                // opcode 0 being a mov instruction!
                m_ea.reset(new X86EffAddr());
                if (!m_ea->setReg(X86RegTmod::Instance().
                                  getReg(X86Register::REG64,
                                         m_opcode.get(0)-0xB8),
                                  &rex_temp, 64))
                {
                    diags.Report(m_ea->m_disp.getSource().getBegin(),
                                 diag::err_high8_rex_conflict);
                    return false;
                }

                // Make the imm32s form permanent.
                m_opcode.MakeAlt1();
                m_imm->setSize(32);
                m_imm->setSigned();
            }
            m_postop = X86_POSTOP_NONE;
            break;
        }
        default:
            break;
    }
    return true;
}

// See if we can optimize a VEX prefix of three byte form into two byte form.
inline void
VexOptimize(X86Opcode& opcode,
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
        opcode.MakeAlt2();
        special_prefix = 0xC5;      // mark as two-byte VEX
    }
}

bool
X86General::CalcLen(Bytecode& bc,
                    /*@out@*/ unsigned long* len,
                    const Bytecode::AddSpanFunc& add_span,
                    Diagnostic& diags)
{
    unsigned long ilen = 0;
    if (m_ea != 0)
    {
        // Check validity of effective address and calc R/M bits of
        // Mod/RM byte and SIB byte.  We won't know the Mod field
        // of the Mod/RM byte until we know more about the
        // displacement.
        bool ip_rel = false;
        if (!m_ea->Check(&m_common.m_addrsize, m_common.m_mode_bits,
                         m_postop == X86_POSTOP_ADDRESS16, &m_rex,
                         &ip_rel, diags))
        {
            // failed, don't bother checking rest of insn
            diags.Report(m_ea->m_disp.getSource().getBegin(),
                         diag::err_ea_length_unknown);
            return false;
        }

        // IP-relative needs to be adjusted to the end of the instruction.
        // However, we may not know the instruction length yet (due to imm
        // size optimization).
        // Since RIP-relative effective addresses are always 32-bit in size,
        // we can instead add in the instruction length in tobytes(), and
        // simply adjust to the *start* of the instruction here.
        // We couldn't do this if effective addresses were variable length.
        if (ip_rel)
        {
            Location sub_loc = {&bc, bc.getFixedLen()};
            if (!m_ea->m_disp.SubRelative(
                    bc.getContainer()->getSection()->getObject(), sub_loc))
                diags.Report(m_ea->m_disp.getSource().getBegin(),
                             diag::err_too_complex_expression);
            m_ea->m_disp.setIPRelative();
        }

        if (m_ea->m_disp.getSize() == 0 && m_ea->m_need_nonzero_len)
        {
            // Handle unknown case, default to byte-sized and set as
            // critical expression.
            m_ea->m_disp.setSize(8);
            add_span(bc, 1, m_ea->m_disp, -128, 127);
        }
        ilen += m_ea->m_disp.getSize()/8;

        // Handle address16 postop case
        if (m_postop == X86_POSTOP_ADDRESS16)
            m_common.m_addrsize = 0;

        // Compute length of ea and add to total
        ilen += m_ea->m_need_modrm + (m_ea->m_need_sib ? 1:0);
        ilen += (m_ea->m_segreg != 0) ? 1 : 0;
    }

    if (m_imm != 0)
    {
        unsigned int immlen = m_imm->getSize();

        // TODO: check imm->len vs. sized len from expr?

        // Handle signext_imm8 postop special-casing
        if (m_postop == X86_POSTOP_SIGNEXT_IMM8)
        {
            IntNum num;
            if (!m_imm->getIntNum(&num, false, diags))
            {
                // Unknown; default to byte form and set as critical
                // expression.
                immlen = 8;
                add_span(bc, 2, *m_imm, -128, 127);
            }
            else
            {
                // Sign extend based on immediate size.  This is so that e.g.
                // a 32-bit value 0xfffffff7 is seen as a large signed number.
                // We can't do mark it as signed in the instruction table
                // because it will result in a warning and result in signed
                // relocations.
                bool ok = num.isOkSize(immlen, 0, 2);
                num.SignExtend(immlen);

                if (num.isInRange(-128, 127))
                {
                    // We can use the sign-extended byte form: shorten
                    // the immediate length to 1 and make the byte form
                    // permanent.

                    // Warn if we truncated.
                    if (!ok)
                    {
                        diags.Report(m_imm->getSource().getBegin(),
                                     m_imm->isSigned() ?
                                     diag::warn_signed_overflow :
                                     diag::warn_unsigned_overflow)
                            << immlen;
                    }

                    m_imm->setSize(8);
                    m_imm->setSigned();
                    immlen = 8;
                    // Set the value to the sign-extended one.
                    if (Expr* abs = m_imm->getAbs())
                        *abs = num;
                }
                else
                {
                    // We can't.  Copy over the word-sized opcode.
                    m_opcode.MakeAlt1();
                }
                m_postop = X86_POSTOP_NONE;
            }
        }

        ilen += immlen/8;
    }

    // VEX and XOP prefixes never have REX (it's embedded in the opcode).
    // For VEX, we can come into this function with the three byte form,
    // so we need to see if we can optimize to the two byte form.
    // We can't do it earlier, as we don't know all of the REX byte until now.
    VexOptimize(m_opcode, m_special_prefix, m_rex);
    if (m_rex != 0xff && m_rex != 0 &&
        m_special_prefix != 0xC5 && m_special_prefix != 0xC4 &&
        m_special_prefix != 0x8F)
        ilen++;

    ilen += m_opcode.getLen();
    ilen += m_common.getLen();
    ilen += (m_special_prefix != 0) ? 1:0;

    *len = ilen;
    return true;
}

bool
X86General::Expand(Bytecode& bc,
                   unsigned long* len,
                   int span,
                   long old_val,
                   long new_val,
                   bool* keep,
                   /*@out@*/ long* neg_thres,
                   /*@out@*/ long* pos_thres,
                   Diagnostic& diags)
{
    if (m_ea != 0 && span == 1)
    {
        // Change displacement length into word-sized
        if (m_ea->m_disp.getSize() == 8)
        {
            unsigned int size = (m_common.m_addrsize == 16) ? 16 : 32;
            m_ea->m_disp.setSize(size);
            m_ea->m_modrm &= ~0300;
            m_ea->m_modrm |= 0200;
            (*len)--;
            (*len) += size/8;
        }
    }

    if (m_imm != 0 && span == 2)
    {
        if (m_postop == X86_POSTOP_SIGNEXT_IMM8)
        {
            // Update len for new opcode and immediate size
            (*len) -= m_opcode.getLen();
            (*len) += m_imm->getSize()/8;

            // Change to the word-sized opcode
            m_opcode.MakeAlt1();
            m_postop = X86_POSTOP_NONE;
        }
    }

    *keep = false;
    return true;
}

static void
GeneralToBytes(Bytes& bytes,
               const X86Common& common,
               X86Opcode opcode,
               const X86EffAddr* ea,
               unsigned char special_prefix,
               unsigned char rex)
{
    VexOptimize(opcode, special_prefix, rex);

    // Prefixes
    common.ToBytes(bytes,
        ea != 0 ? static_cast<const X86SegmentRegister*>(ea->m_segreg) : 0);
    if (special_prefix != 0)
        Write8(bytes, special_prefix);
    if (special_prefix == 0xC4 || special_prefix == 0x8F)
    {
        // 3-byte VEX/XOP; merge in 1s complement of REX.R, REX.X, REX.B
        opcode.Mask(0, 0x1F);
        if (rex != 0xff)
            opcode.Merge(0, ((~rex) & 0x07) << 5);
        // merge REX.W via ORing; there should never be a case in which REX.W
        // is important when VEX.W is already set by the instruction.
        if (rex != 0xff && (rex & 0x8) != 0)
            opcode.Merge(1, 0x80);
    }
    else if (special_prefix == 0xC5)
    {
        // 2-byte VEX; merge in 1s complement of REX.R
        opcode.Mask(0, 0x7F);
        if (rex != 0xff && (rex & 0x4) == 0)
            opcode.Merge(0, 0x80);
        // No other REX bits should be set
        assert((rex == 0xff || (rex & 0xB) == 0)
               && "x86: REX.WXB set, but 2-byte VEX");
    }
    else if (rex != 0xff && rex != 0)
    {
        assert(common.m_mode_bits == 64 &&
               "x86: got a REX prefix in non-64-bit mode");
        Write8(bytes, rex);
    }

    // Opcode
    opcode.ToBytes(bytes);
}

bool
X86General::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    Bytes& bytes = bc_out.getScratch();
    bytes.setLittleEndian();

    GeneralToBytes(bytes, m_common, m_opcode, m_ea.get(), m_special_prefix,
                   m_rex);

    // Effective address: ModR/M (if required), SIB (if required)
    if (m_ea != 0)
    {
        if (m_ea->m_need_modrm)
        {
            assert(m_ea->m_valid_modrm && "invalid Mod/RM in x86 tobytes_insn");
            Write8(bytes, m_ea->m_modrm);
        }

        if (m_ea->m_need_sib)
        {
            assert(m_ea->m_valid_sib && "invalid SIB in x86 tobytes_insn");
            Write8(bytes, m_ea->m_sib);
        }
    }

    unsigned long pos = bytes.size();
    bc_out.OutputBytes(bytes, bc.getSource());

    // Calculate immediate length
    unsigned int imm_len = 0;
    if (m_imm != 0)
    {
        if (m_postop == X86_POSTOP_SIGNEXT_IMM8)
        {
            // If we got here with this postop still set, we need to force
            // imm size to 8 here.
            m_imm->setSize(8);
            m_imm->setSigned();
            imm_len = 1;
        }
        else
            imm_len = m_imm->getSize()/8;
    }

    // Displacement (if required)
    if (m_ea != 0 && m_ea->m_need_disp)
    {
        unsigned int disp_len = m_ea->m_disp.getSize()/8;

        m_ea->m_disp.setInsnStart(pos);
        if (m_ea->m_disp.isIPRelative())
        {
            // Adjust relative displacement to end of bytecode
            m_ea->m_disp.AddAbs(-static_cast<long>(pos+disp_len+imm_len));
            // Distance to end of instruction is the immediate length
            m_ea->m_disp.setNextInsn(imm_len);
        }
        Location loc = {&bc, bc.getFixedLen()+pos};
        pos += disp_len;
        bytes.resize(0);
        bytes.resize(disp_len);
        NumericOutput num_out(bytes);
        m_ea->m_disp.ConfigureOutput(&num_out);
        if (!bc_out.OutputValue(m_ea->m_disp, loc, num_out))
            return false;
    }

    // Immediate (if required)
    if (m_imm != 0)
    {
        m_imm->setInsnStart(pos);
        Location loc = {&bc, bc.getFixedLen()+pos};
        bytes.resize(0);
        bytes.resize(imm_len);
        NumericOutput num_out(bytes);
        m_imm->ConfigureOutput(&num_out);
        if (!bc_out.OutputValue(*m_imm, loc, num_out))
            return false;
    }
    return true;
}

llvm::StringRef
X86General::getType() const
{
    return "yasm::arch::X86General";
}

X86General*
X86General::clone() const
{
    return new X86General(*this);
}

#ifdef WITH_XML
pugi::xml_node
X86General::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("X86General");
    append_data(root, m_common);
    append_data(root, m_opcode);

    // effective address
    if (m_ea)
        append_data(root, *m_ea);

    if (m_imm)
        append_child(root, "Imm", *m_imm);

    append_child(root, "SpecialPrefix",
                 llvm::Twine::utohexstr(m_special_prefix).str());
    append_child(root, "REX", llvm::Twine::utohexstr(m_rex).str());
    if (m_default_rel)
        root.append_attribute("default_rel") = true;
    const char* postop = NULL;
    switch (m_postop)
    {
        case X86_POSTOP_SIGNEXT_IMM8:   postop = "SIGNEXT_IMM8"; break;
        case X86_POSTOP_SHORT_MOV:      postop = "SHORT_MOV"; break;
        case X86_POSTOP_ADDRESS16:      postop = "ADDRESS16"; break;
        case X86_POSTOP_SIMM32_AVAIL:   postop = "SIMM32_AVAIL"; break;
        case X86_POSTOP_NONE:           break;
    }
    if (postop)
        append_child(root, "PostOp", postop);
    return root;
}
#endif // WITH_XML

void
arch::AppendGeneral(BytecodeContainer& container,
                    const X86Common& common,
                    const X86Opcode& opcode,
                    std::auto_ptr<X86EffAddr> ea,
                    std::auto_ptr<Value> imm,
                    unsigned char special_prefix,
                    unsigned char rex,
                    X86GeneralPostOp postop,
                    bool default_rel,
                    SourceLocation source)
{
    Bytecode& bc = container.FreshBytecode();
    ++num_generic;

    // if no postop and no effective address, output the fixed contents
    if (postop == X86_POSTOP_NONE && ea.get() == 0)
    {
        Bytes& bytes = bc.getFixed();
        unsigned long orig_size = bytes.size();
        GeneralToBytes(bytes, common, opcode, ea.get(), special_prefix, rex);
        if (imm.get() != 0)
        {
            imm->setInsnStart(bytes.size()-orig_size);
            bc.AppendFixed(imm);
        }
        return;
    }

    // TODO: optimize EA case
    bc.Transform(Bytecode::Contents::Ptr(new X86General(
        common, opcode, ea, imm, special_prefix, rex, postop, default_rel)));
    bc.setSource(source);
    ++num_generic_bc;
}

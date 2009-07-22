//
// x86 jump bytecode
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

#include "X86Jmp.h"

#include "util.h"

#include <llvm/ADT/Statistic.h>
#include <YAML/emitter.h>
#include <yasmx/Support/errwarn.h>
#include <yasmx/BytecodeContainer.h>
#include <yasmx/BytecodeOutput.h>
#include <yasmx/Bytecode.h>
#include <yasmx/Bytes.h>
#include <yasmx/Expr.h>
#include <yasmx/IntNum.h>
#include <yasmx/Symbol.h>

#include "X86Common.h"
#include "X86Opcode.h"


STATISTIC(num_jmp, "Number of jump instructions appended");
STATISTIC(num_jmp_bc, "Number of jump bytecodes created");

namespace yasm
{
namespace arch
{
namespace x86
{

class X86Jmp : public Bytecode::Contents
{
public:
    X86Jmp(const X86Common& common,
           JmpOpcodeSel op_sel,
           const X86Opcode& shortop,
           const X86Opcode& nearop,
           std::auto_ptr<Expr> target);
    ~X86Jmp();

    void Finalize(Bytecode& bc);
    unsigned long CalcLen(Bytecode& bc, const Bytecode::AddSpanFunc& add_span);
    bool Expand(Bytecode& bc,
                unsigned long& len,
                int span,
                long old_val,
                long new_val,
                /*@out@*/ long* neg_thres,
                /*@out@*/ long* pos_thres);
    void Output(Bytecode& bc, BytecodeOutput& bc_out);

    X86Jmp* clone() const;

    void Write(YAML::Emitter& out) const;

private:
    X86Common m_common;
    X86Opcode m_shortop, m_nearop;

    Value m_target;             // jump target

    // which opcode are we using?
    // The *FORCED forms are specified in the source as such
    JmpOpcodeSel m_op_sel;
};

X86Jmp::X86Jmp(const X86Common& common,
               JmpOpcodeSel op_sel,
               const X86Opcode& shortop,
               const X86Opcode& nearop,
               std::auto_ptr<Expr> target)
    : Bytecode::Contents(),
      m_common(common),
      m_shortop(shortop),
      m_nearop(nearop),
      m_target(0, target),
      m_op_sel(op_sel)
{
    m_target.setJumpTarget();
    m_target.setSigned();
}

X86Jmp::~X86Jmp()
{
}

void
X86Jmp::Finalize(Bytecode& bc)
{
    if (!m_target.Finalize())
        throw TooComplexError(N_("jump target expression too complex"));
    if (m_target.isComplexRelative())
        throw ValueError(N_("invalid jump target"));

    // Need to adjust target to the end of the instruction.
    // However, we don't know the instruction length yet (short/near).
    // So just adjust to the start of the instruction, and handle the
    // difference in calc_len() and tobytes().
    Location sub_loc = {&bc, bc.getFixedLen()};
    m_target.SubRelative(bc.getContainer()->getObject(), sub_loc);
    m_target.setIPRelative();

    Location target_loc;
    if (m_target.isRelative()
        && (!m_target.getRelative()->getLabel(&target_loc)
            || target_loc.bc->getContainer() != bc.getContainer()))
    {
        // External or out of segment, so we can't check distance.
        // Default to near (if explicitly overridden, we never get to
        // this function anyway).
        m_op_sel = JMP_NEAR;
    }
    else
    {
        // Default to short jump
        m_op_sel = JMP_SHORT;
    }
}

unsigned long
X86Jmp::CalcLen(Bytecode& bc, const Bytecode::AddSpanFunc& add_span)
{
    unsigned long len = 0;

    len += m_common.getLen();

    if (m_op_sel == JMP_NEAR)
    {
        len += m_nearop.getLen();
        len += (m_common.m_opersize == 16) ? 2 : 4;
    }
    else
    {
        // Short or maybe long; generate span
        len += m_shortop.getLen() + 1;
        add_span(bc, 1, m_target, -128+len, 127+len);
    }
    return len;
}

bool
X86Jmp::Expand(Bytecode& bc,
               unsigned long& len,
               int span,
               long old_val,
               long new_val,
               /*@out@*/ long* neg_thres,
               /*@out@*/ long* pos_thres)
{
    assert(span == 1 && "unrecognized span id");
    assert(m_op_sel != JMP_NEAR && "trying to expand an already-near jump");

    // Upgrade to a near jump
    m_op_sel = JMP_NEAR;
    len -= m_shortop.getLen() + 1;
    len += m_nearop.getLen();
    len += (m_common.m_opersize == 16) ? 2 : 4;

    return false;
}

void
X86Jmp::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    Bytes& bytes = bc_out.getScratch();

    // Prefixes
    m_common.ToBytes(bytes, 0);

    unsigned int size;
    if (m_op_sel == JMP_SHORT)
    {
        // 1 byte relative displacement
        size = 1;

        // Opcode
        m_shortop.ToBytes(bytes);
    }
    else
    {
        // 2/4 byte relative displacement (depending on operand size)
        size = (m_common.m_opersize == 16) ? 2 : 4;

        // Opcode
        m_nearop.ToBytes(bytes);
    }

    unsigned long pos = bytes.size();
    bc_out.Output(bytes);

    // Adjust relative displacement to end of instruction
    m_target.AddAbs(-static_cast<long>(pos+size));
    m_target.setSize(size*8);

    // Distance from displacement to end of instruction is always 0.
    m_target.setInsnStart(pos);
    m_target.setNextInsn(0);

    // Output displacement
    Location loc = {&bc, bc.getFixedLen()+bytes.size()};
    Bytes& tbytes = bc_out.getScratch();
    tbytes.resize(size);
    bc_out.Output(m_target, tbytes, loc, 1);
}

X86Jmp*
X86Jmp::clone() const
{
    return new X86Jmp(*this);
}

void
X86Jmp::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << "X86Jmp";
    out << YAML::Key << "common" << YAML::Value << m_common;
    out << YAML::Key << "short opcode" << YAML::Value << m_shortop;
    out << YAML::Key << "near opcode" << YAML::Value << m_nearop;
    out << YAML::Key << "target" << YAML::Value << m_target;

    out << YAML::Key << "opcode selection" << YAML::Value;
    switch (m_op_sel)
    {
        case JMP_NONE:  out << "None"; break;
        case JMP_SHORT: out << "Short"; break;
        case JMP_NEAR:  out << "Near"; break;
        default:        out << YAML::Null; break;
    }
    out << YAML::EndMap;
}

void
AppendJmp(BytecodeContainer& container,
          const X86Common& common,
          const X86Opcode& shortop,
          const X86Opcode& nearop,
          std::auto_ptr<Expr> target,
          unsigned long line,
          JmpOpcodeSel op_sel)
{
    Bytecode& bc = container.FreshBytecode();
    ++num_jmp;

    if (shortop.getLen() == 0)
        op_sel = JMP_NEAR;
    if (nearop.getLen() == 0)
        op_sel = JMP_SHORT;

    // jump size not forced near or far, so variable size (need contents)
    // TODO: we can be a bit more optimal for backward jumps within the
    // same bytecode (as the distance is known)
    if (op_sel == JMP_NONE)
    {
        bc.Transform(Bytecode::Contents::Ptr(new X86Jmp(
            common, op_sel, shortop, nearop, target)));
        bc.setLine(line);
        ++num_jmp_bc;
        return;
    }

    // if jump size selected, generate bytes directly.
    // FIXME: if short jump out of range, this results in an overflow warning
    // instead of a "short jump out of range" error.
    Bytes& bytes = bc.getFixed();
    unsigned long orig_size = bytes.size();
    common.ToBytes(bytes, 0);

    Value targetv(0, target);
    targetv.setLine(line);
    targetv.setJumpTarget();
    targetv.setIPRelative();
    targetv.setSigned();
    targetv.setNextInsn(0);     // always 0.

    if (op_sel == JMP_SHORT)
    {
        // Opcode
        shortop.ToBytes(bytes);

        // Adjust relative displacement to end of bytecode
        targetv.AddAbs(-1);
        targetv.setSize(8);
    }
    else
    {
        // Opcode
        nearop.ToBytes(bytes);

        unsigned int i = (common.m_opersize == 16) ? 2 : 4;

        // Adjust relative displacement to end of bytecode
        targetv.AddAbs(-static_cast<long>(i));
        targetv.setSize(i*8);
    }
    targetv.setInsnStart(bytes.size()-orig_size);
    bc.AppendFixed(targetv);
}

}}} // namespace yasm::arch::x86

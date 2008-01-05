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
#include "x86jmp.h"

#include "util.h"

#include <iomanip>
#include <ostream>

#include <libyasm/bytes.h>
#include <libyasm/errwarn.h>
#include <libyasm/expr.h>
#include <libyasm/intnum.h>
#include <libyasm/symbol.h>


namespace yasm { namespace arch { namespace x86 {

X86Jmp::X86Jmp(OpcodeSel op_sel, const X86Opcode& shortop,
               const X86Opcode& nearop, std::auto_ptr<Expr> target,
               Bytecode* bc)
    : m_shortop(shortop),
      m_nearop(nearop),
      m_target(0, target),
      m_op_sel(op_sel)
{
    Location loc = {bc, 0};
    if (m_target.finalize(loc))
        throw TooComplexError(N_("jump target expression too complex"));
    if (m_target.m_seg_of || m_target.m_rshift || m_target.m_curpos_rel)
        throw ValueError(N_("invalid jump target"));
    m_target.set_curpos_rel(*bc, false);
    m_target.m_jump_target = true;
}

X86Jmp::~X86Jmp()
{
}

void
X86Jmp::put(std::ostream& os, int indent_level) const
{
    os << std::setw(indent_level) << "" << "_Jump_\n";

    os << std::setw(indent_level) << "" << "Target:\n";
    m_target.put(os, indent_level+1);
    /* FIXME
    fprintf(f, "%*sOrigin=\n", indent_level, "");
    yasm_symrec_print(jmp->origin, f, indent_level+1);
    */

    os << '\n' << std::setw(indent_level) << "" << "Short Form:\n";
    if (m_shortop.get_len() == 0)
        os << std::setw(indent_level+1) << "" << "None\n";
    else
        m_shortop.put(os, indent_level+1);

    os << std::setw(indent_level) << "" << "Near Form:\n";
    if (m_nearop.get_len() == 0)
        os << std::setw(indent_level+1) << "" << "None\n";
    else
        m_nearop.put(os, indent_level+1);

    os << std::setw(indent_level) << "" << "OpSel=";
    switch (m_op_sel) {
        case NONE:
            os << "None";
            break;
        case SHORT:
            os << "Short";
            break;
        case NEAR:
            os << "Near";
            break;
        case SHORT_FORCED:
            os << "Forced Short";
            break;
        case NEAR_FORCED:
            os << "Forced Near";
            break;
        default:
            os << "UNKNOWN!!";
            break;
    }

    X86Common::put(os, indent_level);
}

void
X86Jmp::finalize(Bytecode& bc)
{
}

unsigned long
X86Jmp::calc_len(Bytecode& bc, Bytecode::AddSpanFunc add_span)
{
    unsigned long len = 0;

    // As opersize may be 0, figure out its "real" value.
    unsigned char opersize = (m_opersize == 0) ?  m_mode_bits : m_opersize;

    len += X86Common::calc_len();

    if (m_op_sel == NEAR_FORCED || m_shortop.get_len() == 0) {
        if (m_nearop.get_len() == 0)
            throw TypeError(N_("near jump does not exist"));

        // Near jump, no spans needed
        if (m_shortop.get_len() == 0)
            m_op_sel = NEAR;
        len += m_nearop.get_len();
        len += (opersize == 16) ? 2 : 4;
        return len;
    }

    if (m_op_sel == SHORT_FORCED || m_nearop.get_len() == 0) {
        if (m_shortop.get_len() == 0)
            throw TypeError(N_("short jump does not exist"));

        // We want to be sure to error if we exceed short length, so
        // put it in as a dependent expression (falling through).
    }

    Location target_loc;
    if (m_target.m_rel
        && (!m_target.m_rel->get_label(&target_loc)
            || target_loc.bc->get_section() != bc.get_section())) {
        // External or out of segment, so we can't check distance.
        // Allowing short jumps depends on the objfmt supporting
        // 8-bit relocs.  While most don't, some might, so allow it here.
        // Otherwise default to word-sized.
        // The objfmt will error if not supported.
        if (m_op_sel == SHORT_FORCED || m_nearop.get_len() == 0) {
            if (m_op_sel == NONE)
                m_op_sel = SHORT;
            len += m_shortop.get_len() + 1;
        } else {
            m_op_sel = NEAR;
            len += m_nearop.get_len();
            len += (opersize == 16) ? 2 : 4;
        }
        return len;
    }

    // Default to short jump and generate span
    if (m_op_sel == NONE)
        m_op_sel = SHORT;
    len += m_shortop.get_len() + 1;
    add_span(bc, 1, m_target, -128+len, 127+len);
    return len;
}

bool
X86Jmp::expand(Bytecode& bc, unsigned long& len, int span,
               long old_val, long new_val,
               /*@out@*/ long& neg_thres, /*@out@*/ long& pos_thres)
{
    if (span != 1)
        throw InternalError(N_("unrecognized span id"));

    // As opersize may be 0, figure out its "real" value.
    unsigned char opersize = (m_opersize == 0) ? m_mode_bits : m_opersize;

    if (m_op_sel == SHORT_FORCED || m_nearop.get_len() == 0)
        throw ValueError(N_("short jump out of range"));

    if (m_op_sel == NEAR)
        throw InternalError(N_("trying to expand an already-near jump"));

    // Upgrade to a near jump
    m_op_sel = NEAR;
    len -= m_shortop.get_len() + 1;
    len += m_nearop.get_len();
    len += (opersize == 16) ? 2 : 4;

    return false;
}

void
X86Jmp::to_bytes(Bytecode& bc, Bytes& bytes,
                 OutputValueFunc output_value,
                 OutputRelocFunc output_reloc)
{
    unsigned long orig = bytes.size();

    // Prefixes
    X86Common::to_bytes(bytes, 0);

    // As opersize may be 0, figure out its "real" value.
    unsigned char opersize = (m_opersize == 0) ? m_mode_bits : m_opersize;

    // Check here again to see if forms are actually legal.
    switch (m_op_sel) {
        case SHORT_FORCED:
        case SHORT:
        {
            // 1 byte relative displacement
            if (m_shortop.get_len() == 0)
                throw InternalError(N_("short jump does not exist"));

            // Opcode
            m_shortop.to_bytes(bytes);

            // Adjust relative displacement to end of bytecode
            std::auto_ptr<IntNum> delta(new IntNum((long)-1));
            m_target.add_abs(delta);
            m_target.m_size = 8;
            m_target.m_sign = 1;
            Location loc = {&bc, bytes.size()-orig};
            output_value(m_target, bytes, 1, loc, 1);
            break;
        }
        case NEAR_FORCED:
        case NEAR:
        {
            // 2/4 byte relative displacement (depending on operand size)
            if (m_nearop.get_len() == 0)
                throw TypeError(N_("near jump does not exist"));

            // Opcode
            m_nearop.to_bytes(bytes);

            unsigned int i = (opersize == 16) ? 2 : 4;

            // Adjust relative displacement to end of bytecode
            std::auto_ptr<IntNum> delta(new IntNum(-(long)i));
            m_target.add_abs(delta);
            m_target.m_size = i*8;
            m_target.m_sign = 1;
            Location loc = {&bc, bytes.size()-orig};
            output_value(m_target, bytes, i, loc, 1);
            break;
        }
        case NONE:
            throw InternalError(N_("jump op_sel cannot be JMP_NONE in tobytes"));
        default:
            throw InternalError(N_("unrecognized relative jump op_sel"));
    }
}

X86Jmp*
X86Jmp::clone() const
{
    return new X86Jmp(*this);
}

}}} // namespace yasm::arch::x86

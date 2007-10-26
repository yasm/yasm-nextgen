//
// x86 jump far bytecode
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
#include "x86jmpfar.h"

#include "util.h"

#include <iomanip>

#include <libyasm/bytes.h>
#include <libyasm/errwarn.h>
#include <libyasm/expr.h>


namespace yasm { namespace arch { namespace x86 {

X86JmpFar::X86JmpFar(const X86Opcode& opcode, std::auto_ptr<Expr> segment,
                     std::auto_ptr<Expr> offset, Bytecode& precbc)
    : m_opcode(opcode),
      m_segment(16, segment),
      m_offset(0, offset)
{
    if (m_segment.finalize(&precbc))
        throw TooComplexError(N_("jump target segment too complex"));
    if (m_offset.finalize(&precbc))
        throw TooComplexError(N_("jump target offset too complex"));
}

X86JmpFar::~X86JmpFar()
{
}

void
X86JmpFar::put(std::ostream& os, int indent_level) const
{
    os << std::setw(indent_level) << "" << "_Far_Jump_\n";
    os << std::setw(indent_level) << "" << "Segment:\n";
    m_segment.put(os, indent_level+1);
    os << std::setw(indent_level) << "" << "Offset:\n";
    m_offset.put(os, indent_level+1);
    m_opcode.put(os, indent_level);
    X86Common::put(os, indent_level);
}

void
X86JmpFar::finalize(Bytecode& bc, Bytecode& prev_bc)
{
}

unsigned long
X86JmpFar::calc_len(Bytecode& bc, Bytecode::AddSpanFunc add_span)
{
    unsigned long len = 0;
   
    unsigned char opersize = (m_opersize == 0) ? m_mode_bits : m_opersize;

    len += m_opcode.get_len();
    len += 2;       // segment
    len += (opersize == 16) ? 2 : 4;
    len += X86Common::calc_len();

    return len;
}

bool
X86JmpFar::expand(Bytecode& bc, unsigned long& len, int span,
                  long old_val, long new_val,
                  /*@out@*/ long& neg_thres, /*@out@*/ long& pos_thres)
{
    return Bytecode::Contents::expand(bc, len, span, old_val, new_val,
                                      neg_thres, pos_thres);
}

void
X86JmpFar::to_bytes(Bytecode& bc, Bytes& bytes,
                    OutputValueFunc output_value,
                    OutputRelocFunc output_reloc)
{
    unsigned long orig = bytes.size();

    X86Common::to_bytes(bytes, 0);
    m_opcode.to_bytes(bytes);

    // As opersize may be 0, figure out its "real" value.
    unsigned char opersize = (m_opersize == 0) ? m_mode_bits : m_opersize;

    // Absolute displacement: segment and offset
    unsigned int i = (opersize == 16) ? 2 : 4;
    m_offset.m_size = i*8;
    output_value(m_offset, bytes, i, bytes.size()-orig, bc, 1);
    m_segment.m_size = 16;
    output_value(m_segment, bytes, 2, bytes.size()-orig, bc, 1);
}

X86JmpFar*
X86JmpFar::clone() const
{
    return new X86JmpFar(*this);
}

}}} // namespace yasm::arch::x86

//
// x86 core bytecode
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
#include "x86opcode.h"

#include "util.h"

#include <iomanip>
#include <ostream>

#include <libyasm/bytes.h>


namespace yasm { namespace arch { namespace x86 {

void
X86Opcode::put(std::ostream& os, int indent_level) const
{
    os << std::setw(indent_level) << "" << "Opcode: ";

    std::ios_base::fmtflags origff = os.flags();
    os << std::hex << std::setfill('0')
       << std::setw(2) << ((unsigned int)m_opcode[0]) << ' '
       << std::setw(2) << ((unsigned int)m_opcode[1]) << ' '
       << std::setw(2) << ((unsigned int)m_opcode[2])
       << std::setfill(' ');
    os.flags(origff);

    os << " OpLen=" << ((unsigned int)m_len);
    os << '\n';
}

void
X86Opcode::to_bytes(Bytes& bytes) const
{
    bytes.write(m_opcode, m_len);
}

void
X86Opcode::make_alt_1()
{
    m_opcode[0] = m_opcode[m_len];
    m_len = 1;
}

}}} // namespace yasm::arch::x86

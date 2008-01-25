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

#include <libyasm/bytecode.h>
#include <libyasm/expr.h>
#include <libyasm/section.h>

#include "x86common.h"
#include "x86opcode.h"


namespace yasm { namespace arch { namespace x86 {

void append_jmpfar(Section& sect,
                   const X86Common& common,
                   const X86Opcode& opcode,
                   std::auto_ptr<Expr> segment,
                   std::auto_ptr<Expr> offset)
{
    Bytecode& bc = sect.fresh_bytecode();
    Bytes& bytes = bc.get_fixed();

    common.to_bytes(bytes, 0);
    opcode.to_bytes(bytes);

    // Absolute displacement: segment and offset
    unsigned int size = (common.m_opersize == 16) ? 2 : 4;
    bc.append_fixed(size, offset);
    bc.append_fixed(2, segment);
}

}}} // namespace yasm::arch::x86

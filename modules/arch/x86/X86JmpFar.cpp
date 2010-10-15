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
#include "X86JmpFar.h"

#include "yasmx/BytecodeContainer.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Expr.h"

#include "X86Common.h"
#include "X86Opcode.h"


void
AppendJmpFar(BytecodeContainer& container,
             const X86Common& common,
             const X86Opcode& opcode,
             std::auto_ptr<Expr> segment,
             std::auto_ptr<Expr> offset,
             SourceLocation source)
{
    Bytecode& bc = container.FreshBytecode();
    Bytes& bytes = bc.getFixed();
    unsigned long orig_size = bytes.size();

    common.ToBytes(bytes, 0);
    opcode.ToBytes(bytes);

    // Absolute displacement: segment and offset
    unsigned int size = (common.m_opersize == 16) ? 2 : 4;
    bc.AppendFixed(size, offset, source).setInsnStart(bytes.size()-orig_size);
    bc.AppendFixed(2, segment, source).setInsnStart(bytes.size()-orig_size);
}

#ifndef YASM_X86JMP_H
#define YASM_X86JMP_H
//
// x86 jump bytecode header file
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
#include <memory>

#include "yasmx/Config/export.h"


namespace clang { class SourceLocation; }

namespace yasm
{

class BytecodeContainer;
class Expr;

namespace arch
{

class X86Common;
class X86Opcode;

enum X86JmpOpcodeSel
{
    X86_JMP_NONE,
    X86_JMP_SHORT,
    X86_JMP_NEAR
};

YASM_STD_EXPORT
void AppendJmp(BytecodeContainer& container,
               const X86Common& common,
               const X86Opcode& shortop,
               const X86Opcode& nearop,
               std::auto_ptr<Expr> target,
               clang::SourceLocation source,
               X86JmpOpcodeSel op_sel = X86_JMP_NONE);

}} // namespace yasm::arch

#endif

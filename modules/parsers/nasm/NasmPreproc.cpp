//
// NASM-compatible preprocessor implementation
//
//  Copyright (C) 2009  Peter Johnson
//
// Based on the LLVM Compiler Infrastructure
// (distributed under the University of Illinois Open Source License.
// See Copying/LLVM.txt for details).
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
#include "NasmPreproc.h"

#include "NasmLexer.h"


using namespace yasm;
using namespace yasm::parser;

NasmPreproc::NasmPreproc(DiagnosticsEngine& diags,
                         SourceManager& sm,
                         HeaderSearch& headers)
    : Preprocessor(diags, sm, headers)
{
}

NasmPreproc::~NasmPreproc()
{
}

void
NasmPreproc::PreInclude(StringRef filename)
{
    Predef p = { Predef::PREINC, filename };
    m_predefs.push_back(p);
}

void
NasmPreproc::PredefineMacro(StringRef macronameval)
{
    Predef p = { Predef::PREDEF, macronameval };
    m_predefs.push_back(p);
}

void
NasmPreproc::UndefineMacro(StringRef macroname)
{
    Predef p = { Predef::UNDEF, macroname };
    m_predefs.push_back(p);
}

void
NasmPreproc::DefineBuiltin(StringRef macronameval)
{
    Predef p = { Predef::BUILTIN, macronameval };
    m_predefs.push_back(p);
}

/// RegisterBuiltinMacro - Register the specified identifier in the identifier
/// table and mark it as a builtin macro to be expanded.
static IdentifierInfo*
RegisterBuiltinMacro(NasmPreproc& pp, const char* name)
{
    // Get the identifier.
    IdentifierInfo* id = pp.getIdentifierInfo(name);

#if 0
    // Mark it as being a macro that is builtin.
    MacroInfo* mi = pp.AllocateMacroInfo(SourceLocation());
    mi->setIsBuiltinMacro();
    pp.setMacroInfo(id, mi);
#endif
    return id;
}

void
NasmPreproc::RegisterBuiltinMacros()
{
    m_LINE = RegisterBuiltinMacro(*this, "__LINE__");
    m_FILE = RegisterBuiltinMacro(*this, "__FILE__");
    m_DATE = RegisterBuiltinMacro(*this, "__DATE__");
    m_TIME = RegisterBuiltinMacro(*this, "__TIME__");
    m_BITS = RegisterBuiltinMacro(*this, "__BITS__");
}

Lexer*
NasmPreproc::CreateLexer(FileID fid, const MemoryBuffer* input_buffer)
{
    return new NasmLexer(fid, input_buffer, *this);
}

#ifndef YASM_NASMPREPROC_H
#define YASM_NASMPREPROC_H
//
// NASM-compatible preprocessor header file
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
#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Config/export.h"
#include "yasmx/Parse/Preprocessor.h"


namespace yasm
{

class IdentifierInfo;

namespace parser
{

class YASM_STD_EXPORT NasmPreproc : public Preprocessor
{
public:
    NasmPreproc(Diagnostic& diags, SourceManager& sm, HeaderSearch& headers);
    ~NasmPreproc();

    virtual void PreInclude(llvm::StringRef filename);
    virtual void PredefineMacro(llvm::StringRef macronameval);
    virtual void UndefineMacro(llvm::StringRef macroname);
    virtual void DefineBuiltin(llvm::StringRef macronameval);

    struct Predef
    {
        enum Type { PREINC, PREDEF, UNDEF, BUILTIN } m_type;
        std::string m_string;
    };

    std::vector<Predef> m_predefs;

protected:
    virtual void RegisterBuiltinMacros();
    virtual Lexer* CreateLexer(FileID fid,
                               const llvm::MemoryBuffer* input_buffer);

private:
    /// Identifiers for builtin macros and other builtins.
    IdentifierInfo *m_LINE, *m_FILE;  // __LINE__, __FILE__
    IdentifierInfo *m_DATE, *m_TIME;  // __DATE__, __TIME__
    IdentifierInfo *m_BITS;           // __BITS__

    SourceLocation m_DATE_loc, m_TIME_loc;
};

}} // namespace yasm::parser

#endif

#ifndef YASM_GASPREPROC_H
#define YASM_GASPREPROC_H
//
// GAS-compatible preprocessor header file
//
//  Copyright (C) 2010  Peter Johnson
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

class YASM_STD_EXPORT GasPreproc : public Preprocessor
{
public:
    GasPreproc(Diagnostic& diags, SourceManager& sm, HeaderSearch& headers);
    ~GasPreproc();

    virtual void PredefineMacro(llvm::StringRef macronameval);
    virtual void UndefineMacro(llvm::StringRef macroname);
    virtual void DefineBuiltin(llvm::StringRef macronameval);

    bool HandleInclude(llvm::StringRef filename, SourceLocation source);

protected:
    virtual void RegisterBuiltinMacros();
    virtual Lexer* CreateLexer(FileID fid,
                               const llvm::MemoryBuffer* input_buffer);

private:
};

}} // namespace yasm::parser

#endif

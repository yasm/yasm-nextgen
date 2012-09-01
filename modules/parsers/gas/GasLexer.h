#ifndef YASM_GASLEXER_H
#define YASM_GASLEXER_H
//
// GAS-compatible lexer header file
//
//  Copyright (C) 2002-2010  Peter Johnson
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
#include "yasmx/Config/export.h"
#include "yasmx/Parse/Lexer.h"
#include "yasmx/Parse/Token.h"


namespace yasm
{

class DiagnosticBuilder;
class Register;

namespace parser
{

class GasPreproc;

class GasToken : public Token
{
public:
    enum Kind
    {
        semi = NUM_COMMON_TOKENS+1,// ;
        char_constant,      // 'x
        locallabelf,        // [0-9]f
        locallabelb,        // [0-9]b
        NUM_GAS_TOKENS
    };
};

class YASM_STD_EXPORT GasLexer : public Lexer
{
public:
    GasLexer(FileID fid,
             const MemoryBuffer* input_buffer,
             Preprocessor& pp);
    GasLexer(SourceLocation file_loc,
             const char* start,
             const char* ptr,
             const char* end);
    virtual ~GasLexer();

protected:
    /// Additional character types.
    enum
    {
        CHAR_UNDER    = 0x10,  // _
        CHAR_PERIOD   = 0x20,  // .
        CHAR_ID_OTHER = 0x40   // e.g. '$', '#', '@', '~', '?'
    };

    /// Return true if this is the body character of an
    /// identifier, which is [a-zA-Z0-9_].
    static inline bool
    isIdentifierBody(unsigned char c)
    {
        return (s_char_info[c] &
                (CHAR_LETTER|CHAR_NUMBER|CHAR_UNDER|CHAR_PERIOD|CHAR_ID_OTHER))
            ? true : false;
    }

    /// Return true if this is the body character of a
    /// preprocessing number, which is [a-zA-Z0-9_.].
    static inline bool
    isNumberBody(unsigned char c)
    {
        return (s_char_info[c] &
                (CHAR_LETTER|CHAR_NUMBER|CHAR_UNDER|CHAR_PERIOD))
            ? true : false;
    }

    void InitCharacterInfo();
    virtual void LexTokenInternal(Token* result);

    bool isEndOfBlockCommentWithEscapedNewLine(const char* cur_ptr);
    bool SkipBlockComment(Token* result, const char* cur_ptr);

    // Helper functions to lex the remainder of a token of the specific type.
    void LexIdentifier      (Token* result, const char* cur_ptr, bool is_label);
    void LexNumericConstant (Token* result, const char* cur_ptr);
    void LexCharConstant    (Token* result, const char* cur_ptr);
    void LexStringLiteral   (Token* result, const char* cur_ptr);
};

}} // namespace yasm::parser

#endif

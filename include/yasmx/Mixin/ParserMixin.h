#ifndef YASM_MIXIN_PARSERMIXIN_H
#define YASM_MIXIN_PARSERMIXIN_H
//
// Parser mixin header file
//
//  Copyright (C) 2009  Peter Johnson
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
#include <util.h>

#include "clang/Basic/SourceLocation.h"
#include <yasmx/Support/Compose.h>
#include <yasmx/Support/errwarn.h>
#include <yasmx/Arch.h>
#include <yasmx/Diagnostic.h>
#include <yasmx/Object.h>
#include <yasmx/Preprocessor.h>

namespace yasm
{

class BytecodeContainer;
class Directives;
class Errwarns;

/// Parser mixin to reduce code duplication in recursive descent parsers.
/// Uses the curiously recurring template pattern.
/// T (the derived class) is expected to implement the following:
///   NONE (from enum TokenType)
///   int Lex(TokenValue* lvalp);
///   static bool isEolTok(int tok);
///   static const char* DescribeToken(int tok);
template <typename T, typename YYSTYPE, typename YYCTYPE = char>
struct ParserMixin
{
    void InitMixin(Object& object,
                   Preprocessor& preproc,
                   Directives& dirs,
                   Diagnostic& diags);
    clang::SourceLocation getTokenSource() const
    { return m_source.getFileLocWithOffset(m_tok-m_bot); }

    DiagnosticBuilder Diag(clang::SourceLocation loc, unsigned int id)
    {
        clang::SourceManager& smgr = m_preproc->getSourceManager();
        return m_diags->Report(clang::FullSourceLoc(loc, smgr), id);
    }

    int getNextToken();
    void getPeekToken();
    bool isEol() { return T::isEolTok(m_token); }

    // Eat all remaining tokens to EOL, discarding all of them.
    void DemandEolNoThrow();

    // Eat all remaining tokens to EOL, discarding all of them.  If there's any
    // intervening tokens, generates an error (junk at end of line).
    void DemandEol();

    void Expect(int token);
    bool ExpectAndConsume(int token, unsigned int diag);

    Object* m_object;
    BytecodeContainer* m_container;
    Preprocessor* m_preproc;
    Directives* m_dirs;
    Diagnostic* m_diags;

    Arch* m_arch;
    unsigned int m_wordsize;

    clang::SourceLocation m_source;

    YYCTYPE *m_bot, *m_tok, *m_ptr, *m_cur, *m_lim;

    int m_token;        // enum T::TokenType or any character
    YYSTYPE m_tokval;
    char m_tokch;       // first character of token

    // one token of lookahead; used sparingly
    int m_peek_token;   // NONE if none
    YYSTYPE m_peek_tokval;
    char m_peek_tokch;
};

template <typename T, typename S, typename C>
inline void
ParserMixin<T, S, C>::InitMixin(Object& object,
                                Preprocessor& preproc,
                                Directives& dirs,
                                Diagnostic& diags)
{
    m_object = &object;
    m_preproc = &preproc;
    m_dirs = &dirs;
    m_diags = &diags;
    m_arch = object.getArch();
    m_wordsize = m_arch->getModule().getWordSize();

    m_token = T::NONE;
    m_peek_token = T::NONE;
}

template <typename T, typename S, typename C>
inline int
ParserMixin<T, S, C>::getNextToken()
{
    m_token = static_cast<T*>(this)->Lex(&m_tokval);
    return m_token;
}

template <typename T, typename S, typename C>
void
ParserMixin<T, S, C>::getPeekToken()
{
    char savech = m_tokch;
    assert(m_peek_token == T::NONE &&
           "only can have one token of lookahead");
    m_peek_token = static_cast<T*>(this)->Lex(&m_peek_tokval);
    m_peek_tokch = m_tokch;
    m_tokch = savech;
}

template <typename T, typename S, typename C>
void
ParserMixin<T, S, C>::DemandEolNoThrow()
{
    if (isEol())
        return;

    if (m_peek_token != T::NONE)
        getNextToken();

    while (!isEol())
        m_token = *m_cur++;
}

template <typename T, typename S, typename C>
void
ParserMixin<T, S, C>::DemandEol()
{
    if (isEol())
        return;

    char tokch = m_tokch;
    DemandEolNoThrow();

    throw SyntaxError(String::Compose(
        N_("junk at end of line, first unrecognized character is `%1'"),
        tokch));
}

template <typename T, typename S, typename C>
void
ParserMixin<T, S, C>::Expect(int token)
{
    if (m_token == token)
        return;

    throw ParseError(String::Compose("expected %1", T::DescribeToken(token)));
}

template <typename T, typename S, typename C>
bool
ParserMixin<T, S, C>::ExpectAndConsume(int token, unsigned int diag)
{
    if (m_token == token)
    {
        getNextToken();
        return true;
    }

    Diag(getTokenSource(), diag);
    return false;
}

} // namespace yasm

#endif

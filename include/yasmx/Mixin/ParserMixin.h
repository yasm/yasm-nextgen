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

#include <yasmx/Support/Compose.h>
#include <yasmx/Support/errwarn.h>
#include <yasmx/Arch.h>
#include <yasmx/Linemap.h>
#include <yasmx/Object.h>

namespace yasm
{

class BytecodeContainer;
class Directives;
class Errwarns;
class Preprocessor;

/// Parser mixin to reduce code duplication in recursive descent parsers.
/// Uses the curiously recurring template pattern.
/// T (the derived class) is expected to implement the following:
///   NONE (from enum TokenType)
///   int lex(TokenValue* lvalp);
///   static bool is_eol_tok(int tok);
///   static const char* describe_token(int tok);
template <typename T, typename YYSTYPE, typename YYCTYPE = char>
struct ParserMixin
{
    void init_mixin(Object& object,
                    Preprocessor& preproc,
                    bool save_input,
                    Directives& dirs,
                    Linemap& linemap);
    unsigned long get_cur_line() const { return m_linemap->get_current(); }

    int get_next_token();
    void get_peek_token();
    bool is_eol() { return T::is_eol_tok(m_token); }

    // Eat all remaining tokens to EOL, discarding all of them.
    void demand_eol_nothrow();

    // Eat all remaining tokens to EOL, discarding all of them.  If there's any
    // intervening tokens, generates an error (junk at end of line).
    void demand_eol();

    void expect(int token);

    Object* m_object;
    BytecodeContainer* m_container;
    Preprocessor* m_preproc;
    Directives* m_dirs;
    Linemap* m_linemap;

    Arch* m_arch;
    unsigned int m_wordsize;

    bool m_save_input;

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
ParserMixin<T, S, C>::init_mixin(Object& object,
                                 Preprocessor& preproc,
                                 bool save_input,
                                 Directives& dirs,
                                 Linemap& linemap)
{
    m_object = &object;
    m_preproc = &preproc;
    m_dirs = &dirs;
    m_linemap = &linemap;
    m_arch = object.get_arch();
    m_wordsize = m_arch->get_module().get_wordsize();

    m_save_input = save_input;
    m_token = T::NONE;
    m_peek_token = T::NONE;
}

template <typename T, typename S, typename C>
inline int
ParserMixin<T, S, C>::get_next_token()
{
    m_token = static_cast<T*>(this)->lex(&m_tokval);
    return m_token;
}

template <typename T, typename S, typename C>
void
ParserMixin<T, S, C>::get_peek_token()
{
    char savech = m_tokch;
    assert(m_peek_token == T::NONE &&
           "only can have one token of lookahead");
    m_peek_token = static_cast<T*>(this)->lex(&m_peek_tokval);
    m_peek_tokch = m_tokch;
    m_tokch = savech;
}

template <typename T, typename S, typename C>
void
ParserMixin<T, S, C>::demand_eol_nothrow()
{
    if (is_eol())
        return;

    if (m_peek_token != T::NONE)
        get_next_token();

    while (!is_eol())
        m_token = *m_cur++;
}

template <typename T, typename S, typename C>
void
ParserMixin<T, S, C>::demand_eol()
{
    if (is_eol())
        return;

    char tokch = m_tokch;
    demand_eol_nothrow();

    throw SyntaxError(String::compose(
        N_("junk at end of line, first unrecognized character is `%1'"),
        tokch));
}

template <typename T, typename S, typename C>
void
ParserMixin<T, S, C>::expect(int token)
{
    if (m_token == token)
        return;

    throw ParseError(String::compose("expected %1", T::describe_token(token)));
}

} // namespace yasm

#endif

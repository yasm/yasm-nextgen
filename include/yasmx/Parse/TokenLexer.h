#ifndef YASM_PARSE_TOKENLEXER_H
#define YASM_PARSE_TOKENLEXER_H
//
// Lex from a token buffer interface
//
// Based on the LLVM Compiler Infrastructure
// (distributed under the University of Illinois Open Source License.
// See Copying/LLVM.txt for details).
//
// Modifications copyright (C) 2010  Peter Johnson
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

namespace yasm
{

#if 0
class MacroArgs;
class MacroInfo;
#endif
class Preprocessor;
class Token;

/// TokenLexer - This implements a lexer that returns token from a macro body
/// or token stream instead of lexing from a character buffer.  This is used for
/// macro expansion, for example.
///
class YASM_LIB_EXPORT TokenLexer
{
#if 0
    /// The macro we are expanding from.  This is null if expanding a
    /// token stream.
    ///
    MacroInfo* m_macro;

    /// The actual arguments specified for a function-like macro, or
    /// null.  The TokenLexer owns the pointed-to object.
    MacroArgs* m_actual_args;
#endif

    /// The current preprocessor object we are expanding for.
    Preprocessor& m_pp;

    /// This is the pointer to an array of tokens that the macro is
    /// defined to, with arguments expanded for function-like macros.  If this is
    /// a token stream, these are the tokens we are returning.  This points into
    /// the macro definition we are lexing from, a scratch buffer allocated from
    /// the preprocessor's bump pointer allocator, or some other buffer that we
    /// may or may not own (depending on OwnsTokens).
    const Token* m_tokens;

    /// This is the length of the Tokens array.
    unsigned m_num_tokens;

    /// This is the next token that Lex will return.
    unsigned m_cur_token;

    /// The source location range where this macro was expanded.
    SourceLocation m_expand_loc_start, m_expand_loc_end;

    /// Lexical information about the expansion point of the macro: the identifier
    /// that the macro expanded from had these properties.
    bool m_at_start_of_line : 1;
    bool m_has_leading_space : 1;

    /// This is true if this TokenLexer allocated the Tokens
    /// array, and thus needs to free it when destroyed.  For simple object-like
    /// macros (for example) we just point into the token buffer of the macro
    /// definition, we don't make a copy of it.
    bool m_owns_tokens : 1;

    /// This is true when tokens lexed from the TokenLexer
    /// should not be subject to further macro expansion.
    bool m_disable_macro_expansion : 1;

    TokenLexer(const TokenLexer&);  // DO NOT IMPLEMENT
    void operator=(const TokenLexer&); // DO NOT IMPLEMENT
public:
#if 0
    /// Create a TokenLexer for the specified macro with the specified actual
    /// arguments.  Note that this ctor takes ownership of the ActualArgs pointer.
    /// ILEnd specifies the location of the ')' for a function-like macro or the
    /// identifier for an object-like macro.
    TokenLexer(Token& tok, SourceLocation il_end, MacroArgs* actual_args,
               Preprocessor& pp)
        : m_macro(0), m_actual_args(0), m_pp(pp), m_owns_tokens(false)
    {
        Init(tok, il_end, actual_args);
    }

    /// Initialize this TokenLexer to expand from the specified macro
    /// with the specified argument information.  Note that this ctor takes
    /// ownership of the ActualArgs pointer.  ILEnd specifies the location of the
    /// ')' for a function-like macro or the identifier for an object-like macro.
    void Init(Token& tok, SourceLocation il_end, MacroArgs* actual_args);
#endif
    /// Create a TokenLexer for the specified token stream.  If 'OwnsTokens' is
    /// specified, this takes ownership of the tokens and delete[]'s them when
    /// the token lexer is empty.
    TokenLexer(const Token* tok_array, unsigned num_toks,
               bool disable_expansion, bool owns_tokens, Preprocessor& pp)
        : /*m_macro(0), m_actual_args(0),*/ m_pp(pp), m_owns_tokens(false)
    {
        Init(tok_array, num_toks, disable_expansion, owns_tokens);
    }

    /// Initialize this TokenLexer with the specified token stream.
    /// This does not take ownership of the specified token vector.
    ///
    /// DisableExpansion is true when macro expansion of tokens lexed from this
    /// stream should be disabled.
    void Init(const Token* tok_array, unsigned num_toks,
              bool disable_macro_expansion, bool owns_tokens);

    ~TokenLexer() { destroy(); }

    /// isNextTokenLParen - If the next token lexed will pop this macro off the
    /// expansion stack, return 2.  If the next unexpanded token is a '(', return
    /// 1, otherwise return 0.
    unsigned isNextTokenLParen() const;

    /// Lex - Lex and return a token from this macro stream.
    void Lex(Token* tok);

private:
    void destroy();

    /// Return true if the next lex call will pop this macro off the
    /// include stack.
    bool isAtEnd() const
    {
        return m_cur_token == m_num_tokens;
    }

#if 0
    /// Tok is the LHS of a ## operator, and CurToken is the ##
    /// operator.  Read the ## and RHS, and paste the LHS/RHS together.  If there
    /// are is another ## after it, chomp it iteratively.  Return the result as
    /// Tok.  If this returns true, the caller should immediately return the
    /// token.
    bool PasteTokens(Token& Tok);

    /// Expand the arguments of a function-like macro so that we can quickly
    /// return preexpanded tokens from Tokens.
    void ExpandFunctionArguments();
#endif
};

} // namespace yasm

#endif

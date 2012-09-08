#ifndef YASM_PARSE_PARSERMIXIN_H
#define YASM_PARSE_PARSERMIXIN_H
///
/// @file
/// @brief Parser mixin interface.
///
/// @license
///  Based on the LLVM Compiler Infrastructure
///  (distributed under the University of Illinois Open Source License.
///  See Copying/LLVM.txt for details).
///
///  Modifications copyright (C) 2009  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
/// 1. Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
/// 2. Redistributions in binary form must reproduce the above copyright
///    notice, this list of conditions and the following disclaimer in the
///    documentation and/or other materials provided with the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
/// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
/// @endlicense
///
#include "yasmx/Basic/LLVM.h"
#include "yasmx/Config/export.h"
#include "yasmx/Parse/Parser.h"
#include "yasmx/Parse/Preprocessor.h"
#include "yasmx/Parse/Token.h"


namespace yasm
{

class DiagnosticBuilder;
class SourceLocation;

/// Interface to override expression term (e.g. the lowest level of an
/// expression) parsing by ParseExpr.
class YASM_LIB_EXPORT ParseExprTerm
{
public:
    virtual ~ParseExprTerm();

    /// Entry point. If term is not recognized, should set handled to false,
    /// not modify the parser state, and return true.
    /// @param e        expression (term value should be assigned to this)
    /// @param parser   parser implementation to get tokens from
    /// @param handled  (output) set to true if term handled and token consumed
    /// @return False to stop expression parsing (should generally return true).
    virtual bool operator() (Expr& e, ParserImpl& parser, bool* handled) const
        = 0;
};

class YASM_LIB_EXPORT ParserImpl
{
public:
    Preprocessor& m_preproc;

    /// The current token we are peeking ahead.  All parsing methods assume
    /// that this is valid.
    Token m_token;

private:
    unsigned short m_paren_count, m_bracket_count;

public:
    ParserImpl(Preprocessor& preproc);
    virtual ~ParserImpl();

    Preprocessor& getPreprocessor() const { return m_preproc; }

    /// Return true if the cur token is '(' or ')'.
    bool isTokenParen() const
    {
        return m_token.is(Token::l_paren) || m_token.is(Token::r_paren);
    }
    /// Return true if the cur token is '[' or ']'.
    bool isTokenBracket() const
    {
        return m_token.is(Token::l_square) || m_token.is(Token::r_square);
    }

    /// Consume the current 'peek token' and lex the next one.
    /// This does not work with all kinds of tokens: specific other
    /// tokens must be consumed with custom methods below.  This returns the
    /// location of the consumed token.
    SourceLocation ConsumeToken()
    {
        assert(!isTokenParen() && !isTokenBracket() &&
               "Should consume special tokens with consume*token");
        SourceLocation prev_tok_location = m_token.getLocation();
        m_preproc.Lex(&m_token);
        return prev_tok_location;
    }

    /// Dispatch to the right consume* method based on the
    /// current token type.  This should only be used in cases where the type
    /// of the token really isn't known, e.g. in error recovery.
    SourceLocation ConsumeAnyToken()
    {
        if (isTokenParen())
            return ConsumeParen();
        else if (isTokenBracket())
            return ConsumeBracket();
        else
            return ConsumeToken();
    }

    /// This consume method keeps the paren count up-to-date.
    SourceLocation ConsumeParen()
    {
        assert(isTokenParen() && "wrong consume method");
        if (m_token.is(Token::l_paren))
            ++m_paren_count;
        else if (m_paren_count)
            --m_paren_count; // Don't let unbalanced drive the count negative.
        SourceLocation prev_tok_location = m_token.getLocation();
        m_preproc.Lex(&m_token);
        return prev_tok_location;
    }

    /// This consume method keeps the bracket count up-to-date.
    SourceLocation ConsumeBracket()
    {
        assert(isTokenBracket() && "wrong consume method");
        if (m_token.is(Token::l_square))
            ++m_bracket_count;
        else if (m_bracket_count)
            --m_bracket_count; // Don't let unbalanced drive the count negative.
        SourceLocation prev_tok_location = m_token.getLocation();
        m_preproc.Lex(&m_token);
        return prev_tok_location;
    }

    /// This peeks ahead N tokens and returns that token
    /// without consuming any tokens.  LookAhead(0) returns 'Tok', LookAhead(1)
    /// returns the token after Tok, etc.
    ///
    /// Note that this differs from the Preprocessor's LookAhead method, because
    /// the Parser always has one token lexed that the preprocessor doesn't.
    ///
    const Token& getLookAheadToken(unsigned int n)
    {
        if (n == 0 || m_token.is(Token::eof))
            return m_token;
        return m_preproc.LookAhead(n-1);
    }

    /// NextToken - This peeks ahead one token and returns it without
    /// consuming it.
    const Token& NextToken() { return m_preproc.LookAhead(0); }

    /// For punctuation with a LHS and RHS (e.g. '['/']'),
    /// this helper function matches and consumes the specified RHS token if
    /// present.  If not present, it emits the specified diagnostic indicating
    /// that the parser failed to match the RHS of the token at LHSLoc.  LHSName
    /// should be the name of the unmatched LHS token.  This returns the
    /// location of the consumed token.
    SourceLocation MatchRHSPunctuation(unsigned int rhs_tok,
                                       SourceLocation lhs_loc);

    /// The parser expects that 'ExpectedTok' is next in the
    /// input.  If so, it is consumed and false is returned.
    ///
    /// If the input is malformed, this emits the specified diagnostic.  Next,
    /// if SkipToTok is specified, it calls SkipUntil(SkipToTok).  Finally,
    /// true is returned.
    bool ExpectAndConsume(unsigned int expected_tok,
                          unsigned int diag_id,
                          const char* diag_msg = "",
                          unsigned int skip_to_tok = Token::unknown);

    DiagnosticBuilder Diag(SourceLocation loc, unsigned int id)
    {
        return m_preproc.Diag(loc, id);
    }
    DiagnosticBuilder Diag(const Token& tok, unsigned int id)
    {
        return m_preproc.Diag(tok, id);
    }

    /// Read tokens until we get to the specified token, then consume
    /// it (unless DontConsume is true).  Because we cannot guarantee that the
    /// token will ever occur, this skips to the next token, or to some likely
    /// good stopping point.  If stop_at_eos is true, skipping will stop at any
    /// end of statement token.
    /// 
    /// If it finds the specified token, it returns true, otherwise it
    /// returns false.  
    bool SkipUntil(unsigned int tok_kind,
                   bool stop_at_eos = true,
                   bool dont_consume = false)
    {
        return SkipUntil(&tok_kind, 1, stop_at_eos, dont_consume);
    }

    bool SkipUntil(unsigned int tk1,
                   unsigned int tk2,
                   bool stop_at_eos = true,
                   bool dont_consume = false)
    {
        unsigned int tok_array[] = {tk1, tk2};
        return SkipUntil(tok_array, 2, stop_at_eos, dont_consume);
    }

    bool SkipUntil(const unsigned int* toks,
                   unsigned int num_toks,
                   bool stop_at_eos = true,
                   bool dont_consume = false);

    /// Merge tokens as a string until we get to one of the specified tokens,
    /// we reach end of statement (if stop_at_eos is true) or we hit a token
    /// with leading whitespace (if stop_at_ws is true).
    StringRef MergeTokensUntil(const unsigned int* toks,
                               unsigned int num_toks,
                               SourceLocation* start,
                               SourceLocation* end,
                               SmallVectorImpl<char>& buffer,
                               bool stop_at_eos = true,
                               bool stop_at_ws = true);

    /// Expression parser.
    virtual bool ParseExpr(Expr& e, const ParseExprTerm* parse_term = 0) = 0;
};

} // namespace yasm

#endif

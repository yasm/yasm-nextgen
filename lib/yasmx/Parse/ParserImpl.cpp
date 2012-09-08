///
/// @file
/// @brief Parser mixin implementation
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
#include "yasmx/Parse/ParserImpl.h"

#include "yasmx/Basic/SourceLocation.h"


using namespace yasm;

ParseExprTerm::~ParseExprTerm()
{
}

ParserImpl::ParserImpl(Preprocessor& preproc)
    : m_preproc(preproc)
    , m_paren_count(0)
    , m_bracket_count(0)
{
    m_token.StartToken();
}

ParserImpl::~ParserImpl()
{
}

SourceLocation
ParserImpl::MatchRHSPunctuation(unsigned int rhs_tok, SourceLocation lhs_loc)
{
    if (m_token.is(rhs_tok))
        return ConsumeAnyToken();

    SourceLocation R = m_token.getLocation();
    const char* LHSName = "unknown";
    diag::kind DID = diag::err_parse_error;
    switch (rhs_tok)
    {
    case Token::r_paren : LHSName = "("; DID = diag::err_expected_rparen; break;
    case Token::r_square: LHSName = "["; DID = diag::err_expected_rsquare; break;
    case Token::greater: LHSName = "<"; DID = diag::err_expected_greater; break;
    default: break;
    }
    Diag(m_token, DID);
    Diag(lhs_loc, diag::note_matching) << LHSName;
    SkipUntil(rhs_tok);
    return R;
}

bool
ParserImpl::ExpectAndConsume(unsigned int expected_tok,
                             unsigned int diag_id,
                             const char* diag_msg,
                             unsigned int skip_to_tok)
{
    if (m_token.is(expected_tok))
    {
        ConsumeAnyToken();
        return false;
    }

#if 0
    const char* spelling = 0;
    SourceLocation end_loc =
        m_preproc.getLocForEndOfToken(m_prev_tok_location);
    if (end_loc.isValid() && 
        (spelling = tok::getTokenSimpleSpelling(expected_tok)))
    {
        // Show what code to insert to fix this problem.
        Diag(end_loc, diag_id) 
            << diag_msg
            << CodeModificationHint::CreateInsertion(end_loc, spelling);
    }
    else
#endif
        Diag(m_token, diag_id) << diag_msg;

    if (skip_to_tok != Token::unknown)
        SkipUntil(skip_to_tok);
    return true;
}

bool
ParserImpl::SkipUntil(const unsigned int* toks,
                      unsigned int num_toks,
                      bool stop_at_eos,
                      bool dont_consume)
{
    // We always want this function to skip at least one token if the first
    // token isn't T and if not at EOF.
    bool isFirstTokenSkipped = true;
    for (;;)
    {
        // If we found one of the tokens, stop and return true.
        for (unsigned i = 0; i != num_toks; ++i)
        {
            if (m_token.is(toks[i]))
            {
                if (dont_consume)
                {
                    // Noop, don't consume the token.
                }
                else
                    ConsumeAnyToken();
                return true;
            }
        }

        switch (m_token.getKind()) {
            case Token::eof:
                // Ran out of tokens.
                return false;

            case Token::l_paren:
                // Recursively skip properly-nested parens.
                ConsumeParen();
                SkipUntil(Token::r_paren, false);
                break;
            case Token::l_square:
                // Recursively skip properly-nested square brackets.
                ConsumeBracket();
                SkipUntil(Token::r_square, false);
                break;
#if 0
            case Token::l_brace:
                // Recursively skip properly-nested braces.
                ConsumeBrace();
                SkipUntil(Token::r_brace, false);
                break;
#endif

    // Okay, we found a ']' or '}' or ')', which we think should be balanced.
    // Since the user wasn't looking for this token (if they were, it would
    // already be handled), this isn't balanced.  If there is a LHS token at a
    // higher level, we will assume that this matches the unbalanced token
    // and return it.  Otherwise, this is a spurious RHS token, which we skip.
            case Token::r_paren:
                if (m_paren_count && !isFirstTokenSkipped)
                    return false;  // Matches something.
                ConsumeParen();
                break;
            case Token::r_square:
                if (m_bracket_count && !isFirstTokenSkipped)
                    return false;  // Matches something.
                ConsumeBracket();
                break;
#if 0
            case tok::r_brace:
                if (BraceCount && !isFirstTokenSkipped)
                    return false;  // Matches something.
                ConsumeBrace();
                break;
#endif

            case Token::string_literal:
                ConsumeToken();
                break;
            default:
                // Stop if requested to on end of statement tokens.
                if (stop_at_eos && m_token.isEndOfStatement())
                    return false;
                // Otherwise skip this token.
                ConsumeToken();
                break;
        }
        isFirstTokenSkipped = false;
    }
}

StringRef
ParserImpl::MergeTokensUntil(const unsigned int* toks,
                             unsigned int num_toks,
                             SourceLocation* start,
                             SourceLocation* end,
                             SmallVectorImpl<char>& buffer,
                             bool stop_at_eos,
                             bool stop_at_ws)
{
    buffer.clear();
    *start = *end = m_token.getLocation();
    for (;;)
    {
        // If we found one of the tokens, stop.
        for (unsigned i = 0; i < num_toks; ++i)
        {
            if (m_token.is(toks[i]))
                goto done;
        }

        // If we hit end of statement, stop.
        if (stop_at_eos && m_token.isEndOfStatement())
            break;

        // Turn the token back into characters.
        // The first if's are optimizations for common cases.
        StringRef data;
        if (m_token.isLiteral())
        {
            data = m_token.getLiteral();
        }
        else if (m_token.is(Token::identifier) || m_token.is(Token::label))
        {
            IdentifierInfo* ii = m_token.getIdentifierInfo();
            data = ii->getName();
        }
        else
        {
            // Get the raw data from the source manager.
            SourceManager& smgr = m_preproc.getSourceManager();
            data = StringRef(smgr.getCharacterData(m_token.getLocation()),
                             m_token.getLength());
        }
        buffer.append(data.begin(), data.end());
        *end = m_token.getEndLocation();
        ConsumeAnyToken();

        // If we hit a token with leading space, stop.
        // We do this down here in case the first token had preceding ws.
        if (stop_at_ws && m_token.hasLeadingSpace())
            break;
    }
done:
    return StringRef(buffer.data(), buffer.size());
}

//===--- PPCaching.cpp - Handle caching lexed tokens ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements pieces of the Preprocessor interface that manage the
// caching of lexed tokens.
//
//===----------------------------------------------------------------------===//

#include "yasmx/Parse/Preprocessor.h"


using namespace yasm;

/// EnableBacktrackAtThisPos - From the point that this method is called, and
/// until CommitBacktrackedTokens() or Backtrack() is called, the Preprocessor
/// keeps track of the lexed tokens so that a subsequent Backtrack() call will
/// make the Preprocessor re-lex the same tokens.
///
/// Nested backtracks are allowed, meaning that EnableBacktrackAtThisPos can
/// be called multiple times and CommitBacktrackedTokens/Backtrack calls will
/// be combined with the EnableBacktrackAtThisPos calls in reverse order.
void
Preprocessor::EnableBacktrackAtThisPos()
{
    m_backtrack_positions.push_back(m_cached_lex_pos);
    EnterCachingLexMode();
}

/// CommitBacktrackedTokens - Disable the last EnableBacktrackAtThisPos call.
void
Preprocessor::CommitBacktrackedTokens()
{
    assert(!m_backtrack_positions.empty()
           && "EnableBacktrackAtThisPos was not called!");
    m_backtrack_positions.pop_back();
}

/// Backtrack - Make Preprocessor re-lex the tokens that were lexed since
/// EnableBacktrackAtThisPos() was previously called. 
void
Preprocessor::Backtrack()
{
    assert(!m_backtrack_positions.empty()
           && "EnableBacktrackAtThisPos was not called!");
    m_cached_lex_pos = m_backtrack_positions.back();
    m_backtrack_positions.pop_back();
}

void
Preprocessor::CachingLex(Token* result)
{
    if (m_cached_lex_pos < m_cached_tokens.size())
    {
        *result = m_cached_tokens[m_cached_lex_pos++];
        return;
    }

    ExitCachingLexMode();
    Lex(result);

    if (!isBacktrackEnabled())
    {
        // All cached tokens were consumed.
        m_cached_tokens.clear();
        m_cached_lex_pos = 0;
        return;
    }

    // We should cache the lexed token.

    EnterCachingLexMode();
    if (result->isNot(Token::eof))
    {
        m_cached_tokens.push_back(*result);
        ++m_cached_lex_pos;
    }
}

void
Preprocessor::EnterCachingLexMode()
{
    if (isInCachingLexMode())
        return;

    PushIncludeMacroStack();
}

const Token&
Preprocessor::PeekAhead(unsigned int n)
{
    assert(m_cached_lex_pos + n > m_cached_tokens.size()
           && "Confused caching.");
    ExitCachingLexMode();
    for (unsigned int c = m_cached_lex_pos + n - m_cached_tokens.size();
         c > 0; --c)
    {
        m_cached_tokens.push_back(Token());
        Lex(&m_cached_tokens.back());
    }
    EnterCachingLexMode();
    return m_cached_tokens.back();
}

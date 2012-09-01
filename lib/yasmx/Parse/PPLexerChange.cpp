//===--- PPLexerChange.cpp - Handle changing lexers in the preprocessor ---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements pieces of the Preprocessor interface that manage the
// current lexer stack.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/MemoryBuffer.h"
#include "yasmx/Basic/SourceManager.h"
#include "yasmx/Parse/HeaderSearch.h"
#include "yasmx/Parse/Preprocessor.h"

using namespace yasm;

bool
Preprocessor::isInPrimaryFile() const
{
    if (IsFileLexer())
        return m_include_macro_stack.empty();
  
    // If there are any stacked lexers, we're in a #include.
    assert(IsFileLexer(m_include_macro_stack[0]) &&
           "Top level include stack isn't our primary lexer?");
    for (unsigned i = 1, e = m_include_macro_stack.size(); i != e; ++i)
        if (IsFileLexer(m_include_macro_stack[i]))
            return false;
    return true;
}

Lexer*
Preprocessor::getCurrentFileLexer() const
{
    if (IsFileLexer())
        return m_cur_lexer.get();
  
    // Look for a stacked lexer.
    for (unsigned i = m_include_macro_stack.size(); i != 0; --i)
    {
        const IncludeStackInfo& ISI = m_include_macro_stack[i-1];
        if (IsFileLexer(ISI))
            return ISI.lexer;
    }
    return 0;
}

void
Preprocessor::EnterSourceFile(FileID FID,
                              const DirectoryLookup* CurDir,
                              SourceLocation Loc)
{
    assert(m_cur_token_lexer == 0 && "Cannot #include a file inside a macro!");
    ++m_NumEnteredSourceFiles;

    if (m_MaxIncludeStackDepth < m_include_macro_stack.size())
        m_MaxIncludeStackDepth = m_include_macro_stack.size();

    // Get the MemoryBuffer for this FID, if it fails, we fail.
    bool Invalid = false;
    const MemoryBuffer *InputFile =
        getSourceManager().getBuffer(FID, Loc, &Invalid);
    if (Invalid) {
        SourceLocation FileStart = m_source_mgr.getLocForStartOfFile(FID);
        Diag(Loc, diag::err_pp_error_opening_file)
            << std::string(m_source_mgr.getBufferName(FileStart)) << "";
        return;
    }

    EnterSourceFileWithLexer(CreateLexer(FID, InputFile), CurDir);
}

void Preprocessor::EnterSourceFileWithLexer(Lexer* lexer,
                                            const DirectoryLookup* cur_dir)
{
    // Add the current lexer to the include stack.
    if (m_cur_lexer || m_cur_token_lexer)
        PushIncludeMacroStack();

    m_cur_lexer.reset(lexer);
    m_cur_dir_lookup = cur_dir;

#if 0
    // Notify the client, if desired, that we are in a new source file.
    if (Callbacks && !CurLexer->Is_PragmaLexer) {
        SrcMgr::CharacteristicKind FileType =
            SourceMgr.getFileCharacteristic(CurLexer->getFileLoc());

        Callbacks->FileChanged(CurLexer->getFileLoc(),
                               PPCallbacks::EnterFile, FileType);
    }
#endif
}

void
Preprocessor::EnterTokenStream(const Token* toks,
                               unsigned int num_toks,
                               bool disable_macro_expansion,
                               bool owns_tokens)
{
    // Save our current state.
    PushIncludeMacroStack();
    m_cur_dir_lookup = 0;

    // Create a macro expander to expand from the specified token stream.
    if (m_num_cached_token_lexers == 0)
    {
        m_cur_token_lexer.reset(new TokenLexer(toks, num_toks,
                                               disable_macro_expansion,
                                               owns_tokens, *this));
    }
    else
    {
        m_cur_token_lexer.reset(m_token_lexer_cache[--m_num_cached_token_lexers]);
        m_cur_token_lexer->Init(toks, num_toks, disable_macro_expansion,
                                owns_tokens);
    }
}

/// HandleEndOfFile - This callback is invoked when the lexer hits the end of
/// the current file.  This either returns the EOF token or pops a level off
/// the include stack and keeps going.
bool
Preprocessor::HandleEndOfFile(Token* Result, bool isEndOfMacro)
{
    assert(!m_cur_token_lexer &&
           "Ending a file when currently in a macro!");

    // If this is a #include'd file, pop it off the include stack and continue
    // lexing the #includer file.
    if (!m_include_macro_stack.empty())
    {
        // We're done with the #included file.
        RemoveTopOfLexerStack();

#if 0
        // Notify the client, if desired, that we are in a new source file.
        if (Callbacks && !isEndOfMacro && CurPPLexer)
        {
            SrcMgr::CharacteristicKind FileType =
                SourceMgr.getFileCharacteristic(CurPPLexer->getSourceLocation());
            Callbacks->FileChanged(CurPPLexer->getSourceLocation(),
                                   PPCallbacks::ExitFile, FileType);
        }
#endif

        // Client should lex another token.
        return false;
    }

    // If the file ends with a newline, form the EOF token on the newline itself,
    // rather than "on the line following it", which doesn't exist.  This makes
    // diagnostics relating to the end of file include the last file that the user
    // actually typed, which is goodness.
    const char *EndPos = m_cur_lexer->m_buf_end;
    if (EndPos != m_cur_lexer->m_buf_start &&
        (EndPos[-1] == '\n' || EndPos[-1] == '\r'))
    {
        --EndPos;

        // Handle \n\r and \r\n:
        if (EndPos != m_cur_lexer->m_buf_start &&
            (EndPos[-1] == '\n' || EndPos[-1] == '\r') &&
            EndPos[-1] != EndPos[0])
            --EndPos;
    }

    Result->StartToken();
    Result->setFlag(Token::EndOfStatement);
    m_cur_lexer->m_buf_ptr = EndPos;
    m_cur_lexer->FormTokenWithChars(Result, EndPos, Token::eof);

    // We're done with the #included file.
    m_cur_lexer.reset();

    // This is the end of the top-level file.
    return true;
}

/// HandleEndOfTokenLexer - This callback is invoked when the current TokenLexer
/// hits the end of its token stream.
bool
Preprocessor::HandleEndOfTokenLexer(Token* Result)
{
    assert(m_cur_token_lexer && !m_cur_lexer &&
           "Ending a macro when currently in a #include file!");

    // Delete or cache the now-dead macro expander.
    if (m_num_cached_token_lexers == TokenLexerCacheSize)
        m_cur_token_lexer.reset();
    else
        m_token_lexer_cache[m_num_cached_token_lexers++] =
            m_cur_token_lexer.take();

    // Handle this like a #include file being popped off the stack.
    return HandleEndOfFile(Result, true);
}

/// RemoveTopOfLexerStack - Pop the current lexer/macro exp off the top of the
/// lexer stack.  This should only be used in situations where the current
/// state of the top-of-stack lexer is unknown.
void
Preprocessor::RemoveTopOfLexerStack()
{
    assert(!m_include_macro_stack.empty() && "Ran out of stack entries to load");

    if (m_cur_token_lexer)
    {
        // Delete or cache the now-dead macro expander.
        if (m_num_cached_token_lexers == TokenLexerCacheSize)
            m_cur_token_lexer.reset();
        else
            m_token_lexer_cache[m_num_cached_token_lexers++] =
                m_cur_token_lexer.take();
    }

    PopIncludeMacroStack();
}

//
// Lexer implementation
//
// Based on the LLVM Compiler Infrastructure
// (distributed under the University of Illinois Open Source License.
// See Copying/LLVM.txt for details).
//
// Modifications copyright (C) 2009  Peter Johnson
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
#include "yasmx/Parse/Lexer.h"

#include "llvm/Support/MemoryBuffer.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Parse/Preprocessor.h"


using namespace yasm;

unsigned char Lexer::s_char_info[256];

void
Lexer::InitLexer(const char* start, const char* ptr, const char* end)
{
    m_buf_start = start;
    m_buf_ptr = ptr;
    m_buf_end = end;
  
    assert(end[0] == 0 &&
           "We assume that the input buffer has a null character at the end"
           " to simplify lexing!");
  
    // Start of the file is a start of line.
    m_is_at_start_of_line = true;
  
    // We are not after parsing a %.
    m_parsing_preprocessor_directive = false;
  
    // We are not in raw mode.  Raw mode disables diagnostics and
    // interpretation of tokens (e.g. identifiers, thus disabling macro
    // expansion).  It is used to quickly lex the tokens of the buffer, e.g.
    // when handling a "%if 0" block or otherwise skipping over tokens.
    m_lexing_raw_mode = false;
}

Lexer::Lexer(FileID fid,
             const llvm::MemoryBuffer* input_file,
             Preprocessor& pp)
    : m_preproc(&pp)
    , m_fid(fid)
    , m_file_loc(pp.getSourceManager().getLocForStartOfFile(fid))
{
    InitLexer(input_file->getBufferStart(), input_file->getBufferStart(),
              input_file->getBufferEnd());
}

Lexer::Lexer(SourceLocation file_loc,
             const char *buf_start,
             const char *buf_ptr,
             const char *buf_end)
    : m_file_loc(file_loc)
{
    InitLexer(buf_start, buf_ptr, buf_end);

    // We *are* in raw mode.
    m_lexing_raw_mode = true;
}

Lexer::Lexer(FileID fid,
             const llvm::MemoryBuffer* from_file,
             const SourceManager& sm)
  : m_file_loc(sm.getLocForStartOfFile(fid))
{
    InitLexer(from_file->getBufferStart(), from_file->getBufferStart(), 
              from_file->getBufferEnd());
  
    // We *are* in raw mode.
    m_lexing_raw_mode = true;
}

Lexer::~Lexer()
{
}

#if 0
unsigned int
Lexer::MeasureTokenLength(SourceLocation loc,
                          const SourceManager& sm)
{
    // TODO: this could be special cased for common tokens like identifiers,
    // ')', etc to make this faster, if it mattered.  Just look at StrData[0]
    // to handle all obviously single-char tokens.  This could use 
    // Lexer::isSimpleCharacter for example to handle identifiers or
    // something.

    // If this comes from a macro expansion, we really do want the macro name,
    // not the token this macro expanded to.
    loc = sm.getInstantiationLoc(loc);
    std::pair<FileID, unsigned> loc_info = sm.getDecomposedLoc(loc);
    std::pair<const char *,const char *> buffer =
        sm.getBufferData(loc_info.first);
    const char* str_data = buffer.first+loc_info.second;

    // Create a lexer starting at the beginning of this token.
    std::auto_ptr<Lexer> lexer =
        MakeRawLexer(loc, buffer.first, str_data, buffer.second);
    Token tok;
    lexer->LexFromRawLexer(&tok);
    return tok.getLength();
}
#endif

SourceLocation
Lexer::getSourceLocation(const char* loc, unsigned int tok_len) const
{
    assert(loc >= m_buf_start && loc <= m_buf_end &&
           "Location out of range for this buffer!");

    // In the normal case, we're just lexing from a simple file buffer, return
    // the file id from FileLoc with the offset specified.
    unsigned char_no = loc-m_buf_start;
    assert(m_file_loc.isFileID() && "Must be lexing from a file");
    return m_file_loc.getFileLocWithOffset(char_no);
}

DiagnosticBuilder
Lexer::Diag(const char* loc, unsigned diag_id) const
{
    return m_preproc->Diag(getSourceLocation(loc), diag_id);
}

unsigned int
Lexer::getEscapedNewLineSize(const char* ptr)
{
    unsigned int size = 0;
    while (isWhitespace(ptr[size]))
    {
        ++size;
    
        if (ptr[size-1] != '\n' && ptr[size-1] != '\r')
            continue;

        // If this is a \r\n or \n\r, skip the other half.
        if ((ptr[size] == '\r' || ptr[size] == '\n') &&
            ptr[size-1] != ptr[size])
            ++size;
      
        return size;
    }

    // Not an escaped newline, must be a \t or something else.
    return 0;
}

const char*
Lexer::SkipEscapedNewLines(const char* p)
{
    for (;;)
    {
        if (*p != '\\')
            return p;
        const char* after_escape = p+1;
    
        unsigned new_line_size = getEscapedNewLineSize(after_escape);
        if (new_line_size == 0)
            return p;
        p = after_escape + new_line_size;
    }
}

/// Peek a single 'character' from the specified buffer, get its size,
/// and return it.  This is tricky if this is an escaped newline (potentially
/// with whitespace between the backslash and newline), in which case we
/// implicitly skip the newline and return the char after it.
///
/// This handles the slow/uncommon case of the getCharAndSize method.
/// Here we know that we can accumulate into size, and that we have already
/// incremented ptr by size bytes.
///
/// NOTE: When this method is updated, getCharAndSizeSlowNoWarn (below)
/// should be updated to match.
char
Lexer::getCharAndSizeSlow(const char* ptr, unsigned int* size, Token* tok)
{
    // If we have a slash, look for an escaped newline.
    if (ptr[0] == '\\')
    {
        ++(*size);
        ++ptr;

        // Common case, backslash-char where the char is not whitespace.
        if (!isWhitespace(ptr[0]))
            return '\\';
    
        // See if we have optional whitespace characters between the slash and
        // newline.
        if (unsigned int escaped_new_line_size = getEscapedNewLineSize(ptr))
        {
            // Remember that this token needs to be cleaned.
            if (tok)
                tok->setFlag(Token::NeedsCleaning);

            // Warn if there was whitespace between the backslash and newline.
            if (ptr[0] != '\n' && ptr[0] != '\r' && tok && !isLexingRawMode())
                Diag(ptr, diag::backslash_newline_space);

            // Found backslash<whitespace><newline>.  Parse the char after it.
            (*size) += escaped_new_line_size;
            ptr     += escaped_new_line_size;
            // Use slow version to accumulate a correct size field.
            return getCharAndSizeSlow(ptr, size, tok);
        }

        // Otherwise, this is not an escaped newline, just return the slash.
        return '\\';
    }
  
    // If this is not an escaped newline, return a single character.
    ++(*size);
    return *ptr;
}


/// Handle the slow/uncommon case of the getCharAndSizeNoWarn method.
/// Here we know that we can accumulate into size, and that we have already
/// incremented ptr by size bytes.
///
/// NOTE: When this method is updated, getCharAndSizeSlow (above) should
/// be updated to match.
char
Lexer::getCharAndSizeSlowNoWarn(const char* ptr, unsigned int* size)
{
    // If we have a slash, look for an escaped newline.
    if (ptr[0] == '\\')
    {
        ++(*size);
        ++ptr;

        // Common case, backslash-char where the char is not whitespace.
        if (!isWhitespace(ptr[0]))
            return '\\';
    
        // See if we have optional whitespace characters followed by a newline.
        if (unsigned int escaped_new_line_size = getEscapedNewLineSize(ptr))
        {
            // Found backslash<whitespace><newline>.  Parse the char after it.
            (*size) += escaped_new_line_size;
            ptr     += escaped_new_line_size;
      
            // Use slow version to accumulate a correct size field.
            return getCharAndSizeSlowNoWarn(ptr, size);
        }
    
        // Otherwise, this is not an escaped newline, just return the slash.
        return '\\';
    }
  
    // If this is neither, return a single character.
    ++(*size);
    return *ptr;
}

/// Efficiently skip over a series of whitespace characters.
/// Update m_buf_ptr to point to the next non-whitespace character and return.
///
/// This method forms a token and returns true if KeepWhitespaceMode is enabled.
///
bool
Lexer::SkipWhitespace(Token* result, const char* cur_ptr)
{
    // Whitespace - Skip it, then return the token after the whitespace.
    unsigned char ch = *cur_ptr;  // Skip consequtive spaces efficiently.
    for (;;)
    {
        // Skip horizontal whitespace very aggressively.
        while (isHorizontalWhitespace(ch))
            ch = *++cur_ptr;
    
        // Otherwise if we have something other than whitespace, we're done.
        if (ch != '\n' && ch != '\r')
            break;
    
        if (m_parsing_preprocessor_directive)
        {
            // End of preprocessor directive line, let LexTokenInternal handle
            // this.
            m_buf_ptr = cur_ptr;
            return false;
        }
    
        // ok, but handle newline.
        // The returned token is at the start of the line.
        result->setFlag(Token::StartOfLine);
        // No leading whitespace seen so far.
        result->clearFlag(Token::LeadingSpace);
        ch = *++cur_ptr;
    }

    // If this isn't immediately after a newline, there is leading space.
    char prevch = cur_ptr[-1];
    if (prevch != '\n' && prevch != '\r')
        result->setFlag(Token::LeadingSpace);

#if 0
    // If the client wants us to return whitespace, return it now.
    if (isKeepWhitespaceMode())
    {
        FormTokenWithChars(result, cur_ptr, Token::unknown);
        return true;
    }
#endif
  
    m_buf_ptr = cur_ptr;
    return false;
}

/// We have just read the ; character from input.  Skip until
/// we find the newline character that terminates the comment.  Then update
/// m_buf_ptr and return.  If we're in KeepCommentMode, this will form the
/// token and return true.
bool
Lexer::SkipLineComment(Token* result, const char* cur_ptr)
{
    // Scan over the body of the comment.  The common case, when scanning, is
    // that the comment contains normal ascii characters with nothing
    // interesting in them.  As such, optimize for this case with the inner
    // loop.
    char ch;
    do {
        ch = *cur_ptr;
        // FIXME: Speedup BCPL comment lexing.  Just scan for a \n or \r
        // character.  If we find a \n character, scan backwards, checking
        // to see if it's an escaped newline, like we do for block comments.

        // Skip over characters in the fast loop.
        while (ch != 0 &&                   // Potentially EOF.
               ch != '\\' &&                // Potentially escaped newline.
               ch != '\n' && ch != '\r')    // Newline or DOS-style newline.
            ch = *++cur_ptr;

        // If this is a newline, we're done.
        if (ch == '\n' || ch == '\r')
            break;  // Found the newline? Break out!
    
        // Otherwise, this is a hard case.  Fall back on getAndAdvanceChar to
        // properly decode the character.  Read it in raw mode to avoid
        // emitting diagnostics about things like trigraphs.  If we see an
        // escaped newline, we'll handle it below.
        const char* old_ptr = cur_ptr;
        bool old_raw_mode = isLexingRawMode();
        m_lexing_raw_mode = true;
        ch = getAndAdvanceChar(cur_ptr, result);
        m_lexing_raw_mode = old_raw_mode;

        // If the char that we finally got was a \n, then we must have had
        // something like \<newline><newline>.  We don't want to have consumed
        // the second newline, we want cur_ptr, to end up pointing to it down
        // below.
        if (ch == '\n' || ch == '\r')
        {
            --cur_ptr;
            ch = 'x'; // doesn't matter what this is.
        }
    
        // If we read multiple characters, and one of those characters was a \r
        // or \n, then we had an escaped newline within the comment.  Emit
        // diagnostic unless the next line is also a EOL comment.
        if (cur_ptr != old_ptr+1 && ch != '/' && cur_ptr[0] != '/')
        {
            for (; old_ptr != cur_ptr; ++old_ptr)
            {
                if (old_ptr[0] == '\n' || old_ptr[0] == '\r')
                {
                    // Okay, we found a EOL comment that ends in a newline, if
                    // the next line is also a EOL comment, but has spaces,
                    // don't emit a diagnostic.
                    if (isspace(ch))
                    {
                        const char* forward_ptr = cur_ptr;
                        while (isspace(*forward_ptr))   // Skip whitespace.
                            ++forward_ptr;
                        if (forward_ptr[0] == '/' && forward_ptr[1] == '/')
                            break;
                    }

                    if (!isLexingRawMode())
                        Diag(old_ptr-1, diag::warn_multi_line_eol_comment);
                    break;
                }
            }
        }
    
        if (cur_ptr == m_buf_end+1)
        {
            --cur_ptr;
            break;
        }
    } while (ch != '\n' && ch != '\r');

    // Found but did not consume the newline.

    // Return immediately, so that the lexer can return this as an EOL token.
    m_buf_ptr = cur_ptr;
    return false;
}

std::string
Lexer::ReadToEndOfLine()
{
    assert(m_parsing_preprocessor_directive &&
           "Must be in a preprocessing directive!");
    std::string result;
    Token tmp;

    // Cache m_buf_ptr in an automatic variable.
    const char* cur_ptr = m_buf_ptr;
    for (;;)
    {
        char ch = getAndAdvanceChar(cur_ptr, &tmp);
        switch (ch)
        {
            default:
                result += ch;
                break;
            case 0:  // Null.
                // Found end of file?
                if (cur_ptr-1 != m_buf_end)
                {
                    // Nope, normal character, continue.
                    result += ch;
                    break;
                }
                // FALL THROUGH.
            case '\r':
            case '\n':
                // Okay, we found the end of the line.
                // First, back up past the \0, \r, \n.
                assert(cur_ptr[-1] == ch && "Couldn't find newline?");
                m_buf_ptr = cur_ptr-1;
      
                // Next, lex the character, which should handle the EOL
                // transition.
                Lex(&tmp);
                assert(tmp.is(Token::eol) && "Unexpected token!");
      
                // Finally, we're done, return the string we found.
                return result;
        }
    }
}

/// cur_ptr points to the end of this file.  Handle this
/// condition, reporting diagnostics and handling other edge cases as required.
/// This returns true if result contains a token, false if PP.Lex should be
/// called again.
bool
Lexer::LexEndOfFile(Token* result, const char* cur_ptr)
{
    // If we hit the end of the file while parsing a preprocessor directive,
    // end the preprocessor directive first.  The next token returned will
    // then be the end of file.
    if (m_parsing_preprocessor_directive)
    {
        // Done parsing the "line".
        m_parsing_preprocessor_directive = false;
        // Update the location of token as well as m_buf_ptr.
        FormTokenWithChars(result, cur_ptr, Token::eol);
        return true;  // Have a token.
    }

    // If we are in raw mode, return this event as an EOF token.  Let the
    // caller that put us in raw mode handle the event.
    if (isLexingRawMode())
    {
        result->StartToken();
        result->setFlag(Token::EndOfStatement);
        m_buf_ptr = m_buf_end;
        FormTokenWithChars(result, m_buf_end, Token::eof);
        return true;
    }

    // Otherwise, issue diagnostics for unterminated %if and missing newline.

#if 0
    // If we are in a %if directive, emit an error.
    while (!m_conditional_stack.empty())
    {
        m_preproc->Diag(m_conditional_stack.back().IfLoc,
                        diag::err_pp_unterminated_conditional);
        m_conditional_stack.pop_back();
    }
#endif

    m_buf_ptr = cur_ptr;

    // Finally, let the preprocessor handle this.
    return m_preproc->HandleEndOfFile(result);
}

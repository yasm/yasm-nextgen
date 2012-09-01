//
// GAS-compatible lexer
//
//  Copyright (C) 2001-2010  Peter Johnson
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
#define DEBUG_TYPE "GasLexer"

#include "GasLexer.h"

#include <cctype>

#include "llvm/ADT/Statistic.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Parse/Preprocessor.h"


STATISTIC(num_identifier, "Number of identifiers lexed");
STATISTIC(num_numeric_constant, "Number of numeric constants lexed");
STATISTIC(num_char_constant, "Number of char constants lexed");
STATISTIC(num_string_literal, "Number of string literals lexed");
STATISTIC(num_eol_comment, "Number of EOL comments lexed");

using namespace yasm;
using namespace yasm::parser;

GasLexer::GasLexer(FileID fid,
                   const MemoryBuffer* input_buffer,
                   Preprocessor& pp)
    : Lexer(fid, input_buffer, pp)
{
    InitCharacterInfo();
}

GasLexer::GasLexer(SourceLocation file_loc,
                   const char* start,
                   const char* ptr,
                   const char* end)
    : Lexer(file_loc, start, ptr, end)
{
    InitCharacterInfo();
}

GasLexer::~GasLexer()
{
}

void
GasLexer::InitCharacterInfo()
{
    // Initialize the character info table.
    s_char_info[(int)' '] = s_char_info[(int)'\t'] = 
    s_char_info[(int)'\f'] = s_char_info[(int)'\v'] = CHAR_HORZ_WS;
    s_char_info[(int)'\n'] = s_char_info[(int)'\r'] = CHAR_VERT_WS;
  
    s_char_info[(int)'_'] = CHAR_UNDER;
    s_char_info[(int)'.'] = CHAR_PERIOD;
    s_char_info[(int)'$'] = CHAR_ID_OTHER;
    //s_char_info[(int)'@'] = CHAR_ID_OTHER;
    for (unsigned i = 'a'; i <= 'z'; ++i)
        s_char_info[i] = s_char_info[i+'A'-'a'] = CHAR_LETTER;
    for (unsigned i = '0'; i <= '9'; ++i)
        s_char_info[i] = CHAR_NUMBER;
}

void
GasLexer::LexIdentifier(Token* result, const char* cur_ptr, bool is_label)
{
    // Match [_$#@~.?A-Za-z0-9]*, we have already matched [_?@A-Za-z]
    unsigned int size;
    unsigned char ch = *cur_ptr++;
    while (isIdentifierBody(ch))
        ch = *cur_ptr++;
    --cur_ptr;  // Back up over the skipped character.

    // Fast path, no \ in identifier found.  '\' might be an escaped newline.
    if (ch != '\\')
    {
FinishIdentifier:
        const char* id_start = m_buf_ptr;
        if (is_label)
            FormTokenWithChars(result, cur_ptr, GasToken::label);
        else
            FormTokenWithChars(result, cur_ptr, GasToken::identifier);

        // If we are in raw mode, return this identifier raw.  There is no need
        // to look up identifier information or attempt to macro expand it.
        if (isLexingRawMode())
            return;

        // Fill in result.IdentifierInfo, looking up the identifier in the
        // identifier table.
        IdentifierInfo* ii = m_preproc->LookUpIdentifierInfo(result, id_start);
    
        // Change the kind of this identifier to the appropriate token kind,
        // e.g. turning "for" into a keyword.
        unsigned int newtokkind = ii->getTokenKind();
        if (newtokkind != Token::unknown)
            result->setKind(newtokkind);
#if 0    
        // Finally, now that we know we have an identifier, pass this off to
        // the preprocessor, which may macro expand it or something.
        if (ii->isHandleIdentifierCase())
            m_preproc->HandleIdentifier(result);
#endif
        ++num_identifier;
        return;
    }
  
    // Otherwise, \ in identifier found.  Enter slower path.
    ch = getCharAndSize(cur_ptr, &size);
    for (;;)
    {
        if (!isIdentifierBody(ch))
            goto FinishIdentifier;  // Found end of identifier.

        // Otherwise, this character is good, consume it.
        cur_ptr = ConsumeChar(cur_ptr, size, result);

        ch = getCharAndSize(cur_ptr, &size);
        while (isIdentifierBody(ch))
        {
            cur_ptr = ConsumeChar(cur_ptr, size, result);
            ch = getCharAndSize(cur_ptr, &size);
        }
    }
}

/// Lex the remainder of a integer or floating point constant.
/// From[-1] is the first character lexed.  Return the end of the constant.
void
GasLexer::LexNumericConstant(Token* result, const char* cur_ptr)
{
    unsigned int size;
    char ch = getCharAndSize(cur_ptr, &size);
    char prevch = 0;
    while (isNumberBody(ch))
    {
        cur_ptr = ConsumeChar(cur_ptr, size, result);
        prevch = ch;
        ch = getCharAndSize(cur_ptr, &size);
    }

    // If we fell out, check for a sign, due to 1e+12 and 0f-123.
    // If we have one, continue.
    if ((ch == '-' || ch == '+') &&
        (prevch == 'D' || prevch == 'd' || prevch == 'E' || prevch == 'e' ||
         prevch == 'F' || prevch == 'f' || prevch == 'T' || prevch == 't'))
        return LexNumericConstant(result, ConsumeChar(cur_ptr, size, result));

    // Update the location of token as well as BufferPtr.
    const char* tok_start = m_buf_ptr;
    FormTokenWithChars(result, cur_ptr, GasToken::numeric_constant);
    result->setFlag(Token::Literal);
    result->setLiteralData(tok_start);
    ++num_numeric_constant;
}

/// Lex the remainder of a character constant literal, after having lexed '.
void
GasLexer::LexCharConstant(Token* result, const char* cur_ptr)
{
    char ch = getAndAdvanceChar(cur_ptr, result);
    if (ch == '\\')
    {
        // handle escape sequence
        ch = getAndAdvanceChar(cur_ptr, result);

        if (isdigit(ch))
        {
            // \ digit digit digit -- octal escape
            ch = getAndAdvanceChar(cur_ptr, result);
            if (isdigit(ch))
                ch = getAndAdvanceChar(cur_ptr, result);
        }
        else if (ch == 'x' || ch == 'X')
        {
            // \x hex-digits... -- hex character code
            do {
                ch = getAndAdvanceChar(cur_ptr, result);
            } while (isxdigit(ch));
            --cur_ptr; // above loop goes one too far; back up
        }
    }

    // An optional trailing ' is allowed.
    if (*cur_ptr == '\'')
        ch = getAndAdvanceChar(cur_ptr, result);

    // Update the location of the token as well as m_buf_ptr.
    const char* tok_start = m_buf_ptr;
    FormTokenWithChars(result, cur_ptr, GasToken::char_constant);
    result->setFlag(Token::Literal);
    result->setLiteralData(tok_start);
    ++num_char_constant;
}

/// Lex the remainder of a string literal, after having lexed ".
void
GasLexer::LexStringLiteral(Token* result, const char* cur_ptr)
{
    const char* nulch = 0; // Does this string contain the \0 character?
  
    char ch = getAndAdvanceChar(cur_ptr, result);
    while (ch != '"')
    {
        // Skip escaped characters
        if (ch == '\\')
        {
            // Skip the escaped character.
            ch = getAndAdvanceChar(cur_ptr, result);
        }
        else if (ch == '\n' || ch == '\r')              // Newline.
        {
            if (!isLexingRawMode())
                Diag(cur_ptr-1, diag::warn_unterminated_string);
        }
        else if (ch == 0 && cur_ptr-1 == m_buf_end)     // End of file.
        {
            if (!isLexingRawMode())
                Diag(m_buf_ptr, diag::err_unterminated_string) << "\"";
            FormTokenWithChars(result, cur_ptr-1, Token::unknown);
            return;
        }
        else if (ch == 0)
        {
            nulch = cur_ptr-1;
        }
        ch = getAndAdvanceChar(cur_ptr, result);
    }

    // If a nul character existed in the string, warn about it.
    if (nulch && !isLexingRawMode())
        Diag(nulch, diag::null_in_string);

    // Update the location of the token as well as m_buf_ptr.
    const char* tok_start = m_buf_ptr;
    FormTokenWithChars(result, cur_ptr, GasToken::string_literal);
    result->setFlag(Token::Literal);
    result->setLiteralData(tok_start);
    ++num_string_literal;
}

/// isBlockCommentEndOfEscapedNewLine - Return true if the specified newline
/// character (either \n or \r) is part of an escaped newline sequence.  Issue a
/// diagnostic if so.  We know that the newline is inside of a block comment.
bool
GasLexer::isEndOfBlockCommentWithEscapedNewLine(const char* cur_ptr)
{
    assert(cur_ptr[0] == '\n' || cur_ptr[0] == '\r');

    // Back up off the newline.
    --cur_ptr;

    // If this is a two-character newline sequence, skip the other character.
    if (cur_ptr[0] == '\n' || cur_ptr[0] == '\r')
    {
        // \n\n or \r\r -> not escaped newline.
        if (cur_ptr[0] == cur_ptr[1])
            return false;
        // \n\r or \r\n -> skip the newline.
        --cur_ptr;
    }

    // If we have horizontal whitespace, skip over it.  We allow whitespace
    // between the slash and newline.
    bool HasSpace = false;
    while (isHorizontalWhitespace(*cur_ptr) || *cur_ptr == 0)
    {
        --cur_ptr;
        HasSpace = true;
    }

    // If we have a slash, we know this is an escaped newline.
    if (*cur_ptr == '\\')
    {
        if (cur_ptr[-1] != '*')
            return false;
    }
    else
        return false;

    // Warn about having an escaped newline between the */ characters.
    if (!isLexingRawMode())
        Diag(cur_ptr, diag::escaped_newline_block_comment_end);

    // If there was space between the backslash and newline, warn about it.
    if (HasSpace && !isLexingRawMode())
        Diag(cur_ptr, diag::backslash_newline_space);

    return true;
}

/// SkipBlockComment - We have just read the /* characters from input.  Read
/// until we find the */ characters that terminate the comment.  Note that we
/// don't bother decoding escaped newlines in block comments,
/// because they cannot cause the comment to end.  The only thing that can
/// happen is the comment could end with an escaped newline between the */ end
/// of comment.
///
/// If we're in KeepCommentMode or any CommentHandler has inserted
/// some tokens, this will store the first token and return true.
bool
GasLexer::SkipBlockComment(Token* result, const char* cur_ptr)
{
    // Scan one character past where we should, looking for a '/' character.
    // Once we find it, check to see if it was preceeded by a *.  This common
    // optimization helps people who like to put a lot of * characters in their
    // comments.

    // The first character we get with newlines skipped to handle
    // the degenerate /*/ case below correctly if the * has an escaped newline
    // after it.
    unsigned size;
    unsigned char ch = getCharAndSize(cur_ptr, &size);
    cur_ptr += size;
    if (ch == 0 && cur_ptr == m_buf_end+1)
    {
        if (!isLexingRawMode())
            Diag(m_buf_ptr, diag::err_unterminated_block_comment);
        --cur_ptr;
#if 0
        // KeepWhitespaceMode should return this broken comment as a token.  Since
        // it isn't a well formed comment, just return it as an 'unknown' token.
        if (isKeepWhitespaceMode())
        {
            FormTokenWithChars(result, cur_ptr, Token::unknown);
            return true;
        }
#endif
        m_buf_ptr = cur_ptr;
        return false;
    }

    // Check to see if the first character after the '/*' is another /.  If so,
    // then this slash does not end the block comment, it is part of it.
    if (ch == '/')
        ch = *cur_ptr++;

    while (1)
    {
        // Skip over all non-interesting characters until we find end of buffer or a
        // (probably ending) '/' character.
        if (cur_ptr + 24 < m_buf_end)
        {
            // While not aligned to a 16-byte boundary.
            while (ch != '/' && ((intptr_t)cur_ptr & 0x0F) != 0)
                ch = *cur_ptr++;

            if (ch == '/')
                goto FoundSlash;

            // Scan for '/' quickly.  Many block comments are very large.
            while (cur_ptr[0] != '/' &&
                   cur_ptr[1] != '/' &&
                   cur_ptr[2] != '/' &&
                   cur_ptr[3] != '/' &&
                   cur_ptr+4 < m_buf_end)
            {
                cur_ptr += 4;
            }

            // It has to be one of the bytes scanned, increment to it and read one.
            ch = *cur_ptr++;
        }

        // Loop to scan the remainder.
        while (ch != '/' && ch != '\0')
            ch = *cur_ptr++;

    FoundSlash:
        if (ch == '/')
        {
            if (cur_ptr[-2] == '*')  // We found the final */.  We're done!
                break;

            if ((cur_ptr[-2] == '\n' || cur_ptr[-2] == '\r'))
            {
                if (isEndOfBlockCommentWithEscapedNewLine(cur_ptr-2))
                {
                    // We found the final */, though it had an escaped newline
                    // between the * and /.  We're done!
                    break;
                }
            }
            if (cur_ptr[0] == '*' && cur_ptr[1] != '/')
            {
                // If this is a /* inside of the comment, emit a warning.
                // Don't do this if this is a /*/, which will end the comment.
                // This misses cases with embedded escaped newlines, but oh well.
                if (!isLexingRawMode())
                    Diag(cur_ptr-1, diag::warn_nested_block_comment);
            }
        }
        else if (ch == 0 && cur_ptr == m_buf_end+1)
        {
            if (!isLexingRawMode())
                Diag(m_buf_ptr, diag::err_unterminated_block_comment);
            // Note: the user probably forgot a */.  We could continue
            // immediately after the /*, but this would involve lexing a lot of
            // what really is the comment, which surely would confuse the parser.
            --cur_ptr;
#if 0
            // KeepWhitespaceMode should return this broken comment as a token.
            // Since it isn't a well formed comment, just return it as an
            // 'unknown' token.
            if (isKeepWhitespaceMode())
            {
                FormTokenWithChars(result, cur_ptr, Token::unknown);
                return true;
            }
#endif
            m_buf_ptr = cur_ptr;
            return false;
        }
        ch = *cur_ptr++;
    }

#if 0
    // Notify comment handlers about the comment unless we're in a #if 0 block.
    if (PP && !isLexingRawMode() &&
        PP->HandleComment(Result, SourceRange(getSourceLocation(m_buf_ptr),
                                              getSourceLocation(cur_ptr))))
    {
        m_buf_ptr = cur_ptr;
        return true; // A token has to be returned.
    }

    // If we are returning comments as tokens, return this comment as a token.
    if (inKeepCommentMode())
    {
        FormTokenWithChars(result, cur_ptr, Token::comment);
        return true;
    }
#endif

    m_buf_ptr = cur_ptr;
    result->setFlag(Token::LeadingSpace);
    return false;
}

/// This implements the main GAS lexer.  It is an extremely performance
/// critical piece of code.  This assumes that the buffer has a null
/// character at the end of the file.  Return true if an error
/// occurred and compilation should terminate, false if normal.  This
/// returns a preprocessing token, not a normal token, as such, it is an
/// internal interface.  It assumes that the flags of result have been
/// cleared before calling this.
void
GasLexer::LexTokenInternal(Token* result)
{
LexNextToken:
    // New token, can't need cleaning yet.
    result->clearFlag(Token::NeedsCleaning);
    result->setIdentifierInfo(0);
  
    // Cache m_buf_ptr in an automatic variable.
    const char* cur_ptr = m_buf_ptr;

    // Small amounts of horizontal whitespace is very common between tokens.
    if ((*cur_ptr == ' ') || (*cur_ptr == '\t'))
    {
        ++cur_ptr;
        while ((*cur_ptr == ' ') || (*cur_ptr == '\t'))
            ++cur_ptr;
    
#if 0
        // If we are keeping whitespace and other tokens, just return what we
        // just skipped.  The next lexer invocation will return the token after
        // the whitespace.
        if (is_keep_whitespace_mode())
        {
            form_token_with_chars(result, cur_ptr, Token::unknown);
            return;
        }
#endif

        m_buf_ptr = cur_ptr;
        result->setFlag(Token::LeadingSpace);
    }

    unsigned int size_tmp;      // Temporary for use in cases below.
  
    // Read a character, advancing over it.
    char ch = getAndAdvanceChar(cur_ptr, result);
    unsigned int kind;

    switch (ch)
    {
    case 0:  // Null.
        // Found end of file?
        if (cur_ptr-1 == m_buf_end)
        {
            // Read the PP instance variable into an automatic variable, because
            // LexEndOfFile will often delete 'this'.
            Preprocessor* PPCache = m_preproc;
            if (LexEndOfFile(result, cur_ptr-1))// Retreat back into the file
                return;   // Got a token to return.
            assert(PPCache && "Raw buffer::LexEndOfFile should return a token");
            return PPCache->Lex(result);
        }

        if (!isLexingRawMode())
            Diag(cur_ptr-1, diag::null_in_file);

        result->setFlag(Token::LeadingSpace);
        if (SkipWhitespace(result, cur_ptr))
            return; // KeepWhitespaceMode

        goto LexNextToken;   // GCC isn't tail call eliminating.
    case '\n':
    case '\r':
        // If we are inside a preprocessor directive and we see the end of line,
        // we know we are done with the directive, so return an EOM token.
        if (m_parsing_preprocessor_directive)
        {
            // Done parsing the "line".
            m_parsing_preprocessor_directive = false;
        }

        // Since we consumed a newline, we are back at the start of a line.
        m_is_at_start_of_line = true;

        kind = Token::eol;
        result->setFlag(Token::EndOfStatement);
        break;
    case ' ':
    case '\t':
    case '\f':
    case '\v':
SkipHorizontalWhitespace:
        result->setFlag(Token::LeadingSpace);
        if (SkipWhitespace(result, cur_ptr))
            return; // KeepWhitespaceMode

SkipIgnoredUnits:
        cur_ptr = m_buf_ptr;
    
        // If the next token is obviously a comment, skip it efficiently
        // too (without going through the big switch stmt).
        if (cur_ptr[0] == '#')
        {
            SkipLineComment(result, cur_ptr+1);
            goto SkipIgnoredUnits;
        }
        else if (cur_ptr[0] == '/' && cur_ptr[1] == '/')
        {
            SkipLineComment(result, cur_ptr+2);
            goto SkipIgnoredUnits;
        }
        else if (cur_ptr[0] == '/' && cur_ptr[1] == '*')
        {
            if (SkipBlockComment(result, cur_ptr+2))
                return; // There is a token to return.
            goto SkipIgnoredUnits;
        }
        else if (isHorizontalWhitespace(*cur_ptr))
        {
            goto SkipHorizontalWhitespace;
        }
        goto LexNextToken;   // GCC isn't tail call eliminating.

    // Integer and Floating Point Constants
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        return LexNumericConstant(result, cur_ptr);

    // Identifiers
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
    case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
    case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
    case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
    case 'v': case 'w': case 'x': case 'y': case 'z':
        return LexIdentifier(result, cur_ptr, false);
    case '_': case '.':
        return LexIdentifier(result, cur_ptr, true);

    // Character Constants
    case '\'':
        return LexCharConstant(result, cur_ptr);

    // String Literals.
    case '"':
        return LexStringLiteral(result, cur_ptr);

    // Punctuators.
    case '$':
        kind = GasToken::dollar;
        break;
    case '[':
        kind = GasToken::l_square;
        break;
    case ']':
        kind = GasToken::r_square;
        break;
    case '(':
        kind = GasToken::l_paren;
        break;
    case ')':
        kind = GasToken::r_paren;
        break;
    case '&':
        if (getCharAndSize(cur_ptr, &size_tmp) == '&')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = GasToken::ampamp;
        }
        else
            kind = GasToken::amp;
        break;
    case '*': 
        kind = GasToken::star;
        break;
    case '+':
        kind = GasToken::plus;
        break;
    case '-':
        kind = GasToken::minus;
        break;
    case '~':
        kind = GasToken::tilde;
        break;
    case '@':
        kind = GasToken::at;
        break;
    case '!':
        kind = GasToken::exclaim;
        break;
    case '/':
        ch = getCharAndSize(cur_ptr, &size_tmp);
        if (ch == '/')  // EOL comment
        {
            SkipLineComment(result, ConsumeChar(cur_ptr, size_tmp, result));

            // It is common for the tokens immediately after a // comment to be
            // whitespace (indentation for the next line).  Instead of going
            // through the big switch, handle it efficiently now.
            goto SkipIgnoredUnits;
        }
        if (ch == '*') // /**/ comment
        {
            if (SkipBlockComment(result, ConsumeChar(cur_ptr, size_tmp, result)))
                return; // There is a token to return.
            goto LexNextToken;  // GCC isn't tail call eliminating.
        }
        kind = GasToken::slash;
        break;
    case '%':
        kind = GasToken::percent;
        break;
    case '<':
        ch = getCharAndSize(cur_ptr, &size_tmp);
        if (ch == '<')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = GasToken::lessless;
        }
        else if (ch == '=')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = GasToken::lessequal;
        }
        else if (ch == '>')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = GasToken::lessgreater;
        }
        else
            kind = GasToken::less;
        break;
    case '>':
        ch = getCharAndSize(cur_ptr, &size_tmp);
        if (ch == '=')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = GasToken::greaterequal;
        }
        else if (ch == '>')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = GasToken::greatergreater;
        }
        else
            kind = GasToken::greater;
        break;
    case '^':
        kind = GasToken::caret;
        break;
    case '|':
        if (getCharAndSize(cur_ptr, &size_tmp) == '|')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = GasToken::pipepipe;
        }
        else
            kind = GasToken::pipe;
        break;
    case ':':
        kind = GasToken::colon;
        break;
    case ';':
        kind = GasToken::semi;
        result->setFlag(Token::EndOfStatement);
        break;
    case '#':
        // EOL comment
        ++num_eol_comment;
        if (SkipLineComment(result, cur_ptr))
            return; // KeepCommentMode
      
        // It is common for the tokens immediately after a # comment to be
        // whitespace (indentation for the next line).  Instead of going
        // through the big switch, handle it efficiently now.
        goto SkipIgnoredUnits;
    case '=':
        if (getCharAndSize(cur_ptr, &size_tmp) == '=')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = GasToken::equalequal;
        }
        else
            kind = GasToken::equal;
        break;
    case ',':
        kind = GasToken::comma;
        break;
    default:
        kind = GasToken::unknown;
        break;
    }
  
    // Update the location of token as well as m_buf_ptr.
    FormTokenWithChars(result, cur_ptr, kind);
}

//
// NASM-compatible lexer
//
//  Copyright (C) 2001-2009  Peter Johnson
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
#define DEBUG_TYPE "NasmLexer"

#include "NasmLexer.h"

#include <cctype>

#include "llvm/ADT/Statistic.h"
#include "yasmx/Parse/Preprocessor.h"
#include "yasmx/Diagnostic.h"


STATISTIC(num_identifier, "Number of identifiers lexed");
STATISTIC(num_numeric_constant, "Number of numeric constants lexed");
STATISTIC(num_string_literal, "Number of string literals lexed");
STATISTIC(num_eol_comment, "Number of EOL comments lexed");

using namespace yasm;
using namespace parser;

NasmLexer::NasmLexer(clang::FileID fid,
                     const llvm::MemoryBuffer* input_buffer,
                     Preprocessor& pp)
    : Lexer(fid, input_buffer, pp)
{
    InitCharacterInfo();
}

NasmLexer::NasmLexer(clang::SourceLocation file_loc,
                     const char* start,
                     const char* ptr,
                     const char* end)
    : Lexer(file_loc, start, ptr, end)
{
    InitCharacterInfo();
}

NasmLexer::~NasmLexer()
{
}

void
NasmLexer::InitCharacterInfo()
{
    // Initialize the character info table.
    s_char_info[(int)' '] = s_char_info[(int)'\t'] = 
    s_char_info[(int)'\f'] = s_char_info[(int)'\v'] = CHAR_HORZ_WS;
    s_char_info[(int)'\n'] = s_char_info[(int)'\r'] = CHAR_VERT_WS;
  
    s_char_info[(int)'_'] = CHAR_UNDER;
    s_char_info[(int)'.'] = CHAR_PERIOD;
    s_char_info[(int)'$'] = CHAR_ID_OTHER;
    s_char_info[(int)'#'] = CHAR_ID_OTHER;
    s_char_info[(int)'@'] = CHAR_ID_OTHER;
    s_char_info[(int)'~'] = CHAR_ID_OTHER;
    s_char_info[(int)'?'] = CHAR_ID_OTHER;
    for (unsigned i = 'a'; i <= 'z'; ++i)
        s_char_info[i] = s_char_info[i+'A'-'a'] = CHAR_LETTER;
    for (unsigned i = '0'; i <= '9'; ++i)
        s_char_info[i] = CHAR_NUMBER;
}

void
NasmLexer::LexIdentifier(Token* result, const char* cur_ptr, bool is_label)
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
            FormTokenWithChars(result, cur_ptr, NasmToken::label);
        else
            FormTokenWithChars(result, cur_ptr, NasmToken::identifier);

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
NasmLexer::LexNumericConstant(Token* result, const char* cur_ptr)
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
  
    // If we fell out, check for a sign, due to 1e+12.
    // If we have one, continue.  Also allow hex FP exponents.
    if ((ch == '-' || ch == '+') &&
        (prevch == 'E' || prevch == 'e' || prevch == 'P' || prevch == 'p'))
        return LexNumericConstant(result, ConsumeChar(cur_ptr, size, result));

    // Update the location of token as well as BufferPtr.
    const char* tok_start = m_buf_ptr;
    FormTokenWithChars(result, cur_ptr, NasmToken::numeric_constant);
    result->setFlag(Token::Literal);
    result->setLiteralData(tok_start);
    ++num_numeric_constant;
}

/// Lex the remainder of a string literal, after having lexed ".
void
NasmLexer::LexStringLiteral(Token* result, const char* cur_ptr, char endch)
{
    const char* nulch = 0; // Does this string contain the \0 character?
  
    char ch = getAndAdvanceChar(cur_ptr, result);
    while (ch != endch)
    {
        if (ch == '\n' || ch == '\r' ||             // Newline.
            (ch == 0 && cur_ptr-1 == m_buf_end))    // End of file.
        {
            if (!isLexingRawMode())
                Diag(m_buf_ptr, diag::err_unterminated_string);
            FormTokenWithChars(result, cur_ptr-1, Token::unknown);
            return;
        }
        else if (ch == 0)
        {
            nulch = cur_ptr-1;
        }
        char prevch = ch;
        ch = getAndAdvanceChar(cur_ptr, result);
        // skip over escaped endch in escaped strings
        if (endch == '`' && ch == '`' && prevch == '\\')
            ch = getAndAdvanceChar(cur_ptr, result);
    }

    // If a nul character existed in the string, warn about it.
    if (nulch && !isLexingRawMode())
        Diag(nulch, diag::null_in_string);

    // Update the location of the token as well as m_buf_ptr.
    const char* tok_start = m_buf_ptr;
    FormTokenWithChars(result, cur_ptr, NasmToken::string_literal);
    result->setFlag(Token::Literal);
    result->setLiteralData(tok_start);
    ++num_string_literal;
}

/// This implements the main NASM lexer.  It is an extremely performance
/// critical piece of code.  This assumes that the buffer has a null
/// character at the end of the file.  Return true if an error
/// occurred and compilation should terminate, false if normal.  This
/// returns a preprocessing token, not a normal token, as such, it is an
/// internal interface.  It assumes that the flags of result have been
/// cleared before calling this.
void
NasmLexer::LexTokenInternal(Token* result)
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
    
        // If the next token is obviously a ; comment, skip it efficiently
        // too (without going through the big switch stmt).
        if (cur_ptr[0] == ';')
        {
            SkipLineComment(result, cur_ptr+1);
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
    case '_': case '?': case '@': case '.':
        return LexIdentifier(result, cur_ptr, true);

    case '$':
        ch = getCharAndSize(cur_ptr, &size_tmp);
        if (ch >= '0' && ch <= '9')
        {
            // If it starts with a digit it's a hexidecimal number
            return LexNumericConstant(result, ConsumeChar(cur_ptr, size_tmp,
                                                          result));
        }
        else if (ch == '$')
        {
            // $$ (start of current section)
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = NasmToken::dollardollar;
        }
        else if (isIdentifierBody(ch))
        {
            // Forced identifier
            return LexIdentifier(result, cur_ptr, true);
        }
        else
            kind = NasmToken::dollar;   // just a plain $ (current position)
        break;

    // String Literals.
    case '\'': case '"': case '`':
        return LexStringLiteral(result, cur_ptr, ch);

    // Punctuators.
    case '[':
        kind = NasmToken::l_square;
        break;
    case ']':
        kind = NasmToken::r_square;
        break;
    case '(':
        kind = NasmToken::l_paren;
        break;
    case ')':
        kind = NasmToken::r_paren;
        break;
    case '&':
        if (getCharAndSize(cur_ptr, &size_tmp) == '&')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = NasmToken::ampamp;
        }
        else
            kind = NasmToken::amp;
        break;
    case '*': 
        kind = NasmToken::star;
        break;
    case '+':
        kind = NasmToken::plus;
        break;
    case '-':
        kind = NasmToken::minus;
        break;
    case '~':
        kind = NasmToken::tilde;
        break;
    case '!':
        if (getCharAndSize(cur_ptr, &size_tmp) == '=')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = NasmToken::exclaimequal;
        }
        else
            kind = NasmToken::exclaim;
        break;
    case '/':
        if (getCharAndSize(cur_ptr, &size_tmp) == '/')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = NasmToken::slashslash;
        }
        else
            kind = NasmToken::slash;
        break;
    case '%':
        ch = getCharAndSize(cur_ptr, &size_tmp);
        if (ch == '%')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = NasmToken::percentpercent;
        }
        else
        {
#if 0
            // We parsed a % character.  If this occurs at the start of the
            // line, it's actually the start of a preprocessing directive.
            // Callback to the preprocessor to handle it.
            if (result->isAtStartOfLine() && !isLexingRawMode())
            {
                FormTokenWithChars(result, cur_ptr, NasmToken::hash);
                m_preproc->HandleDirective(result);

                // As an optimization, if the preprocessor didn't switch
                // lexers, tail recurse.
                if (m_preproc->is_current_lexer(this))
                {
                    // Start a new token.  If this is a %include or something,
                    // the PP may want us starting at the beginning of the
                    // line again.  If so, set the start of line flag.
                    if (m_is_at_start_of_line)
                    {
                        result->setFlag(Token::StartOfLine);
                        m_is_at_start_of_line = false;
                    }
                    goto LexNextToken;   // GCC isn't tail call eliminating.
                }
                return m_preproc->Lex(result);
            }
#endif
            kind = NasmToken::percent;
        }
        break;
    case '<':
        ch = getCharAndSize(cur_ptr, &size_tmp);
        if (ch == '<')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = NasmToken::lessless;
        }
        else if (ch == '>')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = NasmToken::lessgreater;
        }
        else if (ch == '=')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = NasmToken::lessequal;
        }
        else
            kind = NasmToken::less;
        break;
    case '>':
        ch = getCharAndSize(cur_ptr, &size_tmp);
        if (ch == '=')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = NasmToken::greaterequal;
        }
        else if (ch == '>')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = NasmToken::greatergreater;
        }
        else
            kind = NasmToken::greater;
        break;
    case '^':
        if (getCharAndSize(cur_ptr, &size_tmp) == '^')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = NasmToken::caretcaret;
        }
        else
            kind = NasmToken::caret;
        break;
    case '|':
        if (getCharAndSize(cur_ptr, &size_tmp) == '|')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = NasmToken::pipepipe;
        }
        else
            kind = NasmToken::pipe;
        break;
    case ':':
        kind = NasmToken::colon;
        break;
    case ';':
        // EOL comment
        ++num_eol_comment;
        if (SkipLineComment(result, cur_ptr))
            return; // KeepCommentMode
      
        // It is common for the tokens immediately after a ; comment to be
        // whitespace (indentation for the next line).  Instead of going
        // through the big switch, handle it efficiently now.
        goto SkipIgnoredUnits;
    case '=':
        if (getCharAndSize(cur_ptr, &size_tmp) == '=')
        {
            cur_ptr = ConsumeChar(cur_ptr, size_tmp, result);
            kind = NasmToken::equalequal;
        }
        else
            kind = NasmToken::equal;
        break;
    case ',':
        kind = NasmToken::comma;
        break;
    default:
        kind = NasmToken::unknown;
        break;
    }
  
    // Update the location of token as well as m_buf_ptr.
    FormTokenWithChars(result, cur_ptr, kind);
}

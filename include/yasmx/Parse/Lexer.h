#ifndef YASM_PARSE_LEXER_H
#define YASM_PARSE_LEXER_H
//
// Lexer header file
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
#include <memory>
#include <string>

#include "yasmx/Config/export.h"
#include "yasmx/Parse/Token.h"


namespace yasm
{

class DiagnosticBuilder;
class Preprocessor;

class YASM_LIB_EXPORT Lexer
{
public:
    /// Create a new lexer object for the specified buffer with the specified
    /// preprocessor managing the lexing process.  This lexer assumes that
    /// the associated file buffer and Preprocessor objects will outlive it,
    /// so it doesn't take ownership of either of them.
    Lexer(FileID fid, const MemoryBuffer* input_buffer, Preprocessor& pp);

    /// Create a new raw lexer object.  This object is only suitable for calls
    /// to 'LexRawToken'.  This lexer assumes that the text range will outlive
    /// it, so it doesn't take ownership of it.
    Lexer(SourceLocation file_loc,
          const char *buf_start,
          const char *buf_ptr,
          const char *buf_end);
  
    /// Create a new raw lexer object.  This object is only suitable for
    /// calls to 'LexRawToken'.  This lexer assumes that the text range will
    /// outlive it, so it doesn't take ownership of it.
    Lexer(FileID fid,
          const MemoryBuffer* input_buffer,
          const SourceManager& sm);

    virtual ~Lexer();

    /// Return the next token in the file.  If this is the end of file, it
    /// return the Token::eof token.  Return true if an error occurred and
    /// compilation should terminate, false if normal.  This implicitly
    /// involves the preprocessor.
    void Lex(Token* result)
    {
        // Start a new token.
        result->StartToken();
    
        // NOTE, any changes here should also change code after calls to 
        // Preprocessor::HandleDirective
        if (m_is_at_start_of_line)
        {
            result->setFlag(Token::StartOfLine);
            m_is_at_start_of_line = false;
        }
   
        // Get a token.  Note that this may delete the current lexer if the end
        // of file is reached.
        LexTokenInternal(result);
    }

    /// Lex a token from a designated raw lexer (one with no associated
    /// preprocessor object.  Return true if the 'next character to
    /// read' pointer points at the end of the lexer buffer, false otherwise.
    bool LexFromRawLexer(Token* result)
    {
        assert(m_lexing_raw_mode && "Not already in raw mode!");
        Lex(result);
        // Note that lexing to the end of the buffer doesn't implicitly delete
        // the lexer when in raw mode.
        return m_buf_ptr == m_buf_end; 
    }

    /// Inform the lexer whether or not we are currently lexing a
    /// preprocessor directive.
    void setParsingPreprocessorDirective(bool f)
    {
        m_parsing_preprocessor_directive = f;
    }
  
    /// Return true if this lexer is in raw mode or not.
    bool isLexingRawMode() const { return m_lexing_raw_mode; }

    FileID getFileID() const
    {
        assert(m_preproc &&
               "Lexer::getFileID() should only be used with a Preprocessor");
        return m_fid;
    }

    /// Read the rest of the current preprocessor line as an uninterpreted
    /// string.  This switches the lexer out of directive mode.
    std::string ReadToEndOfLine();

    /// Forwarding function for diagnostics.  This translate a source
    /// position in the current buffer into a SourceLocation object for
    /// rendering.
    DiagnosticBuilder Diag(const char* loc, unsigned diag_id) const;

    /// Return a source location identifier for the specified
    /// offset in the current file.
    SourceLocation getSourceLocation(const char* loc,
                                     unsigned int tok_len = 1) const;
    
    /// Return a source location for the next character in the current file.
    SourceLocation getSourceLocation() const
    { return getSourceLocation(m_buf_ptr); }

#if 0
    /// Relex the token at the specified location and return its length in
    /// bytes in the input file.  If the token needs cleaning (e.g. includes an
    /// escaped newline) then this count includes bytes that are part of that.
    static unsigned int MeasureTokenLength(SourceLocation loc,
                                           const SourceManager& sm);
#endif

    /// Return true if the specified character is not possibly part of a more
    /// "complex" character (like backslash-newline).  This can return false
    /// for characters that end up being the same, but it will never returnu
    /// true for something that needs to be mapped.
    static bool isSimpleCharacter(char c) { return c != '\\'; }

    /// Read a single 'character' from the specified buffer, advance over it,
    /// and return it.  This is tricky in several cases.  Here we just handle
    /// the trivial case and fall-back to the non-inlined method to handle the
    /// hard case.
    inline char getAndAdvanceChar(const char* &ptr, Token* tok)
    {
        // If this is an escaped newline, return quickly.
        if (isSimpleCharacter(ptr[0])) return *ptr++;
    
        unsigned int size = 0;
        char c = getCharAndSizeSlow(ptr, &size, tok);
        ptr += size;
        return c;
    }

    /// Like the getCharAndSize method, but does not ever emit a warning.
    static inline char getCharAndSizeNoWarn(const char* ptr, unsigned int* size)
    {
        // If this is not an escaped newline, return quickly.
        if (isSimpleCharacter(ptr[0]))
        {
            *size = 1;
            return *ptr;
        }
    
        *size = 0;
        return getCharAndSizeSlowNoWarn(ptr, size);
    }

    /// Return the size of the specified escaped newline, or 0 if it is not an
    /// escaped newline. p[-1] is known to be a "\" on entry to this function.
    static unsigned int getEscapedNewLineSize(const char* p);
  
    /// If P points to an escaped newline (or a series of them), skip over
    /// them and return the first non-escaped-newline found, otherwise return p.
    static const char* SkipEscapedNewLines(const char* p);
  
    /// Same as getCharAndSizeSlow, but never emits a diagnostic.
    static char getCharAndSizeSlowNoWarn(const char* ptr, unsigned int* Size);

protected:
    Preprocessor* m_preproc;    // Preprocessor object controlling lexing.

    /// The SourceManager FileID corresponding to the file being lexed.
    const FileID m_fid;

    SourceLocation m_file_loc;  // Location for start of file.

    const char* m_buf_start;    // Start of the buffer.
    const char* m_buf_end;      // End of the buffer.

    // Current pointer into the buffer.
    // This is the next character to be lexed.
    const char* m_buf_ptr;

    // True if the next lexed token should get the "start of
    // line" flag set on it.
    bool m_is_at_start_of_line;

    /// This is true when parsing %XXX.
    bool m_parsing_preprocessor_directive;

    /// True if in raw mode:  This flag disables interpretation of
    /// tokens and is a far faster mode to lex in than non-raw-mode.  This flag:
    ///  1. If EOF of the current lexer is found, the include stack isn't
    ///     popped.
    ///  2. Identifier information is not looked up for identifier tokens.  As
    ///     an effect of this, implicit macro expansion is naturally disabled.
    ///  3. "%" tokens at the start of a line are treated as normal tokens, not
    ///     implicitly transformed by the lexer.
    ///  4. All diagnostic messages are disabled.
    ///  5. No callbacks are made into the preprocessor.
    ///
    /// Note that in raw mode that the PP pointer may be null.
    bool m_lexing_raw_mode;

    /// Character information.
    /// XXX: Make non-static.
    static unsigned char s_char_info[256];

    /// Character types.
    enum
    {
        CHAR_HORZ_WS  = 0x01,  // e.g. ' ', '\t', '\f', '\v'.  Note, no '\0'
        CHAR_VERT_WS  = 0x02,  // e.g. '\r', '\n'
        CHAR_LETTER   = 0x04,  // a-z,A-Z
        CHAR_NUMBER   = 0x08   // 0-9
    };

    /// Return true if this character is horizontal whitespace:
    /// e.g. ' ', '\t', '\f', '\v'.  Note that this returns false for '\0'.
    static inline bool
    isHorizontalWhitespace(unsigned char c)
    {
        return (s_char_info[c] & CHAR_HORZ_WS) ? true : false;
    }

    /// Return true if this character is horizontal or vertical whitespace:
    /// e.g. ' ', '\t', '\f', '\v', '\n', '\r'.  Note that this returns false
    /// for '\0'.
    static inline bool
    isWhitespace(unsigned char c)
    {
        return (s_char_info[c] & (CHAR_HORZ_WS|CHAR_VERT_WS)) ? true : false;
    }

    /// Internal interface to lex a preprocessing token. Called by Lex().
    virtual void LexTokenInternal(Token* result) = 0;

    /// When we lex a token, we have identified a span starting at m_buf_ptr,
    /// going to tok_end that forms the token.  This method takes that range
    /// and assigns it to the token as its location and size.  In addition,
    /// since tokens cannot overlap, this also updates m_buf_ptr to be tok_end.
    void FormTokenWithChars(Token* result,
                            const char* tok_end, 
                            unsigned int kind)
    {
        unsigned int tok_len = tok_end-m_buf_ptr;
        result->setLength(tok_len);
        result->setLocation(getSourceLocation(m_buf_ptr, tok_len));
        result->setKind(kind);
        m_buf_ptr = tok_end;
    }

    /// When a character is consumed and added to a given token, check to see
    /// if there are diagnostics that need to be emitted or flags that need to
    /// be set on the token.  If so, do it.
    const char* ConsumeChar(const char* ptr, unsigned int size, Token* tok)
    {
        // Normal case, we consumed exactly one token.  Just return it.
        if (size == 1)
            return ptr+size;

        // Otherwise, re-lex the character with a current token, allowing
        // diagnostics to be emitted and flags to be set.
        size = 0;
        getCharAndSizeSlow(ptr, &size, tok);
        return ptr+size;
    }
  
    /// Peek a single 'character' from the specified buffer, get its size,
    /// and return it.  This is tricky in several cases.  Here we
    /// just handle the trivial case and fall-back to the non-inlined
    /// method to handle the hard case.
    inline char getCharAndSize(const char* ptr, unsigned int* size)
    {
        // If this is not an escaped newline, return quickly.
        if (isSimpleCharacter(ptr[0]))
        {
            *size = 1;
            return *ptr;
        }
    
        *size = 0;
        return getCharAndSizeSlow(ptr, size);
    }
  
    /// Handle the slow/uncommon case of the getCharAndSize method.
    char getCharAndSizeSlow(const char* ptr,
                            unsigned int* Size,
                            Token* tok = 0);
  
    // Helper functions to lex the remainder of a token of the specific type.
    bool LexEndOfFile       (Token* result, const char* cur_ptr);

    bool SkipWhitespace     (Token* result, const char* cur_ptr);
    bool SkipLineComment    (Token* result, const char* cur_ptr);

private:
    Lexer(const Lexer&);            // DO NOT IMPLEMENT
    void operator=(const Lexer&);   // DO NOT IMPLEMENT
    friend class Preprocessor;

    void InitLexer(const char* start, const char* ptr, const char* end);
};

} // namespace yasm

#endif

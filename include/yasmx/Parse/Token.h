#ifndef YASM_PARSE_TOKEN_H
#define YASM_PARSE_TOKEN_H
//
// Token interface
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
#include <cassert>

#include "llvm/ADT/StringRef.h"
#include "yasmx/Basic/LLVM.h"
#include "yasmx/Basic/SourceLocation.h"

namespace yasm
{

class IdentifierInfo;

/// This structure provides full information about a lexed token.
/// It is not intended to be space efficient, it is intended to return as much
/// information as possible about each returned token.  This is expected to be
/// compressed into a smaller form if memory footprint is important.
class Token
{
    /// The location of the token.
    SourceLocation m_loc;

    /// PtrData - This is a union of four different pointer types, which depends
    /// on what type of token this is:
    ///  Identifiers, keywords, etc:
    ///    This is an IdentifierInfo*, which contains the uniqued identifier
    ///    spelling.
    ///  Literals:  is_literal() returns true.
    ///    This is a pointer to the start of the token in a text buffer, which
    ///    may be dirty (have escaped newlines).
    ///  Other:
    ///    This is null.
    void* m_ptr_data;

    /// The length of the token text.
    unsigned int m_len;

    /// Kind - The actual flavor of token this is.
    unsigned int m_kind : 16;
  
    /// Flags - Bits we track about this token, members of the TokenFlags enum.
    unsigned int m_flags : 8;

protected:
    void* getPtrData() const { return m_ptr_data; }
    void setPtrData(void* data) { m_ptr_data = data; }

public:
    // Common token kinds
    enum Kind
    {
        unknown = 0,        // Not a token
        eof,                // End of file
        eol,                // End of line
        l_square,           // [
        r_square,           // ]
        l_paren,            // (
        r_paren,            // )
        period,             // .
        amp,                // &
        ampamp,             // &&
        star,               // *
        plus,               // +
        minus,              // -
        tilde,              // ~
        exclaim,            // !
        exclaimequal,       // !=
        slash,              // /
        percent,            // %
        less,               // <
        lessless,           // <<
        lessequal,          // <=
        greater,            // >
        greatergreater,     // >>
        greaterequal,       // >=
        caret,              // ^
        pipe,               // |
        pipepipe,           // ||
        colon,              // :
        equal,              // =
        equalequal,         // ==
        comma,              // ,
        dollar,             // $
        at,                 // @
        lessgreater,        // <>
        identifier,         // abcde123
        numeric_constant,   // 0x123
        string_literal,     // "foo"
        label,              // an identifier that can only be a label
        NUM_COMMON_TOKENS
    };

    // Various flags set per token:
    enum TokenFlags
    {
        StartOfLine     = 0x01, // At start of line or only after whitespace.
        LeadingSpace    = 0x02, // Whitespace exists before this token.
        DisableExpand   = 0x04, // This identifier may never be macro expanded.
        NeedsCleaning   = 0x08, // Contained an escaped newline
        Literal         = 0x10, // Is a literal
        EndOfStatement  = 0x20  // End of statement token (might be EOL or ';')
    };

    unsigned int getKind() const { return m_kind; }
    void setKind(unsigned int k) { m_kind = k; }

    /// is/is_not - Predicates to check if this token is a specific kind, as in
    /// "if (Tok.is(tok::l_brace)) {...}".
    bool is(unsigned int k) const { return m_kind == k; }
    bool isNot(unsigned int k) const { return m_kind != k; }

    /// getLocation - Return a source location identifier for the specified
    /// offset in the current file.
    SourceLocation getLocation() const { return m_loc; }
    unsigned int getLength() const { return m_len; }

    /// getEndLocation - Return a source location identifier one past the end
    /// of this token.
    SourceLocation getEndLocation() const
    { return m_loc.getLocWithOffset(m_len); }

    /// getSourceRange - Return a source range for the token.
    SourceRange getSourceRange() const
    { return SourceRange(m_loc, getEndLocation()); }

    void setLocation(SourceLocation l) { m_loc = l; }
    void setLength(unsigned int len) { m_len = len; }

#if 0
    const char *getName() const
    {
        return tok::getTokenName( (tok::TokenKind) Kind);
    }
#endif

    /// Reset all flags to cleared.
    void StartToken()
    {
        m_kind = 0;
        m_flags = 0;
        m_ptr_data = 0;
        m_loc = SourceLocation();
    }

    IdentifierInfo* getIdentifierInfo() const
    {
        if (isLiteral())
            return 0;
        return reinterpret_cast<IdentifierInfo*>(m_ptr_data);
    }
    void setIdentifierInfo(IdentifierInfo *ii)
    {
        m_ptr_data = (void*)ii;
    }

    /// Return true if this is a "literal", like a numeric constant,
    /// string, etc.
    bool isLiteral() const
    { return (m_flags & Literal) ? true : false; }

    StringRef getLiteral() const
    {
        return StringRef(getLiteralData(), getLength());
    }

    /// For a literal token (numeric constant, string, etc), this
    /// returns a pointer to the start of it in the text buffer if known, null
    /// otherwise.
    const char* getLiteralData() const
    {
        assert(isLiteral() && "Cannot get literal data of non-literal");
        return reinterpret_cast<const char*>(m_ptr_data);
    }
    void setLiteralData(const char* ptr)
    {
        assert(isLiteral() && "Cannot set literal data of non-literal");
        m_ptr_data = (void*)ptr;
    }

    /// Set the specified flag.
    void setFlag(TokenFlags flag) { m_flags |= flag; }

    /// Unset the specified flag.
    void clearFlag(TokenFlags flag) { m_flags &= ~flag; }

    /// Set a flag to either true or false.
    void setFlagValue(TokenFlags flag, bool val)
    {
        if (val) 
            setFlag(flag);
        else
            clearFlag(flag);
    }

    /// Return true if this token is at the start of a line.
    bool isAtStartOfLine() const
    { return (m_flags & StartOfLine) ? true : false; }

    /// Return true if this token has whitespace before it.
    bool hasLeadingSpace() const
    { return (m_flags & LeadingSpace) ? true : false; }

    /// Return true if this identifier token should never
    /// be expanded in the future.
    bool isExpandDisabled() const
    { return (m_flags & DisableExpand) ? true : false; }

    /// Return true if this token has escaped newlines in it.
    bool needsCleaning() const
    { return (m_flags & NeedsCleaning) ? true : false; }

    /// Return true if this token is the end of a statement.
    bool isEndOfStatement() const
    { return (m_flags & EndOfStatement) ? true : false; }
};

} // namespace yasm

#endif

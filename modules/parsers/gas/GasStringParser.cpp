//
// GAS-compatible string literal parser
//
//  Copyright (C) 2010  Peter Johnson
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
#include "GasStringParser.h"

#include "llvm/ADT/SmallString.h"
#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Parse/Preprocessor.h"
#include "yasmx/IntNum.h"


using namespace yasm;
using namespace yasm::parser;

static inline bool
isoctdigit(char ch)
{
    return (ch == '0' || ch == '1' || ch == '2' || ch == '3' ||
            ch == '4' || ch == '5' || ch == '6' || ch == '7');
}

static int
fromxdigit(char ch)
{
    if (ch >= 'a' && ch <= 'f')
        return (ch-'a'+10);
    else if (ch >= 'A' && ch <= 'F')
        return (ch-'A'+10);
    else
        return ch-'0';
}

/// escaped strings: "..."
/// character constants: '. or '\...
/// Supported escape characters in escaped strings: '"\btnvfr
/// Octal and hex escapes are also supported
///
GasStringParser::GasStringParser(StringRef str,
                                 SourceLocation loc,
                                 Preprocessor& pp)
    : m_chars_begin(str.begin()+1)
    , m_needs_unescape(false)
    , m_had_error(false)
{
    if (str.front() == '\'')
    {
        // Character constants don't have a terminating quote character
        m_chars_end = str.end();
        assert(str.size() >= 2 && "Invalid character constant from lexer?");
    }
    else
    {
        m_chars_end = str.end()-1;
        assert(str.size() >= 2 && str.front() == *m_chars_end &&
               "Invalid string from lexer?");
    }

    // Prescan escape validity
    const char* s = m_chars_begin;
    
    while (s < m_chars_end)
    {
        if (*s++ != '\\')
            continue;
        m_needs_unescape = true;

        assert (s < m_chars_end && "Lexer didn't maximally munch?");
        switch (*s++)
        {
            case '"': case '\\':
            case 'b': case 't': case 'n': case 'v': case 'f': case 'r':
                // normal single character escape
                break;
            case '0': case '1': case '2': case '3': case '4': case '5':
            case '6': case '7':
                // octal escape, up to 3 octal digits
                if (s < m_chars_end && isoctdigit(*s))
                {
                    ++s;
                    if (s < m_chars_end && isoctdigit(*s))
                        ++s;
                }
                break;
            case 'x':
                // hex escape, up to 2 hex digits
                if (s < m_chars_end && isxdigit(*s))
                    ++s;
                else
                {
                    pp.Diag(pp.AdvanceToTokenCharacter(loc, s-str.begin()),
                            diag::warn_expected_hex_digit);
                }
                while (s < m_chars_end && isxdigit(*s))
                    ++s;
                break;
            default:
                pp.Diag(pp.AdvanceToTokenCharacter(loc, s-str.begin()-1),
                        diag::warn_unknown_escape)
                    << std::string(s-1, s);
                break;
        }
    }
}

void
GasStringParser::getIntegerValue(IntNum* val)
{
    SmallString<64> strbuf;
    StringRef str = getString(strbuf);

    // Little endian order, so start from the end and work our way backwards.
    val->Zero();
    for (int i = str.size()-1; i >= 0; --i)
    {
        *val <<= 8;
        *val |= static_cast<unsigned int>(str[i]);
    }
}

std::string
GasStringParser::getString() const
{
    if (!m_needs_unescape)
        return std::string(m_chars_begin, m_chars_end-m_chars_begin);

    SmallString<64> strbuf;
    return getString(strbuf);
}

StringRef
GasStringParser::getString(SmallVectorImpl<char>& buffer) const
{
    if (!m_needs_unescape)
        return StringRef(m_chars_begin, m_chars_end-m_chars_begin);

    // slow path to do unescaping
    buffer.clear();
    const char* s = m_chars_begin;

    while (s < m_chars_end)
    {
        if (*s != '\\')
        {
            buffer.push_back(*s++);
            continue;
        }
        ++s;

        assert (s < m_chars_end && "Lexer didn't maximally munch?");
        switch (*s++)
        {
            case '"': case '\\':
                buffer.push_back(s[-1]);
                break;
            case 'b': buffer.push_back('\x08'); break;
            case 't': buffer.push_back('\x09'); break;
            case 'n': buffer.push_back('\x0a'); break;
            case 'v': buffer.push_back('\x0b'); break;
            case 'f': buffer.push_back('\x0c'); break;
            case 'r': buffer.push_back('\x0d'); break;
            case '0': case '1': case '2': case '3': case '4': case '5':
            case '6': case '7':
            {
                // octal escape, up to 3 octal digits
                int ch = s[-1] - '0';
                if (s < m_chars_end && isoctdigit(*s))
                {
                    ch <<= 3;
                    ch |= *s - '0';
                    ++s;
                    if (s < m_chars_end && isoctdigit(*s))
                    {
                        ch <<= 3;
                        ch |= *s - '0';
                        ++s;
                    }
                }
                buffer.push_back(static_cast<char>(ch));
                break;
            }
            case 'x':
            {
                // hex escape, only last 2 hex digits are used
                int ch = 0;
                while (s < m_chars_end && isxdigit(*s))
                {
                    ch <<= 4;
                    ch |= fromxdigit(*s);
                    ++s;
                }
                buffer.push_back(static_cast<char>(ch));
                break;
            }
            default:
                buffer.push_back(s[-1]);
                break;
        }
    }

    return StringRef(buffer.begin(), buffer.size());
}

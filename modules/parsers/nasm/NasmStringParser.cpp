//
// NASM-compatible string literal parser
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
#include "NasmStringParser.h"

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

/// basic unescaped strings: "..." and '...'
/// escaped strings: `...\...`
/// Supported escape characters in escaped strings: '"`\?abtnvfre
/// Octal, hex, and Unicode escapes are also supported
///
NasmStringParser::NasmStringParser(StringRef str,
                                   SourceLocation loc,
                                   Preprocessor& pp)
    : m_chars_begin(str.begin()+1)
    , m_chars_end(str.end()-1)
    , m_needs_unescape(false)
    , m_had_error(false)
{
    // This routine assumes that the range begin/end matches the regex for
    // string constants, and assumes that the byte at end[-1] is the ending
    // string character (e.g. the same as *begin).
    assert(str.size() >= 2 && str.front() == str.back() &&
           "Invalid string from lexer?");

    // If we don't need to unescape, we're done.
    if (str.front() != '`')
        return;

    // Prescan escape validity
    const char* s = m_chars_begin;
    
    while (s < m_chars_end)
    {
        if (*s++ != '\\')
            continue;
        m_needs_unescape = true;

        switch (*s++)
        {
            case '\'': case '"': case '`': case '\\': case '?':
            case 'a': case 'b': case 't': case 'n': case 'v': case 'f':
            case 'r': case 'e':
                // normal single character escape
                break;
            case '0': case '1': case '2': case '3': case '4': case '5':
            case '6': case '7':
                // octal escape, up to 3 octal digits
                if (isoctdigit(*s))
                {
                    ++s;
                    if (isoctdigit(*s))
                        ++s;
                }
                break;
            case 'x':
                // hex escape, up to 2 hex digits
                if (isxdigit(*s))
                    ++s;
                else
                {
                    pp.Diag(pp.AdvanceToTokenCharacter(loc, s-str.begin()),
                            diag::warn_expected_hex_digit);
                }
                if (isxdigit(*s))
                    ++s;
                break;
            case 'u':
            case 'U':
            {
                // 4 or 8 hex digit unicode character
                int nch = 4;
                if (s[-1] == 'U')
                    nch = 8;
                for (int i=0; i<nch; ++i, ++s)
                {
                    if (!isxdigit(*s))
                    {
                        m_had_error = true;
                        pp.Diag(pp.AdvanceToTokenCharacter(loc, s-str.begin()),
                                diag::err_unicode_escape_requires_hex)
                            << nch;
                        break;
                    }
                }
                break;
            }
            default:
                pp.Diag(pp.AdvanceToTokenCharacter(loc, s-str.begin()-1),
                        diag::warn_unknown_escape)
                    << std::string(s-1, s);
                break;
        }
    }
}

void
NasmStringParser::getIntegerValue(IntNum* val)
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
NasmStringParser::getString() const
{
    if (!m_needs_unescape)
        return std::string(m_chars_begin, m_chars_end-m_chars_begin);

    SmallString<64> strbuf;
    return getString(strbuf);
}

StringRef
NasmStringParser::getString(SmallVectorImpl<char>& buffer) const
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

        switch (*s++)
        {
            case '\'': case '"': case '`': case '\\': case '?':
                buffer.push_back(s[-1]);
                break;
            case 'a': buffer.push_back('\x07'); break;
            case 'b': buffer.push_back('\x08'); break;
            case 't': buffer.push_back('\x09'); break;
            case 'n': buffer.push_back('\x0a'); break;
            case 'v': buffer.push_back('\x0b'); break;
            case 'f': buffer.push_back('\x0c'); break;
            case 'r': buffer.push_back('\x0d'); break;
            case 'e': buffer.push_back('\x1a'); break;
            case '0': case '1': case '2': case '3': case '4': case '5':
            case '6': case '7':
            {
                // octal escape, up to 3 octal digits
                int ch = s[-1] - '0';
                if (isoctdigit(*s))
                {
                    ch <<= 3;
                    ch |= *s - '0';
                    ++s;
                    if (isoctdigit(*s))
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
                // hex escape, up to 2 hex digits
                if (!isxdigit(*s))
                {
                    buffer.push_back('x'); // treat it like a unknown escape
                    break;
                }
                int ch = fromxdigit(*s);
                ++s;
                if (isxdigit(*s))
                {
                    ch <<= 4;
                    ch |= fromxdigit(*s);
                    ++s;
                }
                buffer.push_back(static_cast<char>(ch));
                break;
            }
            case 'u':
            case 'U':
            {
                // 4 or 8 hex digit unicode character
                int nch = 4;
                if (s[-1] == 'U')
                    nch = 8;

                // Load as a full Unicode value
                unsigned long v = 0;
                for (int i=0; i<nch; ++i, ++s)
                {
                    assert (isxdigit(*s) && "got non-hex unicode escape");
                    v <<= 4;
                    v |= fromxdigit(*s);
                }

                // Convert and output as UTF-8
                unsigned long upper = 0;
                int sh = 0;
                if (v <= 0x0000007fUL)      { upper = 0x00; sh = 0; }
                else if (v <= 0x000007ffUL) { upper = 0xc0; sh = 6; }
                else if (v <= 0x0000ffffUL) { upper = 0xe0; sh = 12; }
                else if (v <= 0x001fffffUL) { upper = 0xf0; sh = 18; }
                else if (v <= 0x03ffffffUL) { upper = 0xf8; sh = 24; }
                else                        { upper = 0xfc; sh = 30; }
                for (; sh >= 0; sh -= 6)
                {
                    buffer.push_back(static_cast<char>
                        (upper | ((v >> sh) & 0x3f)));
                    upper = 0x80;
                }
                break;
            }
            default:
                buffer.push_back(s[-1]);
                break;
        }
    }

    return StringRef(buffer.begin(), buffer.size());
}

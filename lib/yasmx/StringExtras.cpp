//
// Extra string functions.
//
//  Copyright (C) 2001-2007  Peter Johnson
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
#include "yasmx/Support/StringExtras.h"

#include <cctype>


namespace yasm
{

std::string
ConvUnprint(int ch)
{
    std::string unprint;

    if (((ch & ~0x7F) != 0) /*!isascii(ch)*/ && !isprint(ch))
    {
        unprint += 'M';
        unprint += '-';
        ch &= 0x7F; /*toascii(ch)*/
    }

    if (iscntrl(ch))
    {
        unprint += '^';
        unprint += (ch == '\177') ? '?' : ch | 0100;
    }
    else
        unprint += ch;

    return unprint;
}

inline char
HexToDec(char ch)
{
    if (ch >= 'a' && ch <= 'f')
        return (ch-'a'+10);
    else if (ch >= 'A' && ch <= 'F')
        return (ch-'A'+10);
    else
        return ch-'0';
}

inline char
OctToDec(char ch, bool* ok)
{
    if (ch > '7')
        *ok = false;
    return ch-'0';
}

bool
Unescape(std::string* str)
{
    bool ok = true;
    std::string::iterator o=str->begin();
    for (std::string::const_iterator i=str->begin(), e=str->end(); i != e; )
    {
        if (*i != '\\')
        {
            *o++ = *i++;
            continue;
        }

        ++i;
        if (i == e)
        {
            // shouldn't happen, but handle it anyway
            if (o != e)
                ++o;
            break;
        }

        switch (*i)
        {
            case 'b': *o = '\b'; ++i; break;
            case 'f': *o = '\f'; ++i; break;
            case 'n': *o = '\n'; ++i; break;
            case 'r': *o = '\r'; ++i; break;
            case 't': *o = '\t'; ++i; break;
            case 'x':
                // hex escape; grab last two digits
                ++i;
                while (i != e && (i+1) != e && (i+2) != e
                       && std::isxdigit(*i) && std::isxdigit(*(i+1))
                       && std::isxdigit(*(i+2)))
                    ++i;
                if (i != e && std::isxdigit(*i))
                {
                    char val = HexToDec(*i++);
                    if (i != e && std::isxdigit(*i))
                    {
                        val <<= 4;
                        val += HexToDec(*i++);
                    }
                    *o = val;
                }
                else
                    *o = 0;
                break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
            {
                // octal escape
                unsigned char val = OctToDec(*i++, &ok);
                if (i != e && std::isdigit(*i))
                {
                    val <<= 3;
                    val += OctToDec(*i++, &ok);
                    if (i != e && std::isdigit(*i))
                    {
                        val <<= 3;
                        val += OctToDec(*i++, &ok);
                    }
                }
                *o = val;
                break;
            }
            default: *o = *i++; break;
        }
        ++o;
    }

    str->erase(o, str->end());
    return ok;
}

} // namespace yasm

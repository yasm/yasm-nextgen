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

#include "util.h"

#include "yasmx/Support/errwarn.h"


namespace yasm
{

std::string
Unescape(const std::string& str)
{
    std::string out;
    std::string::const_iterator i=str.begin(), end=str.end();
    while (i != end)
    {
        if (*i == '\\')
        {
            ++i;
            if (i == end)
            {
                out.push_back('\\');
                return out;
            }
            switch (*i)
            {
                case 'b': out.push_back('\b'); ++i; break;
                case 'f': out.push_back('\f'); ++i; break;
                case 'n': out.push_back('\n'); ++i; break;
                case 'r': out.push_back('\r'); ++i; break;
                case 't': out.push_back('\t'); ++i; break;
                case 'x':
                    // hex escape; grab last two digits
                    ++i;
                    while (i != end && (i+1) != end && (i+2) != end
                           && std::isxdigit(*i) && std::isxdigit(*(i+1))
                           && std::isxdigit(*(i+2)))
                        ++i;
                    if (i != end && std::isxdigit(*i))
                    {
                        char t[3];
                        t[0] = *i++;
                        t[1] = '\0';
                        t[2] = '\0';
                        if (i != end && std::isxdigit(*i))
                            t[1] = *i++;
                        out.push_back(static_cast<char>(
                            strtoul(static_cast<char*>(t), NULL, 16)));
                    }
                    else
                        out.push_back(0);
                    break;
                default:
                    if (isdigit(*i))
                    {
                        bool warn = false;
                        // octal escape
                        if (*i > '7')
                            warn = true;
                        unsigned char v = *i++ - '0';
                        if (i != end && std::isdigit(*i))
                        {
                            if (*i > '7')
                                warn = true;
                            v <<= 3;
                            v += *i++ - '0';
                            if (i != end && std::isdigit(*i))
                            {
                                if (*i > '7')
                                    warn = true;
                                v <<= 3;
                                v += *i++ - '0';
                            }
                        }
                        out.push_back(static_cast<char>(v));
                        if (warn)
                            setWarn(WARN_GENERAL,
                                    N_("octal value out of range"));
                    }
                    else
                        out.push_back(*i++);
                    break;
            }
        }
        else
            out.push_back(*i++);
    }
    return out;
}

} // namespace yasm

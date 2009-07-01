//
// Case-insensitive string functions
//
//  Copyright (C) 2007  Peter Johnson
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
#include "yasmx/Support/nocase.h"

#include <cctype>
#include <cstring>


namespace String
{

bool
nocase_equal(const char* s1, const char* s2)
{
    for (; *s1 != '\0' && *s2 != '\0'; ++s1, ++s2)
    {
        if (tolower(*s1) != tolower(*s2))
            return false;
    }
    return (tolower(*s1) == tolower(*s2));
}

bool
nocase_equal(const char* s1, const char* s2, std::string::size_type n)
{
    if (n == 0)
        return true;
    for (; *s1 != '\0' && *s2 != '\0' && n > 1; ++s1, ++s2, --n)
    {
        if (tolower(*s1) != tolower(*s2))
            return false;
    }
    return (tolower(*s1) == tolower(*s2));
}

std::string
lowercase(const char* in)
{
    std::string ret;
    ret.reserve(std::strlen(in));
    for (; *in != '\0'; ++in)
        ret += tolower(*in);
    return ret;
}

std::string
lowercase(const std::string& in)
{
    std::string ret;
    ret.reserve(in.length());
    for (std::string::const_iterator i = in.begin(), end = in.end();
         i != end; ++i)
        ret += tolower(*i);
    return ret;
}

} // namespace String

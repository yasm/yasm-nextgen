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
#include "Support/nocase.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>


namespace String
{

static inline bool
nocase_equal_char(char a, char b)
{
    return toupper(a) == toupper(b);
}

bool
nocase_equal(const std::string& s1, const std::string& s2)
{
    return (s1.size() == s2.size() &&
            std::equal(s1.begin(), s1.end(), s2.begin(), nocase_equal_char));
}

bool
nocase_equal(const std::string& s1, const char* s2)
{
    return (s1.size() == std::strlen(s2) &&
            std::equal(s1.begin(), s1.end(), s2, nocase_equal_char));
}

bool
nocase_equal(const char* s2, const std::string& s1)
{
    return (s1.size() == std::strlen(s2) &&
            std::equal(s1.begin(), s1.end(), s2, nocase_equal_char));
}

bool
nocase_equal(const std::string& s1, const std::string& s2,
             std::string::size_type n)
{
    if (s1.length() > n || s2.length() > n)
        return false;
    return (std::equal(s1.begin(), s1.begin()+n, s2.begin(),
                       nocase_equal_char));
}

bool
nocase_equal(const std::string& s1, const char* s2,
             std::string::size_type n)
{
    if (s1.length() > n || std::strlen(s2) > n)
        return false;
    return (std::equal(s1.begin(), s1.begin()+n, s2, nocase_equal_char));
}

bool
nocase_equal(const char* s2, const std::string& s1,
             std::string::size_type n)
{
    if (s1.length() > n || std::strlen(s2) > n)
        return false;
    return (std::equal(s1.begin(), s1.begin()+n, s2, nocase_equal_char));
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

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


namespace String
{

bool
NocaseEqual(llvm::StringRef s1, llvm::StringRef s2)
{
    if (s1.size() != s2.size())
        return false;
    for (llvm::StringRef::iterator i = s1.begin(), end = s1.end(),
         j = s2.begin(); i != end; ++i, ++j)
    {
        if (tolower(*i) != tolower(*j))
            return false;
    }
    return true;
}

bool
NocaseEqual(llvm::StringRef s1, llvm::StringRef s2, std::size_t n)
{
    size_t i, s1len = s1.size(), s2len = s2.size();
    for (i = 0; i < n && i < s1len && i < s2len; ++i)
    {
        if (tolower(s1[i]) != tolower(s2[i]))
            return false;
    }
    if (i != n && s1len != s2len)
        return false;
    return true;
}

std::string
Lowercase(llvm::StringRef in)
{
    std::string ret;
    ret.reserve(in.size());
    for (llvm::StringRef::iterator i = in.begin(), end = in.end();
         i != end; ++i)
        ret += tolower(*i);
    return ret;
}

} // namespace String

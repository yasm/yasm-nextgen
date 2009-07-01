#ifndef YASM_NOCASE_H
#define YASM_NOCASE_H
///
/// @file
/// @brief Case-insensitive string functions.
///
/// @license
///  Copyright (C) 2007  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
///  - Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
///  - Redistributions in binary form must reproduce the above copyright
///    notice, this list of conditions and the following disclaimer in the
///    documentation and/or other materials provided with the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
/// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
/// @endlicense
///
#include <cassert>
#include <cstring>
#include <string>

#include "yasmx/Config/export.h"


namespace String
{

YASM_LIB_EXPORT
bool nocase_equal(const char* s1, const char* s2);

static inline bool
nocase_equal(const std::string& s1, const std::string& s2)
{
    assert(std::strlen(s1.c_str()) == s1.length() && "embedded nul in string");
    assert(std::strlen(s2.c_str()) == s2.length() && "embedded nul in string");
    return nocase_equal(s1.c_str(), s2.c_str());
}

static inline bool
nocase_equal(const std::string& s1, const char* s2)
{
    assert(std::strlen(s1.c_str()) == s1.length() && "embedded nul in string");
    return nocase_equal(s1.c_str(), s2);
}

static inline bool
nocase_equal(const char* s1, const std::string& s2)
{
    assert(std::strlen(s2.c_str()) == s2.length() && "embedded nul in string");
    return nocase_equal(s1, s2.c_str());
}

YASM_LIB_EXPORT
bool nocase_equal(const char* s1, const char* s2,
                  std::string::size_type n);

static inline bool
nocase_equal(const std::string& s1, const std::string& s2,
             std::string::size_type n)
{
    assert(std::strlen(s1.c_str()) == s1.length() && "embedded nul in string");
    assert(std::strlen(s2.c_str()) == s2.length() && "embedded nul in string");
    return nocase_equal(s1.c_str(), s2.c_str(), n);
}

static inline bool
nocase_equal(const std::string& s1, const char* s2, std::string::size_type n)
{
    assert(std::strlen(s1.c_str()) == s1.length() && "embedded nul in string");
    return nocase_equal(s1.c_str(), s2, n);
}

static inline bool
nocase_equal(const char* s1, const std::string& s2, std::string::size_type n)
{
    assert(std::strlen(s2.c_str()) == s2.length() && "embedded nul in string");
    return nocase_equal(s1, s2.c_str(), n);
}

YASM_LIB_EXPORT
std::string lowercase(const char* in);
YASM_LIB_EXPORT
std::string lowercase(const std::string& in);

} // namespace String

#endif

//
//  Copyright (C) 2006-2007  Peter Johnson
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
#include <gtest/gtest.h>

#include <string>

#include "yasmx/Support/StringExtras.h"

using namespace yasm;

TEST(UnescapeTest, Basic)
{
    std::string s;

    s = "";
    EXPECT_EQ(true, Unescape(&s));
    EXPECT_EQ("", s);

    s = "noescape";
    EXPECT_EQ(true, Unescape(&s));
    EXPECT_EQ("noescape", s);

    s = "\\\\\\b\\f\\n\\r\\t\\\"";
    EXPECT_EQ(true, Unescape(&s));
    EXPECT_EQ("\\\b\f\n\r\t\"", s);

    s = "\\a";
    EXPECT_EQ(true, Unescape(&s));
    EXPECT_EQ("a", s);

    s = "\\";
    EXPECT_EQ(true, Unescape(&s));
    EXPECT_EQ("\\", s);
}

// hex tests
TEST(UnescapeTest, Hex)
{
    std::string s, cmp;

    s = "\\x";
    EXPECT_EQ(true, Unescape(&s));
    cmp.push_back(0);
    EXPECT_EQ(cmp, s);

    s = "\\x12";
    EXPECT_EQ(true, Unescape(&s));
    EXPECT_EQ("\x12", s);

    s = "\\x1234";
    EXPECT_EQ(true, Unescape(&s));
    EXPECT_EQ("\x34", s);

    s = "\\xg";
    EXPECT_EQ(true, Unescape(&s));
    cmp.clear(); cmp.push_back(0); cmp.push_back('g');
    EXPECT_EQ(cmp, s);

    s = "\\xaga";
    EXPECT_EQ(true, Unescape(&s));
    EXPECT_EQ("\x0aga", s);

    s = "\\xaag";
    EXPECT_EQ(true, Unescape(&s));
    EXPECT_EQ("\xaag", s);

    s = "\\xaaa";
    EXPECT_EQ(true, Unescape(&s));
    EXPECT_EQ("\xaa", s);

    s = "\\x55559";
    EXPECT_EQ(true, Unescape(&s));
    EXPECT_EQ("\x59", s);
}

// oct tests
TEST(UnescapeTest, Oct)
{
    std::string s, cmp, wmsg;

    s = "\\778";
    EXPECT_EQ(false, Unescape(&s));
    cmp.push_back(0);
    EXPECT_EQ(cmp, s);

    s = "\\779";
    EXPECT_EQ(false, Unescape(&s));
    EXPECT_EQ("\001", s);

    s = "\\1x";
    EXPECT_EQ(true, Unescape(&s));
    EXPECT_EQ("\001x", s);

    s = "\\7779";
    EXPECT_EQ(true, Unescape(&s));
    EXPECT_EQ("\xff" "9", s);

    s = "\\7999";
    EXPECT_EQ(false, Unescape(&s));
    EXPECT_EQ("\x11" "9", s);

    s = "\\77a";
    EXPECT_EQ(true, Unescape(&s));
    EXPECT_EQ("\077a", s);

    s = "\\5555555";
    EXPECT_EQ(true, Unescape(&s));
    EXPECT_EQ("\x6d" "5555", s);

    s = "\\9999";
    EXPECT_EQ(false, Unescape(&s));
    EXPECT_EQ("\x91" "9", s);
}

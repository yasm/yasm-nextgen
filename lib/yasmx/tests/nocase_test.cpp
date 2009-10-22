//
//  Copyright (C) 2009  Peter Johnson
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

#include "llvm/ADT/StringRef.h"
#include "yasmx/Support/nocase.h"

TEST(NocaseEqualTest, NoLength)
{
    EXPECT_EQ(true, String::NocaseEqual("foo", "foo"));
    EXPECT_EQ(false, String::NocaseEqual("foo", "foo1"));
    EXPECT_EQ(false, String::NocaseEqual("foo1", "foo"));
}

TEST(NocaseEqualTest, Length)
{
    EXPECT_EQ(true, String::NocaseEqual("foo", "foo", 2));
    EXPECT_EQ(true, String::NocaseEqual("foo", "foo", 3));
    EXPECT_EQ(true, String::NocaseEqual("foo", "foo", 4));
    EXPECT_EQ(true, String::NocaseEqual("foo", "foo", 5));
    EXPECT_EQ(true, String::NocaseEqual("foo", "foo1", 3));
    EXPECT_EQ(false, String::NocaseEqual("foo", "foo1", 4));
    EXPECT_EQ(true, String::NocaseEqual("foo1", "foo", 3));
    EXPECT_EQ(false, String::NocaseEqual("foo1", "foo", 4));
    EXPECT_EQ(true, String::NocaseEqual("foo", "bar", 0));
}

TEST(LowercaseTest, CharPointer)
{
    EXPECT_EQ("foo", String::Lowercase("foo"));
    EXPECT_EQ("foo", String::Lowercase("Foo"));
    EXPECT_EQ("bar", String::Lowercase("BAR"));
}

TEST(LowercaseTest, EmbeddedNul)
{
    // std::string version accepts embedded nuls
    EXPECT_EQ("foo\0ba", String::Lowercase(std::string("foo\0ba")));
    EXPECT_EQ("foo\0ba", String::Lowercase(std::string("Foo\0Ba")));
    EXPECT_EQ("foo\0ba", String::Lowercase(std::string("FOO\0BA")));
}

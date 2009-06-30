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
#include <cxxtest/TestSuite.h>

#include "yasmx/Support/nocase.h"

using namespace yasm;

class NocaseTestSuite : public CxxTest::TestSuite
{
public:
    void testNoN()
    {
        TS_ASSERT_EQUALS(String::nocase_equal("foo", "foo"), true);
        TS_ASSERT_EQUALS(String::nocase_equal("foo", "foo1"), false);
        TS_ASSERT_EQUALS(String::nocase_equal("foo1", "foo"), false);
    }

    void testN()
    {
        TS_ASSERT_EQUALS(String::nocase_equal("foo", "foo", 2), true);
        TS_ASSERT_EQUALS(String::nocase_equal("foo", "foo", 3), true);
        TS_ASSERT_EQUALS(String::nocase_equal("foo", "foo", 4), true);
        TS_ASSERT_EQUALS(String::nocase_equal("foo", "foo", 5), true);
        TS_ASSERT_EQUALS(String::nocase_equal("foo", "foo1", 3), true);
        TS_ASSERT_EQUALS(String::nocase_equal("foo", "foo1", 4), false);
        TS_ASSERT_EQUALS(String::nocase_equal("foo1", "foo", 3), true);
        TS_ASSERT_EQUALS(String::nocase_equal("foo1", "foo", 4), false);
        TS_ASSERT_EQUALS(String::nocase_equal("foo", "bar", 0), true);
    }

    void testLowercase()
    {
        TS_ASSERT_EQUALS(String::lowercase("foo"), "foo");
        TS_ASSERT_EQUALS(String::lowercase("Foo"), "foo");
        TS_ASSERT_EQUALS(String::lowercase("BAR"), "bar");
        // std::string version has separate implementation
        TS_ASSERT_EQUALS(String::lowercase(std::string("foo")), "foo");
        TS_ASSERT_EQUALS(String::lowercase(std::string("Foo")), "foo");
        TS_ASSERT_EQUALS(String::lowercase(std::string("BAR")), "bar");
        // std::string version accepts embedded nuls
        TS_ASSERT_EQUALS(String::lowercase(std::string("foo\0ba")), "foo\0ba");
        TS_ASSERT_EQUALS(String::lowercase(std::string("Foo\0Ba")), "foo\0ba");
        TS_ASSERT_EQUALS(String::lowercase(std::string("FOO\0BA")), "foo\0ba");
    }
};

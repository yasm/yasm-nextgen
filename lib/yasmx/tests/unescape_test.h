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
#include <cxxtest/TestSuite.h>

#include "yasmx/Support/StringExtras.h"

using namespace yasm;

class UnescapeTestSuite : public CxxTest::TestSuite
{
    std::string s;

public:
    void testBasic()
    {
        s = "";
        TS_ASSERT_EQUALS(Unescape(&s), true);
        TS_ASSERT_EQUALS(s, "");

        s = "noescape";
        TS_ASSERT_EQUALS(Unescape(&s), true);
        TS_ASSERT_EQUALS(s, "noescape");

        s = "\\\\\\b\\f\\n\\r\\t\\\"";
        TS_ASSERT_EQUALS(Unescape(&s), true);
        TS_ASSERT_EQUALS(s, "\\\b\f\n\r\t\"");

        s = "\\a";
        TS_ASSERT_EQUALS(Unescape(&s), true);
        TS_ASSERT_EQUALS(s, "a");

        s = "\\";
        TS_ASSERT_EQUALS(Unescape(&s), true);
        TS_ASSERT_EQUALS(s, "\\");
    }

    // hex tests
    void testHex()
    {
        std::string cmp;

        s = "\\x";
        TS_ASSERT_EQUALS(Unescape(&s), true);
        cmp.push_back(0);
        TS_ASSERT_EQUALS(s, cmp);

        s = "\\x12";
        TS_ASSERT_EQUALS(Unescape(&s), true);
        TS_ASSERT_EQUALS(s, "\x12");

        s = "\\x1234";
        TS_ASSERT_EQUALS(Unescape(&s), true);
        TS_ASSERT_EQUALS(s, "\x34");

        s = "\\xg";
        TS_ASSERT_EQUALS(Unescape(&s), true);
        cmp.clear(); cmp.push_back(0); cmp.push_back('g');
        TS_ASSERT_EQUALS(s, cmp);

        s = "\\xaga";
        TS_ASSERT_EQUALS(Unescape(&s), true);
        TS_ASSERT_EQUALS(s, "\x0aga");

        s = "\\xaag";
        TS_ASSERT_EQUALS(Unescape(&s), true);
        TS_ASSERT_EQUALS(s, "\xaag");

        s = "\\xaaa";
        TS_ASSERT_EQUALS(Unescape(&s), true);
        TS_ASSERT_EQUALS(s, "\xaa");

        s = "\\x55559";
        TS_ASSERT_EQUALS(Unescape(&s), true);
        TS_ASSERT_EQUALS(s, "\x59");
    }

    // oct tests
    void testOct()
    {
        std::string cmp, wmsg;

        s = "\\778";
        TS_ASSERT_EQUALS(Unescape(&s), false);
        cmp.push_back(0);
        TS_ASSERT_EQUALS(s, cmp);

        s = "\\779";
        TS_ASSERT_EQUALS(Unescape(&s), false);
        TS_ASSERT_EQUALS(s, "\001");

        s = "\\1x";
        TS_ASSERT_EQUALS(Unescape(&s), true);
        TS_ASSERT_EQUALS(s, "\001x");

        s = "\\7779";
        TS_ASSERT_EQUALS(Unescape(&s), true);
        TS_ASSERT_EQUALS(s, "\xff" "9");

        s = "\\7999";
        TS_ASSERT_EQUALS(Unescape(&s), false);
        TS_ASSERT_EQUALS(s, "\x11" "9");

        s = "\\77a";
        TS_ASSERT_EQUALS(Unescape(&s), true);
        TS_ASSERT_EQUALS(s, "\077a");

        s = "\\5555555";
        TS_ASSERT_EQUALS(Unescape(&s), true);
        TS_ASSERT_EQUALS(s, "\x6d" "5555");

        s = "\\9999";
        TS_ASSERT_EQUALS(Unescape(&s), false);
        TS_ASSERT_EQUALS(s, "\x91" "9");
    }
};

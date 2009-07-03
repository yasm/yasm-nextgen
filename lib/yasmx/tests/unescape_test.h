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

#include "yasmx/Support/errwarn.h"
#include "yasmx/System/file.h"

using namespace yasm;

class UnescapeTestSuite : public CxxTest::TestSuite
{
public:
    void testBasic()
    {
        TS_ASSERT_EQUALS(Unescape("noescape"), "noescape");
        TS_ASSERT_EQUALS(Unescape("\\\\\\b\\f\\n\\r\\t\\\""), "\\\b\f\n\r\t\"");
        TS_ASSERT_EQUALS(Unescape("\\a"), "a");
        TS_ASSERT_EQUALS(Unescape("\\"), "\\");

        // should not have gotten any warnings
        TS_ASSERT_EQUALS(WarnOccurred(), WARN_NONE);
    }

    // hex tests
    void testHex()
    {
        std::string cmp;

        cmp.push_back(0);
        TS_ASSERT_EQUALS(Unescape("\\x"), cmp);

        TS_ASSERT_EQUALS(Unescape("\\x12"), "\x12");
        TS_ASSERT_EQUALS(Unescape("\\x1234"), "\x34");

        cmp.clear(); cmp.push_back(0); cmp.push_back('g');
        TS_ASSERT_EQUALS(Unescape("\\xg"), cmp);
        TS_ASSERT_EQUALS(Unescape("\\xaga"), "\x0aga");
        TS_ASSERT_EQUALS(Unescape("\\xaag"), "\xaag");
        TS_ASSERT_EQUALS(Unescape("\\xaaa"), "\xaa");
        TS_ASSERT_EQUALS(Unescape("\\x55559"), "\x59");

        // should not have gotten any warnings
        TS_ASSERT_EQUALS(WarnOccurred(), WARN_NONE);
    }

    // oct tests
    void testOct()
    {
        std::string cmp, wmsg;

        cmp.push_back(0);
        TS_ASSERT_EQUALS(Unescape("\\778"), cmp);
        TS_ASSERT_EQUALS(FetchWarn(&wmsg), WARN_GENERAL);
        TS_ASSERT_EQUALS(wmsg, "octal value out of range");

        TS_ASSERT_EQUALS(Unescape("\\779"), "\001");
        TS_ASSERT_EQUALS(FetchWarn(&wmsg), WARN_GENERAL);
        TS_ASSERT_EQUALS(wmsg, "octal value out of range");

        TS_ASSERT_EQUALS(Unescape("\\1x"), "\001x");
        TS_ASSERT_EQUALS(WarnOccurred(), WARN_NONE);
        TS_ASSERT_EQUALS(Unescape("\\7779"), "\xff" "9");
        TS_ASSERT_EQUALS(WarnOccurred(), WARN_NONE);

        TS_ASSERT_EQUALS(Unescape("\\7999"), "\x11" "9");
        TS_ASSERT_EQUALS(FetchWarn(&wmsg), WARN_GENERAL);
        TS_ASSERT_EQUALS(wmsg, "octal value out of range");

        TS_ASSERT_EQUALS(Unescape("\\77a"), "\077a");
        TS_ASSERT_EQUALS(WarnOccurred(), WARN_NONE);
        TS_ASSERT_EQUALS(Unescape("\\5555555"), "\x6d" "5555");
        TS_ASSERT_EQUALS(WarnOccurred(), WARN_NONE);

        TS_ASSERT_EQUALS(Unescape("\\9999"), "\x91" "9");
        TS_ASSERT_EQUALS(FetchWarn(&wmsg), WARN_GENERAL);
        TS_ASSERT_EQUALS(wmsg, "octal value out of range");
    }
};

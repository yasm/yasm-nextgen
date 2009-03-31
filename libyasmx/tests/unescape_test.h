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

#include "yasmx/System/file.h"
#include "yasmx/errwarn.h"

using namespace yasm;

class UnescapeTestSuite : public CxxTest::TestSuite
{
public:
    void testBasic()
    {
        TS_ASSERT_EQUALS(unescape("noescape"), "noescape");
        TS_ASSERT_EQUALS(unescape("\\\\\\b\\f\\n\\r\\t\\\""), "\\\b\f\n\r\t\"");
        TS_ASSERT_EQUALS(unescape("\\a"), "a");
        TS_ASSERT_EQUALS(unescape("\\"), "\\");

        // should not have gotten any warnings
        TS_ASSERT_EQUALS(warn_occurred(), WARN_NONE);
    }

    // hex tests
    void testHex()
    {
        std::string cmp;

        cmp.push_back(0);
        TS_ASSERT_EQUALS(unescape("\\x"), cmp);

        TS_ASSERT_EQUALS(unescape("\\x12"), "\x12");
        TS_ASSERT_EQUALS(unescape("\\x1234"), "\x34");

        cmp.clear(); cmp.push_back(0); cmp.push_back('g');
        TS_ASSERT_EQUALS(unescape("\\xg"), cmp);
        TS_ASSERT_EQUALS(unescape("\\xaga"), "\x0aga");
        TS_ASSERT_EQUALS(unescape("\\xaag"), "\xaag");
        TS_ASSERT_EQUALS(unescape("\\xaaa"), "\xaa");
        TS_ASSERT_EQUALS(unescape("\\x55559"), "\x59");

        // should not have gotten any warnings
        TS_ASSERT_EQUALS(warn_occurred(), WARN_NONE);
    }

    // oct tests
    void testOct()
    {
        std::string cmp, wmsg;

        cmp.push_back(0);
        TS_ASSERT_EQUALS(unescape("\\778"), cmp);
        TS_ASSERT_EQUALS(warn_fetch(&wmsg), WARN_GENERAL);
        TS_ASSERT_EQUALS(wmsg, "octal value out of range");

        TS_ASSERT_EQUALS(unescape("\\779"), "\001");
        TS_ASSERT_EQUALS(warn_fetch(&wmsg), WARN_GENERAL);
        TS_ASSERT_EQUALS(wmsg, "octal value out of range");

        TS_ASSERT_EQUALS(unescape("\\1x"), "\001x");
        TS_ASSERT_EQUALS(warn_occurred(), WARN_NONE);
        TS_ASSERT_EQUALS(unescape("\\7779"), "\xff" "9");
        TS_ASSERT_EQUALS(warn_occurred(), WARN_NONE);

        TS_ASSERT_EQUALS(unescape("\\7999"), "\x11" "9");
        TS_ASSERT_EQUALS(warn_fetch(&wmsg), WARN_GENERAL);
        TS_ASSERT_EQUALS(wmsg, "octal value out of range");

        TS_ASSERT_EQUALS(unescape("\\77a"), "\077a");
        TS_ASSERT_EQUALS(warn_occurred(), WARN_NONE);
        TS_ASSERT_EQUALS(unescape("\\5555555"), "\x6d" "5555");
        TS_ASSERT_EQUALS(warn_occurred(), WARN_NONE);

        TS_ASSERT_EQUALS(unescape("\\9999"), "\x91" "9");
        TS_ASSERT_EQUALS(warn_fetch(&wmsg), WARN_GENERAL);
        TS_ASSERT_EQUALS(wmsg, "octal value out of range");
    }
};

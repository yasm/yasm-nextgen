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
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE file_combpath_test
#include <boost/test/unit_test.hpp>

#include "errwarn.h"
#include "file.h"

using namespace yasm;

BOOST_AUTO_TEST_CASE(Basic)
{
    BOOST_CHECK_EQUAL(unescape("noescape"), "noescape");
    BOOST_CHECK_EQUAL(unescape("\\\\\\b\\f\\n\\r\\t\\\""), "\\\b\f\n\r\t\"");
    BOOST_CHECK_EQUAL(unescape("\\a"), "a");
    BOOST_CHECK_EQUAL(unescape("\\"), "\\");

    // should not have gotten any warnings
    BOOST_CHECK_EQUAL(warn_occurred(), WARN_NONE);
}

// hex tests
BOOST_AUTO_TEST_CASE(Hex)
{
    std::string cmp;

    cmp.push_back(0);
    BOOST_CHECK_EQUAL(unescape("\\x"), cmp);

    BOOST_CHECK_EQUAL(unescape("\\x12"), "\x12");
    BOOST_CHECK_EQUAL(unescape("\\x1234"), "\x34");

    cmp.clear(); cmp.push_back(0); cmp.push_back('g');
    BOOST_CHECK_EQUAL(unescape("\\xg"), cmp);
    BOOST_CHECK_EQUAL(unescape("\\xaga"), "\x0aga");
    BOOST_CHECK_EQUAL(unescape("\\xaag"), "\xaag");
    BOOST_CHECK_EQUAL(unescape("\\xaaa"), "\xaa");
    BOOST_CHECK_EQUAL(unescape("\\x55559"), "\x59");

    // should not have gotten any warnings
    BOOST_CHECK_EQUAL(warn_occurred(), WARN_NONE);
}

// oct tests
BOOST_AUTO_TEST_CASE(Oct)
{
    std::string cmp, wmsg;

    cmp.push_back(0);
    BOOST_CHECK_EQUAL(unescape("\\778"), cmp);
    BOOST_CHECK_EQUAL(warn_fetch(&wmsg), WARN_GENERAL);
    BOOST_CHECK_EQUAL(wmsg, "octal value out of range");

    BOOST_CHECK_EQUAL(unescape("\\779"), "\001");
    BOOST_CHECK_EQUAL(warn_fetch(&wmsg), WARN_GENERAL);
    BOOST_CHECK_EQUAL(wmsg, "octal value out of range");

    BOOST_CHECK_EQUAL(unescape("\\1x"), "\001x");
    BOOST_CHECK_EQUAL(warn_occurred(), WARN_NONE);
    BOOST_CHECK_EQUAL(unescape("\\7779"), "\xff" "9");
    BOOST_CHECK_EQUAL(warn_occurred(), WARN_NONE);

    BOOST_CHECK_EQUAL(unescape("\\7999"), "\x11" "9");
    BOOST_CHECK_EQUAL(warn_fetch(&wmsg), WARN_GENERAL);
    BOOST_CHECK_EQUAL(wmsg, "octal value out of range");

    BOOST_CHECK_EQUAL(unescape("\\77a"), "\077a");
    BOOST_CHECK_EQUAL(warn_occurred(), WARN_NONE);
    BOOST_CHECK_EQUAL(unescape("\\5555555"), "\x6d" "5555");
    BOOST_CHECK_EQUAL(warn_occurred(), WARN_NONE);

    BOOST_CHECK_EQUAL(unescape("\\9999"), "\x91" "9");
    BOOST_CHECK_EQUAL(warn_fetch(&wmsg), WARN_GENERAL);
    BOOST_CHECK_EQUAL(wmsg, "octal value out of range");
}

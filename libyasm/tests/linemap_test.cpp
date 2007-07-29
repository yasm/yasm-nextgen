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
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE linemap_test
#include <boost/test/unit_test.hpp>

#include "linemap.h"

using namespace yasm;

BOOST_AUTO_TEST_CASE(TestCase1)
{
    Linemap lm;
    bool ok;
    Bytecode* bc;
    std::string source;
    unsigned long line;

    // initial line number
    BOOST_CHECK_EQUAL(lm.get_current(), 1UL);

    // get source with no source available
    ok = lm.get_source(1, bc, source);
    BOOST_CHECK_EQUAL(ok, false);
    BOOST_CHECK(bc == 0);
    BOOST_CHECK_EQUAL(source, "");

    // add source for line 1, no bytecode
    lm.add_source(0, "line 1 source");

    // line number increment
    line = lm.goto_next();
    BOOST_CHECK_EQUAL(line, 2UL);
    BOOST_CHECK_EQUAL(lm.get_current(), 2UL);

    // add source for line 2, no bytecode
    lm.add_source(0, "line 2 source");

    // get source for line 1
    ok = lm.get_source(1, bc, source);
    BOOST_CHECK_EQUAL(ok, true);
    BOOST_CHECK(bc == 0);
    BOOST_CHECK_EQUAL(source, "line 1 source");

    // get source for line 2
    ok = lm.get_source(2, bc, source);
    BOOST_CHECK_EQUAL(ok, true);
    BOOST_CHECK(bc == 0);
    BOOST_CHECK_EQUAL(source, "line 2 source");
}

#define LOOKUP_CHECK(lm, line, fn_result, fl_result) \
do { \
    std::string filename; \
    unsigned long file_line; \
\
    bool ok = lm.lookup(line, filename, file_line); \
    BOOST_CHECK_EQUAL(ok, true); \
    BOOST_CHECK_EQUAL(filename, fn_result); \
    BOOST_CHECK_EQUAL(file_line, fl_result); \
} while(0)

BOOST_AUTO_TEST_CASE(TestCase2)
{
    Linemap lm;
    bool ok;
    std::string filename;
    unsigned long file_line;

    // lookup with no filename info available
    ok = lm.lookup(1, filename, file_line);
    BOOST_CHECK_EQUAL(ok, false);
    BOOST_CHECK_EQUAL(filename, "unknown");
    BOOST_CHECK_EQUAL(file_line, 0UL);

    // Physical line setup
    lm.set("file 1", 1, 1); // line 1 -> "file 1", 1, +1  --> 1 = "file 1", 1
    lm.goto_next();         //                            --> 2 = "file 1", 2
    lm.goto_next();         //                            --> 3 = "file 1", 3
    lm.goto_next();
    lm.set(4, 0);           // line 4 -> "file 1", 4, +0  --> 4 = "file 1", 4
    lm.goto_next();         //                            --> 5 = "file 1", 4
    lm.goto_next();         //                            --> 6 = "file 1", 4
    lm.goto_next();
    lm.set("file 1", 5, 1); // line 7 -> "file 1", 5, +1  --> 7 = "file 1", 5
    lm.goto_next();         //                            --> 8 = "file 1", 6
    lm.goto_next();
    lm.set("file 2", 1, 1); // line 9 -> "file 2", 1, +1  --> 9 = "file 2", 1
    lm.goto_next();         //                            --> 10 = "file 2", 2
    lm.goto_next();         //                            --> 11 = "file 2", 3
    lm.goto_next();
    lm.set("file 1", 7, 1); // line 12 -> "file 1", 7, +1 --> 12 = "file 1", 7
    lm.goto_next();         //                            --> 13 = "file 1", 8
    lm.goto_next();         //                            --> 14 = "file 1", 9

    // Poke tests

    // 15 = "file 3", 5
    // 16 = "file 1", 9
    file_line = lm.poke("file 3", 5);
    BOOST_REQUIRE_EQUAL(file_line, 15UL);

    // 17 = "file 1", 7
    // 18 = "file 1", 9
    file_line = lm.poke(7);
    BOOST_REQUIRE_EQUAL(file_line, 17UL);

    // Physical line check
    LOOKUP_CHECK(lm, 1, "file 1", 1UL);
    LOOKUP_CHECK(lm, 2, "file 1", 2UL);
    LOOKUP_CHECK(lm, 3, "file 1", 3UL);
    LOOKUP_CHECK(lm, 4, "file 1", 4UL);
    LOOKUP_CHECK(lm, 5, "file 1", 4UL);
    LOOKUP_CHECK(lm, 6, "file 1", 4UL);
    LOOKUP_CHECK(lm, 7, "file 1", 5UL);
    LOOKUP_CHECK(lm, 8, "file 1", 6UL);
    LOOKUP_CHECK(lm, 9, "file 2", 1UL);
    LOOKUP_CHECK(lm, 10, "file 2", 2UL);
    LOOKUP_CHECK(lm, 11, "file 2", 3UL);
    LOOKUP_CHECK(lm, 12, "file 1", 7UL);
    LOOKUP_CHECK(lm, 13, "file 1", 8UL);
    LOOKUP_CHECK(lm, 14, "file 1", 9UL);
    LOOKUP_CHECK(lm, 15, "file 3", 5UL);
    LOOKUP_CHECK(lm, 16, "file 1", 9UL);
    LOOKUP_CHECK(lm, 17, "file 1", 7UL);
    LOOKUP_CHECK(lm, 18, "file 1", 9UL);

    // Filenames check
    Linemap::Filenames correct_fns;
    correct_fns.insert("file 1");
    correct_fns.insert("file 2");
    correct_fns.insert("file 3");
    BOOST_CHECK(lm.get_filenames() == correct_fns);
}

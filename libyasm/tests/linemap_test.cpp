/*
 *  Copyright (C) 2007  Peter Johnson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>

#include "linemap.h"

#ifndef NELEMS
#define NELEMS(array)   (sizeof(array) / sizeof(array[0]))
#endif

using namespace yasm;

static std::string failed;
static std::string failmsg;

static bool
check_assertion(bool ok, const char* msg, unsigned long line)
{
    if (!ok) {
        std::ostringstream buf;
        buf << " ** F: " << msg << ", line " << line << std::endl;
        failmsg = buf.str();
        return false;
    }
    return true;
}
#define TASSERT(cond) if(!check_assertion(cond, #cond, __LINE__)) return false
#define TASSERTL(cond, l) if(!check_assertion(cond, #cond, l)) return false

static bool
test1()
{
    Linemap lm;
    bool ok;
    Bytecode* bc;
    std::string source;
    unsigned long line;

    // initial line number
    TASSERT(lm.get_current() == 1);

    // get source with no source available
    ok = lm.get_source(1, bc, source);
    TASSERT(ok == false);
    TASSERT(bc == 0);
    TASSERT(source == "");

    // add source for line 1, no bytecode
    lm.add_source(0, "line 1 source");

    // line number increment
    line = lm.goto_next();
    TASSERT(line == 2);
    TASSERT(lm.get_current() == 2);

    // add source for line 2, no bytecode
    lm.add_source(0, "line 2 source");

    // get source for line 1
    ok = lm.get_source(1, bc, source);
    TASSERT(ok == true);
    TASSERT(bc == 0);
    TASSERT(source == "line 1 source");

    // get source for line 2
    ok = lm.get_source(2, bc, source);
    TASSERT(ok == true);
    TASSERT(bc == 0);
    TASSERT(source == "line 2 source");

    return true;
}

static bool
lookup_check(const Linemap& lm, unsigned long line, const char* fn_result,
             unsigned long fl_result, unsigned long testline)
{
    std::string filename;
    unsigned long file_line;

    bool ok = lm.lookup(line, filename, file_line);
    TASSERTL(ok == true, testline);
    TASSERTL(filename == fn_result, testline);
    TASSERTL(file_line == fl_result, testline);

    return true;
}
#define LOOKUP_CHECK(lm, ln, fn, fl) \
    if(!lookup_check(lm, ln, fn, fl, __LINE__)) return false

static bool
test2()
{
    Linemap lm;
    bool ok;
    std::string filename;
    unsigned long file_line;

    // lookup with no filename info available
    ok = lm.lookup(1, filename, file_line);
    TASSERT(ok == false);
    TASSERT(filename == "unknown");
    TASSERT(file_line == 0);

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
    TASSERT(file_line == 15);

    // 17 = "file 1", 7
    // 18 = "file 1", 9
    file_line = lm.poke(7);
    TASSERT(file_line == 17);

    // Physical line check
    LOOKUP_CHECK(lm, 1, "file 1", 1);
    LOOKUP_CHECK(lm, 2, "file 1", 2);
    LOOKUP_CHECK(lm, 3, "file 1", 3);
    LOOKUP_CHECK(lm, 4, "file 1", 4);
    LOOKUP_CHECK(lm, 5, "file 1", 4);
    LOOKUP_CHECK(lm, 6, "file 1", 4);
    LOOKUP_CHECK(lm, 7, "file 1", 5);
    LOOKUP_CHECK(lm, 8, "file 1", 6);
    LOOKUP_CHECK(lm, 9, "file 2", 1);
    LOOKUP_CHECK(lm, 10, "file 2", 2);
    LOOKUP_CHECK(lm, 11, "file 2", 3);
    LOOKUP_CHECK(lm, 12, "file 1", 7);
    LOOKUP_CHECK(lm, 13, "file 1", 8);
    LOOKUP_CHECK(lm, 14, "file 1", 9);
    LOOKUP_CHECK(lm, 15, "file 3", 5);
    LOOKUP_CHECK(lm, 16, "file 1", 9);
    LOOKUP_CHECK(lm, 17, "file 1", 7);
    LOOKUP_CHECK(lm, 18, "file 1", 9);

    // Filenames check
    Linemap::Filenames correct_fns;
    correct_fns.insert("file 1");
    correct_fns.insert("file 2");
    correct_fns.insert("file 3");
    TASSERT(lm.get_filenames() == correct_fns);

    return true;
}

int
main()
{
    int nf = 0;
    bool (*tests[]) () = {
        test1,
        test2
    };
    const int nt = NELEMS(tests);
    int i;

    std::cout << "Test linemap_test: ";
    for (i=0; i<nt; i++) {
        bool pass = tests[i]();
        std::cout << (pass ? '.':'F');
        std::cout.flush();
        if (!pass) {
            nf++;
            failed += failmsg;
        }
    }

    std::cout << " +" << (nt-nf) << "-" << nf << "/" << nt;
    std::cout << " " << (100*(nt-nf)/nt) << "%" << std::endl;
    std::cout << failed;
    return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

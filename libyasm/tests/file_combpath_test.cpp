/*
 *  Copyright (C) 2006-2007  Peter Johnson
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

#include "file.h"

#ifndef NELEMS
#define NELEMS(array)   (sizeof(array) / sizeof(array[0]))
#endif

using namespace yasm;

typedef struct Test_Entry {
    /* combpath function to test */
    char* (*combpath) (const char* from, const char* to);

    /* input "from" path */
    const char* from;

    /* input "to" path */
    const char* to;

    /* correct path returned */
    const char* out;
} Test_Entry;

static const Test_Entry tests[] = {
    /* UNIX */
    {combpath_unix, "file1", "file2", "file2"},
    {combpath_unix, "./file1.ext", "./file2.ext", "file2.ext"},
    {combpath_unix, "/file1", "file2", "/file2"},
    {combpath_unix, "file1", "/file2", "/file2"},
    {combpath_unix, "/foo/file1", "../../file2", "/file2"},
    {combpath_unix, "/foo//file1", "../../file2", "/file2"},
    {combpath_unix, "foo/bar/file1", "../file2", "foo/file2"},
    {combpath_unix, "foo/bar/file1", "../../../file2", "../file2"},
    {combpath_unix, "foo/bar//file1", "../..//..//file2", "../file2"},
    {combpath_unix, "foo/bar/", "file2", "foo/bar/file2"},
    {combpath_unix, "../../file1", "../../file2", "../../../../file2"},
    {combpath_unix, "../foo/bar/../file1", "../../file2", "../foo/bar/../../../file2"},
    {combpath_unix, "/", "../file2", "/file2"},
    {combpath_unix, "../foo/", "../file2", "../file2"},
    {combpath_unix, "../foo/file1", "../../bar/file2", "../../bar/file2"},

    /* Windows */
    {combpath_win, "file1", "file2", "file2"},
    {combpath_win, "./file1.ext", "./file2.ext", "file2.ext"},
    {combpath_win, "./file1.ext", ".\\file2.ext", "file2.ext"},
    {combpath_win, ".\\file1.ext", "./file2.ext", "file2.ext"},
    {combpath_win, "/file1", "file2", "\\file2"},
    {combpath_win, "\\file1", "file2", "\\file2"},
    {combpath_win, "file1", "/file2", "\\file2"},
    {combpath_win, "file1", "\\file2", "\\file2"},
    {combpath_win, "/foo\\file1", "../../file2", "\\file2"},
    {combpath_win, "\\foo\\\\file1", "..\\../file2", "\\file2"},
    {combpath_win, "foo/bar/file1", "../file2", "foo\\file2"},
    {combpath_win, "foo/bar/file1", "../..\\../file2", "..\\file2"},
    {combpath_win, "foo/bar//file1", "../..\\\\..//file2", "..\\file2"},
    {combpath_win, "foo/bar/", "file2", "foo\\bar\\file2"},
    {combpath_win, "..\\../file1", "../..\\file2", "..\\..\\..\\..\\file2"},
    {combpath_win, "../foo/bar\\\\../file1", "../..\\file2", "..\\foo\\bar\\..\\..\\..\\file2"},
    {combpath_win, "/", "../file2", "\\file2"},
    {combpath_win, "../foo/", "../file2", "..\\file2"},
    {combpath_win, "../foo/file1", "../..\\bar\\file2", "..\\..\\bar\\file2"},
    {combpath_win, "c:/file1.ext", "./file2.ext", "c:\\file2.ext"},
    {combpath_win, "e:\\path\\to/file1.ext", ".\\file2.ext", "e:\\path\\to\\file2.ext"},
    {combpath_win, ".\\file1.ext", "g:file2.ext", "g:file2.ext"},
};

static std::string failed;
static std::string failmsg;

static bool
run_test(const Test_Entry& test)
{
    char* out;
    const char* funcname;

    if (test.combpath == &combpath_unix)
        funcname = "unix";
    else
        funcname = "win";

    out = test.combpath(test.from, test.to);

    if (strcmp(out, test.out) != 0) {
        failmsg = "combpath_";
        failmsg += funcname;
        failmsg += "(\"";
        failmsg += test.from;
        failmsg += "\", \"";
        failmsg += test.to;
        failmsg += "): expected \"";
        failmsg += test.out;
        failmsg += "\", got \"";
        failmsg += out;
        failmsg += "\"!";
        delete[] out;
        return false;
    }

    delete[] out;
    return true;
}

int
main()
{
    int nf = 0;
    int nt = NELEMS(tests);
    int i;

    std::cout << "Test file_combpath_test: ";
    for (i=0; i<nt; i++) {
        bool pass = run_test(tests[i]);
        std::cout << (pass ? '.':'F');
        std::cout.flush();
        if (!pass) {
            nf++;
            failed += " ** F: ";
            failed += failmsg;
            failed += '\n';
        }
    }

    std::cout << " +" << (nt-nf) << "-" << nf << "/" << nt;
    std::cout << " " << (100*(nt-nf)/nt) << "%" << std::endl;
    std::cout << failed;
    return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

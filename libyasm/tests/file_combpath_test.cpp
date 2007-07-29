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

#include "file.h"

using namespace yasm;

BOOST_AUTO_TEST_CASE(UnixTestCaseCurDir)
{
    char* out;

    out = combpath_unix("file1", "file2");
    BOOST_CHECK(strcmp(out, "file2") == 0);

    out = combpath_unix("./file1.ext", "./file2.ext");
    BOOST_CHECK(strcmp(out, "file2.ext") == 0);

    out = combpath_unix("foo/bar/", "file2");
    BOOST_CHECK(strcmp(out, "foo/bar/file2") == 0);
}

BOOST_AUTO_TEST_CASE(UnixTestCaseParentDir)
{
    char* out;

    out = combpath_unix("foo/bar/file1", "../file2");
    BOOST_CHECK(strcmp(out, "foo/file2") == 0);

    out = combpath_unix("foo/bar/file1", "../../../file2");
    BOOST_CHECK(strcmp(out, "../file2") == 0);

    out = combpath_unix("foo/bar//file1", "../..//..//file2");
    BOOST_CHECK(strcmp(out, "../file2") == 0);

    out = combpath_unix("../../file1", "../../file2");
    BOOST_CHECK(strcmp(out, "../../../../file2") == 0);

    out = combpath_unix("../foo/bar/../file1", "../../file2");
    BOOST_CHECK(strcmp(out, "../foo/bar/../../../file2") == 0);

    out = combpath_unix("../foo/", "../file2");
    BOOST_CHECK(strcmp(out, "../file2") == 0);

    out = combpath_unix("../foo/file1", "../../bar/file2");
    BOOST_CHECK(strcmp(out, "../../bar/file2") == 0);
}

BOOST_AUTO_TEST_CASE(UnixTestCaseRootDir)
{
    char* out;

    out = combpath_unix("/file1", "file2");
    BOOST_CHECK(strcmp(out, "/file2") == 0);

    out = combpath_unix("file1", "/file2");
    BOOST_CHECK(strcmp(out, "/file2") == 0);

    out = combpath_unix("/foo/file1", "../../file2");
    BOOST_CHECK(strcmp(out, "/file2") == 0);

    out = combpath_unix("/foo//file1", "../../file2");
    BOOST_CHECK(strcmp(out, "/file2") == 0);

    out = combpath_unix("/", "../file2");
    BOOST_CHECK(strcmp(out, "/file2") == 0);
}

BOOST_AUTO_TEST_CASE(WindowsTestCaseCurDir)
{
    char* out;

    out = combpath_win("file1", "file2");
    BOOST_CHECK(strcmp(out, "file2") == 0);

    out = combpath_win("./file1.ext", "./file2.ext");
    BOOST_CHECK(strcmp(out, "file2.ext") == 0);

    out = combpath_win("./file1.ext", ".\\file2.ext");
    BOOST_CHECK(strcmp(out, "file2.ext") == 0);

    out = combpath_win(".\\file1.ext", "./file2.ext");
    BOOST_CHECK(strcmp(out, "file2.ext") == 0);

    out = combpath_win("/file1", "file2");
    BOOST_CHECK(strcmp(out, "\\file2") == 0);

    out = combpath_win("\\file1", "file2");
    BOOST_CHECK(strcmp(out, "\\file2") == 0);

    out = combpath_win("file1", "/file2");
    BOOST_CHECK(strcmp(out, "\\file2") == 0);

    out = combpath_win("file1", "\\file2");
    BOOST_CHECK(strcmp(out, "\\file2") == 0);
}

BOOST_AUTO_TEST_CASE(WindowsTestCaseParentDir)
{
    char* out;

    out = combpath_win("/foo\\file1", "../../file2");
    BOOST_CHECK(strcmp(out, "\\file2") == 0);

    out = combpath_win("\\foo\\\\file1", "..\\../file2");
    BOOST_CHECK(strcmp(out, "\\file2") == 0);

    out = combpath_win("foo/bar/file1", "../file2");
    BOOST_CHECK(strcmp(out, "foo\\file2") == 0);

    out = combpath_win("foo/bar/file1", "../..\\../file2");
    BOOST_CHECK(strcmp(out, "..\\file2") == 0);

    out = combpath_win("foo/bar//file1", "../..\\\\..//file2");
    BOOST_CHECK(strcmp(out, "..\\file2") == 0);

    out = combpath_win("foo/bar/", "file2");
    BOOST_CHECK(strcmp(out, "foo\\bar\\file2") == 0);

    out = combpath_win("..\\../file1", "../..\\file2");
    BOOST_CHECK(strcmp(out, "..\\..\\..\\..\\file2") == 0);

    out = combpath_win("../foo/bar\\\\../file1", "../..\\file2");
    BOOST_CHECK(strcmp(out, "..\\foo\\bar\\..\\..\\..\\file2") == 0);

    out = combpath_win("../foo/", "../file2");
    BOOST_CHECK(strcmp(out, "..\\file2") == 0);

    out = combpath_win("../foo/file1", "../..\\bar\\file2");
    BOOST_CHECK(strcmp(out, "..\\..\\bar\\file2") == 0);
}

BOOST_AUTO_TEST_CASE(WindowsTestCaseRootDir)
{
    char* out;

    out = combpath_win("/", "../file2");
    BOOST_CHECK(strcmp(out, "\\file2") == 0);

    out = combpath_win("c:/file1.ext", "./file2.ext");
    BOOST_CHECK(strcmp(out, "c:\\file2.ext") == 0);

    out = combpath_win("e:\\path\\to/file1.ext", ".\\file2.ext");
    BOOST_CHECK(strcmp(out, "e:\\path\\to\\file2.ext") == 0);

    out = combpath_win(".\\file1.ext", "g:file2.ext");
    BOOST_CHECK(strcmp(out, "g:file2.ext") == 0);
}

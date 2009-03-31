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

#include "System/file.h"

using namespace yasm;

class FileCombineTestSuite : public CxxTest::TestSuite
{
public:
    std::string out;

    void testUnixTestCaseCurDir()
    {
        out = combpath_unix("file1", "file2");
        TS_ASSERT_EQUALS(out, "file2");

        out = combpath_unix("./file1.ext", "./file2.ext");
        TS_ASSERT_EQUALS(out, "file2.ext");

        out = combpath_unix("foo/bar/", "file2");
        TS_ASSERT_EQUALS(out, "foo/bar/file2");
    }

    void testUnixTestCaseParentDir()
    {
        out = combpath_unix("foo/bar/file1", "../file2");
        TS_ASSERT_EQUALS(out, "foo/file2");

        out = combpath_unix("foo/bar/file1", "../../../file2");
        TS_ASSERT_EQUALS(out, "../file2");

        out = combpath_unix("foo/bar//file1", "../..//..//file2");
        TS_ASSERT_EQUALS(out, "../file2");

        out = combpath_unix("../../file1", "../../file2");
        TS_ASSERT_EQUALS(out, "../../../../file2");

        out = combpath_unix("../foo/bar/../file1", "../../file2");
        TS_ASSERT_EQUALS(out, "../foo/bar/../../../file2");

        out = combpath_unix("../foo/", "../file2");
        TS_ASSERT_EQUALS(out, "../file2");

        out = combpath_unix("../foo/file1", "../../bar/file2");
        TS_ASSERT_EQUALS(out, "../../bar/file2");
    }

    void testUnixTestCaseRootDir()
    {
        out = combpath_unix("/file1", "file2");
        TS_ASSERT_EQUALS(out, "/file2");

        out = combpath_unix("file1", "/file2");
        TS_ASSERT_EQUALS(out, "/file2");

        out = combpath_unix("/foo/file1", "../../file2");
        TS_ASSERT_EQUALS(out, "/file2");

        out = combpath_unix("/foo//file1", "../../file2");
        TS_ASSERT_EQUALS(out, "/file2");

        out = combpath_unix("/", "../file2");
        TS_ASSERT_EQUALS(out, "/file2");
    }

    void testWindowsTestCaseCurDir()
    {
        out = combpath_win("file1", "file2");
        TS_ASSERT_EQUALS(out, "file2");

        out = combpath_win("./file1.ext", "./file2.ext");
        TS_ASSERT_EQUALS(out, "file2.ext");

        out = combpath_win("./file1.ext", ".\\file2.ext");
        TS_ASSERT_EQUALS(out, "file2.ext");

        out = combpath_win(".\\file1.ext", "./file2.ext");
        TS_ASSERT_EQUALS(out, "file2.ext");

        out = combpath_win("/file1", "file2");
        TS_ASSERT_EQUALS(out, "\\file2");

        out = combpath_win("\\file1", "file2");
        TS_ASSERT_EQUALS(out, "\\file2");

        out = combpath_win("file1", "/file2");
        TS_ASSERT_EQUALS(out, "\\file2");

        out = combpath_win("file1", "\\file2");
        TS_ASSERT_EQUALS(out, "\\file2");
    }

    void testWindowsTestCaseParentDir()
    {
        out = combpath_win("/foo\\file1", "../../file2");
        TS_ASSERT_EQUALS(out, "\\file2");

        out = combpath_win("\\foo\\\\file1", "..\\../file2");
        TS_ASSERT_EQUALS(out, "\\file2");

        out = combpath_win("foo/bar/file1", "../file2");
        TS_ASSERT_EQUALS(out, "foo\\file2");

        out = combpath_win("foo/bar/file1", "../..\\../file2");
        TS_ASSERT_EQUALS(out, "..\\file2");

        out = combpath_win("foo/bar//file1", "../..\\\\..//file2");
        TS_ASSERT_EQUALS(out, "..\\file2");

        out = combpath_win("foo/bar/", "file2");
        TS_ASSERT_EQUALS(out, "foo\\bar\\file2");

        out = combpath_win("..\\../file1", "../..\\file2");
        TS_ASSERT_EQUALS(out, "..\\..\\..\\..\\file2");

        out = combpath_win("../foo/bar\\\\../file1", "../..\\file2");
        TS_ASSERT_EQUALS(out, "..\\foo\\bar\\..\\..\\..\\file2");

        out = combpath_win("../foo/", "../file2");
        TS_ASSERT_EQUALS(out, "..\\file2");

        out = combpath_win("../foo/file1", "../..\\bar\\file2");
        TS_ASSERT_EQUALS(out, "..\\..\\bar\\file2");
    }

    void testWindowsTestCaseRootDir()
    {
        out = combpath_win("/", "../file2");
        TS_ASSERT_EQUALS(out, "\\file2");

        out = combpath_win("c:/file1.ext", "./file2.ext");
        TS_ASSERT_EQUALS(out, "c:\\file2.ext");

        out = combpath_win("c:/file1.ext", "../file2.ext");
        TS_ASSERT_EQUALS(out, "c:\\file2.ext");

        out = combpath_win("g:/path/file1.ext", "../file2.ext");
        TS_ASSERT_EQUALS(out, "g:\\file2.ext");

        out = combpath_win("g:path/file1.ext", "../file2.ext");
        TS_ASSERT_EQUALS(out, "g:file2.ext");

        out = combpath_win("g:path/file1.ext", "../../file2.ext");
        TS_ASSERT_EQUALS(out, "g:..\\file2.ext");

        out = combpath_win("g:file1.ext", "file2.ext");
        TS_ASSERT_EQUALS(out, "g:file2.ext");

        out = combpath_win("g:file1.ext", "../file2.ext");
        TS_ASSERT_EQUALS(out, "g:..\\file2.ext");

        out = combpath_win("e:\\path\\to/file1.ext", ".\\file2.ext");
        TS_ASSERT_EQUALS(out, "e:\\path\\to\\file2.ext");

        out = combpath_win(".\\file1.ext", "g:file2.ext");
        TS_ASSERT_EQUALS(out, "g:file2.ext");

        out = combpath_win(".\\file1.ext", "g:../file2.ext");
        TS_ASSERT_EQUALS(out, "g:..\\file2.ext");

        out = combpath_win(".\\file1.ext", "g:\\file2.ext");
        TS_ASSERT_EQUALS(out, "g:\\file2.ext");

        out = combpath_win("g:", "\\file2.ext");
        TS_ASSERT_EQUALS(out, "\\file2.ext");
    }
};

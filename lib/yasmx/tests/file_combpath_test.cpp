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
#include <gtest/gtest.h>

#include "yasmx/System/file.h"

struct TestValue
{
    const char* from;
    const char* to;
    const char* result;
};

class CombPathUnixTest : public ::testing::TestWithParam<TestValue> {};

TEST_P(CombPathUnixTest, Result)
{
    std::string out = yasm::CombPath_unix(GetParam().from, GetParam().to);
    EXPECT_EQ(GetParam().result, out);
}

class CombPathWinTest : public ::testing::TestWithParam<TestValue> {};

TEST_P(CombPathWinTest, Result)
{
    std::string out = yasm::CombPath_win(GetParam().from, GetParam().to);
    EXPECT_EQ(GetParam().result, out);
}

TestValue UnixCurDirValues[] =
{
    {"file1", "file2", "file2"},
    {"./file1.ext", "./file2.ext", "file2.ext"},
    {"foo/bar/", "file2", "foo/bar/file2"},
};
INSTANTIATE_TEST_CASE_P(UnixCurDir, CombPathUnixTest,
                        ::testing::ValuesIn(UnixCurDirValues));

TestValue UnixParentDirValues[] =
{
    {"foo/bar/file1", "../file2", "foo/file2"},
    {"foo/bar/file1", "../../../file2", "../file2"},
    {"foo/bar//file1", "../..//..//file2", "../file2"},
    {"../../file1", "../../file2", "../../../../file2"},
    {"../foo/bar/../file1", "../../file2", "../foo/bar/../../../file2"},
    {"../foo/", "../file2", "../file2"},
    {"../foo/file1", "../../bar/file2", "../../bar/file2"},
};
INSTANTIATE_TEST_CASE_P(UnixParentDir, CombPathUnixTest,
                        ::testing::ValuesIn(UnixParentDirValues));

TestValue UnixRootDirValues[] =
{
    {"/file1", "file2", "/file2"},
    {"file1", "/file2", "/file2"},
    {"/foo/file1", "../../file2", "/file2"},
    {"/foo//file1", "../../file2", "/file2"},
    {"/", "../file2", "/file2"},
};
INSTANTIATE_TEST_CASE_P(UnixRootDir, CombPathUnixTest,
                        ::testing::ValuesIn(UnixRootDirValues));

TestValue WinCurDirValues[] =
{
    {"file1", "file2", "file2"},
    {"./file1.ext", "./file2.ext", "file2.ext"},
    {"./file1.ext", ".\\file2.ext", "file2.ext"},
    {".\\file1.ext", "./file2.ext", "file2.ext"},
    {"/file1", "file2", "\\file2"},
    {"\\file1", "file2", "\\file2"},
    {"file1", "/file2", "\\file2"},
    {"file1", "\\file2", "\\file2"},
};
INSTANTIATE_TEST_CASE_P(WinCurDir, CombPathWinTest,
                        ::testing::ValuesIn(WinCurDirValues));

TestValue WinParentDirValues[] =
{
    {"/foo\\file1", "../../file2", "\\file2"},
    {"\\foo\\\\file1", "..\\../file2", "\\file2"},
    {"foo/bar/file1", "../file2", "foo\\file2"},
    {"foo/bar/file1", "../..\\../file2", "..\\file2"},
    {"foo/bar//file1", "../..\\\\..//file2", "..\\file2"},
    {"foo/bar/", "file2", "foo\\bar\\file2"},
    {"..\\../file1", "../..\\file2", "..\\..\\..\\..\\file2"},
    {"../foo/bar\\\\../file1", "../..\\file2", "..\\foo\\bar\\..\\..\\..\\file2"},
    {"../foo/", "../file2", "..\\file2"},
    {"../foo/file1", "../..\\bar\\file2", "..\\..\\bar\\file2"},
};
INSTANTIATE_TEST_CASE_P(WinParentDir, CombPathWinTest,
                        ::testing::ValuesIn(WinParentDirValues));

TestValue WinRootDirValues[] =
{
    {"/", "../file2", "\\file2"},
    {"c:/file1.ext", "./file2.ext", "c:\\file2.ext"},
    {"c:/file1.ext", "../file2.ext", "c:\\file2.ext"},
    {"g:/path/file1.ext", "../file2.ext", "g:\\file2.ext"},
    {"g:path/file1.ext", "../file2.ext", "g:file2.ext"},
    {"g:path/file1.ext", "../../file2.ext", "g:..\\file2.ext"},
    {"g:file1.ext", "file2.ext", "g:file2.ext"},
    {"g:file1.ext", "../file2.ext", "g:..\\file2.ext"},
    {"e:\\path\\to/file1.ext", ".\\file2.ext", "e:\\path\\to\\file2.ext"},
    {".\\file1.ext", "g:file2.ext", "g:file2.ext"},
    {".\\file1.ext", "g:../file2.ext", "g:..\\file2.ext"},
    {".\\file1.ext", "g:\\file2.ext", "g:\\file2.ext"},
    {"g:", "\\file2.ext", "\\file2.ext"},
};
INSTANTIATE_TEST_CASE_P(WinRootDir, CombPathWinTest,
                        ::testing::ValuesIn(WinRootDirValues));

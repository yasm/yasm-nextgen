//
//  Copyright (C) 2010  Peter Johnson
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
#include <string>

#include <gtest/gtest.h>

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBuffer.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Basic/FileManager.h"
#include "yasmx/Basic/SourceManager.h"
#include "yasmx/Parse/HeaderSearch.h"

#include "modules/parsers/nasm/NasmPreproc.h"
#include "modules/parsers/nasm/NasmStringParser.h"

#include "unittests/diag_mock.h"


using namespace yasm;
using namespace yasm::parser;
using namespace yasmunit;

struct strtest
{
    const char* in_str;
    std::string out_str;
    bool has_error;
    bool has_warning;
    unsigned int warnerr_no;
    int warnerr_loc;
};
const strtest NasmStringParserTestValues[] =
{
    {"''",          std::string(""),                false, false, 0, 0},
    {"\"foo\"",     std::string("foo"),             false, false, 0, 0},
    {"'foo'",       std::string("foo"),             false, false, 0, 0},
    {"`foo`",       std::string("foo"),             false, false, 0, 0},
    {"`\\0007`",    std::string("\000" "7", 2),     false, false, 0, 0},
    {"`\\xffff`",   std::string("\xff" "ff"),       false, false, 0, 0},
    {"`\\u263a`",   std::string("\xe2\x98\xba"),    false, false, 0, 0},
    {"`\\U0000263a`", std::string("\xe2\x98\xba"),  false, false, 0, 0},
    {"`\\'\\\"\\`\\\\\\?\\a\\b\\t\\n\\v\\f\\r\\e`",
     std::string("'\"`\\?\x07\x08\x09\x0a\x0b\x0c\x0d\x1a"),
     false, false, 0, 0},
    // warning cases
    {"`\\x`", std::string("x"), false, true, diag::warn_expected_hex_digit, 3},
    {"`\\y`", std::string("y"), false, true, diag::warn_unknown_escape, 2},
    {"`\\8`", std::string("8"), false, true, diag::warn_unknown_escape, 2},
    // error cases
    {"`\\uabcg`", std::string(), true, false, diag::err_unicode_escape_requires_hex, 6},
    {"`\\Uabc`", std::string(), true, false, diag::err_unicode_escape_requires_hex, 6},
    {"`\\Uabcdefg0`", std::string(), true, false, diag::err_unicode_escape_requires_hex, 9},
};

class NasmStringParserTest : public ::testing::TestWithParam<strtest>
{};

INSTANTIATE_TEST_CASE_P(NasmStringParserTests, NasmStringParserTest,
                        ::testing::ValuesIn(NasmStringParserTestValues));

TEST_P(NasmStringParserTest, StringTest)
{
    FileManager fmgr;
    MockDiagnosticClient mock_client;
    Diagnostic diags(&mock_client);
    SourceManager smgr(diags);
    diags.setSourceManager(&smgr);
    HeaderSearch headers(fmgr);
    NasmPreproc pp(diags, smgr, headers);

    // create a main "file" consisting solely of the test string
    llvm::StringRef in_str(GetParam().in_str);
    smgr.createMainFileIDForMemBuffer(
        llvm::MemoryBuffer::getMemBuffer(in_str, "<string>"));
    pp.EnterMainSourceFile();
    SourceLocation sof = smgr.getLocForStartOfFile(smgr.getMainFileID());

    using ::testing::_;
    if (GetParam().has_error || GetParam().has_warning)
    {
        // expect an error/warning of the specified type and location
        FullSourceLoc
            full_loc(sof.getFileLocWithOffset(GetParam().warnerr_loc), smgr);
        using ::testing::Property;
        using ::testing::Eq;
        EXPECT_CALL(mock_client, HandleDiagnostic(_,
            AllOf(Property(&DiagnosticInfo::getID, Eq(GetParam().warnerr_no)),
                  Property(&DiagnosticInfo::getLocation, Eq(full_loc)))));
    }
    else
    {
        // expect no diagnostic calls
        EXPECT_CALL(mock_client, HandleDiagnostic(_, _))
            .Times(0);
    }

    // parse the string
    NasmStringParser str(in_str, sof, pp);

    // check for errors first
    ASSERT_EQ(GetParam().has_error, str.hadError());
    if (str.hadError())
        return;

    // check string result
    EXPECT_EQ(GetParam().out_str, str.getString());
}

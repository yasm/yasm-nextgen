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

#include "modules/parsers/gas/GasPreproc.h"
#include "modules/parsers/gas/GasStringParser.h"

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
const strtest GasStringParserTestValues[] =
{
    // strings
    {"\"\"",        std::string(""),                false, false, 0, 0},
    {"\"foo\"",     std::string("foo"),             false, false, 0, 0},
    {"\"\\0007\"",  std::string("\000" "7", 2),     false, false, 0, 0},
    {"\"\\7\"",     std::string("\007"),            false, false, 0, 0},
    {"\"\\xffff\"", std::string("\xff"),            false, false, 0, 0},
    {"\"\\\"\\\\\\b\\t\\n\\v\\f\\r\"",
     std::string("\"\\\x08\x09\x0a\x0b\x0c\x0d"),  false, false, 0, 0},
    // warning cases
    {"\"\\x\"", std::string("\000", 1), false, true,
     diag::warn_expected_hex_digit, 3},
    {"\"\\y\"", std::string("y"), false, true, diag::warn_unknown_escape, 2},
    {"\"\\8\"", std::string("8"), false, true, diag::warn_unknown_escape, 2},
    // character constants
    {"'\\003",  std::string("\003"),    false, false, 0, 0},
    {"'\\x55",  std::string("\x55"),    false, false, 0, 0},
    {"'\\b",    std::string("\x08"),    false, false, 0, 0},
    {"'\\\\",   std::string("\\"),      false, false, 0, 0},
    {"' ",      std::string(" "),       false, false, 0, 0},
    {"'\\ ",    std::string(" "), false, true, diag::warn_unknown_escape, 2},
};

class GasStringParserTest : public ::testing::TestWithParam<strtest>
{};

INSTANTIATE_TEST_CASE_P(GasStringParserTests, GasStringParserTest,
                        ::testing::ValuesIn(GasStringParserTestValues));

TEST_P(GasStringParserTest, StringTest)
{
    MockDiagnosticConsumer mock_consumer;
    llvm::IntrusiveRefCntPtr<DiagnosticIDs> diagids(new DiagnosticIDs);
    DiagnosticsEngine diags(diagids, &mock_consumer, false);
    FileSystemOptions opts;
    FileManager fmgr(opts);
    SourceManager smgr(diags, fmgr);
    diags.setSourceManager(&smgr);
    HeaderSearch headers(fmgr);
    GasPreproc pp(diags, smgr, headers);

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
            full_loc(sof.getLocWithOffset(GetParam().warnerr_loc), smgr);
        using ::testing::Property;
        using ::testing::Eq;
        EXPECT_CALL(mock_consumer, HandleDiagnostic(_,
            AllOf(Property(&Diagnostic::getID, Eq(GetParam().warnerr_no)),
                  Property(&Diagnostic::getLocation, Eq(full_loc)))));
    }
    else
    {
        // expect no diagnostic calls
        EXPECT_CALL(mock_consumer, HandleDiagnostic(_, _))
            .Times(0);
    }

    // parse the string
    GasStringParser str(in_str, sof, pp);

    // check for errors first
    ASSERT_EQ(GetParam().has_error, str.hadError());
    if (str.hadError())
        return;

    // check string result
    EXPECT_EQ(GetParam().out_str, str.getString());
}

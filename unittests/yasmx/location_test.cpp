//
//  Copyright (C) 2008  Peter Johnson
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

#include "yasmx/Basic/FileManager.h"
#include "yasmx/Basic/SourceManager.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Expr.h"
#include "yasmx/Location.h"
#include "yasmx/Location_util.h"

#include "unittests/diag_mock.h"
#include "unittests/unittest_util.h"


using namespace yasm;

class LocationTest : public testing::Test
{
protected:
    Bytecode bc1, bc2;
    Location loc1, loc2, loc3;

    virtual void SetUp()
    {
        loc1.bc = &bc1;
        loc1.off = 10;
        loc2.bc = &bc1;
        loc2.off = 40;
        loc3.bc = &bc2;
        loc3.off = 5;

        bc1.setOffset(100);
        bc2.setOffset(200);
    }
};

TEST_F(LocationTest, GetOffset)
{
    EXPECT_EQ(140U, loc2.getOffset());
}

TEST_F(LocationTest, CalcDistNoBC)
{
    IntNum dist;
    EXPECT_TRUE(CalcDistNoBC(loc1, loc2, &dist));
    EXPECT_EQ(30, dist.getInt());
    EXPECT_TRUE(CalcDistNoBC(loc2, loc1, &dist));
    EXPECT_EQ(-30, dist.getInt());
    EXPECT_FALSE(CalcDistNoBC(loc1, loc3, &dist));
    EXPECT_FALSE(CalcDistNoBC(loc3, loc2, &dist));
}

TEST_F(LocationTest, CalcDist)
{
    IntNum dist;
    EXPECT_TRUE(CalcDist(loc1, loc2, &dist));
    EXPECT_EQ(30, dist.getInt());
    EXPECT_TRUE(CalcDist(loc2, loc1, &dist));
    EXPECT_EQ(-30, dist.getInt());
    EXPECT_TRUE(CalcDist(loc1, loc3, &dist));
    EXPECT_EQ(95, dist.getInt());
    EXPECT_TRUE(CalcDist(loc3, loc2, &dist));
    EXPECT_EQ(-65, dist.getInt());
}

TEST_F(LocationTest, SimplifyCalcDistNoBC)
{
    yasmunit::MockDiagnosticConsumer mock_consumer;
    llvm::IntrusiveRefCntPtr<DiagnosticIDs> diagids(new DiagnosticIDs);
    DiagnosticsEngine diags(diagids, &mock_consumer, false);
    FileSystemOptions opts;
    FileManager fmgr(opts);
    SourceManager smgr(diags, fmgr);
    diags.setSourceManager(&smgr);

    Expr e;

    e = SUB(loc2, loc1);
    SimplifyCalcDistNoBC(e, diags);
    EXPECT_EQ("30", String::Format(e));

    e = ADD(10, SUB(loc2, loc1));
    SimplifyCalcDistNoBC(e, diags);
    EXPECT_EQ("40", String::Format(e));

    e = SUB(loc3, loc1);
    SimplifyCalcDistNoBC(e, diags);
    EXPECT_EQ("{LOC}+({LOC}*-1)", String::Format(e));
}

TEST_F(LocationTest, SimplifyCalcDist)
{
    yasmunit::MockDiagnosticConsumer mock_consumer;
    llvm::IntrusiveRefCntPtr<DiagnosticIDs> diagids(new DiagnosticIDs);
    DiagnosticsEngine diags(diagids, &mock_consumer, false);
    FileSystemOptions opts;
    FileManager fmgr(opts);
    SourceManager smgr(diags, fmgr);
    diags.setSourceManager(&smgr);

    Expr e;

    e = SUB(loc2, loc1);
    SimplifyCalcDist(e, diags);
    EXPECT_EQ("30", String::Format(e));

    e = ADD(10, SUB(loc2, loc1));
    SimplifyCalcDist(e, diags);
    EXPECT_EQ("40", String::Format(e));

    e = SUB(loc3, loc1);
    SimplifyCalcDist(e, diags);
    EXPECT_EQ("95", String::Format(e));

    e = ADD(SUB(loc3, loc1), SUB(loc2, loc1));
    SimplifyCalcDist(e, diags);
    EXPECT_EQ("125", String::Format(e));

    e = SUB(SUB(loc3, loc1), SUB(loc2, loc1));
    SimplifyCalcDist(e, diags);
    EXPECT_EQ("65", String::Format(e));

    e = MUL(SUB(loc2, loc1), SUB(loc3, loc2));
    SimplifyCalcDist(e, diags);
    EXPECT_EQ("1950", String::Format(e));
}

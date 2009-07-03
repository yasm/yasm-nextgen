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
#include <cxxtest/TestSuite.h>

#include "yasmx/Expr.h"
#include "yasmx/Location.h"
#include "yasmx/Location_util.h"

class LocationTestSuite : public CxxTest::TestSuite
{
public:
    Bytecode bc1, bc2;
    Location loc1, loc2, loc3;

    void setUp()
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

    void testGetOffset()
    {
        TS_ASSERT_EQUALS(loc2.getOffset(), 140U);
    }

    void testCalcDistNoBC()
    {
        IntNum dist;
        TS_ASSERT_EQUALS(CalcDistNoBC(loc1, loc2, &dist), true);
        TS_ASSERT_EQUALS(dist, 30);
        TS_ASSERT_EQUALS(CalcDistNoBC(loc2, loc1, &dist), true);
        TS_ASSERT_EQUALS(dist, -30);
        TS_ASSERT_EQUALS(CalcDistNoBC(loc1, loc3, &dist), false);
        TS_ASSERT_EQUALS(CalcDistNoBC(loc3, loc2, &dist), false);
    }

    void test_CalcDist()
    {
        IntNum dist;
        TS_ASSERT_EQUALS(CalcDist(loc1, loc2, &dist), true);
        TS_ASSERT_EQUALS(dist, 30);
        TS_ASSERT_EQUALS(CalcDist(loc2, loc1, &dist), true);
        TS_ASSERT_EQUALS(dist, -30);
        TS_ASSERT_EQUALS(CalcDist(loc1, loc3, &dist), true);
        TS_ASSERT_EQUALS(dist, 95);
        TS_ASSERT_EQUALS(CalcDist(loc3, loc2, &dist), true);
        TS_ASSERT_EQUALS(dist, -65);
    }

    void test_simplify_CalcDistNoBC()
    {
        Expr e;

        e = SUB(loc2, loc1);
        SimplifyCalcDistNoBC(e);
        TS_ASSERT_EQUALS(String::Format(e), "30");

        e = ADD(10, SUB(loc2, loc1));
        SimplifyCalcDistNoBC(e);
        TS_ASSERT_EQUALS(String::Format(e), "40");

        e = SUB(loc3, loc1);
        SimplifyCalcDistNoBC(e);
        TS_ASSERT_EQUALS(String::Format(e), "{LOC}+({LOC}*-1)");
    }

    void test_SimplifyCalcDist()
    {
        Expr e;

        e = SUB(loc2, loc1);
        SimplifyCalcDist(e);
        TS_ASSERT_EQUALS(String::Format(e), "30");

        e = ADD(10, SUB(loc2, loc1));
        SimplifyCalcDist(e);
        TS_ASSERT_EQUALS(String::Format(e), "40");

        e = SUB(loc3, loc1);
        SimplifyCalcDist(e);
        TS_ASSERT_EQUALS(String::Format(e), "95");

        e = ADD(SUB(loc3, loc1), SUB(loc2, loc1));
        SimplifyCalcDist(e);
        TS_ASSERT_EQUALS(String::Format(e), "125");

        e = SUB(SUB(loc3, loc1), SUB(loc2, loc1));
        SimplifyCalcDist(e);
        TS_ASSERT_EQUALS(String::Format(e), "65");

        e = MUL(SUB(loc2, loc1), SUB(loc3, loc2));
        SimplifyCalcDist(e);
        TS_ASSERT_EQUALS(String::Format(e), "1950");
    }
};

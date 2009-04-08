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

        bc1.set_offset(100);
        bc2.set_offset(200);
    }

    void test_get_offset()
    {
        TS_ASSERT_EQUALS(loc2.get_offset(), 140U);
    }

    void test_calc_dist_no_bc()
    {
        IntNum dist;
        TS_ASSERT_EQUALS(calc_dist_no_bc(loc1, loc2, &dist), true);
        TS_ASSERT_EQUALS(dist, 30);
        TS_ASSERT_EQUALS(calc_dist_no_bc(loc2, loc1, &dist), true);
        TS_ASSERT_EQUALS(dist, -30);
        TS_ASSERT_EQUALS(calc_dist_no_bc(loc1, loc3, &dist), false);
        TS_ASSERT_EQUALS(calc_dist_no_bc(loc3, loc2, &dist), false);
    }

    void test_calc_dist()
    {
        IntNum dist;
        TS_ASSERT_EQUALS(calc_dist(loc1, loc2, &dist), true);
        TS_ASSERT_EQUALS(dist, 30);
        TS_ASSERT_EQUALS(calc_dist(loc2, loc1, &dist), true);
        TS_ASSERT_EQUALS(dist, -30);
        TS_ASSERT_EQUALS(calc_dist(loc1, loc3, &dist), true);
        TS_ASSERT_EQUALS(dist, 95);
        TS_ASSERT_EQUALS(calc_dist(loc3, loc2, &dist), true);
        TS_ASSERT_EQUALS(dist, -65);
    }

    void test_simplify_calc_dist_no_bc()
    {
        Expr e;

        e = SUB(loc2, loc1);
        simplify_calc_dist_no_bc(e);
        TS_ASSERT_EQUALS(String::format(e), "30");

        e = ADD(10, SUB(loc2, loc1));
        simplify_calc_dist_no_bc(e);
        TS_ASSERT_EQUALS(String::format(e), "40");

        e = SUB(loc3, loc1);
        simplify_calc_dist_no_bc(e);
        TS_ASSERT_EQUALS(String::format(e), "{LOC}+({LOC}*-1)");
    }

    void test_simplify_calc_dist()
    {
        Expr e;

        e = SUB(loc2, loc1);
        simplify_calc_dist(e);
        TS_ASSERT_EQUALS(String::format(e), "30");

        e = ADD(10, SUB(loc2, loc1));
        simplify_calc_dist(e);
        TS_ASSERT_EQUALS(String::format(e), "40");

        e = SUB(loc3, loc1);
        simplify_calc_dist(e);
        TS_ASSERT_EQUALS(String::format(e), "95");

        e = ADD(SUB(loc3, loc1), SUB(loc2, loc1));
        simplify_calc_dist(e);
        TS_ASSERT_EQUALS(String::format(e), "125");

        e = SUB(SUB(loc3, loc1), SUB(loc2, loc1));
        simplify_calc_dist(e);
        TS_ASSERT_EQUALS(String::format(e), "65");

        e = MUL(SUB(loc2, loc1), SUB(loc3, loc2));
        simplify_calc_dist(e);
        TS_ASSERT_EQUALS(String::format(e), "1950");
    }
};

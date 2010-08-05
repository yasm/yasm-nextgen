//
//  Copyright (C) 2007  Peter Johnson
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

#include <cstdio>

#include "llvm/Support/raw_ostream.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/IntNum.h"

using namespace yasm;

TEST(IntNumOperatorOverloadTest, Equal)
{
    // Check comparison operators first; we'll use TS_ASSERT_EQUALS directly
    // on intnums later, so it's critical these work.

    // == operator
    EXPECT_TRUE(IntNum(5) == IntNum(5));
    EXPECT_TRUE(IntNum(5) == 5);
    EXPECT_TRUE(5 == IntNum(5));
    EXPECT_FALSE(IntNum(5) == IntNum(7));
    EXPECT_FALSE(IntNum(5) == 7);
    EXPECT_FALSE(5 == IntNum(7));

    // != operator
    EXPECT_FALSE(IntNum(5) != IntNum(5));
    EXPECT_FALSE(IntNum(5) != 5);
    EXPECT_FALSE(5 != IntNum(5));
    EXPECT_TRUE(IntNum(5) != IntNum(7));
    EXPECT_TRUE(IntNum(5) != 7);
    EXPECT_TRUE(5 != IntNum(7));
}

TEST(IntNumOperatorOverloadTest, Comparison)
{
    // < operator
    EXPECT_TRUE(IntNum(5) < IntNum(7));
    EXPECT_TRUE(IntNum(5) < 7);
    EXPECT_TRUE(5 < IntNum(7));
    EXPECT_FALSE(IntNum(7) < IntNum(5));
    EXPECT_FALSE(IntNum(7) < 5);
    EXPECT_FALSE(7 < IntNum(5));

    // > operator
    EXPECT_TRUE(IntNum(7) > IntNum(5));
    EXPECT_TRUE(IntNum(7) > 5);
    EXPECT_TRUE(7 > IntNum(5));
    EXPECT_FALSE(IntNum(5) > IntNum(7));
    EXPECT_FALSE(IntNum(5) > 7);
    EXPECT_FALSE(5 > IntNum(7));

    // <= operator
    EXPECT_TRUE(IntNum(5) <= IntNum(5));
    EXPECT_TRUE(IntNum(5) <= 5);
    EXPECT_TRUE(5 <= IntNum(5));
    EXPECT_TRUE(IntNum(5) <= IntNum(7));
    EXPECT_TRUE(IntNum(5) <= 7);
    EXPECT_TRUE(5 <= IntNum(7));
    EXPECT_FALSE(IntNum(7) <= IntNum(5));
    EXPECT_FALSE(IntNum(7) <= 5);
    EXPECT_FALSE(7 <= IntNum(5));

    // >= operator
    EXPECT_TRUE(IntNum(5) >= IntNum(5));
    EXPECT_TRUE(IntNum(5) >= 5);
    EXPECT_TRUE(5 >= IntNum(5));
    EXPECT_TRUE(IntNum(7) >= IntNum(5));
    EXPECT_TRUE(IntNum(7) >= 5);
    EXPECT_TRUE(7 >= IntNum(5));
    EXPECT_FALSE(IntNum(5) >= IntNum(7));
    EXPECT_FALSE(IntNum(5) >= 7);
    EXPECT_FALSE(5 >= IntNum(7));
}

TEST(IntNumOperatorOverloadTest, Binary)
{
    EXPECT_EQ(7, (IntNum(5)+2).getInt());
    EXPECT_EQ(7, (2+IntNum(5)).getInt());
    EXPECT_EQ(3, (IntNum(5)-2).getInt());
    EXPECT_EQ(-3, (2-IntNum(5)).getInt());
    EXPECT_EQ(10, (IntNum(5)*2).getInt());
    EXPECT_EQ(10, (2*IntNum(5)).getInt());
    EXPECT_EQ(2, (IntNum(5)/2).getInt());
    EXPECT_EQ(2, (5/IntNum(2)).getInt());
    EXPECT_EQ(1, (IntNum(5)%2).getInt());
    EXPECT_EQ(1, (5%IntNum(2)).getInt());
    EXPECT_EQ(4, (IntNum(7)^3).getInt());
    EXPECT_EQ(4, (7^IntNum(3)).getInt());
    EXPECT_EQ(2, (IntNum(10)&7).getInt());
    EXPECT_EQ(2, (10&IntNum(7)).getInt());
    EXPECT_EQ(11, (IntNum(10)|3).getInt());
    EXPECT_EQ(11, (10|IntNum(3)).getInt());
    EXPECT_EQ(2, (IntNum(10)>>2).getInt());
    EXPECT_EQ(2, (10>>IntNum(2)).getInt());
    EXPECT_EQ(40, (IntNum(10)<<2).getInt());
    EXPECT_EQ(40, (10<<IntNum(2)).getInt());
}

TEST(IntNumOperatorOverloadTest, Unary)
{
    EXPECT_EQ(-5, (-IntNum(5)).getInt());
    EXPECT_EQ(5, (-IntNum(-5)).getInt());
    EXPECT_EQ(5, (+IntNum(5)).getInt());
    EXPECT_EQ(-5, (+IntNum(-5)).getInt());

    EXPECT_EQ(10, ((~IntNum(5))&0xF).getInt());

    EXPECT_TRUE(!IntNum(0));
    EXPECT_FALSE(!IntNum(5));
}

TEST(IntNumOperatorOverloadTest, BinaryAssignment)
{
    IntNum x(0);
    ASSERT_EQ(6, (x+=6).getInt());
    ASSERT_EQ(2, (x-=4).getInt());
    ASSERT_EQ(16, (x*=8).getInt());
    ASSERT_EQ(8, (x/=2).getInt());
    ASSERT_EQ(2, (x%=3).getInt());
    ASSERT_EQ(3, (x^=1).getInt());
    ASSERT_EQ(2, (x&=2).getInt());
    ASSERT_EQ(7, (x|=5).getInt());
    ASSERT_EQ(1, (x>>=2).getInt());
    ASSERT_EQ(4, (x<<=2).getInt());
}

TEST(IntNumOperatorOverloadTest, IncDec)
{
    IntNum x(5);
    ASSERT_EQ(6, (++x).getInt());
    ASSERT_EQ(6, (x++).getInt());
    ASSERT_EQ(7, x.getInt());
    ASSERT_EQ(6, (--x).getInt());
    ASSERT_EQ(6, (x--).getInt());
    ASSERT_EQ(5, x.getInt());
}

class IntNumStreamOutputTest : public ::testing::TestWithParam<long> {};


TEST_P(IntNumStreamOutputTest, Small)
{
    long v = GetParam();
    std::string s;
    llvm::raw_string_ostream oss(s);
    char golden[100];
    IntNum x(v);

    x.Print(oss, 8, true, false, 64);
    if (v < 0)
        sprintf(golden, "-000000000000%010lo", (-v)&0x3fffffff);
    else
        sprintf(golden, "000000000000%010lo", v&0x3fffffff);
    EXPECT_EQ(golden, oss.str());

    s.clear();
    x.Print(oss, 16, false, false, 64);
    if (v < 0)
        sprintf(golden, "-00000000%08lX", (-v)&0xffffffff);
    else
        sprintf(golden, "00000000%08lX", v);
    EXPECT_EQ(golden, oss.str());

    s.clear();
    x.Print(oss, 16, true, false, 64);
    if (v < 0)
        sprintf(golden, "-00000000%08lx", (-v)&0xffffffff);
    else
        sprintf(golden, "00000000%08lx", v);
    EXPECT_EQ(golden, oss.str());

    s.clear();
    x.Print(oss);
    sprintf(golden, "%ld", v);
    EXPECT_EQ(golden, oss.str());
}

TEST_P(IntNumStreamOutputTest, Big)
{
    long v = GetParam();
    std::string s;
    llvm::raw_string_ostream oss(s);
    char golden[100];
    IntNum x(v);
    IntNum y;

    y = (x<<33) + x;
    s.clear();
    y.Print(oss, 8, true, false, 64);
    if (v < 0)
        sprintf(golden, "-0%010lo0%010lo", (-v)&0x3fffffff, (-v)&0x3fffffff);
    else
        sprintf(golden, "0%010lo0%010lo", v, v);
    EXPECT_EQ(golden, oss.str());

    y = (x<<32) + x;
    s.clear();
    y.Print(oss, 16, false, false, 64);
    if (v < 0)
        sprintf(golden, "-%08lX%08lX", (-v)&0xffffffff, (-v)&0xffffffff);
    else
        sprintf(golden, "%08lX%08lX", v, v);
    EXPECT_EQ(golden, oss.str());

    y = (x<<32) + x;
    s.clear();
    y.Print(oss, 16, true, false, 64);
    if (v < 0)
        sprintf(golden, "-%08lx%08lx", (-v)&0xffffffff, (-v)&0xffffffff);
    else
        sprintf(golden, "%08lx%08lx", v, v);
    EXPECT_EQ(golden, oss.str());

    y = x * 1000 * 1000 * 1000;
    s.clear();
    y.Print(oss, 10);
    if (v == 0)
        sprintf(golden, "0");
    else
        sprintf(golden, "%ld000000000", v);
    EXPECT_EQ(golden, oss.str());
}

INSTANTIATE_TEST_CASE_P(IntNumStreamOutputTests, IntNumStreamOutputTest,
                        ::testing::Range(-1000L, 1001L));

// parameters are size N (in bits), right shift (in bits), range type
// range type = 0: (0, 2^N-1) range
// range type = 1: (-2^(N-1), 2^(N-1)-1) range
// range type = 2: (-2^(N-1), 2^N-1) range
TEST(IntNumOkSizeTest, Zero)
{
    IntNum intn = 0;
    EXPECT_TRUE( intn.isOkSize(8, 0, 0));
    EXPECT_TRUE( intn.isOkSize(8, 1, 0));
    EXPECT_TRUE( intn.isOkSize(8, 0, 1));
    EXPECT_TRUE( intn.isOkSize(8, 1, 1));
    EXPECT_TRUE( intn.isOkSize(8, 0, 2));
    EXPECT_TRUE( intn.isOkSize(8, 1, 2));
}

TEST(IntNumOkSizeTest, Neg1)
{
    IntNum intn = -1;
    EXPECT_FALSE(intn.isOkSize(8, 0, 0));  // <0
    EXPECT_FALSE(intn.isOkSize(8, 1, 0));  // <0
    EXPECT_TRUE( intn.isOkSize(8, 0, 1));
    EXPECT_TRUE( intn.isOkSize(8, 1, 1));
    EXPECT_TRUE( intn.isOkSize(8, 0, 2));
    EXPECT_TRUE( intn.isOkSize(8, 1, 2));
}

TEST(IntNumOkSizeTest, SmallPos)
{
    IntNum intn = 1;
    EXPECT_TRUE( intn.isOkSize(8, 0, 0));
    EXPECT_TRUE( intn.isOkSize(8, 1, 0));
    EXPECT_TRUE( intn.isOkSize(8, 0, 1));
    EXPECT_TRUE( intn.isOkSize(8, 1, 1));
    EXPECT_TRUE( intn.isOkSize(8, 0, 2));
    EXPECT_TRUE( intn.isOkSize(8, 1, 2));

    intn = 2;
    EXPECT_TRUE( intn.isOkSize(8, 0, 0));
    EXPECT_TRUE( intn.isOkSize(8, 1, 0));
    EXPECT_TRUE( intn.isOkSize(8, 0, 1));
    EXPECT_TRUE( intn.isOkSize(8, 1, 1));
    EXPECT_TRUE( intn.isOkSize(8, 0, 2));
    EXPECT_TRUE( intn.isOkSize(8, 1, 2));
}

TEST(IntNumOkSizeTest, Boundary8)
{
    // 8-bit boundary conditions (signed and unsigned)
    IntNum intn = -128;
    EXPECT_TRUE( intn.isOkSize(8, 0, 1));
    EXPECT_TRUE( intn.isOkSize(8, 0, 2));

    intn = -129;
    EXPECT_FALSE(intn.isOkSize(8, 0, 1));
    EXPECT_FALSE(intn.isOkSize(8, 0, 2));

    intn = 127;
    EXPECT_TRUE( intn.isOkSize(8, 0, 1));

    intn = 128;
    EXPECT_TRUE(!intn.isOkSize(8, 0, 1));

    intn = 255;
    EXPECT_TRUE( intn.isOkSize(8, 0, 0));
    EXPECT_TRUE( intn.isOkSize(8, 0, 2));

    intn = 256;
    EXPECT_FALSE(intn.isOkSize(8, 0, 0));
    EXPECT_FALSE(intn.isOkSize(8, 0, 2));
}

TEST(IntNumOkSizeTest, Boundary16)
{
    // 16-bit boundary conditions (signed and unsigned)
    IntNum intn = -32768;
    EXPECT_TRUE( intn.isOkSize(16, 0, 1));
    EXPECT_TRUE( intn.isOkSize(16, 0, 2));

    intn = -32769;
    EXPECT_FALSE(intn.isOkSize(16, 0, 1));
    EXPECT_FALSE(intn.isOkSize(16, 0, 2));

    intn = 32767;
    EXPECT_TRUE( intn.isOkSize(16, 0, 1));

    intn = 32768;
    EXPECT_FALSE(intn.isOkSize(16, 0, 1));

    intn = 65535;
    EXPECT_TRUE( intn.isOkSize(16, 0, 0));
    EXPECT_TRUE( intn.isOkSize(16, 0, 2));

    intn = 65536;
    EXPECT_FALSE(intn.isOkSize(16, 0, 0));
    EXPECT_FALSE(intn.isOkSize(16, 0, 2));
}

TEST(IntNumOkSizeTest, Boundary31)
{
    // 31-bit boundary conditions (signed and unsigned)
    IntNum intn = 1; intn <<= 30; intn = -intn;
    EXPECT_TRUE( intn.isOkSize(31, 0, 1));
    EXPECT_TRUE( intn.isOkSize(31, 0, 2));
    EXPECT_TRUE( intn.isOkSize(32, 0, 1));
    EXPECT_TRUE( intn.isOkSize(32, 0, 2));

    intn = 1; intn <<= 30; intn = -intn; --intn;
    EXPECT_FALSE(intn.isOkSize(31, 0, 1));
    EXPECT_FALSE(intn.isOkSize(31, 0, 2));
    EXPECT_TRUE( intn.isOkSize(32, 0, 1));
    EXPECT_TRUE( intn.isOkSize(32, 0, 2));

    intn = 1; intn <<= 30; --intn;
    EXPECT_TRUE( intn.isOkSize(31, 0, 1));
    EXPECT_TRUE( intn.isOkSize(32, 0, 1));

    intn = 1; intn <<= 30;
    EXPECT_FALSE(intn.isOkSize(31, 0, 1));
    EXPECT_TRUE( intn.isOkSize(32, 0, 1));

    intn = 1; intn <<= 31; --intn;
    EXPECT_TRUE( intn.isOkSize(31, 0, 0));
    EXPECT_TRUE( intn.isOkSize(31, 0, 2));
    EXPECT_TRUE( intn.isOkSize(32, 0, 0));
    EXPECT_TRUE( intn.isOkSize(32, 0, 2));

    intn = 1; intn <<= 31;
    EXPECT_FALSE(intn.isOkSize(31, 0, 0));
    EXPECT_FALSE(intn.isOkSize(31, 0, 2));
    EXPECT_TRUE( intn.isOkSize(32, 0, 0));
    EXPECT_TRUE( intn.isOkSize(32, 0, 2));
}

TEST(IntNumOkSizeTest, Boundary32)
{
    // 32-bit boundary conditions (signed and unsigned)
    IntNum intn = 1; intn <<= 31; intn = -intn;
    EXPECT_TRUE( intn.isOkSize(32, 0, 1));
    EXPECT_TRUE( intn.isOkSize(32, 0, 2));

    intn = 1; intn <<= 31; intn = -intn; --intn;
    EXPECT_FALSE(intn.isOkSize(32, 0, 1));
    EXPECT_FALSE(intn.isOkSize(32, 0, 2));

    intn = 1; intn <<= 31; --intn;
    EXPECT_TRUE( intn.isOkSize(32, 0, 1));

    intn = 1; intn <<= 31;
    EXPECT_FALSE(intn.isOkSize(32, 0, 1));

    intn = 1; intn <<= 32; --intn;
    EXPECT_TRUE( intn.isOkSize(32, 0, 0));
    EXPECT_TRUE( intn.isOkSize(32, 0, 2));

    intn = 1; intn <<= 32;
    EXPECT_FALSE(intn.isOkSize(32, 0, 0));
    EXPECT_FALSE(intn.isOkSize(32, 0, 2));
}

TEST(IntNumOkSizeTest, Boundary63)
{
    // 63-bit boundary conditions (signed and unsigned)
    IntNum intn = 1; intn <<= 62; intn = -intn;
    EXPECT_TRUE( intn.isOkSize(63, 0, 1));
    EXPECT_TRUE( intn.isOkSize(63, 0, 2));
    EXPECT_TRUE( intn.isOkSize(64, 0, 1));
    EXPECT_TRUE( intn.isOkSize(64, 0, 2));

    intn = 1; intn <<= 62; intn = -intn; --intn;
    EXPECT_FALSE(intn.isOkSize(63, 0, 1));
    EXPECT_FALSE(intn.isOkSize(63, 0, 2));
    EXPECT_TRUE( intn.isOkSize(64, 0, 1));
    EXPECT_TRUE( intn.isOkSize(64, 0, 2));

    intn = 1; intn <<= 62; --intn;
    EXPECT_TRUE( intn.isOkSize(63, 0, 1));
    EXPECT_TRUE( intn.isOkSize(64, 0, 1));

    intn = 1; intn <<= 62;
    EXPECT_FALSE(intn.isOkSize(63, 0, 1));
    EXPECT_TRUE( intn.isOkSize(64, 0, 1));

    intn = 1; intn <<= 63; --intn;
    EXPECT_TRUE( intn.isOkSize(63, 0, 0));
    EXPECT_TRUE( intn.isOkSize(63, 0, 2));
    EXPECT_TRUE( intn.isOkSize(64, 0, 0));
    EXPECT_TRUE( intn.isOkSize(64, 0, 2));

    intn = 1; intn <<= 63;
    EXPECT_FALSE(intn.isOkSize(63, 0, 0));
    EXPECT_FALSE(intn.isOkSize(63, 0, 2));
    EXPECT_TRUE( intn.isOkSize(64, 0, 0));
    EXPECT_TRUE( intn.isOkSize(64, 0, 2));
}

TEST(IntNumOkSizeTest, Boundary64)
{
    // 64-bit boundary conditions (signed and unsigned)
    IntNum intn = 1; intn <<= 63; intn = -intn;
    EXPECT_TRUE( intn.isOkSize(64, 0, 1));
    EXPECT_TRUE( intn.isOkSize(64, 0, 2));

    intn = 1; intn <<= 63; intn = -intn; --intn;
    EXPECT_FALSE(intn.isOkSize(64, 0, 1));
    EXPECT_FALSE(intn.isOkSize(64, 0, 2));

    intn = 1; intn <<= 63; --intn;
    EXPECT_TRUE( intn.isOkSize(64, 0, 1));

    intn = 1; intn <<= 63;
    EXPECT_FALSE(intn.isOkSize(64, 0, 1));

    intn = 1; intn <<= 64; --intn;
    EXPECT_TRUE( intn.isOkSize(64, 0, 0));
    EXPECT_TRUE( intn.isOkSize(64, 0, 2));

    intn = 1; intn <<= 64;
    EXPECT_FALSE(intn.isOkSize(64, 0, 0));
    EXPECT_FALSE(intn.isOkSize(64, 0, 2));
}

TEST(IntNumOkSizeTest, RightShift)
{
    // with rshift
    EXPECT_TRUE(IntNum(255).isOkSize(8, 1, 1));
    EXPECT_FALSE(IntNum(256).isOkSize(8, 1, 1));
    EXPECT_TRUE(IntNum(-256).isOkSize(8, 1, 1));
    EXPECT_FALSE(IntNum(-257).isOkSize(8, 1, 1));
    EXPECT_TRUE(IntNum(511).isOkSize(8, 1, 2));
    EXPECT_FALSE(IntNum(512).isOkSize(8, 1, 2));
    EXPECT_TRUE(IntNum(-256).isOkSize(8, 1, 2));
    EXPECT_FALSE(IntNum(-257).isOkSize(8, 1, 2));
}

struct GetSizedLongTestValue
{
    long val;
    unsigned int destsize;
    unsigned int valsize;
    int shift;
    bool bigendian;
    unsigned char inbuf[4];
    unsigned char outbuf[4];
};
static GetSizedLongTestValue GetSizedLongTestValues[] =
{
    // full value should overwrite completely
    {0x1234, 2, 16, 0, false, {0x00, 0x00}, {0x34, 0x12}},
    {0x1234, 2, 16, 0, false, {0xff, 0xff}, {0x34, 0x12}},
    // single byte
    {0x1234, 2, 8, 0, false, {0x55, 0xaa}, {0x34, 0xaa}},
    // bit-level masking
    {0x1234, 2, 4, 0, false, {0xff, 0x55}, {0xf4, 0x55}},
    {0x1234, 2, 12, 0, false, {0xff, 0xee}, {0x34, 0xe2}},
    {0x1234, 2, 14, 0, false, {0xff, 0xff}, {0x34, 0xd2}},
    // right shifts
    {0x1234, 2, 16, -4, false, {0xff, 0xff}, {0x23, 0x01}},
    {0x1234, 2, 12, -4, false, {0xff, 0xff}, {0x23, 0xf1}},
    // left shifts preserve what was to the right
    {0x1234, 3, 16, 4, false, {0xff, 0xff, 0xff}, {0x4f, 0x23, 0xf1}},
    {0x1234, 3, 12, 4, false, {0xff, 0xff, 0xff}, {0x4f, 0x23, 0xff}},
    {0x1234, 2, 16, 4, false, {0xff, 0xff, 0x00}, {0x4f, 0x23, 0x00}},
    {0x1234, 2, 12, 4, false, {0xff, 0xff, 0x00}, {0x4f, 0x23, 0x00}},
    {0x1234, 3, 16, 8, false, {0xff, 0xff, 0xff}, {0xff, 0x34, 0x12}},
    {0x1234, 3, 12, 12, false, {0x55, 0xaa, 0xff}, {0x55, 0x4a, 0x23}},
    //
    // negative numbers
    //
    {-1, 2, 16, 0, false, {0x00, 0x00}, {0xff, 0xff}},
    {-1, 2, 12, 0, false, {0x00, 0x00}, {0xff, 0x0f}},
    {-1, 2, 12, 4, false, {0x55, 0xaa}, {0xf5, 0xff}},
};

class IntNumGetSizedTest : public ::testing::TestWithParam<GetSizedLongTestValue> {};

TEST_P(IntNumGetSizedTest, Overwrite)
{
    GetSizedLongTestValue test = GetParam();
    IntNum intn = test.val;
    Bytes buf;
    buf.resize(test.destsize);
    memcpy(&buf[0], test.inbuf, test.destsize);
    if (test.bigendian)
        buf.setBigEndian();
    else
        buf.setLittleEndian();
    Overwrite(buf, intn, test.valsize, test.shift, 0);
    for (unsigned int i=0; i<test.destsize; ++i)
        EXPECT_EQ(test.outbuf[i], buf[i]);
}

INSTANTIATE_TEST_CASE_P(IntNumGetSizedTests, IntNumGetSizedTest,
                        ::testing::ValuesIn(GetSizedLongTestValues));
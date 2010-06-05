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

#include "yasmx/Bytes_util.h"
#include "yasmx/IntNum.h"

using namespace yasm;

struct LTest
{
    long val;
    unsigned char expect[4];
};

struct ULTest
{
    unsigned long val;
    unsigned char expect[4];
};

/// 8-bit signed //////////////////////////////////////////////////////////////
class Write8SignedTest : public ::testing::TestWithParam<LTest>
{
protected:
    Bytes bytes;
    void Check()
    {
        ASSERT_EQ(1U, bytes.size());
        EXPECT_EQ(GetParam().expect[0], bytes[0]);
    }
};

TEST_P(Write8SignedTest, Int)
{
    Write8(bytes, GetParam().val);
    Check();
}

TEST_P(Write8SignedTest, IntNum)
{
    Write8(bytes, IntNum(GetParam().val));
    Check();
}

static const LTest Write8SignedValues[] =
{
    {0,         {0x00}},
    {-1,        {0xff}},
    {-127,      {0x81}},
    {-128,      {0x80}},
    {-129,      {0x7f}},
    {-254,      {0x02}},
    {-255,      {0x01}},
    {-256,      {0x00}},
    {1,         {0x01}},
    {127,       {0x7f}},
    {128,       {0x80}},
    {129,       {0x81}},
    {254,       {0xfe}},
    {255,       {0xff}},
    {256,       {0x00}},
};

INSTANTIATE_TEST_CASE_P(Write8SignedTests, Write8SignedTest,
                        ::testing::ValuesIn(Write8SignedValues));

/// 8-bit unsigned ////////////////////////////////////////////////////////////
class Write8UnsignedTest : public ::testing::TestWithParam<ULTest>
{
protected:
    Bytes bytes;
    void Check()
    {
        ASSERT_EQ(1U, bytes.size());
        EXPECT_EQ(GetParam().expect[0], bytes[0]);
    }
};

TEST_P(Write8UnsignedTest, Int)
{
    Write8(bytes, GetParam().val);
    Check();
}

TEST_P(Write8UnsignedTest, IntNum)
{
    Write8(bytes, IntNum(GetParam().val));
    Check();
}

static const ULTest Write8UnsignedValues[] =
{
    {0,         {0x00}},
    {1,         {0x01}},
    {127,       {0x7f}},
    {128,       {0x80}},
    {129,       {0x81}},
    {254,       {0xfe}},
    {255,       {0xff}},
    {256,       {0x00}},
};

INSTANTIATE_TEST_CASE_P(Write8UnsignedTests, Write8UnsignedTest,
                        ::testing::ValuesIn(Write8UnsignedValues));

/// 16-bit signed /////////////////////////////////////////////////////////////
class Write16SignedTest : public ::testing::TestWithParam<LTest>
{
protected:
    Bytes bytes;
    void CheckLE()
    {
        ASSERT_EQ(2U, bytes.size());
        EXPECT_EQ(GetParam().expect[0], bytes[0]);
        EXPECT_EQ(GetParam().expect[1], bytes[1]);
    }
    void CheckBE()
    {
        ASSERT_EQ(2U, bytes.size());
        EXPECT_EQ(GetParam().expect[1], bytes[0]);
        EXPECT_EQ(GetParam().expect[0], bytes[1]);
    }
};

TEST_P(Write16SignedTest, IntLE)
{
    bytes.setLittleEndian();
    Write16(bytes, GetParam().val);
    CheckLE();
}

TEST_P(Write16SignedTest, IntNumLE)
{
    bytes.setLittleEndian();
    Write16(bytes, IntNum(GetParam().val));
    CheckLE();
}

TEST_P(Write16SignedTest, IntBE)
{
    bytes.setBigEndian();
    Write16(bytes, GetParam().val);
    CheckBE();
}

TEST_P(Write16SignedTest, IntNumBE)
{
    bytes.setBigEndian();
    Write16(bytes, IntNum(GetParam().val));
    CheckBE();
}

static const LTest Write16SignedValues[] =
{
    {0,         {0x00, 0x00}},
    {-1,        {0xff, 0xff}},
    {-255,      {0x01, 0xff}},
    {-256,      {0x00, 0xff}},
    {-257,      {0xff, 0xfe}},
    {-32767,    {0x01, 0x80}},
    {-32768,    {0x00, 0x80}},
    {-32769,    {0xff, 0x7f}},
    {1,         {0x01, 0x00}},
    {254,       {0xfe, 0x00}},
    {255,       {0xff, 0x00}},
    {256,       {0x00, 0x01}},
    {32766,     {0xfe, 0x7f}},
    {32767,     {0xff, 0x7f}},
    {32768,     {0x00, 0x80}},
};

INSTANTIATE_TEST_CASE_P(Write16SignedTests, Write16SignedTest,
                        ::testing::ValuesIn(Write16SignedValues));

/// 16-bit unsigned ///////////////////////////////////////////////////////////
class Write16UnsignedTest : public ::testing::TestWithParam<ULTest>
{
protected:
    Bytes bytes;
    void CheckLE()
    {
        ASSERT_EQ(2U, bytes.size());
        EXPECT_EQ(GetParam().expect[0], bytes[0]);
        EXPECT_EQ(GetParam().expect[1], bytes[1]);
    }
    void CheckBE()
    {
        ASSERT_EQ(2U, bytes.size());
        EXPECT_EQ(GetParam().expect[1], bytes[0]);
        EXPECT_EQ(GetParam().expect[0], bytes[1]);
    }
};

TEST_P(Write16UnsignedTest, IntLE)
{
    bytes.setLittleEndian();
    Write16(bytes, GetParam().val);
    CheckLE();
}

TEST_P(Write16UnsignedTest, IntNumLE)
{
    bytes.setLittleEndian();
    Write16(bytes, IntNum(GetParam().val));
    CheckLE();
}

TEST_P(Write16UnsignedTest, IntBE)
{
    bytes.setBigEndian();
    Write16(bytes, GetParam().val);
    CheckBE();
}

TEST_P(Write16UnsignedTest, IntNumBE)
{
    bytes.setBigEndian();
    Write16(bytes, IntNum(GetParam().val));
    CheckBE();
}

static const ULTest Write16UnsignedValues[] =
{
    {0,         {0x00, 0x00}},
    {1,         {0x01, 0x00}},
    {254,       {0xfe, 0x00}},
    {255,       {0xff, 0x00}},
    {256,       {0x00, 0x01}},
    {65534,     {0xfe, 0xff}},
    {65535,     {0xff, 0xff}},
    {65536,     {0x00, 0x00}},
};

INSTANTIATE_TEST_CASE_P(Write16UnsignedTests, Write16UnsignedTest,
                        ::testing::ValuesIn(Write16UnsignedValues));


/// 32-bit signed /////////////////////////////////////////////////////////////
class Write32SignedTest : public ::testing::TestWithParam<LTest>
{
protected:
    Bytes bytes;
    void CheckLE()
    {
        ASSERT_EQ(4U, bytes.size());
        EXPECT_EQ(GetParam().expect[0], bytes[0]);
        EXPECT_EQ(GetParam().expect[1], bytes[1]);
        EXPECT_EQ(GetParam().expect[2], bytes[2]);
        EXPECT_EQ(GetParam().expect[3], bytes[3]);
    }
    void CheckBE()
    {
        ASSERT_EQ(4U, bytes.size());
        EXPECT_EQ(GetParam().expect[3], bytes[0]);
        EXPECT_EQ(GetParam().expect[2], bytes[1]);
        EXPECT_EQ(GetParam().expect[1], bytes[2]);
        EXPECT_EQ(GetParam().expect[0], bytes[3]);
    }
};

TEST_P(Write32SignedTest, IntLE)
{
    bytes.setLittleEndian();
    Write32(bytes, GetParam().val);
    CheckLE();
}

TEST_P(Write32SignedTest, IntNumLE)
{
    bytes.setLittleEndian();
    Write32(bytes, IntNum(GetParam().val));
    CheckLE();
}

TEST_P(Write32SignedTest, IntBE)
{
    bytes.setBigEndian();
    Write32(bytes, GetParam().val);
    CheckBE();
}

TEST_P(Write32SignedTest, IntNumBE)
{
    bytes.setBigEndian();
    Write32(bytes, IntNum(GetParam().val));
    CheckBE();
}

static const LTest Write32SignedValues[] =
{
    {0,             {0x00, 0x00, 0x00, 0x00}},
    {-1,            {0xff, 0xff, 0xff, 0xff}},
    {-2147483647L,  {0x01, 0x00, 0x00, 0x80}},
    {-2147483648L,  {0x00, 0x00, 0x00, 0x80}},
    {1,             {0x01, 0x00, 0x00, 0x00}},
    {2147483646L,   {0xfe, 0xff, 0xff, 0x7f}},
    {2147483647L,   {0xff, 0xff, 0xff, 0x7f}},
};

INSTANTIATE_TEST_CASE_P(Write32SignedTests, Write32SignedTest,
                        ::testing::ValuesIn(Write32SignedValues));

/// 32-bit unsigned ///////////////////////////////////////////////////////////
class Write32UnsignedTest : public ::testing::TestWithParam<ULTest>
{
protected:
    Bytes bytes;
    void CheckLE()
    {
        ASSERT_EQ(4U, bytes.size());
        EXPECT_EQ(GetParam().expect[0], bytes[0]);
        EXPECT_EQ(GetParam().expect[1], bytes[1]);
        EXPECT_EQ(GetParam().expect[2], bytes[2]);
        EXPECT_EQ(GetParam().expect[3], bytes[3]);
    }
    void CheckBE()
    {
        ASSERT_EQ(4U, bytes.size());
        EXPECT_EQ(GetParam().expect[3], bytes[0]);
        EXPECT_EQ(GetParam().expect[2], bytes[1]);
        EXPECT_EQ(GetParam().expect[1], bytes[2]);
        EXPECT_EQ(GetParam().expect[0], bytes[3]);
    }
};

TEST_P(Write32UnsignedTest, IntLE)
{
    bytes.setLittleEndian();
    Write32(bytes, GetParam().val);
    CheckLE();
}

TEST_P(Write32UnsignedTest, IntNumLE)
{
    bytes.setLittleEndian();
    Write32(bytes, IntNum(GetParam().val));
    CheckLE();
}

TEST_P(Write32UnsignedTest, IntBE)
{
    bytes.setBigEndian();
    Write32(bytes, GetParam().val);
    CheckBE();
}

TEST_P(Write32UnsignedTest, IntNumBE)
{
    bytes.setBigEndian();
    Write32(bytes, IntNum(GetParam().val));
    CheckBE();
}

static const ULTest Write32UnsignedValues[] =
{
    {0,             {0x00, 0x00, 0x00, 0x00}},
    {1,             {0x01, 0x00, 0x00, 0x00}},
    {65534,         {0xfe, 0xff, 0x00, 0x00}},
    {65535,         {0xff, 0xff, 0x00, 0x00}},
    {65536,         {0x00, 0x00, 0x01, 0x00}},
    {4294967294UL,  {0xfe, 0xff, 0xff, 0xff}},
    {4294967295UL,  {0xff, 0xff, 0xff, 0xff}},
};

INSTANTIATE_TEST_CASE_P(Write32UnsignedTests, Write32UnsignedTest,
                        ::testing::ValuesIn(Write32UnsignedValues));

/// 64-bit 32-bit signed //////////////////////////////////////////////////////
class Write6432SignedTest : public ::testing::TestWithParam<LTest>
{
protected:
    Bytes bytes;
    void CheckLE()
    {
        ASSERT_EQ(8U, bytes.size());
        EXPECT_EQ(GetParam().expect[0], bytes[0]);
        EXPECT_EQ(GetParam().expect[1], bytes[1]);
        EXPECT_EQ(GetParam().expect[2], bytes[2]);
        EXPECT_EQ(GetParam().expect[3], bytes[3]);
        EXPECT_EQ((GetParam().expect[3]&0x80) ? 0xff : 0x00, bytes[4]);
        EXPECT_EQ((GetParam().expect[3]&0x80) ? 0xff : 0x00, bytes[5]);
        EXPECT_EQ((GetParam().expect[3]&0x80) ? 0xff : 0x00, bytes[6]);
        EXPECT_EQ((GetParam().expect[3]&0x80) ? 0xff : 0x00, bytes[7]);
    }
    void CheckBE()
    {
        ASSERT_EQ(8U, bytes.size());
        EXPECT_EQ((GetParam().expect[3]&0x80) ? 0xff : 0x00, bytes[0]);
        EXPECT_EQ((GetParam().expect[3]&0x80) ? 0xff : 0x00, bytes[1]);
        EXPECT_EQ((GetParam().expect[3]&0x80) ? 0xff : 0x00, bytes[2]);
        EXPECT_EQ((GetParam().expect[3]&0x80) ? 0xff : 0x00, bytes[3]);
        EXPECT_EQ(GetParam().expect[3], bytes[4]);
        EXPECT_EQ(GetParam().expect[2], bytes[5]);
        EXPECT_EQ(GetParam().expect[1], bytes[6]);
        EXPECT_EQ(GetParam().expect[0], bytes[7]);
    }
};

TEST_P(Write6432SignedTest, IntLE)
{
    bytes.setLittleEndian();
    Write64(bytes, GetParam().val);
    CheckLE();
}

TEST_P(Write6432SignedTest, IntNumLE)
{
    bytes.setLittleEndian();
    Write64(bytes, IntNum(GetParam().val));
    CheckLE();
}

TEST_P(Write6432SignedTest, IntBE)
{
    bytes.setBigEndian();
    Write64(bytes, GetParam().val);
    CheckBE();
}

TEST_P(Write6432SignedTest, IntNumBE)
{
    bytes.setBigEndian();
    Write64(bytes, IntNum(GetParam().val));
    CheckBE();
}

static const LTest Write6432SignedValues[] =
{
    {0,             {0x00, 0x00, 0x00, 0x00}},
    {-1,            {0xff, 0xff, 0xff, 0xff}},
    {-2147483647,   {0x01, 0x00, 0x00, 0x80}},
    {-2147483648,   {0x00, 0x00, 0x00, 0x80}},
    {1,             {0x01, 0x00, 0x00, 0x00}},
    {2147483646,    {0xfe, 0xff, 0xff, 0x7f}},
    {2147483647,    {0xff, 0xff, 0xff, 0x7f}},
};

INSTANTIATE_TEST_CASE_P(Write6432SignedTests, Write6432SignedTest,
                        ::testing::ValuesIn(Write6432SignedValues));

/// 64-bit 32-bit unsigned ////////////////////////////////////////////////////
class Write6432UnsignedTest : public ::testing::TestWithParam<ULTest>
{
protected:
    Bytes bytes;
    void CheckLE()
    {
        ASSERT_EQ(8U, bytes.size());
        EXPECT_EQ(GetParam().expect[0], bytes[0]);
        EXPECT_EQ(GetParam().expect[1], bytes[1]);
        EXPECT_EQ(GetParam().expect[2], bytes[2]);
        EXPECT_EQ(GetParam().expect[3], bytes[3]);
        EXPECT_EQ(0x00, bytes[4]);
        EXPECT_EQ(0x00, bytes[5]);
        EXPECT_EQ(0x00, bytes[6]);
        EXPECT_EQ(0x00, bytes[7]);
    }
    void CheckBE()
    {
        ASSERT_EQ(8U, bytes.size());
        EXPECT_EQ(0x00, bytes[0]);
        EXPECT_EQ(0x00, bytes[1]);
        EXPECT_EQ(0x00, bytes[2]);
        EXPECT_EQ(0x00, bytes[3]);
        EXPECT_EQ(GetParam().expect[3], bytes[4]);
        EXPECT_EQ(GetParam().expect[2], bytes[5]);
        EXPECT_EQ(GetParam().expect[1], bytes[6]);
        EXPECT_EQ(GetParam().expect[0], bytes[7]);
    }
};

TEST_P(Write6432UnsignedTest, IntLE)
{
    bytes.setLittleEndian();
    Write64(bytes, GetParam().val);
    CheckLE();
}

TEST_P(Write6432UnsignedTest, IntNumLE)
{
    bytes.setLittleEndian();
    Write64(bytes, IntNum(GetParam().val));
    CheckLE();
}

TEST_P(Write6432UnsignedTest, IntBE)
{
    bytes.setBigEndian();
    Write64(bytes, GetParam().val);
    CheckBE();
}

TEST_P(Write6432UnsignedTest, IntNumBE)
{
    bytes.setBigEndian();
    Write64(bytes, IntNum(GetParam().val));
    CheckBE();
}

static const ULTest Write6432UnsignedValues[] =
{
    {0,         {0x00, 0x00, 0x00, 0x00}},
    {1,         {0x01, 0x00, 0x00, 0x00}},
    {65534,     {0xfe, 0xff, 0x00, 0x00}},
    {65535,     {0xff, 0xff, 0x00, 0x00}},
    {65536,     {0x00, 0x00, 0x01, 0x00}},
    {4294967294,{0xfe, 0xff, 0xff, 0xff}},
    {4294967295,{0xff, 0xff, 0xff, 0xff}},
};

INSTANTIATE_TEST_CASE_P(Write6432UnsignedTests, Write6432UnsignedTest,
                        ::testing::ValuesIn(Write6432UnsignedValues));

/// 64-bit ////////////////////////////////////////////////////////////////////
class Write64Test : public ::testing::TestWithParam<int>
{
protected:
    Bytes bytes;
    IntNum intn;
    void SetUp()
    {
        intn = 1;
        intn <<= GetParam();
    }
    void CheckLE()
    {
        ASSERT_EQ(8U, bytes.size());
        int i = GetParam();
        EXPECT_EQ((i < 8) ? (1<<i) : 0x00, (int)bytes[0]);
        EXPECT_EQ((i >= 8 && i < 16) ? (1<<(i-8)) : 0x00, (int)bytes[1]);
        EXPECT_EQ((i >= 16 && i < 24) ? (1<<(i-16)) : 0x00, (int)bytes[2]);
        EXPECT_EQ((i >= 24 && i < 32) ? (1<<(i-24)) : 0x00, (int)bytes[3]);
        EXPECT_EQ((i >= 32 && i < 40) ? (1<<(i-32)) : 0x00, (int)bytes[4]);
        EXPECT_EQ((i >= 40 && i < 48) ? (1<<(i-40)) : 0x00, (int)bytes[5]);
        EXPECT_EQ((i >= 48 && i < 56) ? (1<<(i-48)) : 0x00, (int)bytes[6]);
        EXPECT_EQ((i >= 56 && i < 64) ? (1<<(i-56)) : 0x00, (int)bytes[7]);
    }
    void CheckBE()
    {
        ASSERT_EQ(8U, bytes.size());
        int i = GetParam();
        EXPECT_EQ((i < 8) ? (1<<i) : 0x00, (int)bytes[7]);
        EXPECT_EQ((i >= 8 && i < 16) ? (1<<(i-8)) : 0x00, (int)bytes[6]);
        EXPECT_EQ((i >= 16 && i < 24) ? (1<<(i-16)) : 0x00, (int)bytes[5]);
        EXPECT_EQ((i >= 24 && i < 32) ? (1<<(i-24)) : 0x00, (int)bytes[4]);
        EXPECT_EQ((i >= 32 && i < 40) ? (1<<(i-32)) : 0x00, (int)bytes[3]);
        EXPECT_EQ((i >= 40 && i < 48) ? (1<<(i-40)) : 0x00, (int)bytes[2]);
        EXPECT_EQ((i >= 48 && i < 56) ? (1<<(i-48)) : 0x00, (int)bytes[1]);
        EXPECT_EQ((i >= 56 && i < 64) ? (1<<(i-56)) : 0x00, (int)bytes[0]);
    }
    void CheckInvLE()
    {
        ASSERT_EQ(8U, bytes.size());
        int i = GetParam();
        EXPECT_EQ((i < 8) ? ~(1<<i) & 0xff : 0xff, (int)bytes[0]);
        EXPECT_EQ((i >= 8 && i < 16) ? ~(1<<(i-8)) & 0xff : 0xff, (int)bytes[1]);
        EXPECT_EQ((i >= 16 && i < 24) ? ~(1<<(i-16)) & 0xff : 0xff, (int)bytes[2]);
        EXPECT_EQ((i >= 24 && i < 32) ? ~(1<<(i-24)) & 0xff : 0xff, (int)bytes[3]);
        EXPECT_EQ((i >= 32 && i < 40) ? ~(1<<(i-32)) & 0xff : 0xff, (int)bytes[4]);
        EXPECT_EQ((i >= 40 && i < 48) ? ~(1<<(i-40)) & 0xff : 0xff, (int)bytes[5]);
        EXPECT_EQ((i >= 48 && i < 56) ? ~(1<<(i-48)) & 0xff : 0xff, (int)bytes[6]);
        EXPECT_EQ((i >= 56 && i < 64) ? ~(1<<(i-56)) & 0xff : 0xff, (int)bytes[7]);
    }
    void CheckInvBE()
    {
        ASSERT_EQ(8U, bytes.size());
        int i = GetParam();
        EXPECT_EQ((i < 8) ? ~(1<<i) & 0xff : 0xff, (int)bytes[7]);
        EXPECT_EQ((i >= 8 && i < 16) ? ~(1<<(i-8)) & 0xff : 0xff, (int)bytes[6]);
        EXPECT_EQ((i >= 16 && i < 24) ? ~(1<<(i-16)) & 0xff : 0xff, (int)bytes[5]);
        EXPECT_EQ((i >= 24 && i < 32) ? ~(1<<(i-24)) & 0xff : 0xff, (int)bytes[4]);
        EXPECT_EQ((i >= 32 && i < 40) ? ~(1<<(i-32)) & 0xff : 0xff, (int)bytes[3]);
        EXPECT_EQ((i >= 40 && i < 48) ? ~(1<<(i-40)) & 0xff : 0xff, (int)bytes[2]);
        EXPECT_EQ((i >= 48 && i < 56) ? ~(1<<(i-48)) & 0xff : 0xff, (int)bytes[1]);
        EXPECT_EQ((i >= 56 && i < 64) ? ~(1<<(i-56)) & 0xff : 0xff, (int)bytes[0]);
    }
};

TEST_P(Write64Test, LE)
{
    bytes.setLittleEndian();
    Write64(bytes, intn);
    CheckLE();
}

TEST_P(Write64Test, BE)
{
    bytes.setBigEndian();
    Write64(bytes, intn);
    CheckBE();
}

TEST_P(Write64Test, InvLE)
{
    intn.Calc(Op::NOT);
    bytes.setLittleEndian();
    Write64(bytes, intn);
    CheckInvLE();
}

TEST_P(Write64Test, InvBE)
{
    intn.Calc(Op::NOT);
    bytes.setBigEndian();
    Write64(bytes, intn);
    CheckInvBE();
}

INSTANTIATE_TEST_CASE_P(Write64Tests, Write64Test, ::testing::Range(0, 64));

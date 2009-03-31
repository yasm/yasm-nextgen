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
#include <cxxtest/TestSuite.h>

#include <sstream>
#include <iomanip>
#include <cstdio>

#include "yasmx/IntNum.h"
#include "yasmx/IntNum_iomanip.h"

using namespace yasm;

class IntNumTestSuite : public CxxTest::TestSuite
{
public:
    void testEqualOperatorOverload()
    {
        // Check comparison operators first; we'll use TS_ASSERT_EQUALS directly
        // on intnums later, so it's critical these work.

        // == operator
        TS_ASSERT_EQUALS(IntNum(5) == IntNum(5), true);
        TS_ASSERT_EQUALS(IntNum(5) == 5, true);
        TS_ASSERT_EQUALS(5 == IntNum(5), true);
        TS_ASSERT_EQUALS(IntNum(5) == IntNum(7), false);
        TS_ASSERT_EQUALS(IntNum(5) == 7, false);
        TS_ASSERT_EQUALS(5 == IntNum(7), false);

        // != operator
        TS_ASSERT_EQUALS(IntNum(5) != IntNum(5), false);
        TS_ASSERT_EQUALS(IntNum(5) != 5, false);
        TS_ASSERT_EQUALS(5 != IntNum(5), false);
        TS_ASSERT_EQUALS(IntNum(5) != IntNum(7), true);
        TS_ASSERT_EQUALS(IntNum(5) != 7, true);
        TS_ASSERT_EQUALS(5 != IntNum(7), true);
    }

    void testComparisonOperatorOverload()
    {
        // < operator
        TS_ASSERT(IntNum(5) < IntNum(7));
        TS_ASSERT(IntNum(5) < 7);
        TS_ASSERT(5 < IntNum(7));
        TS_ASSERT(!(IntNum(7) < IntNum(5)));
        TS_ASSERT(!(IntNum(7) < 5));
        TS_ASSERT(!(7 < IntNum(5)));

        // > operator
        TS_ASSERT(IntNum(7) > IntNum(5));
        TS_ASSERT(IntNum(7) > 5);
        TS_ASSERT(7 > IntNum(5));
        TS_ASSERT(!(IntNum(5) > IntNum(7)));
        TS_ASSERT(!(IntNum(5) > 7));
        TS_ASSERT(!(5 > IntNum(7)));

        // <= operator
        TS_ASSERT(IntNum(5) <= IntNum(5));
        TS_ASSERT(IntNum(5) <= 5);
        TS_ASSERT(5 <= IntNum(5));
        TS_ASSERT(IntNum(5) <= IntNum(7));
        TS_ASSERT(IntNum(5) <= 7);
        TS_ASSERT(5 <= IntNum(7));
        TS_ASSERT(!(IntNum(7) <= IntNum(5)));
        TS_ASSERT(!(IntNum(7) <= 5));
        TS_ASSERT(!(7 <= IntNum(5)));

        // >= operator
        TS_ASSERT(IntNum(5) >= IntNum(5));
        TS_ASSERT(IntNum(5) >= 5);
        TS_ASSERT(5 >= IntNum(5));
        TS_ASSERT(IntNum(7) >= IntNum(5));
        TS_ASSERT(IntNum(7) >= 5);
        TS_ASSERT(7 >= IntNum(5));
        TS_ASSERT(!(IntNum(5) >= IntNum(7)));
        TS_ASSERT(!(IntNum(5) >= 7));
        TS_ASSERT(!(5 >= IntNum(7)));
    }

    void testBinaryOperatorOverload()
    {
        TS_ASSERT_EQUALS(IntNum(5)+2, 7);
        TS_ASSERT_EQUALS(2+IntNum(5), 7);
        TS_ASSERT_EQUALS(IntNum(5)-2, 3);
        TS_ASSERT_EQUALS(2-IntNum(5), -3);
        TS_ASSERT_EQUALS(IntNum(5)*2, 10);
        TS_ASSERT_EQUALS(2*IntNum(5), 10);
        TS_ASSERT_EQUALS(IntNum(5)/2, 2);
        TS_ASSERT_EQUALS(5/IntNum(2), 2);
        TS_ASSERT_EQUALS(IntNum(5)%2, 1);
        TS_ASSERT_EQUALS(5%IntNum(2), 1);
        TS_ASSERT_EQUALS(IntNum(7)^3, 4);
        TS_ASSERT_EQUALS(7^IntNum(3), 4);
        TS_ASSERT_EQUALS(IntNum(10)&7, 2);
        TS_ASSERT_EQUALS(10&IntNum(7), 2);
        TS_ASSERT_EQUALS(IntNum(10)|3, 11);
        TS_ASSERT_EQUALS(10|IntNum(3), 11);
        TS_ASSERT_EQUALS(IntNum(10)>>2, 2);
        TS_ASSERT_EQUALS(10>>IntNum(2), 2);
        TS_ASSERT_EQUALS(IntNum(10)<<2, 40);
        TS_ASSERT_EQUALS(10<<IntNum(2), 40);
    }

    void testUnaryOperatorOverload()
    {
        TS_ASSERT_EQUALS(-IntNum(5), -5);
        TS_ASSERT_EQUALS(-IntNum(-5), 5);
        TS_ASSERT_EQUALS(+IntNum(5), 5);
        TS_ASSERT_EQUALS(+IntNum(-5), -5);

        TS_ASSERT_EQUALS((~IntNum(5))&0xF, 10);

        TS_ASSERT_EQUALS(!IntNum(0), true);
        TS_ASSERT_EQUALS(!IntNum(5), false);
    }

    void testBinaryAssignmentOperatorOverload()
    {
        IntNum x(0);
        TS_ASSERT_EQUALS(x+=6, 6);
        TS_ASSERT_EQUALS(x-=4, 2);
        TS_ASSERT_EQUALS(x*=8, 16);
        TS_ASSERT_EQUALS(x/=2, 8);
        TS_ASSERT_EQUALS(x%=3, 2);
        TS_ASSERT_EQUALS(x^=1, 3);
        TS_ASSERT_EQUALS(x&=2, 2);
        TS_ASSERT_EQUALS(x|=5, 7);
        TS_ASSERT_EQUALS(x>>=2, 1);
        TS_ASSERT_EQUALS(x<<=2, 4);
    }

    void testIncDecOperatorOverload()
    {
        IntNum x(5);
        TS_ASSERT_EQUALS(++x, 6);
        TS_ASSERT_EQUALS(x++, 6);
        TS_ASSERT_EQUALS(x, 7);
        TS_ASSERT_EQUALS(--x, 6);
        TS_ASSERT_EQUALS(x--, 6);
        TS_ASSERT_EQUALS(x, 5);
    }

    void testStreamOutput()
    {
        for (long v=-1000; v<=1000; ++v)
        {
            std::ostringstream oss;
            char golden[100];

            oss << set_intnum_bits(64);

            // Test small values
            IntNum x(v);

            oss << std::oct << x;
            if (v < 0)
                sprintf(golden, "777777777777%010lo", v&0x3fffffff);
            else
                sprintf(golden, "000000000000%010lo", v&0x3fffffff);
            TS_ASSERT_EQUALS(oss.str(), golden);

            oss.str("");
            oss << std::hex << std::uppercase << x;
            if (v < 0)
                sprintf(golden, "FFFFFFFF%08lX", v&0xffffffff);
            else
                sprintf(golden, "00000000%08lX", v);
            TS_ASSERT_EQUALS(oss.str(), golden);

            oss.str("");
            oss << std::hex << std::nouppercase << x;
            if (v < 0)
                sprintf(golden, "ffffffff%08lx", v&0xffffffff);
            else
                sprintf(golden, "00000000%08lx", v);
            TS_ASSERT_EQUALS(oss.str(), golden);

            oss.str("");
            oss << std::dec << x;
            sprintf(golden, "%ld", v);
            TS_ASSERT_EQUALS(oss.str(), golden);

            // Test big values
            IntNum y;

            y = (x<<33) + x;
            oss.str("");
            oss << std::oct << y;
            if (v < 0)
                sprintf(golden, "7%010lo7%010lo", (v-1)&0x3fffffff, v&0x3fffffff);
            else
                sprintf(golden, "0%010lo0%010lo", v, v);
            TS_ASSERT_EQUALS(oss.str(), golden);

            y = (x<<32) + x;
            oss.str("");
            oss << std::hex << std::uppercase << y;
            if (v < 0)
                sprintf(golden, "%08lX%08lX", (v-1)&0xffffffff, v&0xffffffff);
            else
                sprintf(golden, "%08lX%08lX", v, v);
            TS_ASSERT_EQUALS(oss.str(), golden);

            y = (x<<32) + x;
            oss.str("");
            oss << std::hex << std::nouppercase << y;
            if (v < 0)
                sprintf(golden, "%08lx%08lx", (v-1)&0xffffffff, v&0xffffffff);
            else
                sprintf(golden, "%08lx%08lx", v, v);
            TS_ASSERT_EQUALS(oss.str(), golden);

            y = x * 1000 * 1000 * 1000;
            oss.str("");
            oss << std::dec << y;
            if (v == 0)
                sprintf(golden, "0");
            else
                sprintf(golden, "%ld000000000", v);
            TS_ASSERT_EQUALS(oss.str(), golden);
        }
    }

    void testOkSize()
    {
        IntNum intn;

        // parameters are size N (in bits), right shift (in bits), range type
        // range type = 0: (0, 2^N-1) range
        // range type = 1: (-2^(N-1), 2^(N-1)-1) range
        // range type = 2: (-2^(N-1), 2^N-1) range
        intn = 0;
        TS_ASSERT( intn.ok_size(8, 0, 0));
        TS_ASSERT( intn.ok_size(8, 1, 0));
        TS_ASSERT( intn.ok_size(8, 0, 1));
        TS_ASSERT( intn.ok_size(8, 1, 1));
        TS_ASSERT( intn.ok_size(8, 0, 2));
        TS_ASSERT( intn.ok_size(8, 1, 2));

        intn = -1;
        TS_ASSERT(!intn.ok_size(8, 0, 0));  // <0
        TS_ASSERT(!intn.ok_size(8, 1, 0));  // <0
        TS_ASSERT( intn.ok_size(8, 0, 1));
        TS_ASSERT( intn.ok_size(8, 1, 1));
        TS_ASSERT( intn.ok_size(8, 0, 2));
        TS_ASSERT( intn.ok_size(8, 1, 2));

        intn = 1;
        TS_ASSERT( intn.ok_size(8, 0, 0));
        TS_ASSERT( intn.ok_size(8, 1, 0));
        TS_ASSERT( intn.ok_size(8, 0, 1));
        TS_ASSERT( intn.ok_size(8, 1, 1));
        TS_ASSERT( intn.ok_size(8, 0, 2));
        TS_ASSERT( intn.ok_size(8, 1, 2));

        intn = 2;
        TS_ASSERT( intn.ok_size(8, 0, 0));
        TS_ASSERT( intn.ok_size(8, 1, 0));
        TS_ASSERT( intn.ok_size(8, 0, 1));
        TS_ASSERT( intn.ok_size(8, 1, 1));
        TS_ASSERT( intn.ok_size(8, 0, 2));
        TS_ASSERT( intn.ok_size(8, 1, 2));

        // 8-bit boundary conditions (signed and unsigned)
        intn = -128;
        TS_ASSERT( intn.ok_size(8, 0, 1));
        TS_ASSERT( intn.ok_size(8, 0, 2));

        intn = -129;
        TS_ASSERT(!intn.ok_size(8, 0, 1));
        TS_ASSERT(!intn.ok_size(8, 0, 2));

        intn = 127;
        TS_ASSERT( intn.ok_size(8, 0, 1));

        intn = 128;
        TS_ASSERT(!intn.ok_size(8, 0, 1));

        intn = 255;
        TS_ASSERT( intn.ok_size(8, 0, 0));
        TS_ASSERT( intn.ok_size(8, 0, 2));

        intn = 256;
        TS_ASSERT(!intn.ok_size(8, 0, 0));
        TS_ASSERT(!intn.ok_size(8, 0, 2));

        // 16-bit boundary conditions (signed and unsigned)
        intn = -32768;
        TS_ASSERT( intn.ok_size(16, 0, 1));
        TS_ASSERT( intn.ok_size(16, 0, 2));

        intn = -32769;
        TS_ASSERT(!intn.ok_size(16, 0, 1));
        TS_ASSERT(!intn.ok_size(16, 0, 2));

        intn = 32767;
        TS_ASSERT( intn.ok_size(16, 0, 1));

        intn = 32768;
        TS_ASSERT(!intn.ok_size(16, 0, 1));

        intn = 65535;
        TS_ASSERT( intn.ok_size(16, 0, 0));
        TS_ASSERT( intn.ok_size(16, 0, 2));

        intn = 65536;
        TS_ASSERT(!intn.ok_size(16, 0, 0));
        TS_ASSERT(!intn.ok_size(16, 0, 2));

        // 31-bit boundary conditions (signed and unsigned)
        intn = 1; intn <<= 30; intn = -intn;
        TS_ASSERT( intn.ok_size(31, 0, 1));
        TS_ASSERT( intn.ok_size(31, 0, 2));
        TS_ASSERT( intn.ok_size(32, 0, 1));
        TS_ASSERT( intn.ok_size(32, 0, 2));

        intn = 1; intn <<= 30; intn = -intn; --intn;
        TS_ASSERT(!intn.ok_size(31, 0, 1));
        TS_ASSERT(!intn.ok_size(31, 0, 2));
        TS_ASSERT( intn.ok_size(32, 0, 1));
        TS_ASSERT( intn.ok_size(32, 0, 2));

        intn = 1; intn <<= 30; --intn;
        TS_ASSERT( intn.ok_size(31, 0, 1));
        TS_ASSERT( intn.ok_size(32, 0, 1));

        intn = 1; intn <<= 30;
        TS_ASSERT(!intn.ok_size(31, 0, 1));
        TS_ASSERT( intn.ok_size(32, 0, 1));

        intn = 1; intn <<= 31; --intn;
        TS_ASSERT( intn.ok_size(31, 0, 0));
        TS_ASSERT( intn.ok_size(31, 0, 2));
        TS_ASSERT( intn.ok_size(32, 0, 0));
        TS_ASSERT( intn.ok_size(32, 0, 2));

        intn = 1; intn <<= 31;
        TS_ASSERT(!intn.ok_size(31, 0, 0));
        TS_ASSERT(!intn.ok_size(31, 0, 2));
        TS_ASSERT( intn.ok_size(32, 0, 0));
        TS_ASSERT( intn.ok_size(32, 0, 2));

        // 32-bit boundary conditions (signed and unsigned)
        intn = 1; intn <<= 31; intn = -intn;
        TS_ASSERT( intn.ok_size(32, 0, 1));
        TS_ASSERT( intn.ok_size(32, 0, 2));

        intn = 1; intn <<= 31; intn = -intn; --intn;
        TS_ASSERT(!intn.ok_size(32, 0, 1));
        TS_ASSERT(!intn.ok_size(32, 0, 2));

        intn = 1; intn <<= 31; --intn;
        TS_ASSERT( intn.ok_size(32, 0, 1));

        intn = 1; intn <<= 31;
        TS_ASSERT(!intn.ok_size(32, 0, 1));

        intn = 1; intn <<= 32; --intn;
        TS_ASSERT( intn.ok_size(32, 0, 0));
        TS_ASSERT( intn.ok_size(32, 0, 2));

        intn = 1; intn <<= 32;
        TS_ASSERT(!intn.ok_size(32, 0, 0));
        TS_ASSERT(!intn.ok_size(32, 0, 2));

        // 63-bit boundary conditions (signed and unsigned)
        intn = 1; intn <<= 62; intn = -intn;
        TS_ASSERT( intn.ok_size(63, 0, 1));
        TS_ASSERT( intn.ok_size(63, 0, 2));
        TS_ASSERT( intn.ok_size(64, 0, 1));
        TS_ASSERT( intn.ok_size(64, 0, 2));

        intn = 1; intn <<= 62; intn = -intn; --intn;
        TS_ASSERT(!intn.ok_size(63, 0, 1));
        TS_ASSERT(!intn.ok_size(63, 0, 2));
        TS_ASSERT( intn.ok_size(64, 0, 1));
        TS_ASSERT( intn.ok_size(64, 0, 2));

        intn = 1; intn <<= 62; --intn;
        TS_ASSERT( intn.ok_size(63, 0, 1));
        TS_ASSERT( intn.ok_size(64, 0, 1));

        intn = 1; intn <<= 62;
        TS_ASSERT(!intn.ok_size(63, 0, 1));
        TS_ASSERT( intn.ok_size(64, 0, 1));

        intn = 1; intn <<= 63; --intn;
        TS_ASSERT( intn.ok_size(63, 0, 0));
        TS_ASSERT( intn.ok_size(63, 0, 2));
        TS_ASSERT( intn.ok_size(64, 0, 0));
        TS_ASSERT( intn.ok_size(64, 0, 2));

        intn = 1; intn <<= 63;
        TS_ASSERT(!intn.ok_size(63, 0, 0));
        TS_ASSERT(!intn.ok_size(63, 0, 2));
        TS_ASSERT( intn.ok_size(64, 0, 0));
        TS_ASSERT( intn.ok_size(64, 0, 2));

        // 64-bit boundary conditions (signed and unsigned)
        intn = 1; intn <<= 63; intn = -intn;
        TS_ASSERT( intn.ok_size(64, 0, 1));
        TS_ASSERT( intn.ok_size(64, 0, 2));

        intn = 1; intn <<= 63; intn = -intn; --intn;
        TS_ASSERT(!intn.ok_size(64, 0, 1));
        TS_ASSERT(!intn.ok_size(64, 0, 2));

        intn = 1; intn <<= 63; --intn;
        TS_ASSERT( intn.ok_size(64, 0, 1));

        intn = 1; intn <<= 63;
        TS_ASSERT(!intn.ok_size(64, 0, 1));

        intn = 1; intn <<= 64; --intn;
        TS_ASSERT( intn.ok_size(64, 0, 0));
        TS_ASSERT( intn.ok_size(64, 0, 2));

        intn = 1; intn <<= 64;
        TS_ASSERT(!intn.ok_size(64, 0, 0));
        TS_ASSERT(!intn.ok_size(64, 0, 2));

        // with rshift
        TS_ASSERT(IntNum(255).ok_size(8, 1, 1));
        TS_ASSERT(!IntNum(256).ok_size(8, 1, 1));
        TS_ASSERT(IntNum(-256).ok_size(8, 1, 1));
        TS_ASSERT(!IntNum(-257).ok_size(8, 1, 1));
        TS_ASSERT(IntNum(511).ok_size(8, 1, 2));
        TS_ASSERT(!IntNum(512).ok_size(8, 1, 2));
        TS_ASSERT(IntNum(-256).ok_size(8, 1, 2));
        TS_ASSERT(!IntNum(-257).ok_size(8, 1, 2));
    }

    void testGetSized_long()
    {
        struct LongTest
        {
            long val;
            unsigned int destsize;
            unsigned int valsize;
            int shift;
            bool bigendian;
            unsigned char inbuf[4];
            unsigned char outbuf[4];
        };
        LongTest tests[] =
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
        unsigned char buf[4];
        IntNum intn;
        for (unsigned int i=0; i<NELEMS(tests); ++i)
        {
            LongTest& test = tests[i];
            intn = test.val;
            memcpy(buf, test.inbuf, 4);
            intn.get_sized(buf, test.destsize, test.valsize, test.shift,
                           test.bigendian, 0);
            //TS_TRACE(i);
            TS_ASSERT_SAME_DATA(buf, test.outbuf, 4);
        }
    }
};

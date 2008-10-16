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

#include "intnum.h"
#include "intnum_iomanip.h"

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
};

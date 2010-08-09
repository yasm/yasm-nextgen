//
//  Copyright (C) 2001-2007  Peter Johnson
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

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/StringRef.h"
#include "yasmx/Bytes.h"
#include "yasmx/NumericOutput.h"

// constants describing parameters of internal floating point format.
//  (these should match those in libyasmx/floatnum.cpp !)
static const unsigned int MANT_BITS = 80;
static const unsigned int MANT_BYTES = 10;

struct InitEntry {
    // input ASCII value
    const char *ascii;

    // correct output conversions - these should be *exact* matches
    int ret32;
    unsigned char result32[4];
    int ret64;
    unsigned char result64[8];
    int ret80;
    unsigned char result80[10];
};

// Values used for normalized tests
static const InitEntry normalized_vals[] = {
    {   "3.141592653589793",
         0, {0xdb,0x0f,0x49,0x40},
         0, {0x18,0x2d,0x44,0x54,0xfb,0x21,0x09,0x40},
         0, {0xe9,0xbd,0x68,0x21,0xa2,0xda,0x0f,0xc9,0x00,0x40}
    },
    {   "-3.141592653589793",
         0, {0xdb,0x0f,0x49,0xc0},
         0, {0x18,0x2d,0x44,0x54,0xfb,0x21,0x09,0xc0},
         0, {0xe9,0xbd,0x68,0x21,0xa2,0xda,0x0f,0xc9,0x00,0xc0}
    },
    {   "1.e16",
         0, {0xca,0x1b,0x0e,0x5a},
         0, {0x00,0x80,0xe0,0x37,0x79,0xc3,0x41,0x43},
         0, {0x00,0x00,0x00,0x04,0xbf,0xc9,0x1b,0x8e,0x34,0x40}
    },
    {   "1.6e-20",
         0, {0xa0,0x1d,0x97,0x1e},
         0, {0x4f,0x9b,0x0e,0x0a,0xb4,0xe3,0xd2,0x3b},
         0, {0xef,0x7b,0xda,0x74,0x50,0xa0,0x1d,0x97,0xbd,0x3f}
    },
    {   "-5876.",
         0, {0x00,0xa0,0xb7,0xc5},
         0, {0x00,0x00,0x00,0x00,0x00,0xf4,0xb6,0xc0},
         0, {0x00,0x00,0x00,0x00,0x00,0x00,0xa0,0xb7,0x0b,0xc0}
    },
    // Edge cases for rounding wrap.
    {   "1.00000",
         0, {0x00,0x00,0x80,0x3f},
         0, {0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x3f},
         0, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xff,0x3f}
    },
    {   "1.000000",
         0, {0x00,0x00,0x80,0x3f},
         0, {0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x3f},
         0, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xff,0x3f}
    },
};

// Still normalized values, but edge cases of various sizes, testing
// underflow/overflow checks as well.
static const InitEntry normalized_edgecase_vals[] = {
    // 32-bit edges
    {   "1.1754943508222875e-38",
         0, {0x00,0x00,0x80,0x00},
         0, {0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x38},
         0, {0x83,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x80,0x3f}
    },
    {   "3.4028234663852886e+38",
         0, {0xff,0xff,0x7f,0x7f},
         0, {0x00,0x00,0x00,0xe0,0xff,0xff,0xef,0x47},
         0, {0x0a,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0x7e,0x40}
    },
    // 64-bit edges
    {   "2.2250738585072014E-308",
        -1, {0x00,0x00,0x00,0x00},
         0, {0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00},
         0, {0x46,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x01,0x3c}
    },
    {   "1.7976931348623157E+308",
         1, {0x00,0x00,0x80,0x7f},
         0, {0xff,0xff,0xff,0xff,0xff,0xff,0xef,0x7f},
         0, {0xac,0xf7,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,0x43}
    },
    // 80-bit edges
/*    { "3.3621E-4932",
        {},,0,0,
        -1, {0x00,0x00,0x00,0x00},
        -1, {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
         0, {}
    },
    {   "1.1897E+4932",
        {},,0,0,
         1, {0x00,0x00,0x80,0x7f},
         1, {},
         0, {}
    },*/
    // internal format edges
/*    {
    },
    {
    },*/
};

class FloatNumTest : public testing::TestWithParam<InitEntry>
{
protected:
    void CheckGetSized(int destsize,
                       int valsize,
                       int correct_retval,
                       const unsigned char* correct_result)
    {
        llvm::APFloat flt(llvm::APFloat::x87DoubleExtended, GetParam().ascii);
        yasm::Bytes result;
        result.resize(destsize);
        result.setLittleEndian();
        yasm::NumericOutput num_out(result);
        num_out.setSize(valsize);
        num_out.OutputFloat(flt);
        for (int i=0; i<destsize; ++i)
            EXPECT_EQ(correct_result[i], result[i]);
    }
};

TEST_P(FloatNumTest, Test32)
{
    CheckGetSized(4, 32, GetParam().ret32, GetParam().result32);
}

TEST_P(FloatNumTest, Test64)
{
    CheckGetSized(8, 64, GetParam().ret64, GetParam().result64);
}

TEST_P(FloatNumTest, Test80)
{
    CheckGetSized(10, 80, GetParam().ret80, GetParam().result80);
}

INSTANTIATE_TEST_CASE_P(FloatNumNormalizedTest, FloatNumTest,
                        ::testing::ValuesIn(normalized_vals));
INSTANTIATE_TEST_CASE_P(FloatNumNormalizedEdgecaseTest, FloatNumTest,
                        ::testing::ValuesIn(normalized_edgecase_vals));

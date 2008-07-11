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
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE floatnum_get_test
#include <boost/test/unit_test.hpp>

#include "floatnum.h"
#include "floatnum_test.h"

#ifndef NELEMS
#define NELEMS(array)   (sizeof(array) / sizeof(array[0]))
#endif

namespace yasm
{

class FloatNumTest
{
public:
    FloatNumTest(const Init_Entry& val);
    void check(int destsize, int valsize);

private:
    const Init_Entry& m_val;
    FloatNum *m_flt;
};

FloatNumTest::FloatNumTest(const Init_Entry& val)
    : m_val(val),
      m_flt(new FloatNum(val.mantissa, val.exponent))
{
    // set up flt
    m_flt->m_sign = val.sign;
    m_flt->m_flags = val.flags;
}

void
FloatNumTest::check(int destsize, int valsize)
{
    int correct_retval;
    const unsigned char* correct_result;
    switch (valsize)
    {
        case 32:
            correct_retval = m_val.ret32;
            correct_result = m_val.result32;
            break;
        case 64:
            correct_retval = m_val.ret64;
            correct_result = m_val.result64;
            break;
        case 80:
            correct_retval = m_val.ret80;
            correct_result = m_val.result80;
            break;
        default:
            return;
    }
    unsigned char result[10];
    int retval = m_flt->get_sized(result, destsize, valsize, 0, 0, 0);
    BOOST_CHECK_EQUAL(retval, correct_retval);

    bool match = true;
    for (int i=0;i<destsize;i++)
        if (result[i] != correct_result[i])
            match = false;
    BOOST_CHECK(match);
}

} // namespace yasm

static void
test_get_common(const Init_Entry* vals, int num, int destsize, int valsize)
{
    for (int i=0; i<num; i++)
    {
        yasm::FloatNumTest test(vals[i]);
        test.check(destsize, valsize);
    }
}

//
// get_single tests
//

BOOST_AUTO_TEST_CASE(GetSingleNormalized)
{
    test_get_common(normalized_vals, NELEMS(normalized_vals), 4, 32);
}

BOOST_AUTO_TEST_CASE(GetSingleNormalizedEdgecase)
{
    test_get_common(normalized_edgecase_vals,
                    NELEMS(normalized_edgecase_vals), 4, 32);
}

//
// get_double tests
//

BOOST_AUTO_TEST_CASE(GetDoubleNormalized)
{
    test_get_common(normalized_vals, NELEMS(normalized_vals), 8, 64);
}

BOOST_AUTO_TEST_CASE(GetDoubleNormalizedEdgecase)
{
    test_get_common(normalized_edgecase_vals,
                    NELEMS(normalized_edgecase_vals), 8, 64);
}

//
// get_extended tests
//

BOOST_AUTO_TEST_CASE(GetExtendedNormalized)
{
    test_get_common(normalized_vals, NELEMS(normalized_vals), 10, 80);
}

BOOST_AUTO_TEST_CASE(GetExtendedNormalizedEdgecase)
{
    test_get_common(normalized_edgecase_vals,
                    NELEMS(normalized_edgecase_vals), 10, 80);
}

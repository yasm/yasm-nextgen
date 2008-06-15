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
    void check();

private:
    const Init_Entry& m_val;
    FloatNum *m_flt;
};

FloatNumTest::FloatNumTest(const Init_Entry& val)
    : m_val(val),
      m_flt(new FloatNum(val.ascii))
{
}

void
FloatNumTest::check()
{
    bool match = true;
    unsigned int len;

    unsigned char* mantissa = BitVector::Block_Read(m_flt->m_mantissa, &len);
    for (unsigned int i=1;i<MANT_BYTES;i++)      // don't compare first byte
        if (mantissa[i] != m_val.mantissa[i])
            match = false;
    BitVector::Dispose(mantissa);
    BOOST_CHECK(match);

    BOOST_CHECK_EQUAL(m_flt->m_exponent, m_val.exponent);
    BOOST_CHECK_EQUAL(m_flt->m_sign, m_val.sign);
    BOOST_CHECK_EQUAL(m_flt->m_flags, m_val.flags);
}

} // namespace yasm

static void
test_new_common(const Init_Entry* vals, int num)
{
    for (int i=0; i<num; i++)
    {
        yasm::FloatNumTest test(vals[i]);
        test.check();
    }
}

BOOST_AUTO_TEST_CASE(NewNormalized)
{
    test_new_common(normalized_vals, NELEMS(normalized_vals));
}

BOOST_AUTO_TEST_CASE(NewNormalizedEdgecase)
{
    test_new_common(normalized_edgecase_vals,
                    NELEMS(normalized_edgecase_vals));
}

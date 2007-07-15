/*
 *  Copyright (C) 2001-2007  Peter Johnson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <cstdio>
#include <string>
#include <iostream>

#include "floatnum.h"
#include "floatnum_test.h"

#ifndef NELEMS
#define NELEMS(array)   (sizeof(array) / sizeof(array[0]))
#endif

// failure messages
static std::string result_msg;

namespace yasm {

class FloatNumTest {
public:
    FloatNumTest(const Init_Entry& val);
    bool check();

private:
    const Init_Entry& m_val;
    FloatNum *m_flt;
};

FloatNumTest::FloatNumTest(const Init_Entry& val)
    : m_val(val)
{
    m_flt = new FloatNum(val.ascii);
    result_msg = val.ascii;
    result_msg += ": incorrect ";
}

bool
FloatNumTest::check()
{
    bool badmantissa = false;
    unsigned int len;

    unsigned char* mantissa = BitVector::Block_Read(m_flt->m_mantissa, &len);
    for (unsigned int i=1;i<MANT_BYTES;i++)      // don't compare first byte
        if (mantissa[i] != m_val.mantissa[i])
            badmantissa = true;
    BitVector::Dispose(mantissa);
    if (badmantissa) {
        result_msg += "mantissa";
        return false;
    }

    if (m_flt->m_exponent != m_val.exponent) {
        result_msg += "exponent";
        return false;
    }
    if (m_flt->m_sign != m_val.sign) {
        result_msg += "sign";
        return false;
    }
    if (m_flt->m_flags != m_val.flags) {
        result_msg += "flags";
        return false;
    }
    result_msg = "";
    return true;
}

} // namespace yasm

static bool
test_new_common(const Init_Entry* vals, int num)
{
    for (int i=0; i<num; i++) {
        yasm::FloatNumTest test(vals[i]);
        if (!test.check())
            return false;
    }
    return true;
}

static bool
test_new_normalized()
{
    return test_new_common(normalized_vals, NELEMS(normalized_vals));
}

static bool
test_new_normalized_edgecase()
{
    return test_new_common(normalized_edgecase_vals,
                           NELEMS(normalized_edgecase_vals));
}

static std::string failed;

static int
runtest_(const char *testname, bool (*testfunc)())
{
    bool s;
    s = testfunc();
    std::cout << (s ? '.':'F');
    std::cout.flush();
    if (!s) {
        failed += " ** F: ";
        failed += testname;
        failed += " failed: ";
        failed += result_msg;
        failed += "!\n";
    }
    return s?0:1;
}
#define runtest(x)  runtest_(#x,test_##x)

int
main()
{
    const int nt = 2;
    int nf = 0;

    std::cout << "Test floatnum_new_test: ";
    nf += runtest(new_normalized);
    nf += runtest(new_normalized_edgecase);
    std::cout << " +" << (nt-nf) << "-" << nf << "/" << nt;
    std::cout << " " << (100*(nt-nf)/nt) << "%" << std::endl;
    std::cout << failed;
    return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

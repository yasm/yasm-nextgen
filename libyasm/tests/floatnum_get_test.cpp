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
    bool check(int destsize, int valsize);

private:
    const Init_Entry& m_val;
    FloatNum *m_flt;
};

FloatNumTest::FloatNumTest(const Init_Entry& val)
    : m_val(val)
{
    /* set up flt */
    m_flt = new FloatNum(val.mantissa, val.exponent);
    m_flt->m_sign = val.sign;
    m_flt->m_flags = val.flags;

    // set failure message
    result_msg = val.ascii;
    result_msg += ": incorrect ";
}

bool
FloatNumTest::check(int destsize, int valsize)
{
    unsigned char result[10];
    char str[20];

    int correct_retval;
    const unsigned char* correct_result;
    switch (valsize) {
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
            return false;
    }
    int retval = m_flt->get_sized(result, destsize, valsize, 0, 0, 0);
    if (retval != correct_retval) {
        result_msg += "return value: ";
        sprintf(str, "%d", retval);
        result_msg += str;
    }
    bool match = true;
    for (int i=0;i<destsize;i++)
        if (result[i] != correct_result[i])
            match = false;

    if (!match) {
        result_msg += "result generated: ";
        for (int i=0; i<destsize; i++) {
            sprintf(str, "%02x ", result[i]);
            result_msg += str;
        }
    }

    return match;
}

} // namespace yasm

/*
 * get_single tests
 */

static int
test_get_common(const Init_Entry* vals, int num, int destsize, int valsize)
{
    for (int i=0; i<num; i++) {
        yasm::FloatNumTest test(vals[i]);
        if (!test.check(destsize, valsize))
            return false;
    }
    return true;
}

/*
 * get_single tests
 */

static bool
test_get_single_normalized()
{
    return test_get_common(normalized_vals, NELEMS(normalized_vals), 4, 32);
}

static bool
test_get_single_normalized_edgecase()
{
    return test_get_common(normalized_edgecase_vals,
                           NELEMS(normalized_edgecase_vals), 4, 32);
}

/*
 * get_double tests
 */

static bool
test_get_double_normalized()
{
    return test_get_common(normalized_vals, NELEMS(normalized_vals), 8, 64);
}

static bool
test_get_double_normalized_edgecase()
{
    return test_get_common(normalized_edgecase_vals,
                           NELEMS(normalized_edgecase_vals), 8, 64);
}

/*
 * get_extended tests
 */

static bool
test_get_extended_normalized()
{
    return test_get_common(normalized_vals, NELEMS(normalized_vals), 10, 80);
}

static bool
test_get_extended_normalized_edgecase(void)
{
    return test_get_common(normalized_edgecase_vals,
                           NELEMS(normalized_edgecase_vals), 10, 80);
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
    const int nt = 6;
    int nf = 0;

    // We can't rely on FloatNumManager to boot BitVector due to the
    // implementation of FloatNum::FloatNum
    BitVector::Boot();

    std::cout << "Test floatnum_get_test: ";
    nf += runtest(get_single_normalized);
    nf += runtest(get_single_normalized_edgecase);
    nf += runtest(get_double_normalized);
    nf += runtest(get_double_normalized_edgecase);
    nf += runtest(get_extended_normalized);
    nf += runtest(get_extended_normalized_edgecase);
    std::cout << " +" << (nt-nf) << "-" << nf << "/" << nt;
    std::cout << " " << (100*(nt-nf)/nt) << "%" << std::endl;
    std::cout << failed;

    BitVector::Shutdown();

    return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

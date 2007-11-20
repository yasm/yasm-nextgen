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
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE intnum_test
#include <boost/test/unit_test.hpp>

#include "intnum.h"

using namespace yasm;

BOOST_AUTO_TEST_CASE(EqualOperatorOverload)
{
    // Check comparison operators first; we'll use BOOST_CHECK_EQUAL directly
    // on intnums later, so it's critical these work.

    // == operator
    BOOST_REQUIRE_EQUAL(IntNum(5) == IntNum(5), true);
    BOOST_REQUIRE_EQUAL(IntNum(5) == 5, true);
    BOOST_REQUIRE_EQUAL(5 == IntNum(5), true);
    BOOST_REQUIRE_EQUAL(IntNum(5) == IntNum(7), false);
    BOOST_REQUIRE_EQUAL(IntNum(5) == 7, false);
    BOOST_REQUIRE_EQUAL(5 == IntNum(7), false);

    // != operator
    BOOST_REQUIRE_EQUAL(IntNum(5) != IntNum(5), false);
    BOOST_REQUIRE_EQUAL(IntNum(5) != 5, false);
    BOOST_REQUIRE_EQUAL(5 != IntNum(5), false);
    BOOST_REQUIRE_EQUAL(IntNum(5) != IntNum(7), true);
    BOOST_REQUIRE_EQUAL(IntNum(5) != 7, true);
    BOOST_REQUIRE_EQUAL(5 != IntNum(7), true);
}

BOOST_AUTO_TEST_CASE(ComparisonOperatorOverload)
{
    // < operator
    BOOST_CHECK(IntNum(5) < IntNum(7));
    BOOST_CHECK(IntNum(5) < 7);
    BOOST_CHECK(5 < IntNum(7));
    BOOST_CHECK(!(IntNum(7) < IntNum(5)));
    BOOST_CHECK(!(IntNum(7) < 5));
    BOOST_CHECK(!(7 < IntNum(5)));

    // > operator
    BOOST_CHECK(IntNum(7) > IntNum(5));
    BOOST_CHECK(IntNum(7) > 5);
    BOOST_CHECK(7 > IntNum(5));
    BOOST_CHECK(!(IntNum(5) > IntNum(7)));
    BOOST_CHECK(!(IntNum(5) > 7));
    BOOST_CHECK(!(5 > IntNum(7)));

    // <= operator
    BOOST_CHECK(IntNum(5) <= IntNum(5));
    BOOST_CHECK(IntNum(5) <= 5);
    BOOST_CHECK(5 <= IntNum(5));
    BOOST_CHECK(IntNum(5) <= IntNum(7));
    BOOST_CHECK(IntNum(5) <= 7);
    BOOST_CHECK(5 <= IntNum(7));
    BOOST_CHECK(!(IntNum(7) <= IntNum(5)));
    BOOST_CHECK(!(IntNum(7) <= 5));
    BOOST_CHECK(!(7 <= IntNum(5)));

    // >= operator
    BOOST_CHECK(IntNum(5) >= IntNum(5));
    BOOST_CHECK(IntNum(5) >= 5);
    BOOST_CHECK(5 >= IntNum(5));
    BOOST_CHECK(IntNum(7) >= IntNum(5));
    BOOST_CHECK(IntNum(7) >= 5);
    BOOST_CHECK(7 >= IntNum(5));
    BOOST_CHECK(!(IntNum(5) >= IntNum(7)));
    BOOST_CHECK(!(IntNum(5) >= 7));
    BOOST_CHECK(!(5 >= IntNum(7)));
}

BOOST_AUTO_TEST_CASE(BinaryOperatorOverload)
{
    BOOST_CHECK_EQUAL(IntNum(5)+2, 7);
    BOOST_CHECK_EQUAL(2+IntNum(5), 7);
    BOOST_CHECK_EQUAL(IntNum(5)-2, 3);
    BOOST_CHECK_EQUAL(2-IntNum(5), -3);
    BOOST_CHECK_EQUAL(IntNum(5)*2, 10);
    BOOST_CHECK_EQUAL(2*IntNum(5), 10);
    BOOST_CHECK_EQUAL(IntNum(5)/2, 2);
    BOOST_CHECK_EQUAL(5/IntNum(2), 2);
    BOOST_CHECK_EQUAL(IntNum(5)%2, 1);
    BOOST_CHECK_EQUAL(5%IntNum(2), 1);
    BOOST_CHECK_EQUAL(IntNum(7)^3, 4);
    BOOST_CHECK_EQUAL(7^IntNum(3), 4);
    BOOST_CHECK_EQUAL(IntNum(10)&7, 2);
    BOOST_CHECK_EQUAL(10&IntNum(7), 2);
    BOOST_CHECK_EQUAL(IntNum(10)|3, 11);
    BOOST_CHECK_EQUAL(10|IntNum(3), 11);
    BOOST_CHECK_EQUAL(IntNum(10)>>2, 2);
    BOOST_CHECK_EQUAL(10>>IntNum(2), 2);
    BOOST_CHECK_EQUAL(IntNum(10)<<2, 40);
    BOOST_CHECK_EQUAL(10<<IntNum(2), 40);
}

BOOST_AUTO_TEST_CASE(UnaryOperatorOverload)
{
    BOOST_CHECK_EQUAL(-IntNum(5), -5);
    BOOST_CHECK_EQUAL(-IntNum(-5), 5);
    BOOST_CHECK_EQUAL(+IntNum(5), 5);
    BOOST_CHECK_EQUAL(+IntNum(-5), -5);

    BOOST_CHECK_EQUAL((~IntNum(5))&0xF, 10);

    BOOST_CHECK_EQUAL(!IntNum(0), true);
    BOOST_CHECK_EQUAL(!IntNum(5), false);
}

BOOST_AUTO_TEST_CASE(BinaryAssignmentOperatorOverload)
{
    IntNum x(0);
    BOOST_CHECK_EQUAL(x+=6, 6);
    BOOST_CHECK_EQUAL(x-=4, 2);
    BOOST_CHECK_EQUAL(x*=8, 16);
    BOOST_CHECK_EQUAL(x/=2, 8);
    BOOST_CHECK_EQUAL(x%=3, 2);
    BOOST_CHECK_EQUAL(x^=1, 3);
    BOOST_CHECK_EQUAL(x&=2, 2);
    BOOST_CHECK_EQUAL(x|=5, 7);
    BOOST_CHECK_EQUAL(x>>=2, 1);
    BOOST_CHECK_EQUAL(x<<=2, 4);
}

BOOST_AUTO_TEST_CASE(IncDecOperatorOverload)
{
    IntNum x(5);
    BOOST_CHECK_EQUAL(++x, 6);
    BOOST_CHECK_EQUAL(x++, 6);
    BOOST_CHECK_EQUAL(x, 7);
    BOOST_CHECK_EQUAL(--x, 6);
    BOOST_CHECK_EQUAL(x--, 6);
    BOOST_CHECK_EQUAL(x, 5);
}

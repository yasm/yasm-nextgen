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
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE x86effaddr_test
#include <boost/test/unit_test.hpp>

#include "x86effaddr.h"
#include <libyasmx/errwarn.h>


using namespace yasm::arch::x86;

BOOST_AUTO_TEST_SUITE(SetRexFromReg)

BOOST_AUTO_TEST_CASE(SetRexFromRegDrex)
{
    unsigned char rex;
    unsigned char drex;
    unsigned char low3;

    // Test bits != 64; should only set low 3 bits
    rex = drex = low3 = 0;
    set_rex_from_reg(&rex, &drex, &low3, X86Register::REG32, 7, 32, X86_REX_W);
    BOOST_CHECK_EQUAL(low3, 7);
    BOOST_CHECK_EQUAL(rex, 0);
    BOOST_CHECK_EQUAL(drex, 0);

    rex = drex = low3 = 0;
    set_rex_from_reg(&rex, &drex, &low3, X86Register::REG32, 13, 32, X86_REX_W);
    BOOST_CHECK_EQUAL(low3, 5);
    BOOST_CHECK_EQUAL(rex, 0);
    BOOST_CHECK_EQUAL(drex, 0);

    // Test reg < 8 in 64-bit mode, should not set REX or DREX for non-REG8X.
    rex = drex = low3 = 0;
    set_rex_from_reg(&rex, &drex, &low3, X86Register::REG32, 4, 64, X86_REX_B);
    BOOST_CHECK_EQUAL(low3, 4);
    BOOST_CHECK_EQUAL(rex, 0);
    BOOST_CHECK_EQUAL(drex, 0);

    // reg >= 8 in 64-bit mode should set drex if provided.
    rex = drex = low3 = 0;
    set_rex_from_reg(&rex, &drex, &low3, X86Register::REG32, 13, 64, X86_REX_B);
    BOOST_CHECK_EQUAL(low3, 5);
    BOOST_CHECK_EQUAL(rex, 0);
    BOOST_CHECK_EQUAL(drex, 0x01);

    rex = drex = low3 = 0;
    set_rex_from_reg(&rex, &drex, &low3, X86Register::REG32, 13, 64, X86_REX_X);
    BOOST_CHECK_EQUAL(low3, 5);
    BOOST_CHECK_EQUAL(rex, 0);
    BOOST_CHECK_EQUAL(drex, 0x02);

    rex = drex = low3 = 0;
    set_rex_from_reg(&rex, &drex, &low3, X86Register::REG32, 13, 64, X86_REX_R);
    BOOST_CHECK_EQUAL(low3, 5);
    BOOST_CHECK_EQUAL(rex, 0);
    BOOST_CHECK_EQUAL(drex, 0x04);

    rex = drex = low3 = 0;
    set_rex_from_reg(&rex, &drex, &low3, X86Register::REG32, 13, 64, X86_REX_W);
    BOOST_CHECK_EQUAL(low3, 5);
    BOOST_CHECK_EQUAL(rex, 0);
    BOOST_CHECK_EQUAL(drex, 0x08);

    // DREX should OR into existing value
    rex = low3 = 0; drex = 0x30;
    set_rex_from_reg(&rex, &drex, &low3, X86Register::REG32, 13, 64, X86_REX_R);
    BOOST_CHECK_EQUAL(low3, 5);
    BOOST_CHECK_EQUAL(rex, 0);
    BOOST_CHECK_EQUAL(drex, 0x34);
}

BOOST_AUTO_TEST_CASE(SetRexFromRegNoDrex)
{
    unsigned char rex;
    unsigned char low3;

    // reg >= 8 should set rex.
    rex = low3 = 0;
    set_rex_from_reg(&rex, 0, &low3, X86Register::REG32, 13, 64, X86_REX_B);
    BOOST_CHECK_EQUAL(low3, 5);
    BOOST_CHECK_EQUAL(rex, 0x41);

    rex = low3 = 0;
    set_rex_from_reg(&rex, 0, &low3, X86Register::REG32, 13, 64, X86_REX_X);
    BOOST_CHECK_EQUAL(low3, 5);
    BOOST_CHECK_EQUAL(rex, 0x42);

    rex = low3 = 0;
    set_rex_from_reg(&rex, 0, &low3, X86Register::REG32, 13, 64, X86_REX_R);
    BOOST_CHECK_EQUAL(low3, 5);
    BOOST_CHECK_EQUAL(rex, 0x44);

    rex = low3 = 0;
    set_rex_from_reg(&rex, 0, &low3, X86Register::REG32, 13, 64, X86_REX_W);
    BOOST_CHECK_EQUAL(low3, 5);
    BOOST_CHECK_EQUAL(rex, 0x48);

    // REX should OR into existing value
    low3 = 0; rex = 0x44;
    set_rex_from_reg(&rex, 0, &low3, X86Register::REG32, 13, 64, X86_REX_W);
    BOOST_CHECK_EQUAL(low3, 5);
    BOOST_CHECK_EQUAL(rex, 0x4C);
}

BOOST_AUTO_TEST_CASE(SetRexFromRegNoRex)
{
    unsigned char rex;
    unsigned char drex;
    unsigned char low3;

    // Check for errors with reg_num >= 8 and neither REX or DREX available
    low3 = 0; rex = 0xff;
    BOOST_CHECK_THROW(set_rex_from_reg(&rex, 0, &low3, X86Register::REG32, 13,
                                       64, X86_REX_W),
                      yasm::TypeError);

    // If DREX available but REX isn't, reg_num >= 8 should not error
    drex = low3 = 0; rex = 0xff;
    set_rex_from_reg(&rex, &drex, &low3, X86Register::REG32, 13, 64, X86_REX_W);
    BOOST_CHECK_EQUAL(low3, 5);
    BOOST_CHECK_EQUAL(rex, 0xff);
    BOOST_CHECK_EQUAL(drex, 0x08);
}

BOOST_AUTO_TEST_CASE(SetRexFromReg8X)
{
    unsigned char rex;
    unsigned char drex;
    unsigned char low3;

    // REG8X should always result in a REX value, regardless of reg_num

    // REG8X should set drex if provided.
    // (note: cannot tell due to no other bits being set in DREX for <8).
    rex = low3 = 0; drex = 0x10;
    set_rex_from_reg(&rex, &drex, &low3, X86Register::REG8X, 3, 64, X86_REX_B);
    BOOST_CHECK_EQUAL(low3, 3);
    BOOST_CHECK_EQUAL(rex, 0);
    BOOST_CHECK_EQUAL(drex, 0x10);

    rex = low3 = 0; drex = 0x10;
    set_rex_from_reg(&rex, &drex, &low3, X86Register::REG8X, 13, 64, X86_REX_B);
    BOOST_CHECK_EQUAL(low3, 5);
    BOOST_CHECK_EQUAL(rex, 0);
    BOOST_CHECK_EQUAL(drex, 0x11);

    // if drex not provided, REG8X should set rex.
    rex = low3 = 0;
    set_rex_from_reg(&rex, 0, &low3, X86Register::REG8X, 3, 64, X86_REX_B);
    BOOST_CHECK_EQUAL(low3, 3);
    BOOST_CHECK_EQUAL(rex, 0x40);

    rex = low3 = 0;
    set_rex_from_reg(&rex, 0, &low3, X86Register::REG8X, 13, 64, X86_REX_B);
    BOOST_CHECK_EQUAL(low3, 5);
    BOOST_CHECK_EQUAL(rex, 0x41);

    // Check for errors with REG8X and neither REX or DREX available
    low3 = 0; rex = 0xff;
    BOOST_CHECK_THROW(set_rex_from_reg(&rex, 0, &low3, X86Register::REG8X, 3,
                                       64, X86_REX_W),
                      yasm::TypeError);

    // If DREX available but REX isn't, REG8X should not error
    low3 = 0; drex = 0x10; rex = 0xff;
    set_rex_from_reg(&rex, &drex, &low3, X86Register::REG8X, 3, 64, X86_REX_W);
    BOOST_CHECK_EQUAL(low3, 3);
    BOOST_CHECK_EQUAL(rex, 0xff);
    BOOST_CHECK_EQUAL(drex, 0x10);

    drex = low3 = 0; rex = 0xff;
    set_rex_from_reg(&rex, &drex, &low3, X86Register::REG8X, 13, 64, X86_REX_W);
    BOOST_CHECK_EQUAL(low3, 5);
    BOOST_CHECK_EQUAL(rex, 0xff);
    BOOST_CHECK_EQUAL(drex, 0x08);
}

BOOST_AUTO_TEST_CASE(SetRexFromReg8High)
{
    unsigned char rex;
    unsigned char low3;

    // Use of AH/BH/CH/DH should result in disallowed REX
    rex = low3 = 0;
    set_rex_from_reg(&rex, 0, &low3, X86Register::REG8, 4, 64, X86_REX_B);
    BOOST_CHECK_EQUAL(low3, 4);
    BOOST_CHECK_EQUAL(rex, 0xff);

    // If REX set, use of AH/BH/CH/DH should error
    low3 = 0; rex = 0x40;
    BOOST_CHECK_THROW(set_rex_from_reg(&rex, 0, &low3, X86Register::REG8, 4,
                                       64, X86_REX_W),
                      yasm::TypeError);

    // If REX is disallowed, use of AH/BH/CH/DH is still okay
    low3 = 0; rex = 0xff;
    set_rex_from_reg(&rex, 0, &low3, X86Register::REG8, 4, 64, X86_REX_B);
    BOOST_CHECK_EQUAL(low3, 4);
    BOOST_CHECK_EQUAL(rex, 0xff);

    // Use of AL/BL/CL/DL should NOT error and should still allow REX.
    low3 = 0; rex = 0x40;
    set_rex_from_reg(&rex, 0, &low3, X86Register::REG8, 3, 64, X86_REX_W);
    BOOST_CHECK_EQUAL(low3, 3);
    BOOST_CHECK_EQUAL(rex, 0x40);
}

BOOST_AUTO_TEST_SUITE_END()


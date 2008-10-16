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
#include <algorithm>
#include <cstring>

#include <cxxtest/TestSuite.h>

#include "x86effaddr.h"
#include <libyasmx/errwarn.h>
#include <libyasmx/expr.h>
#include <libyasmx/intnum.h>
#include <libyasmx/symbol.h>

#include "x86regtmod.cpp"

#ifndef NELEMS
#define NELEMS(array)   (sizeof(array) / sizeof(array[0]))
#endif

using namespace yasm;
using namespace yasm::arch::x86;

class X86EffAddrTestSuite : public CxxTest::TestSuite
{
public:
    X86Register BX, BP, SI, DI;
    X86Register EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI;

    X86EffAddrTestSuite()
        : BX(X86Register::REG16, 3)
        , BP(X86Register::REG16, 5)
        , SI(X86Register::REG16, 6)
        , DI(X86Register::REG16, 7)
        , EAX(X86Register::REG32, 0)
        , ECX(X86Register::REG32, 1)
        , EDX(X86Register::REG32, 2)
        , EBX(X86Register::REG32, 3)
        , ESP(X86Register::REG32, 4)
        , EBP(X86Register::REG32, 5)
        , ESI(X86Register::REG32, 6)
        , EDI(X86Register::REG32, 7)
    {
    }

    void testSetRexFromRegDrex()
    {
        unsigned char rex;
        unsigned char drex;
        unsigned char low3;

        // Test bits != 64; should only set low 3 bits
        rex = drex = low3 = 0;
        set_rex_from_reg(&rex, &drex, &low3, X86Register::REG32, 7, 32, X86_REX_W);
        TS_ASSERT_EQUALS(low3, 7);
        TS_ASSERT_EQUALS(rex, 0);
        TS_ASSERT_EQUALS(drex, 0);

        rex = drex = low3 = 0;
        set_rex_from_reg(&rex, &drex, &low3, X86Register::REG32, 13, 32, X86_REX_W);
        TS_ASSERT_EQUALS(low3, 5);
        TS_ASSERT_EQUALS(rex, 0);
        TS_ASSERT_EQUALS(drex, 0);

        // Test reg < 8 in 64-bit mode, should not set REX or DREX for non-REG8X.
        rex = drex = low3 = 0;
        set_rex_from_reg(&rex, &drex, &low3, X86Register::REG32, 4, 64, X86_REX_B);
        TS_ASSERT_EQUALS(low3, 4);
        TS_ASSERT_EQUALS(rex, 0);
        TS_ASSERT_EQUALS(drex, 0);

        // reg >= 8 in 64-bit mode should set drex if provided.
        rex = drex = low3 = 0;
        set_rex_from_reg(&rex, &drex, &low3, X86Register::REG32, 13, 64, X86_REX_B);
        TS_ASSERT_EQUALS(low3, 5);
        TS_ASSERT_EQUALS(rex, 0);
        TS_ASSERT_EQUALS(drex, 0x01);

        rex = drex = low3 = 0;
        set_rex_from_reg(&rex, &drex, &low3, X86Register::REG32, 13, 64, X86_REX_X);
        TS_ASSERT_EQUALS(low3, 5);
        TS_ASSERT_EQUALS(rex, 0);
        TS_ASSERT_EQUALS(drex, 0x02);

        rex = drex = low3 = 0;
        set_rex_from_reg(&rex, &drex, &low3, X86Register::REG32, 13, 64, X86_REX_R);
        TS_ASSERT_EQUALS(low3, 5);
        TS_ASSERT_EQUALS(rex, 0);
        TS_ASSERT_EQUALS(drex, 0x04);

        rex = drex = low3 = 0;
        set_rex_from_reg(&rex, &drex, &low3, X86Register::REG32, 13, 64, X86_REX_W);
        TS_ASSERT_EQUALS(low3, 5);
        TS_ASSERT_EQUALS(rex, 0);
        TS_ASSERT_EQUALS(drex, 0x08);

        // DREX should OR into existing value
        rex = low3 = 0; drex = 0x30;
        set_rex_from_reg(&rex, &drex, &low3, X86Register::REG32, 13, 64, X86_REX_R);
        TS_ASSERT_EQUALS(low3, 5);
        TS_ASSERT_EQUALS(rex, 0);
        TS_ASSERT_EQUALS(drex, 0x34);
    }

    void testSetRexFromRegNoDrex()
    {
        unsigned char rex;
        unsigned char low3;

        // reg >= 8 should set rex.
        rex = low3 = 0;
        set_rex_from_reg(&rex, 0, &low3, X86Register::REG32, 13, 64, X86_REX_B);
        TS_ASSERT_EQUALS(low3, 5);
        TS_ASSERT_EQUALS(rex, 0x41);

        rex = low3 = 0;
        set_rex_from_reg(&rex, 0, &low3, X86Register::REG32, 13, 64, X86_REX_X);
        TS_ASSERT_EQUALS(low3, 5);
        TS_ASSERT_EQUALS(rex, 0x42);

        rex = low3 = 0;
        set_rex_from_reg(&rex, 0, &low3, X86Register::REG32, 13, 64, X86_REX_R);
        TS_ASSERT_EQUALS(low3, 5);
        TS_ASSERT_EQUALS(rex, 0x44);

        rex = low3 = 0;
        set_rex_from_reg(&rex, 0, &low3, X86Register::REG32, 13, 64, X86_REX_W);
        TS_ASSERT_EQUALS(low3, 5);
        TS_ASSERT_EQUALS(rex, 0x48);

        // REX should OR into existing value
        low3 = 0; rex = 0x44;
        set_rex_from_reg(&rex, 0, &low3, X86Register::REG32, 13, 64, X86_REX_W);
        TS_ASSERT_EQUALS(low3, 5);
        TS_ASSERT_EQUALS(rex, 0x4C);
    }

    void testSetRexFromRegNoRex()
    {
        unsigned char rex;
        unsigned char drex;
        unsigned char low3;

        // Check for errors with reg_num >= 8 and neither REX or DREX available
        low3 = 0; rex = 0xff;
        TS_ASSERT_THROWS(set_rex_from_reg(&rex, 0, &low3, X86Register::REG32,
                                          13, 64, X86_REX_W),
                         TypeError);

        // If DREX available but REX isn't, reg_num >= 8 should not error
        drex = low3 = 0; rex = 0xff;
        set_rex_from_reg(&rex, &drex, &low3, X86Register::REG32, 13, 64, X86_REX_W);
        TS_ASSERT_EQUALS(low3, 5);
        TS_ASSERT_EQUALS(rex, 0xff);
        TS_ASSERT_EQUALS(drex, 0x08);
    }

    void testSetRexFromReg8X()
    {
        unsigned char rex;
        unsigned char drex;
        unsigned char low3;

        // REG8X should always result in a REX value, regardless of reg_num

        // REG8X should set drex if provided.
        // (note: cannot tell due to no other bits being set in DREX for <8).
        rex = low3 = 0; drex = 0x10;
        set_rex_from_reg(&rex, &drex, &low3, X86Register::REG8X, 3, 64, X86_REX_B);
        TS_ASSERT_EQUALS(low3, 3);
        TS_ASSERT_EQUALS(rex, 0);
        TS_ASSERT_EQUALS(drex, 0x10);

        rex = low3 = 0; drex = 0x10;
        set_rex_from_reg(&rex, &drex, &low3, X86Register::REG8X, 13, 64, X86_REX_B);
        TS_ASSERT_EQUALS(low3, 5);
        TS_ASSERT_EQUALS(rex, 0);
        TS_ASSERT_EQUALS(drex, 0x11);

        // if drex not provided, REG8X should set rex.
        rex = low3 = 0;
        set_rex_from_reg(&rex, 0, &low3, X86Register::REG8X, 3, 64, X86_REX_B);
        TS_ASSERT_EQUALS(low3, 3);
        TS_ASSERT_EQUALS(rex, 0x40);

        rex = low3 = 0;
        set_rex_from_reg(&rex, 0, &low3, X86Register::REG8X, 13, 64, X86_REX_B);
        TS_ASSERT_EQUALS(low3, 5);
        TS_ASSERT_EQUALS(rex, 0x41);

        // Check for errors with REG8X and neither REX or DREX available
        low3 = 0; rex = 0xff;
        TS_ASSERT_THROWS(set_rex_from_reg(&rex, 0, &low3, X86Register::REG8X, 3,
                                          64, X86_REX_W),
                         yasm::TypeError);

        // If DREX available but REX isn't, REG8X should not error
        low3 = 0; drex = 0x10; rex = 0xff;
        set_rex_from_reg(&rex, &drex, &low3, X86Register::REG8X, 3, 64, X86_REX_W);
        TS_ASSERT_EQUALS(low3, 3);
        TS_ASSERT_EQUALS(rex, 0xff);
        TS_ASSERT_EQUALS(drex, 0x10);

        drex = low3 = 0; rex = 0xff;
        set_rex_from_reg(&rex, &drex, &low3, X86Register::REG8X, 13, 64, X86_REX_W);
        TS_ASSERT_EQUALS(low3, 5);
        TS_ASSERT_EQUALS(rex, 0xff);
        TS_ASSERT_EQUALS(drex, 0x08);
    }

    void testSetRexFromReg8High()
    {
        unsigned char rex;
        unsigned char low3;

        // Use of AH/BH/CH/DH should result in disallowed REX
        rex = low3 = 0;
        set_rex_from_reg(&rex, 0, &low3, X86Register::REG8, 4, 64, X86_REX_B);
        TS_ASSERT_EQUALS(low3, 4);
        TS_ASSERT_EQUALS(rex, 0xff);

        // If REX set, use of AH/BH/CH/DH should error
        low3 = 0; rex = 0x40;
        TS_ASSERT_THROWS(set_rex_from_reg(&rex, 0, &low3, X86Register::REG8, 4,
                                          64, X86_REX_W),
                         yasm::TypeError);

        // If REX is disallowed, use of AH/BH/CH/DH is still okay
        low3 = 0; rex = 0xff;
        set_rex_from_reg(&rex, 0, &low3, X86Register::REG8, 4, 64, X86_REX_B);
        TS_ASSERT_EQUALS(low3, 4);
        TS_ASSERT_EQUALS(rex, 0xff);

        // Use of AL/BL/CL/DL should NOT error and should still allow REX.
        low3 = 0; rex = 0x40;
        set_rex_from_reg(&rex, 0, &low3, X86Register::REG8, 3, 64, X86_REX_W);
        TS_ASSERT_EQUALS(low3, 3);
        TS_ASSERT_EQUALS(rex, 0x40);
    }

    void testInitBasic()
    {
        X86EffAddr ea;
        TS_ASSERT_EQUALS(ea.m_modrm, 0);
        TS_ASSERT_EQUALS(ea.m_sib, 0);
        TS_ASSERT_EQUALS(ea.m_drex, 0);
        TS_ASSERT_EQUALS(ea.m_need_sib, 0);
        TS_ASSERT_EQUALS(ea.m_valid_modrm, false);
        TS_ASSERT_EQUALS(ea.m_need_modrm, false);
        TS_ASSERT_EQUALS(ea.m_valid_sib, false);
        TS_ASSERT_EQUALS(ea.m_need_drex, false);
        TS_ASSERT_EQUALS(ea.m_disp.has_abs(), false);
    }

    void testInitReg()
    {
        unsigned char rex;
        unsigned char drex;

        {
            X86Register reg32_5(X86Register::REG32, 5);
            rex = drex = 0;
            X86EffAddr ea(&reg32_5, &rex, &drex, 32);
            TS_ASSERT_EQUALS(ea.m_modrm, 0xC5);
            TS_ASSERT_EQUALS(ea.m_sib, 0);
            TS_ASSERT_EQUALS(ea.m_drex, 0);
            TS_ASSERT_EQUALS(ea.m_need_sib, 0);
            TS_ASSERT_EQUALS(ea.m_valid_modrm, true);
            TS_ASSERT_EQUALS(ea.m_need_modrm, true);
            TS_ASSERT_EQUALS(ea.m_valid_sib, false);
            TS_ASSERT_EQUALS(ea.m_need_drex, false);
            TS_ASSERT_EQUALS(ea.m_disp.has_abs(), false);
            TS_ASSERT_EQUALS(rex, 0);
            TS_ASSERT_EQUALS(drex, 0);
        }
    }

    // General 16-bit exhaustive expression tests
    void testInitExpr16()
    {
        struct eaform16
        {
            const char* reg[2];
            unsigned char rm;
        };
        const eaform16 forms[] =
        {
            {{"bx", "si"}, 0},
            {{"bx", "di"}, 1},
            {{"bp", "si"}, 2},
            {{"bp", "di"}, 3},
            {{"si", 0},    4},
            {{"di", 0},    5},
            {{0,    0},    6},
            {{"bx", 0},    7},
        };
        const long disps[] = {0, 16, 127, 128, -128, -129, 255, -256};

        for (const eaform16* form=forms; form != forms+NELEMS(forms); ++form)
        {
            X86Register* reg[2];

            for (unsigned int i=0; i<2; ++i)
            {
                if (!form->reg[i])
                    reg[i] = 0;
                else if (std::strcmp(form->reg[i], "bx") == 0)
                    reg[i] = &BX;
                else if (std::strcmp(form->reg[i], "bp") == 0)
                    reg[i] = &BP;
                else if (std::strcmp(form->reg[i], "si") == 0)
                    reg[i] = &SI;
                else if (std::strcmp(form->reg[i], "di") == 0)
                    reg[i] = &DI;
                else
                    TS_FAIL("unrecognized test register");
            }

            for (const long* disp=disps; disp != disps+NELEMS(disps); ++disp)
            {
                ExprTerms terms;
                if (reg[0] != 0)
                    terms.push_back(reg[0]);
                if (reg[1] != 0)
                    terms.push_back(reg[1]);
                terms.push_back(IntNum(*disp));

                do
                {
                    unsigned int expect_modrm = 0;
                    if (*disp == 0 || (reg[0] == 0 && reg[1] == 0))
                        ;                       // mod=00
                    else if (*disp >= -128 && *disp <= 127)
                        expect_modrm |= 0100;   // mod=01
                    else
                        expect_modrm |= 0200;   // mod=10

                    expect_modrm |= form->rm;

                    Expr::Ptr e(0);
                    if (terms.size() == 1)
                        e.reset(new Expr(Op::IDENT, terms));
                    else
                        e.reset(new Expr(Op::ADD, terms));
                    TS_TRACE(*e);
                    X86EffAddr ea(false, e);
                    unsigned char addrsize = 0;
                    unsigned char rex = 0;
                    TS_ASSERT_EQUALS(ea.check(&addrsize, 16, false, &rex, 0),
                                      true);
                    TS_ASSERT_EQUALS(ea.m_need_modrm, true);
                    TS_ASSERT_EQUALS(ea.m_modrm, expect_modrm);
                    TS_ASSERT_EQUALS(ea.m_need_sib, 0);
                    TS_ASSERT_EQUALS(ea.m_valid_sib, false);
                    TS_ASSERT_EQUALS(ea.m_need_drex, false);
                    TS_ASSERT_EQUALS(addrsize, 16);
                    TS_ASSERT_EQUALS(rex, 0);
                }
                while (std::next_permutation(terms.begin(), terms.end()));
                // clean up after ourselves
                std::for_each(terms.begin(), terms.end(),
                              MEMFN::mem_fn(&ExprTerm::destroy));
            }
        }
    }

    // General 32-bit exhaustive expression tests
    void testInitExpr32()
    {
        const X86Register* baseregs[] =
        {0, &EAX, &ECX, &EDX, &EBX, &ESP, &EBP, &ESI, &EDI};
        const X86Register* indexregs[] =
        {0, &EAX, &ECX, &EDX, &EBX, &EBP, &ESI, &EDI};
        const unsigned long indexes[] = {0, 1, 2, 4, 8};
        const long disps[] = {0, 16, 127, 128, -128, -129, 255, -256};

        for (const X86Register** basereg=baseregs;
             basereg != baseregs+NELEMS(baseregs); ++basereg)
        {
            for (const X86Register** indexreg=indexregs;
                 indexreg != indexregs+NELEMS(indexregs); ++indexreg)
            {
                for (const unsigned long* index=indexes;
                     index != indexes+NELEMS(indexes); ++index)
                {
                    // don't test multiplying cases if no indexreg
                    if (!*indexreg && *index != 0)
                        continue;
                    // don't test plain indexreg if no basereg (equiv expression)
                    if (!*basereg && *index == 0)
                        continue;

                    for (const long* disp=disps; disp != disps+NELEMS(disps);
                         ++disp)
                    {
                        ExprTerms terms;
                        if (*basereg != 0)
                            terms.push_back(*basereg);
                        if (*indexreg != 0)
                        {
                            if (*index == 0)
                                terms.push_back(*indexreg);
                            else
                                terms.push_back(Expr::Ptr(
                                    new Expr(*indexreg, Op::MUL, IntNum(*index))));
                        }
                        terms.push_back(IntNum(*disp));

                        //do
                        {

                const X86Register* breg = *basereg;
                const X86Register* ireg = *indexreg;
                unsigned long times = *index;

                // map indexreg*1 to basereg (optimization)
                if (breg == 0 && (times == 0 || times == 1))
                {
                    breg = ireg;
                    ireg = 0;
                }

                // map indexreg*2 to basereg+basereg
                // (optimization if not nosplit)
                if (breg == 0 && (times == 2))
                {
                    breg = ireg;
                    times = 0;
                }

                bool expect_error = false;

                // SIB is required if any indexreg or if ESP basereg
                bool need_sib =
                    (ireg != 0) ||
                    (breg == &ESP);

                // Can't use ESP as an index register
                if (ireg == &ESP)
                {
                    if (breg != &ESP && (times == 0 || times == 1))
                    {
                        // swap with base register
                        std::swap(breg, ireg);
                    }
                    else
                        expect_error = true;
                }

                unsigned char expect_modrm = 0;
                unsigned char expect_sib = 0;

                if ((*disp == 0 && breg != &EBP) || breg == 0)
                    ;                       // mod=00
                else if (*disp >= -128 && *disp <= 127)
                    expect_modrm |= 0100;   // mod=01
                else
                    expect_modrm |= 0200;   // mod=10

                if (need_sib)
                {
                    expect_modrm |= 4;
                    if (times == 0 || times == 1)
                        ;                   // ss=00
                    else if (times == 2)
                        expect_sib |= 0100; // ss=01
                    else if (times == 4)
                        expect_sib |= 0200; // ss=02
                    else if (times == 8)
                        expect_sib |= 0300; // ss=03
                    else
                        expect_error = true;

                    if (ireg)
                        expect_sib |= (ireg->num()&7)<<3;
                    else
                        expect_sib |= 4<<3;

                    if (breg)
                        expect_sib |= breg->num()&7;
                    else
                        expect_sib |= 5;
                }
                else if (breg)
                    expect_modrm |= breg->num();
                else
                    expect_modrm |= 5;

                Expr::Ptr e(0);
                if (terms.size() == 1)
                    e.reset(new Expr(Op::IDENT, terms));
                else
                    e.reset(new Expr(Op::ADD, terms));
                TS_TRACE(*e);
                X86EffAddr ea(false, e);
                unsigned char addrsize = 0;
                unsigned char rex = 0;
                if (expect_error)
                {
                    TS_ASSERT_THROWS(ea.check(&addrsize, 32, false, &rex, 0),
                                     ValueError);
                }
                else
                {
                    TS_ASSERT_EQUALS(ea.check(&addrsize, 32, false, &rex, 0),
                                      true);
                    TS_ASSERT_EQUALS(ea.m_need_modrm, true);
                    TS_ASSERT_EQUALS(ea.m_modrm, expect_modrm);
                    TS_ASSERT_EQUALS(ea.m_need_sib, need_sib);
                    TS_ASSERT_EQUALS(ea.m_valid_sib, need_sib);
                    if (need_sib)
                        TS_ASSERT_EQUALS(ea.m_sib, expect_sib);
                    TS_ASSERT_EQUALS(ea.m_need_drex, false);
                    TS_ASSERT_EQUALS(addrsize, 32);
                    TS_ASSERT_EQUALS(rex, 0);
                }

                        }
                        //while (std::next_permutation(terms.begin(), terms.end()));
                        // clean up after ourselves
                        std::for_each(terms.begin(), terms.end(),
                                      MEMFN::mem_fn(&ExprTerm::destroy));
                    }
                }
            }
        }
    }

    // Test for the hinting mechanism
    // First reg is preferred base register, unless it has *1, in which case it's
    // the preferred index register.
    void testInitExpr32Hints()
    {
        const X86Register* baseregs[] =
        {&EAX, &ECX, &EDX, &EBX, &ESP, &EBP, &ESI, &EDI};
        const X86Register* indexregs[] =
        {&EAX, &ECX, &EDX, &EBX, &EBP, &ESI, &EDI};

        for (const X86Register** basereg=baseregs;
             basereg != baseregs+NELEMS(baseregs); ++basereg)
        {
            for (const X86Register** indexreg=indexregs;
                 indexreg != indexregs+NELEMS(indexregs); ++indexreg)
            {
                Expr::Ptr e(
                    new Expr(
                        new Expr(*indexreg, Op::MUL, IntNum(1)),
                        Op::ADD,
                        *basereg));

                unsigned char expect_sib = 0;
                expect_sib |= ((*indexreg)->num()&7)<<3;
                expect_sib |= (*basereg)->num()&7;

                TS_TRACE(*e);
                X86EffAddr ea(false, e);
                unsigned char addrsize = 0;
                unsigned char rex = 0;
                TS_ASSERT_EQUALS(ea.check(&addrsize, 32, false, &rex, 0),
                                  true);
                TS_ASSERT_EQUALS(ea.m_need_modrm, true);
                TS_ASSERT_EQUALS(ea.m_need_sib, true);
                TS_ASSERT_EQUALS(ea.m_valid_sib, true);
                TS_ASSERT_EQUALS(ea.m_sib, expect_sib);
            }
        }
    }

    // ESP can't be used as an index register, make sure ESP*1+EAX works.
    void testInitExpr32HintESP()
    {
        const X86Register* indexregs[] =
        {&EAX, &ECX, &EDX, &EBX, &EBP, &ESI, &EDI};

        for (const X86Register** indexreg=indexregs;
             indexreg != indexregs+NELEMS(indexregs); ++indexreg)
        {
            Expr::Ptr e(
                new Expr(
                    new Expr(&ESP, Op::MUL, IntNum(1)),
                    Op::ADD,
                    *indexreg));

            unsigned char expect_sib = 0;
            expect_sib |= ((*indexreg)->num()&7)<<3;
            expect_sib |= ESP.num()&7;

            TS_TRACE(*e);
            X86EffAddr ea(false, e);
            unsigned char addrsize = 0;
            unsigned char rex = 0;
            TS_ASSERT_EQUALS(ea.check(&addrsize, 32, false, &rex, 0),
                              true);
            TS_ASSERT_EQUALS(ea.m_need_modrm, true);
            TS_ASSERT_EQUALS(ea.m_need_sib, true);
            TS_ASSERT_EQUALS(ea.m_valid_sib, true);
            TS_ASSERT_EQUALS(ea.m_sib, expect_sib);
        }
    }
};

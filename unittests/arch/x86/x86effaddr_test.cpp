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

#include <gtest/gtest.h>

#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Basic/FileManager.h"
#include "yasmx/Basic/SourceManager.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/Symbol.h"

#include "modules/arch/x86/X86EffAddr.h"
#include "modules/arch/x86/X86Register.h"

#include "unittests/diag_mock.h"
#include "unittests/unittest_util.h"


#ifndef NELEMS
#define NELEMS(array)   (sizeof(array) / sizeof(array[0]))
#endif

using namespace yasm;
using namespace yasm::arch;

class X86EffAddrTestBase
{
protected:
    X86Register BX, BP, SI, DI;
    X86Register EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI;

    ::testing::StrictMock<yasmunit::MockDiagnosticConsumer> mock_consumer;
    llvm::IntrusiveRefCntPtr<DiagnosticIDs> diagids;
    DiagnosticsEngine diags;
    FileSystemOptions opts;
    FileManager fmgr;
    SourceManager smgr;

    X86EffAddrTestBase()
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
        , diagids(new DiagnosticIDs)
        , diags(diagids, &mock_consumer, false)
        , fmgr(opts)
        , smgr(diags, fmgr)
    {
        diags.setSourceManager(&smgr);
    }
};

class X86EffAddrTest : public X86EffAddrTestBase, public ::testing::Test {};

TEST_F(X86EffAddrTest, SetRexFromReg)
{
    unsigned char rex;
    unsigned char low3;

    // reg >= 8 should set rex.
    rex = low3 = 0;
    EXPECT_TRUE(setRexFromReg(&rex, &low3, X86Register::REG32, 13, 64, X86_REX_B));
    EXPECT_EQ(5, low3);
    EXPECT_EQ(0x41, rex);

    rex = low3 = 0;
    EXPECT_TRUE(setRexFromReg(&rex, &low3, X86Register::REG32, 13, 64, X86_REX_X));
    EXPECT_EQ(5, low3);
    EXPECT_EQ(0x42, rex);

    rex = low3 = 0;
    EXPECT_TRUE(setRexFromReg(&rex, &low3, X86Register::REG32, 13, 64, X86_REX_R));
    EXPECT_EQ(5, low3);
    EXPECT_EQ(0x44, rex);

    rex = low3 = 0;
    EXPECT_TRUE(setRexFromReg(&rex, &low3, X86Register::REG32, 13, 64, X86_REX_W));
    EXPECT_EQ(5, low3);
    EXPECT_EQ(0x48, rex);

    // REX should OR into existing value
    low3 = 0; rex = 0x44;
    EXPECT_TRUE(setRexFromReg(&rex, &low3, X86Register::REG32, 13, 64, X86_REX_W));
    EXPECT_EQ(5, low3);
    EXPECT_EQ(0x4C, rex);
}

TEST_F(X86EffAddrTest, SetRexFromRegNoRex)
{
    unsigned char rex;
    unsigned char low3;

    // Check for errors with reg_num >= 8 and REX not available
    low3 = 0; rex = 0xff;
    EXPECT_FALSE(setRexFromReg(&rex, &low3, X86Register::REG32, 13, 64, X86_REX_W));
}

TEST_F(X86EffAddrTest, SetRexFromReg8X)
{
    unsigned char rex;
    unsigned char low3;

    // REG8X should set rex.
    rex = low3 = 0;
    EXPECT_TRUE(setRexFromReg(&rex, &low3, X86Register::REG8X, 3, 64, X86_REX_B));
    EXPECT_EQ(3, low3);
    EXPECT_EQ(0x40, rex);

    rex = low3 = 0;
    EXPECT_TRUE(setRexFromReg(&rex, &low3, X86Register::REG8X, 13, 64, X86_REX_B));
    EXPECT_EQ(5, low3);
    EXPECT_EQ(0x41, rex);

    // Check for errors with REG8X and REX not available
    low3 = 0; rex = 0xff;
    EXPECT_FALSE(setRexFromReg(&rex, &low3, X86Register::REG8X, 3, 64, X86_REX_W));
}

TEST_F(X86EffAddrTest, SetRexFromReg8High)
{
    unsigned char rex;
    unsigned char low3;

    // Use of AH/BH/CH/DH should result in disallowed REX
    rex = low3 = 0;
    EXPECT_TRUE(setRexFromReg(&rex, &low3, X86Register::REG8, 4, 64, X86_REX_B));
    EXPECT_EQ(4, low3);
    EXPECT_EQ(0xff, rex);

    // If REX set, use of AH/BH/CH/DH should error
    low3 = 0; rex = 0x40;
    EXPECT_FALSE(setRexFromReg(&rex, &low3, X86Register::REG8, 4, 64, X86_REX_W));

    // If REX is disallowed, use of AH/BH/CH/DH is still okay
    low3 = 0; rex = 0xff;
    EXPECT_TRUE(setRexFromReg(&rex, &low3, X86Register::REG8, 4, 64, X86_REX_B));
    EXPECT_EQ(4, low3);
    EXPECT_EQ(0xff, rex);

    // Use of AL/BL/CL/DL should NOT error and should still allow REX.
    low3 = 0; rex = 0x40;
    EXPECT_TRUE(setRexFromReg(&rex, &low3, X86Register::REG8, 3, 64, X86_REX_W));
    EXPECT_EQ(3, low3);
    EXPECT_EQ(0x40, rex);
}

TEST_F(X86EffAddrTest, InitBasic)
{
    X86EffAddr ea;
    EXPECT_EQ(0, ea.m_modrm);
    EXPECT_EQ(0, ea.m_sib);
    EXPECT_EQ(0, ea.m_need_sib);
    EXPECT_FALSE(ea.m_valid_modrm);
    EXPECT_FALSE(ea.m_need_modrm);
    EXPECT_FALSE(ea.m_valid_sib);
    EXPECT_FALSE(ea.m_disp.hasAbs());
}

TEST_F(X86EffAddrTest, InitReg)
{
    unsigned char rex;

    X86Register reg32_5(X86Register::REG32, 5);
    rex = 0;
    X86EffAddr ea;
    ea.setReg(&reg32_5, &rex, 32);
    EXPECT_EQ(0xC5, ea.m_modrm);
    EXPECT_EQ(0, ea.m_sib);
    EXPECT_EQ(0, ea.m_need_sib);
    EXPECT_TRUE(ea.m_valid_modrm);
    EXPECT_TRUE(ea.m_need_modrm);
    EXPECT_FALSE(ea.m_valid_sib);
    EXPECT_FALSE(ea.m_disp.hasAbs());
    EXPECT_EQ(0, rex);
}

// General 16-bit exhaustive expression tests
struct eaform16
{
    const char* reg[2];
    unsigned char rm;
};
const eaform16 X86EffAddr16TestValues[] =
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

class X86EffAddr16Test : public X86EffAddrTestBase,
                         public ::testing::TestWithParam<eaform16>
{};

INSTANTIATE_TEST_CASE_P(X86EffAddr16Tests, X86EffAddr16Test,
                        ::testing::ValuesIn(X86EffAddr16TestValues));

TEST_P(X86EffAddr16Test, InitExpr16)
{
    static const long disps[] = {0, 16, 127, 128, -128, -129, 255, -256};
    X86Register* reg[2];

    for (unsigned int i=0; i<2; ++i)
    {
        if (!GetParam().reg[i])
            reg[i] = 0;
        else if (std::strcmp(GetParam().reg[i], "bx") == 0)
            reg[i] = &BX;
        else if (std::strcmp(GetParam().reg[i], "bp") == 0)
            reg[i] = &BP;
        else if (std::strcmp(GetParam().reg[i], "si") == 0)
            reg[i] = &SI;
        else if (std::strcmp(GetParam().reg[i], "di") == 0)
            reg[i] = &DI;
        else
            FAIL() << "unrecognized test register";
    }

    for (const long* disp=disps; disp != disps+NELEMS(disps); ++disp)
    {
        ExprTerms terms;
        if (reg[0] != 0)
            terms.push_back(ExprTerm(*reg[0]));
        if (reg[1] != 0)
            terms.push_back(ExprTerm(*reg[1]));
        terms.push_back(ExprTerm(*disp));

        do
        {
            unsigned int expect_modrm = 0;
            if (*disp == 0 || (reg[0] == 0 && reg[1] == 0))
                ;                       // mod=00
            else if (*disp >= -128 && *disp <= 127)
                expect_modrm |= 0100;   // mod=01
            else
                expect_modrm |= 0200;   // mod=10

            expect_modrm |= GetParam().rm;

            Expr e = ADD(terms);
            SCOPED_TRACE(String::Format(e));
            X86EffAddr ea(false, Expr::Ptr(e.clone()));
            unsigned char addrsize = 0;
            unsigned char rex = 0;
            EXPECT_TRUE(ea.Check(&addrsize, 16, false, &rex, 0, diags));
            EXPECT_TRUE(ea.m_need_modrm);
            EXPECT_EQ(expect_modrm, ea.m_modrm);
            EXPECT_EQ(0, ea.m_need_sib);
            EXPECT_FALSE(ea.m_valid_sib);
            EXPECT_EQ(16, addrsize);
            EXPECT_EQ(0, rex);
        }
        while (std::next_permutation(terms.begin(), terms.end()));
    }
}

// General 32-bit exhaustive expression tests
TEST_F(X86EffAddrTest, InitExpr32)
{
    const X86Register* baseregs[] =
    {0, &EAX, &ECX, &EDX, &EBX, &ESP, &EBP, &ESI, &EDI};
    const X86Register* indexregs[] =
    {0, &EAX, &ECX, &EDX, &EBX, &ESP, &EBP, &ESI, &EDI};
    static const unsigned long indexes[] = {0, 1, 2, 4, 8, 10};
    static const long disps[] = {0, 16, 127, 128, -128, -129, 255, -256};

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
                    Expr e;
                    if (*basereg != 0)
                        e += **basereg;
                    if (*indexreg != 0)
                    {
                        if (*index == 0)
                            e += **indexreg;
                        else
                            e += MUL(**indexreg, *index);
                    }
                    e += IntNum(*disp);

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
                    expect_sib |= (ireg->getNum()&7)<<3;
                else
                    expect_sib |= 4<<3;

                if (breg)
                    expect_sib |= breg->getNum()&7;
                else
                    expect_sib |= 5;
            }
            else if (breg)
                expect_modrm |= breg->getNum();
            else
                expect_modrm |= 5;

            SCOPED_TRACE(String::Format(e));
            X86EffAddr ea(false, Expr::Ptr(e.clone()));
            unsigned char addrsize = 0;
            unsigned char rex = 0;
            if (expect_error)
            {
                ::testing::StrictMock<yasmunit::MockDiagnosticConsumer> mock_consumer2;
                DiagnosticsEngine diags2(diagids, &mock_consumer2, false);
                diags2.setSourceManager(&smgr);
                EXPECT_CALL(mock_consumer2,
                            HandleDiagnostic(DiagnosticsEngine::Error, ::testing::_));
                EXPECT_FALSE(ea.Check(&addrsize, 32, false, &rex, 0, diags2));
            }
            else
            {
                EXPECT_TRUE(ea.Check(&addrsize, 32, false, &rex, 0, diags));
                EXPECT_TRUE(ea.m_need_modrm);
                EXPECT_EQ(expect_modrm, ea.m_modrm);
                EXPECT_EQ(need_sib?1:0, ea.m_need_sib);
                EXPECT_EQ(need_sib, ea.m_valid_sib);
                if (need_sib)
                    EXPECT_EQ(expect_sib, ea.m_sib);
                EXPECT_EQ(32, addrsize);
                EXPECT_EQ(0, rex);
            }

                    }
                    //while (std::next_permutation(terms.begin(), terms.end()));
                }
            }
        }
    }
}

// Test for the hinting mechanism
// First reg is preferred base register, unless it has *1, in which case it's
// the preferred index register.
TEST_F(X86EffAddrTest, InitExpr32Hints)
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
            Expr e(**indexreg);
            e *= IntNum(1);
            e += **basereg;

            unsigned char expect_sib = 0;
            expect_sib |= ((*indexreg)->getNum()&7)<<3;
            expect_sib |= (*basereg)->getNum()&7;

            SCOPED_TRACE(String::Format(e));
            X86EffAddr ea(false, Expr::Ptr(e.clone()));
            unsigned char addrsize = 0;
            unsigned char rex = 0;
            EXPECT_TRUE(ea.Check(&addrsize, 32, false, &rex, 0, diags));
            EXPECT_TRUE(ea.m_need_modrm);
            EXPECT_EQ(1, ea.m_need_sib);
            EXPECT_TRUE(ea.m_valid_sib);
            EXPECT_EQ(expect_sib, ea.m_sib);
        }
    }
}

// ESP can't be used as an index register, make sure ESP*1+EAX works.
TEST_F(X86EffAddrTest, InitExpr32HintESP)
{
    const X86Register* indexregs[] = {&EAX, &ECX, &EDX, &EBX, &EBP, &ESI, &EDI};

    for (const X86Register** indexreg=indexregs;
         indexreg != indexregs+NELEMS(indexregs); ++indexreg)
    {
        Expr e(ESP);
        e *= IntNum(1);
        e += **indexreg;

        unsigned char expect_sib = 0;
        expect_sib |= ((*indexreg)->getNum()&7)<<3;
        expect_sib |= ESP.getNum()&7;

        SCOPED_TRACE(String::Format(e));
        X86EffAddr ea(false, Expr::Ptr(e.clone()));
        unsigned char addrsize = 0;
        unsigned char rex = 0;
        EXPECT_TRUE(ea.Check(&addrsize, 32, false, &rex, 0, diags));
        EXPECT_TRUE(ea.m_need_modrm);
        EXPECT_EQ(1, ea.m_need_sib);
        EXPECT_TRUE(ea.m_valid_sib);
        EXPECT_EQ(expect_sib, ea.m_sib);
    }
}

TEST_F(X86EffAddrTest, Check32MulSub)
{
    // eax*2+ebx*2-ebx
    // Needs to realize ebx can't be an indexreg
    Expr e = ADD(MUL(EAX, 2), MUL(EBX, 2), NEG(EBX));
    SCOPED_TRACE(String::Format(e));
    X86EffAddr ea(false, Expr::Ptr(e.clone()));
    unsigned char addrsize = 0;
    unsigned char rex = 0;
    EXPECT_TRUE(ea.Check(&addrsize, 32, false, &rex, 0, diags));
    EXPECT_TRUE(ea.m_need_modrm);
    EXPECT_EQ(1, ea.m_need_sib);
    EXPECT_TRUE(ea.m_valid_sib);
    unsigned char expect_sib = 1<<6;
    expect_sib |= (EAX.getNum()&7)<<3;
    expect_sib |= EBX.getNum()&7;
    EXPECT_EQ(expect_sib, ea.m_sib);
}

class X86EffAddrMultTest : public X86EffAddrTestBase,
                           public ::testing::TestWithParam<int>
{};

INSTANTIATE_TEST_CASE_P(X86EffAddrMultTests, X86EffAddrMultTest,
                        ::testing::Values(2, 3, 4, 5, 8, 9));

TEST_P(X86EffAddrMultTest, DistExpr)
{
    int mult = GetParam();
    unsigned char addrsize, rex, expect_sib;
    Expr e = ADD(EAX, 5);
    e *= IntNum(mult);
    {
        SCOPED_TRACE(String::Format(e));
        X86EffAddr ea(false, Expr::Ptr(e.clone()));
        addrsize = 0;
        rex = 0;
        EXPECT_TRUE(ea.Check(&addrsize, 32, false, &rex, 0, diags));
        EXPECT_TRUE(ea.m_need_modrm);
        EXPECT_EQ(1, ea.m_need_sib);
        EXPECT_TRUE(ea.m_valid_sib);
        // EAX*2 will get split to EAX+EAX
        if (mult>7)
            expect_sib = 3<<6;
        else if (mult>3)
            expect_sib = 2<<6;
        else if (mult>2)
            expect_sib = 1<<6;
        else
            expect_sib = 0;
        expect_sib |= (EAX.getNum()&7)<<3;
        expect_sib |= (mult % 2 == 0 && mult != 2) ? 5 : (EAX.getNum()&7);
        EXPECT_EQ(expect_sib, ea.m_sib);
        EXPECT_EQ(String::Format(mult*5),
                  String::Format(*ea.m_disp.getAbs()));
    }

    // try it one level down too
    e += 6;
    {
        SCOPED_TRACE(String::Format(e));
        X86EffAddr ea(false, Expr::Ptr(e.clone()));
        addrsize = 0;
        rex = 0;
        EXPECT_TRUE(ea.Check(&addrsize, 32, false, &rex, 0, diags));
        EXPECT_TRUE(ea.m_need_modrm);
        EXPECT_EQ(1, ea.m_need_sib);
        EXPECT_TRUE(ea.m_valid_sib);
        EXPECT_EQ(expect_sib, ea.m_sib);
        EXPECT_EQ(String::Format(mult*5+6),
                  String::Format(*ea.m_disp.getAbs()));
    }
}

TEST_F(X86EffAddrTest, DistExprMultilevel)
{
    // (((eax+5)*2)+6)*2 ==> eax*4+32
    // ((eax*2+10)+6)*2
    // (eax*2+16)*2
    // eax*4+32
    Expr e;
    unsigned char addrsize, rex, expect_sib;

    {
        e = ADD(EAX, 5);
        e *= 2;
        e += 6;
        e *= 2;
        SCOPED_TRACE(String::Format(e));
        X86EffAddr ea(false, Expr::Ptr(e.clone()));
        addrsize = 0;
        rex = 0;
        EXPECT_TRUE(ea.Check(&addrsize, 32, false, &rex, 0, diags));
        EXPECT_TRUE(ea.m_need_modrm);
        EXPECT_EQ(1, ea.m_need_sib);
        EXPECT_TRUE(ea.m_valid_sib);
        expect_sib = 2<<6;
        expect_sib |= (EAX.getNum()&7)<<3;
        expect_sib |= 5;
        EXPECT_EQ(expect_sib, ea.m_sib);
        EXPECT_EQ(String::Format(((5*2)+6)*2),
                  String::Format(*ea.m_disp.getAbs()));
    }

    // (6+(eax+5)*2)*2 ==> 32+eax*4
    // (6+eax*2+10)*2
    // (16+eax*2)*2
    // 32+eax*4
    {
        e = 6;
        e += MUL(ADD(EAX, 5), 2);
        e *= 2;
        SCOPED_TRACE(String::Format(e));
        X86EffAddr ea(false, Expr::Ptr(e.clone()));
        addrsize = 0;
        rex = 0;
        EXPECT_TRUE(ea.Check(&addrsize, 32, false, &rex, 0, diags));
        EXPECT_TRUE(ea.m_need_modrm);
        EXPECT_EQ(1, ea.m_need_sib);
        EXPECT_TRUE(ea.m_valid_sib);
        EXPECT_EQ(expect_sib, ea.m_sib);
        EXPECT_EQ(String::Format((6+(5*2))*2),
                  String::Format(*ea.m_disp.getAbs()));
    }
}

TEST_F(X86EffAddrTest, DistExprMultiple)
{
    // (eax+1)*2+(eax+1)*3
    Expr e = ADD(EAX, 1);
    e *= 2;
    e += MUL(ADD(EAX, 1), 3);
    SCOPED_TRACE(String::Format(e));
    X86EffAddr ea(false, Expr::Ptr(e.clone()));
    unsigned char addrsize = 0;
    unsigned char rex = 0;
    EXPECT_TRUE(ea.Check(&addrsize, 32, false, &rex, 0, diags));
    EXPECT_TRUE(ea.m_need_modrm);
    EXPECT_EQ(1, ea.m_need_sib);
    EXPECT_TRUE(ea.m_valid_sib);
    unsigned char expect_sib = 2<<6;
    expect_sib |= (EAX.getNum()&7)<<3;
    expect_sib |= EAX.getNum()&7;
    EXPECT_EQ(expect_sib, ea.m_sib);
    EXPECT_EQ("5", String::Format(*ea.m_disp.getAbs()));
}

TEST_F(X86EffAddrTest, DistExprMultiple2)
{
    // (eax+ebx+1)*2-ebx
    Expr e = ADD(EAX, EBX, 1);
    e *= 2;
    e -= EBX;
    SCOPED_TRACE(String::Format(e));
    X86EffAddr ea(false, Expr::Ptr(e.clone()));
    unsigned char addrsize = 0;
    unsigned char rex = 0;
    EXPECT_TRUE(ea.Check(&addrsize, 32, false, &rex, 0, diags));
    EXPECT_TRUE(ea.m_need_modrm);
    EXPECT_EQ(1, ea.m_need_sib);
    EXPECT_TRUE(ea.m_valid_sib);
    unsigned char expect_sib = 1<<6;
    expect_sib |= (EAX.getNum()&7)<<3;
    expect_sib |= EBX.getNum()&7;
    EXPECT_EQ(expect_sib, ea.m_sib);
    EXPECT_EQ("2", String::Format(*ea.m_disp.getAbs()));
}

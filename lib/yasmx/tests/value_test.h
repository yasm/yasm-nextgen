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
#include <cxxtest/TestSuite.h>

#include "yasmx/Expr.h"
#include "yasmx/Object.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"
#include "yasmx/Value.h"

class ValueTest : public CxxTest::TestSuite
{
public:
    class MockRegister : public yasm::Register
    {
    public:
        MockRegister(const char* name) : m_name(name) {}
        unsigned int getSize() const { return 0; }
        unsigned int getNum() const { return m_name[0]-'a'; }
        void Put(std::ostream& os) const { os << m_name; }

    private:
        const char* m_name;
    };

    Symbol sym1_sym, sym2_sym, wrt_sym;
    SymbolRef sym1, sym2, wrt;

    ValueTest()
        : sym1_sym("sym1")
        , sym2_sym("sym2")
        , wrt_sym("wrt")
        , sym1(&sym1_sym)
        , sym2(&sym2_sym)
        , wrt(&wrt_sym)
    {
    }

    void testConstructSize()
    {
        Value v(4);
        TS_ASSERT_EQUALS(v.hasAbs(), false);
        TS_ASSERT_EQUALS(v.isRelative(), false);
        TS_ASSERT_EQUALS(v.isWRT(), false);
        TS_ASSERT_EQUALS(v.hasSubRelative(), false);
        TS_ASSERT_EQUALS(v.m_next_insn, 0U);
        TS_ASSERT_EQUALS(v.m_seg_of, false);
        TS_ASSERT_EQUALS(v.m_rshift, 0U);
        TS_ASSERT_EQUALS(v.m_ip_rel, false);
        TS_ASSERT_EQUALS(v.m_jump_target, false);
        TS_ASSERT_EQUALS(v.m_section_rel, false);
        TS_ASSERT_EQUALS(v.m_no_warn, false);
        TS_ASSERT_EQUALS(v.m_sign, false);
        TS_ASSERT_EQUALS(v.m_size, 4U);
    }

    void testConstructExpr()
    {
        Expr::Ptr ep(new Expr(sym1));
        Expr* e = ep.get();
        Value v(6, ep);
        TS_ASSERT_EQUALS(v.getAbs(), e);
        TS_ASSERT_EQUALS(v.isRelative(), false);
        TS_ASSERT_EQUALS(v.isWRT(), false);
        TS_ASSERT_EQUALS(v.hasSubRelative(), false);
        TS_ASSERT_EQUALS(v.m_next_insn, 0U);
        TS_ASSERT_EQUALS(v.m_seg_of, false);
        TS_ASSERT_EQUALS(v.m_rshift, 0U);
        TS_ASSERT_EQUALS(v.m_ip_rel, false);
        TS_ASSERT_EQUALS(v.m_jump_target, false);
        TS_ASSERT_EQUALS(v.m_section_rel, false);
        TS_ASSERT_EQUALS(v.m_no_warn, false);
        TS_ASSERT_EQUALS(v.m_sign, false);
        TS_ASSERT_EQUALS(v.m_size, 6U);
    }

    void testConstructSymbol()
    {
        Value v(8, sym1);
        TS_ASSERT_EQUALS(v.hasAbs(), false);
        TS_ASSERT_EQUALS(v.getRelative(), sym1);
        TS_ASSERT_EQUALS(v.isWRT(), false);
        TS_ASSERT_EQUALS(v.hasSubRelative(), false);
        TS_ASSERT_EQUALS(v.m_next_insn, 0U);
        TS_ASSERT_EQUALS(v.m_seg_of, false);
        TS_ASSERT_EQUALS(v.m_rshift, 0U);
        TS_ASSERT_EQUALS(v.m_ip_rel, false);
        TS_ASSERT_EQUALS(v.m_jump_target, false);
        TS_ASSERT_EQUALS(v.m_section_rel, false);
        TS_ASSERT_EQUALS(v.m_no_warn, false);
        TS_ASSERT_EQUALS(v.m_sign, false);
        TS_ASSERT_EQUALS(v.m_size, 8U);
    }

    void testFinalize()
    {
        Object object("x", "y", 0);
        SymbolRef a = object.getSymbol("a");    // external
        SymbolRef b = object.getSymbol("b");    // external
        SymbolRef c = object.getSymbol("c");    // in section x
        SymbolRef d = object.getSymbol("d");    // in section x
        SymbolRef e = object.getSymbol("e");    // in section y
        SymbolRef f = object.getSymbol("f");    // in section y
        const MockRegister g("g");

        Section* x = new Section("x", false, false, 0);
        object.AppendSection(std::auto_ptr<Section>(x));
        Section* y = new Section("y", false, false, 0);
        object.AppendSection(std::auto_ptr<Section>(y));

        Location loc = {&x->FreshBytecode(), 0};
        c->DefineLabel(loc, 0);
        d->DefineLabel(loc, 0);
        loc.bc = &y->FreshBytecode();
        e->DefineLabel(loc, 0);
        f->DefineLabel(loc, 0);

        Value v(8);

        // just an integer
        v = Value(8, Expr::Ptr(new Expr(4)));
        TS_ASSERT_EQUALS(v.Finalize(), true);
        TS_ASSERT_EQUALS(v.hasAbs(), true);
        TS_ASSERT_EQUALS(String::Format(*v.getAbs()), "4");
        TS_ASSERT_EQUALS(v.isRelative(), false);

        // simple relative
        v = Value(8, Expr::Ptr(new Expr(a)));
        TS_ASSERT_EQUALS(v.Finalize(), true);
        TS_ASSERT_EQUALS(v.hasAbs(), false);
        TS_ASSERT_EQUALS(v.getRelative(), a);

        // masked relative
        v = Value(8, Expr::Ptr(new Expr(AND(a, 0xff))));
        TS_ASSERT_EQUALS(v.Finalize(), true);
        TS_ASSERT_EQUALS(v.hasAbs(), false);
        TS_ASSERT_EQUALS(v.getRelative(), a);
        TS_ASSERT_EQUALS(v.m_no_warn, true);

        v = Value(8, Expr::Ptr(new Expr(AND(a, 0x7f))));
        TS_ASSERT_EQUALS(v.Finalize(), false);      // invalid
        TS_ASSERT_EQUALS(v.hasAbs(), true);
        TS_ASSERT_EQUALS(String::Format(*v.getAbs()), "a&127");
        TS_ASSERT_EQUALS(v.isRelative(), false);
        TS_ASSERT_EQUALS(v.m_no_warn, false);

        // rel-rel (rel may be external)
        v = Value(8, Expr::Ptr(new Expr(SUB(a, a))));
        TS_ASSERT_EQUALS(v.Finalize(), true);
        TS_ASSERT_EQUALS(v.hasAbs(), false);
        TS_ASSERT_EQUALS(v.isRelative(), false);

        // abs+(rel-rel)
        v = Value(8, Expr::Ptr(new Expr(ADD(5, SUB(a, a)))));
        TS_ASSERT_EQUALS(v.Finalize(), true);
        TS_ASSERT_EQUALS(v.hasAbs(), true);
        TS_ASSERT_EQUALS(String::Format(*v.getAbs()), "5");
        TS_ASSERT_EQUALS(v.isRelative(), false);

        // (rel1+rel2)-rel2, all external
        v = Value(8, Expr::Ptr(new Expr(SUB(ADD(a, b), b))));
        TS_ASSERT_EQUALS(v.Finalize(), true);
        TS_ASSERT_EQUALS(v.hasAbs(), false);
        TS_ASSERT_EQUALS(v.getRelative(), a);

        // rel1-rel2 in same section gets left in abs portion
        v = Value(8, Expr::Ptr(new Expr(SUB(c, d))));
        TS_ASSERT_EQUALS(v.Finalize(), true);
        TS_ASSERT_EQUALS(v.hasAbs(), true);
        TS_ASSERT_EQUALS(String::Format(*v.getAbs()), "c+(d*-1)");
        TS_ASSERT_EQUALS(v.isRelative(), false);

        // rel1-rel2 in different sections -> rel and sub portions, no abs
        v = Value(8, Expr::Ptr(new Expr(SUB(d, e))));
        TS_ASSERT_EQUALS(v.Finalize(), true);
        TS_ASSERT_EQUALS(v.hasAbs(), false);
        TS_ASSERT_EQUALS(v.getRelative(), d);
        TS_ASSERT_EQUALS(v.getSubSymbol(), e);

        // rel1 WRT rel2
        v = Value(8, Expr::Ptr(new Expr(WRT(a, b))));
        TS_ASSERT_EQUALS(v.Finalize(), true);
        TS_ASSERT_EQUALS(v.hasAbs(), false);
        TS_ASSERT_EQUALS(v.getRelative(), a);
        TS_ASSERT_EQUALS(v.getWRT(), b);

        // rel1 WRT reg
        v = Value(8, Expr::Ptr(new Expr(WRT(a, g))));
        TS_ASSERT_EQUALS(v.Finalize(), true);
        TS_ASSERT_EQUALS(v.hasAbs(), true);
        TS_ASSERT_EQUALS(String::Format(*v.getAbs()), "0 WRT g");
        TS_ASSERT_EQUALS(v.getRelative(), a);

        // rel1 WRT 5 --> error
        v = Value(8, Expr::Ptr(new Expr(WRT(a, 5))));
        TS_ASSERT_EQUALS(v.Finalize(), false);

        // rel1 WRT (5+rel2) --> error
        v = Value(8, Expr::Ptr(new Expr(WRT(a, ADD(5, b)))));
        TS_ASSERT_EQUALS(v.Finalize(), false);

        // 5+(rel1 WRT rel2)
        v = Value(8, Expr::Ptr(new Expr(ADD(5, WRT(a, b)))));
        TS_ASSERT_EQUALS(v.Finalize(), true);
        TS_ASSERT_EQUALS(v.hasAbs(), true);
        TS_ASSERT_EQUALS(String::Format(*v.getAbs()), "5");
        TS_ASSERT_EQUALS(v.getRelative(), a);
        TS_ASSERT_EQUALS(v.getWRT(), b);

        // (5+rel1) WRT rel2
        v = Value(8, Expr::Ptr(new Expr(WRT(ADD(5, a), b))));
        TS_ASSERT_EQUALS(v.Finalize(), true);
        TS_ASSERT_EQUALS(v.hasAbs(), true);
        TS_ASSERT_EQUALS(String::Format(*v.getAbs()), "5");
        TS_ASSERT_EQUALS(v.getRelative(), a);
        TS_ASSERT_EQUALS(v.getWRT(), b);

        // (rel1 WRT reg) WRT rel2 --> OK
        v = Value(8, Expr::Ptr(new Expr(WRT(ADD(5, a), b))));
        TS_ASSERT_EQUALS(v.Finalize(), true);
        TS_ASSERT_EQUALS(v.hasAbs(), true);
        TS_ASSERT_EQUALS(String::Format(*v.getAbs()), "5");
        TS_ASSERT_EQUALS(v.getRelative(), a);
        TS_ASSERT_EQUALS(v.getWRT(), b);

        // (rel1 WRT rel2) WRT rel3 --> error
        v = Value(8, Expr::Ptr(new Expr(WRT(WRT(a, b), c))));
        TS_ASSERT_EQUALS(v.Finalize(), false);

        // SEG reg1
        v = Value(8, Expr::Ptr(new Expr(SEG(a))));
        TS_ASSERT_EQUALS(v.Finalize(), true);
        TS_ASSERT_EQUALS(v.hasAbs(), false);
        TS_ASSERT_EQUALS(v.getRelative(), a);
        TS_ASSERT_EQUALS(v.m_seg_of, true);

        // SEG 5 --> error
        v = Value(8, Expr::Ptr(new Expr(SEG(5))));
        TS_ASSERT_EQUALS(v.Finalize(), false);

        // rel1+SEG rel1 --> error
        v = Value(8, Expr::Ptr(new Expr(ADD(a, SEG(a)))));
        TS_ASSERT_EQUALS(v.Finalize(), false);

        // rel1>>5
        v = Value(8, Expr::Ptr(new Expr(SHR(a, 5))));
        TS_ASSERT_EQUALS(v.Finalize(), true);
        TS_ASSERT_EQUALS(v.hasAbs(), false);
        if (Expr* expr = v.getAbs())
            TS_TRACE(String::Format(*expr));
        TS_ASSERT_EQUALS(v.getRelative(), a);
        TS_ASSERT_EQUALS(v.m_rshift, 5U);

        // (rel1>>5)>>6
        v = Value(8, Expr::Ptr(new Expr(SHR(SHR(a, 5), 6))));
        TS_ASSERT_EQUALS(v.Finalize(), true);
        TS_ASSERT_EQUALS(v.hasAbs(), false);
        TS_ASSERT_EQUALS(v.getRelative(), a);
        TS_ASSERT_EQUALS(v.m_rshift, 11U);

        // rel1>>reg --> error
        v = Value(8, Expr::Ptr(new Expr(SHR(a, g))));
        TS_ASSERT_EQUALS(v.Finalize(), false);

        // rel1+rel1>>5 --> error
        v = Value(8, Expr::Ptr(new Expr(ADD(a, SHR(a, 5)))));
        TS_ASSERT_EQUALS(v.Finalize(), false);

        // 5>>rel1 --> error
        v = Value(8, Expr::Ptr(new Expr(SHR(5, a))));
        TS_ASSERT_EQUALS(v.Finalize(), false);
    }

    void testClear()
    {
        Value v(6, Expr::Ptr(new Expr(WRT(sym1, wrt))));
        v.Finalize();
        Bytecode bc;
        Location loc = {&bc, 0};
        v.SubRelative(0, loc);
        TS_ASSERT_EQUALS(v.hasAbs(), false);
        TS_ASSERT_EQUALS(v.getRelative(), sym1);
        TS_ASSERT_EQUALS(v.getWRT(), wrt);
        TS_ASSERT_EQUALS(v.hasSubRelative(), true);
        v.setLine(4);
        v.m_next_insn = 3;
        v.m_seg_of = true;
        v.m_rshift = 5;
        v.m_ip_rel = true;
        v.m_jump_target = true;
        v.m_section_rel = true;
        v.m_no_warn = true;
        v.m_sign = true;

        v.Clear();

        TS_ASSERT_EQUALS(v.hasAbs(), false);
        TS_ASSERT_EQUALS(v.isRelative(), false);
        TS_ASSERT_EQUALS(v.isWRT(), false);
        TS_ASSERT_EQUALS(v.hasSubRelative(), false);
        TS_ASSERT_EQUALS(v.getLine(), 0U);
        TS_ASSERT_EQUALS(v.m_next_insn, 0U);
        TS_ASSERT_EQUALS(v.m_seg_of, false);
        TS_ASSERT_EQUALS(v.m_rshift, 0U);
        TS_ASSERT_EQUALS(v.m_ip_rel, false);
        TS_ASSERT_EQUALS(v.m_jump_target, false);
        TS_ASSERT_EQUALS(v.m_section_rel, false);
        TS_ASSERT_EQUALS(v.m_no_warn, false);
        TS_ASSERT_EQUALS(v.m_sign, false);
        TS_ASSERT_EQUALS(v.m_size, 0U);
    }

    void testClearRelative()
    {
        Value v(6, Expr::Ptr(new Expr(WRT(sym1, wrt))));
        v.Finalize();
        Bytecode bc;
        Location loc = {&bc, 0};
        v.SubRelative(0, loc);
        v.m_next_insn = 3;
        v.m_seg_of = true;
        v.m_rshift = 5;
        v.m_ip_rel = true;
        v.m_jump_target = true;
        v.m_section_rel = true;
        v.m_no_warn = true;
        v.m_sign = true;

        v.ClearRelative();

        TS_ASSERT_EQUALS(v.hasAbs(), false);
        TS_ASSERT_EQUALS(v.isRelative(), false);
        TS_ASSERT_EQUALS(v.isWRT(), false);
        TS_ASSERT_EQUALS(v.hasSubRelative(), false);
        TS_ASSERT_EQUALS(v.m_next_insn, 3U);
        TS_ASSERT_EQUALS(v.m_seg_of, false);
        TS_ASSERT_EQUALS(v.m_rshift, 0U);
        TS_ASSERT_EQUALS(v.m_ip_rel, false);
        TS_ASSERT_EQUALS(v.m_jump_target, true);
        TS_ASSERT_EQUALS(v.m_section_rel, false);
        TS_ASSERT_EQUALS(v.m_no_warn, true);
        TS_ASSERT_EQUALS(v.m_sign, true);
        TS_ASSERT_EQUALS(v.m_size, 6U);
    }

    void testAddAbsInt()
    {
        Value v(4);
        TS_ASSERT_EQUALS(v.hasAbs(), false);
        // add to an empty abs
        v.AddAbs(6);
        TS_ASSERT_EQUALS(v.hasAbs(), true);
        TS_ASSERT_EQUALS(v.getAbs()->getIntNum(), 6);
        // add to an abs with a value
        v.AddAbs(8);
        v.getAbs()->Simplify();
        TS_ASSERT_EQUALS(v.getAbs()->getIntNum(), 14);
    }

    void testAddAbsExpr()
    {
        Value v(4);
        TS_ASSERT_EQUALS(v.hasAbs(), false);
        // add to an empty abs
        v.AddAbs(Expr(6));
        TS_ASSERT_EQUALS(v.hasAbs(), true);
        v.getAbs()->Simplify();
        TS_ASSERT_EQUALS(v.getAbs()->getIntNum(), 6);
        // add to an abs with a value
        v.AddAbs(Expr(8));
        v.getAbs()->Simplify();
        TS_ASSERT_EQUALS(v.getAbs()->getIntNum(), 14);
    }

    void testIsRelative()
    {
        Value v1(4);
        TS_ASSERT_EQUALS(v1.isRelative(), false);
        TS_ASSERT_EQUALS(v1.getRelative(), SymbolRef(0));

        Value v2(4, sym1);
        TS_ASSERT_EQUALS(v2.isRelative(), true);
        TS_ASSERT_EQUALS(v2.getRelative(), sym1);
    }

    void testIsWRT()
    {
        Value v1(4);
        TS_ASSERT_EQUALS(v1.isWRT(), false);
        TS_ASSERT_EQUALS(v1.getWRT(), SymbolRef(0));

        Value v2(6, Expr::Ptr(new Expr(WRT(sym1, wrt))));
        v2.Finalize();
        TS_ASSERT_EQUALS(v2.isWRT(), true);
        TS_ASSERT_EQUALS(v2.getWRT(), wrt);
    }

    void test_RSHIFT_MAX()
    {
        Value v(4);
        unsigned int last_rshift;
        do {
            last_rshift = v.m_rshift;
            ++v.m_rshift;
        } while (v.m_rshift != 0 && last_rshift < 60000);   // 60000 is safety
        TS_ASSERT_EQUALS(last_rshift, Value::RSHIFT_MAX);
    }

    void testSubRelative()
    {
        Bytecode bc;
        Location loc = {&bc, 0};
        Location loc2;
        Value v(4, sym1);
        TS_ASSERT_EQUALS(v.getRelative(), sym1);
        v.SubRelative(0, loc); // object=0 okay if m_rel set
        TS_ASSERT_EQUALS(v.getRelative(), sym1);
        TS_ASSERT_EQUALS(v.getSubLocation(&loc2), true);
        TS_ASSERT_EQUALS(loc, loc2);

        Object object("x", "y", 0);
        v = Value(4, sym1);
        v.SubRelative(&object, loc);
        TS_ASSERT_EQUALS(v.getRelative(), sym1);    // shouldn't change m_rel
        loc2.bc = 0;
        TS_ASSERT_EQUALS(v.getSubLocation(&loc2), true);
        TS_ASSERT_EQUALS(loc, loc2);

        v = Value(4);
        v.SubRelative(&object, loc);
        TS_ASSERT_EQUALS(v.getRelative(), object.getAbsoluteSymbol());
        loc2.bc = 0;
        TS_ASSERT_EQUALS(v.getSubLocation(&loc2), true);
        TS_ASSERT_EQUALS(loc, loc2);
    }

    void testCalcPCRelSub()
    {
        // TODO
    }

    void testGetSetLine()
    {
        Value v(4);
        TS_ASSERT_EQUALS(v.getLine(), 0U);
        v.setLine(5);
        TS_ASSERT_EQUALS(v.getLine(), 5U);
    }

    void testGetIntNum()
    {
        IntNum intn;
        bool rv;

        // just a size, should be =0
        Value v(4);
        rv = v.getIntNum(&intn, false);
        TS_ASSERT_EQUALS(rv, true);
        TS_ASSERT_EQUALS(intn, 0);

        // just an integer, should be =int
        v.AddAbs(5);
        rv = v.getIntNum(&intn, false);
        TS_ASSERT_EQUALS(rv, true);
        TS_ASSERT_EQUALS(intn, 5);

        // with relative portion, not possible (returns false)
        Value v2(6, sym1);
        rv = v2.getIntNum(&intn, false);
        TS_ASSERT_EQUALS(rv, false);

        // TODO: calc_bc_dist-using tests
    }
};

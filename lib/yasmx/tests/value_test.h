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
        unsigned int get_size() const { return 0; }
        unsigned int get_num() const { return m_name[0]-'a'; }
        void put(std::ostream& os) const { os << m_name; }

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

    void test_construct_size()
    {
        Value v(4);
        TS_ASSERT_EQUALS(v.has_abs(), false);
        TS_ASSERT_EQUALS(v.is_relative(), false);
        TS_ASSERT_EQUALS(v.is_wrt(), false);
        TS_ASSERT_EQUALS(v.has_sub(), false);
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

    void test_construct_expr()
    {
        Expr::Ptr ep(new Expr(sym1));
        Expr* e = ep.get();
        Value v(6, ep);
        TS_ASSERT_EQUALS(v.get_abs(), e);
        TS_ASSERT_EQUALS(v.is_relative(), false);
        TS_ASSERT_EQUALS(v.is_wrt(), false);
        TS_ASSERT_EQUALS(v.has_sub(), false);
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

    void test_construct_symbol()
    {
        Value v(8, sym1);
        TS_ASSERT_EQUALS(v.has_abs(), false);
        TS_ASSERT_EQUALS(v.get_rel(), sym1);
        TS_ASSERT_EQUALS(v.is_wrt(), false);
        TS_ASSERT_EQUALS(v.has_sub(), false);
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

    void test_finalize()
    {
        Object object("x", "y", 0);
        SymbolRef a = object.get_symbol("a");   // external
        SymbolRef b = object.get_symbol("b");   // external
        SymbolRef c = object.get_symbol("c");   // in section x
        SymbolRef d = object.get_symbol("d");   // in section x
        SymbolRef e = object.get_symbol("e");   // in section y
        SymbolRef f = object.get_symbol("f");   // in section y
        const MockRegister g("g");

        Section* x = new Section("x", false, false, 0);
        object.append_section(std::auto_ptr<Section>(x));
        Section* y = new Section("y", false, false, 0);
        object.append_section(std::auto_ptr<Section>(y));

        Location loc = {&x->fresh_bytecode(), 0};
        c->define_label(loc, 0);
        d->define_label(loc, 0);
        loc.bc = &y->fresh_bytecode();
        e->define_label(loc, 0);
        f->define_label(loc, 0);

        Value v(8);

        // just an integer
        v = Value(8, Expr::Ptr(new Expr(4)));
        TS_ASSERT_EQUALS(v.finalize(), true);
        TS_ASSERT_EQUALS(v.has_abs(), true);
        TS_ASSERT_EQUALS(String::format(*v.get_abs()), "4");
        TS_ASSERT_EQUALS(v.is_relative(), false);

        // simple relative
        v = Value(8, Expr::Ptr(new Expr(a)));
        TS_ASSERT_EQUALS(v.finalize(), true);
        TS_ASSERT_EQUALS(v.has_abs(), false);
        TS_ASSERT_EQUALS(v.get_rel(), a);

        // masked relative
        v = Value(8, Expr::Ptr(new Expr(AND(a, 0xff))));
        TS_ASSERT_EQUALS(v.finalize(), true);
        TS_ASSERT_EQUALS(v.has_abs(), false);
        TS_ASSERT_EQUALS(v.get_rel(), a);
        TS_ASSERT_EQUALS(v.m_no_warn, true);

        v = Value(8, Expr::Ptr(new Expr(AND(a, 0x7f))));
        TS_ASSERT_EQUALS(v.finalize(), false);      // invalid
        TS_ASSERT_EQUALS(v.has_abs(), true);
        TS_ASSERT_EQUALS(String::format(*v.get_abs()), "a&127");
        TS_ASSERT_EQUALS(v.is_relative(), false);
        TS_ASSERT_EQUALS(v.m_no_warn, false);

        // rel-rel (rel may be external)
        v = Value(8, Expr::Ptr(new Expr(SUB(a, a))));
        TS_ASSERT_EQUALS(v.finalize(), true);
        TS_ASSERT_EQUALS(v.has_abs(), false);
        TS_ASSERT_EQUALS(v.is_relative(), false);

        // abs+(rel-rel)
        v = Value(8, Expr::Ptr(new Expr(ADD(5, SUB(a, a)))));
        TS_ASSERT_EQUALS(v.finalize(), true);
        TS_ASSERT_EQUALS(v.has_abs(), true);
        TS_ASSERT_EQUALS(String::format(*v.get_abs()), "5");
        TS_ASSERT_EQUALS(v.is_relative(), false);

        // (rel1+rel2)-rel2, all external
        v = Value(8, Expr::Ptr(new Expr(SUB(ADD(a, b), b))));
        TS_ASSERT_EQUALS(v.finalize(), true);
        TS_ASSERT_EQUALS(v.has_abs(), false);
        TS_ASSERT_EQUALS(v.get_rel(), a);

        // rel1-rel2 in same section gets left in abs portion
        v = Value(8, Expr::Ptr(new Expr(SUB(c, d))));
        TS_ASSERT_EQUALS(v.finalize(), true);
        TS_ASSERT_EQUALS(v.has_abs(), true);
        TS_ASSERT_EQUALS(String::format(*v.get_abs()), "c+(d*-1)");
        TS_ASSERT_EQUALS(v.is_relative(), false);

        // rel1-rel2 in different sections -> rel and sub portions, no abs
        v = Value(8, Expr::Ptr(new Expr(SUB(d, e))));
        TS_ASSERT_EQUALS(v.finalize(), true);
        TS_ASSERT_EQUALS(v.has_abs(), false);
        TS_ASSERT_EQUALS(v.get_rel(), d);
        TS_ASSERT_EQUALS(v.get_sub_sym(), e);

        // rel1 WRT rel2
        v = Value(8, Expr::Ptr(new Expr(WRT(a, b))));
        TS_ASSERT_EQUALS(v.finalize(), true);
        TS_ASSERT_EQUALS(v.has_abs(), false);
        TS_ASSERT_EQUALS(v.get_rel(), a);
        TS_ASSERT_EQUALS(v.get_wrt(), b);

        // rel1 WRT reg
        v = Value(8, Expr::Ptr(new Expr(WRT(a, g))));
        TS_ASSERT_EQUALS(v.finalize(), true);
        TS_ASSERT_EQUALS(v.has_abs(), true);
        TS_ASSERT_EQUALS(String::format(*v.get_abs()), "0 WRT g");
        TS_ASSERT_EQUALS(v.get_rel(), a);

        // rel1 WRT 5 --> error
        v = Value(8, Expr::Ptr(new Expr(WRT(a, 5))));
        TS_ASSERT_EQUALS(v.finalize(), false);

        // rel1 WRT (5+rel2) --> error
        v = Value(8, Expr::Ptr(new Expr(WRT(a, ADD(5, b)))));
        TS_ASSERT_EQUALS(v.finalize(), false);

        // 5+(rel1 WRT rel2)
        v = Value(8, Expr::Ptr(new Expr(ADD(5, WRT(a, b)))));
        TS_ASSERT_EQUALS(v.finalize(), true);
        TS_ASSERT_EQUALS(v.has_abs(), true);
        TS_ASSERT_EQUALS(String::format(*v.get_abs()), "5");
        TS_ASSERT_EQUALS(v.get_rel(), a);
        TS_ASSERT_EQUALS(v.get_wrt(), b);

        // (5+rel1) WRT rel2
        v = Value(8, Expr::Ptr(new Expr(WRT(ADD(5, a), b))));
        TS_ASSERT_EQUALS(v.finalize(), true);
        TS_ASSERT_EQUALS(v.has_abs(), true);
        TS_ASSERT_EQUALS(String::format(*v.get_abs()), "5");
        TS_ASSERT_EQUALS(v.get_rel(), a);
        TS_ASSERT_EQUALS(v.get_wrt(), b);

        // (rel1 WRT reg) WRT rel2 --> OK
        v = Value(8, Expr::Ptr(new Expr(WRT(ADD(5, a), b))));
        TS_ASSERT_EQUALS(v.finalize(), true);
        TS_ASSERT_EQUALS(v.has_abs(), true);
        TS_ASSERT_EQUALS(String::format(*v.get_abs()), "5");
        TS_ASSERT_EQUALS(v.get_rel(), a);
        TS_ASSERT_EQUALS(v.get_wrt(), b);

        // (rel1 WRT rel2) WRT rel3 --> error
        v = Value(8, Expr::Ptr(new Expr(WRT(WRT(a, b), c))));
        TS_ASSERT_EQUALS(v.finalize(), false);

        // SEG reg1
        v = Value(8, Expr::Ptr(new Expr(SEG(a))));
        TS_ASSERT_EQUALS(v.finalize(), true);
        TS_ASSERT_EQUALS(v.has_abs(), false);
        TS_ASSERT_EQUALS(v.get_rel(), a);
        TS_ASSERT_EQUALS(v.m_seg_of, true);

        // SEG 5 --> error
        v = Value(8, Expr::Ptr(new Expr(SEG(5))));
        TS_ASSERT_EQUALS(v.finalize(), false);

        // rel1+SEG rel1 --> error
        v = Value(8, Expr::Ptr(new Expr(ADD(a, SEG(a)))));
        TS_ASSERT_EQUALS(v.finalize(), false);

        // rel1>>5
        v = Value(8, Expr::Ptr(new Expr(SHR(a, 5))));
        TS_ASSERT_EQUALS(v.finalize(), true);
        TS_ASSERT_EQUALS(v.has_abs(), false);
        if (Expr* expr = v.get_abs())
            TS_TRACE(String::format(*expr));
        TS_ASSERT_EQUALS(v.get_rel(), a);
        TS_ASSERT_EQUALS(v.m_rshift, 5U);

        // (rel1>>5)>>6
        v = Value(8, Expr::Ptr(new Expr(SHR(SHR(a, 5), 6))));
        TS_ASSERT_EQUALS(v.finalize(), true);
        TS_ASSERT_EQUALS(v.has_abs(), false);
        TS_ASSERT_EQUALS(v.get_rel(), a);
        TS_ASSERT_EQUALS(v.m_rshift, 11U);

        // rel1>>reg --> error
        v = Value(8, Expr::Ptr(new Expr(SHR(a, g))));
        TS_ASSERT_EQUALS(v.finalize(), false);

        // rel1+rel1>>5 --> error
        v = Value(8, Expr::Ptr(new Expr(ADD(a, SHR(a, 5)))));
        TS_ASSERT_EQUALS(v.finalize(), false);

        // 5>>rel1 --> error
        v = Value(8, Expr::Ptr(new Expr(SHR(5, a))));
        TS_ASSERT_EQUALS(v.finalize(), false);
    }

    void test_clear()
    {
        Value v(6, Expr::Ptr(new Expr(WRT(sym1, wrt))));
        v.finalize();
        Bytecode bc;
        Location loc = {&bc, 0};
        v.sub_rel(0, loc);
        TS_ASSERT_EQUALS(v.has_abs(), false);
        TS_ASSERT_EQUALS(v.get_rel(), sym1);
        TS_ASSERT_EQUALS(v.get_wrt(), wrt);
        TS_ASSERT_EQUALS(v.has_sub(), true);
        v.set_line(4);
        v.m_next_insn = 3;
        v.m_seg_of = true;
        v.m_rshift = 5;
        v.m_ip_rel = true;
        v.m_jump_target = true;
        v.m_section_rel = true;
        v.m_no_warn = true;
        v.m_sign = true;

        v.clear();

        TS_ASSERT_EQUALS(v.has_abs(), false);
        TS_ASSERT_EQUALS(v.is_relative(), false);
        TS_ASSERT_EQUALS(v.is_wrt(), false);
        TS_ASSERT_EQUALS(v.has_sub(), false);
        TS_ASSERT_EQUALS(v.get_line(), 0U);
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

    void test_clear_rel()
    {
        Value v(6, Expr::Ptr(new Expr(WRT(sym1, wrt))));
        v.finalize();
        Bytecode bc;
        Location loc = {&bc, 0};
        v.sub_rel(0, loc);
        v.m_next_insn = 3;
        v.m_seg_of = true;
        v.m_rshift = 5;
        v.m_ip_rel = true;
        v.m_jump_target = true;
        v.m_section_rel = true;
        v.m_no_warn = true;
        v.m_sign = true;

        v.clear_rel();

        TS_ASSERT_EQUALS(v.has_abs(), false);
        TS_ASSERT_EQUALS(v.is_relative(), false);
        TS_ASSERT_EQUALS(v.is_wrt(), false);
        TS_ASSERT_EQUALS(v.has_sub(), false);
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

    void test_add_abs_int()
    {
        Value v(4);
        TS_ASSERT_EQUALS(v.has_abs(), false);
        // add to an empty abs
        v.add_abs(6);
        TS_ASSERT_EQUALS(v.has_abs(), true);
        TS_ASSERT_EQUALS(*v.get_abs()->get_intnum(), 6);
        // add to an abs with a value
        v.add_abs(8);
        v.get_abs()->simplify();
        TS_ASSERT_EQUALS(*v.get_abs()->get_intnum(), 14);
    }

    void test_add_abs_expr()
    {
        Value v(4);
        TS_ASSERT_EQUALS(v.has_abs(), false);
        // add to an empty abs
        v.add_abs(Expr(6));
        TS_ASSERT_EQUALS(v.has_abs(), true);
        v.get_abs()->simplify();
        TS_ASSERT_EQUALS(*v.get_abs()->get_intnum(), 6);
        // add to an abs with a value
        v.add_abs(Expr(8));
        v.get_abs()->simplify();
        TS_ASSERT_EQUALS(*v.get_abs()->get_intnum(), 14);
    }

    void test_is_relative()
    {
        Value v1(4);
        TS_ASSERT_EQUALS(v1.is_relative(), false);
        TS_ASSERT_EQUALS(v1.get_rel(), SymbolRef(0));

        Value v2(4, sym1);
        TS_ASSERT_EQUALS(v2.is_relative(), true);
        TS_ASSERT_EQUALS(v2.get_rel(), sym1);
    }

    void test_is_wrt()
    {
        Value v1(4);
        TS_ASSERT_EQUALS(v1.is_wrt(), false);
        TS_ASSERT_EQUALS(v1.get_wrt(), SymbolRef(0));

        Value v2(6, Expr::Ptr(new Expr(WRT(sym1, wrt))));
        v2.finalize();
        TS_ASSERT_EQUALS(v2.is_wrt(), true);
        TS_ASSERT_EQUALS(v2.get_wrt(), wrt);
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

    void test_sub_rel()
    {
        Bytecode bc;
        Location loc = {&bc, 0};
        Location loc2;
        Value v(4, sym1);
        TS_ASSERT_EQUALS(v.get_rel(), sym1);
        v.sub_rel(0, loc); // object=0 okay if m_rel set
        TS_ASSERT_EQUALS(v.get_rel(), sym1);
        TS_ASSERT_EQUALS(v.get_sub_loc(&loc2), true);
        TS_ASSERT_EQUALS(loc, loc2);

        Object object("x", "y", 0);
        v = Value(4, sym1);
        v.sub_rel(&object, loc);
        TS_ASSERT_EQUALS(v.get_rel(), sym1);    // shouldn't change m_rel
        loc2.bc = 0;
        TS_ASSERT_EQUALS(v.get_sub_loc(&loc2), true);
        TS_ASSERT_EQUALS(loc, loc2);

        v = Value(4);
        v.sub_rel(&object, loc);
        TS_ASSERT_EQUALS(v.get_rel(), object.get_absolute_symbol());
        loc2.bc = 0;
        TS_ASSERT_EQUALS(v.get_sub_loc(&loc2), true);
        TS_ASSERT_EQUALS(loc, loc2);
    }

    void test_calc_pcrel_sub()
    {
        // TODO
    }

    void test_get_set_line()
    {
        Value v(4);
        TS_ASSERT_EQUALS(v.get_line(), 0U);
        v.set_line(5);
        TS_ASSERT_EQUALS(v.get_line(), 5U);
    }

    void test_get_intnum()
    {
        IntNum intn;
        bool rv;

        // just a size, should be =0
        Value v(4);
        rv = v.get_intnum(&intn, false);
        TS_ASSERT_EQUALS(rv, true);
        TS_ASSERT_EQUALS(intn, 0);

        // just an integer, should be =int
        v.add_abs(5);
        rv = v.get_intnum(&intn, false);
        TS_ASSERT_EQUALS(rv, true);
        TS_ASSERT_EQUALS(intn, 5);

        // with relative portion, not possible (returns false)
        Value v2(6, sym1);
        rv = v2.get_intnum(&intn, false);
        TS_ASSERT_EQUALS(rv, false);

        // TODO: calc_bc_dist-using tests
    }
};

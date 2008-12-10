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

#include "expr.h"
#include "object.h"
#include "symbol.h"
#include "value.h"

class ValueTestSuite : public CxxTest::TestSuite
{
public:
    Symbol sym1_sym, sym2_sym, wrt_sym;
    SymbolRef sym1, sym2, wrt;

    ValueTestSuite()
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
        Expr::Ptr ep(new Expr(sym1, 0));
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

    void test_clear()
    {
        Value v(6, Expr::Ptr(new Expr(sym1, Op::WRT, wrt, 0)));
        v.finalize();
        v.sub_rel(0, sym2);
        TS_ASSERT_EQUALS(v.has_abs(), false);
        TS_ASSERT_EQUALS(v.get_rel(), sym1);
        TS_ASSERT_EQUALS(v.get_wrt(), wrt);
        TS_ASSERT_EQUALS(v.get_sub(), sym2);
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
        Value v(6, Expr::Ptr(new Expr(sym1, Op::WRT, wrt, 0)));
        v.finalize();
        v.sub_rel(0, sym2);
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
        v.add_abs(Expr::Ptr(new Expr(IntNum(6), 2)));
        TS_ASSERT_EQUALS(v.has_abs(), true);
        TS_ASSERT_EQUALS(v.get_abs()->get_line(), 2U);
        v.get_abs()->simplify();
        TS_ASSERT_EQUALS(*v.get_abs()->get_intnum(), 6);
        // add to an abs with a value
        v.add_abs(Expr::Ptr(new Expr(IntNum(8), 4)));
        TS_ASSERT_EQUALS(v.get_abs()->get_line(), 2U); // shouldn't change line
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

        Value v2(6, Expr::Ptr(new Expr(sym1, Op::WRT, wrt, 0)));
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
        Value v(4, sym1);
        TS_ASSERT_EQUALS(v.get_rel(), sym1);
        v.sub_rel(0, sym2); // object=0 okay if m_rel set
        TS_ASSERT_EQUALS(v.get_rel(), sym1);
        TS_ASSERT_EQUALS(v.get_sub(), sym2);

        Object object("x", "y", 0);
        v.sub_rel(&object, sym2);
        TS_ASSERT_EQUALS(v.get_rel(), sym1);    // shouldn't change m_rel
        TS_ASSERT_EQUALS(v.get_sub(), sym2);

        v = Value(4);
        v.sub_rel(&object, sym2);
        TS_ASSERT_EQUALS(v.get_rel(), object.get_absolute_symbol());
        TS_ASSERT_EQUALS(v.get_sub(), sym2);
    }

    void test_calc_pcrel_sub()
    {
        // TODO
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

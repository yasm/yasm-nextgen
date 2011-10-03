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
#include <gtest/gtest.h>

#include "yasmx/Basic/FileManager.h"
#include "yasmx/Basic/SourceManager.h"
#include "llvm/Support/raw_ostream.h"
#include "yasmx/Arch.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Expr.h"
#include "yasmx/Object.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"
#include "yasmx/Value.h"

#include "unittests/diag_mock.h"
#include "unittests/unittest_util.h"


using namespace yasm;
using namespace yasmunit;

using ::testing::Mock;

class ValueTest : public ::testing::Test
{
protected:
    class MockRegister : public yasm::Register
    {
    public:
        MockRegister(const char* name) : m_name(name) {}
        unsigned int getSize() const { return 0; }
        unsigned int getNum() const { return m_name[0]-'a'; }
        void Put(llvm::raw_ostream& os) const { os << m_name; }
#ifdef WITH_XML
        pugi::xml_node Write(pugi::xml_node out) const { return out; }
#endif // WITH_XML

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

    void InitClear(Value* v)
    {
        v->m_next_insn = 3;
        v->m_seg_of = true;
        v->m_rshift = 5;
        v->m_ip_rel = true;
        v->m_jump_target = true;
        v->m_section_rel = true;
        v->m_no_warn = true;
        v->m_sign = true;
    }

    void setRShift(Value& v, unsigned int rshift) { v.m_rshift = rshift; }
};

TEST_F(ValueTest, ConstructSize)
{
    Value v(4);
    EXPECT_FALSE(v.hasAbs());
    EXPECT_FALSE(v.isRelative());
    EXPECT_FALSE(v.isWRT());
    EXPECT_FALSE(v.hasSubRelative());
    EXPECT_EQ(0U, v.getNextInsn());
    EXPECT_FALSE(v.isSegOf());
    EXPECT_EQ(0U, v.getRShift());
    EXPECT_FALSE(v.isIPRelative());
    EXPECT_FALSE(v.isJumpTarget());
    EXPECT_FALSE(v.isSectionRelative());
    EXPECT_TRUE(v.isWarnEnabled());
    EXPECT_FALSE(v.isSigned());
    EXPECT_EQ(4U, v.getSize());
}

TEST_F(ValueTest, ConstructExpr)
{
    Expr::Ptr ep(new Expr(sym1));
    Expr* e = ep.get();
    Value v(6, ep);
    ASSERT_TRUE(v.hasAbs());
    EXPECT_EQ(e, v.getAbs());
    EXPECT_FALSE(v.isRelative());
    EXPECT_FALSE(v.isWRT());
    EXPECT_FALSE(v.hasSubRelative());
    EXPECT_EQ(0U, v.getNextInsn());
    EXPECT_FALSE(v.isSegOf());
    EXPECT_EQ(0U, v.getRShift());
    EXPECT_FALSE(v.isIPRelative());
    EXPECT_FALSE(v.isJumpTarget());
    EXPECT_FALSE(v.isSectionRelative());
    EXPECT_TRUE(v.isWarnEnabled());
    EXPECT_FALSE(v.isSigned());
    EXPECT_EQ(6U, v.getSize());
}

TEST_F(ValueTest, ConstructSymbol)
{
    Value v(8, sym1);
    EXPECT_FALSE(v.hasAbs());
    EXPECT_EQ(sym1, v.getRelative());
    EXPECT_FALSE(v.isWRT());
    EXPECT_FALSE(v.hasSubRelative());
    EXPECT_EQ(0U, v.getNextInsn());
    EXPECT_FALSE(v.isSegOf());
    EXPECT_EQ(0U, v.getRShift());
    EXPECT_FALSE(v.isIPRelative());
    EXPECT_FALSE(v.isJumpTarget());
    EXPECT_FALSE(v.isSectionRelative());
    EXPECT_TRUE(v.isWarnEnabled());
    EXPECT_FALSE(v.isSigned());
    EXPECT_EQ(8U, v.getSize());
}

TEST_F(ValueTest, Finalize)
{
    Object object("x", "y", 0);
    SymbolRef a = object.getSymbol("a");    // external
    SymbolRef b = object.getSymbol("b");    // external
    SymbolRef c = object.getSymbol("c");    // in section x
    SymbolRef d = object.getSymbol("d");    // in section x
    SymbolRef e = object.getSymbol("e");    // in section y
    SymbolRef f = object.getSymbol("f");    // in section y
    const MockRegister g("g");

    Section* x = new Section("x", false, false, SourceLocation());
    object.AppendSection(std::auto_ptr<Section>(x));
    Section* y = new Section("y", false, false, SourceLocation());
    object.AppendSection(std::auto_ptr<Section>(y));

    Location loc = {&x->FreshBytecode(), 0};
    c->DefineLabel(loc);
    d->DefineLabel(loc);
    loc.bc = &y->FreshBytecode();
    e->DefineLabel(loc);
    f->DefineLabel(loc);

    Value v(8);

    MockDiagnosticId mock_consumer;
    llvm::IntrusiveRefCntPtr<DiagnosticIDs> diagids(new DiagnosticIDs);
    DiagnosticsEngine diags(diagids, &mock_consumer, false);
    FileSystemOptions opts;
    FileManager fmgr(opts);
    SourceManager smgr(diags, fmgr);
    diags.setSourceManager(&smgr);

    // just an integer
    v = Value(8, Expr::Ptr(new Expr(4)));
    EXPECT_TRUE(v.Finalize(diags));
    ASSERT_TRUE(v.hasAbs());
    EXPECT_EQ("4", String::Format(*v.getAbs()));
    EXPECT_FALSE(v.isRelative());

    // simple relative
    v = Value(8, Expr::Ptr(new Expr(a)));
    EXPECT_TRUE(v.Finalize(diags));
    EXPECT_FALSE(v.hasAbs());
    EXPECT_EQ(a, v.getRelative());

    // masked relative
    v = Value(8, Expr::Ptr(new Expr(AND(a, 0xff))));
    EXPECT_TRUE(v.Finalize(diags));
    EXPECT_FALSE(v.hasAbs());
    EXPECT_EQ(a, v.getRelative());
    EXPECT_FALSE(v.isWarnEnabled());

    v = Value(8, Expr::Ptr(new Expr(AND(a, 0x7f))));
    EXPECT_CALL(mock_consumer, DiagId(diag::err_too_complex_expression));
    EXPECT_FALSE(v.Finalize(diags));     // invalid
    Mock::VerifyAndClear(&mock_consumer);
    ASSERT_TRUE(v.hasAbs());
    EXPECT_EQ("a&127", String::Format(*v.getAbs()));
    EXPECT_FALSE(v.isRelative());
    EXPECT_TRUE(v.isWarnEnabled());

    // rel-rel (rel may be external)
    v = Value(8, Expr::Ptr(new Expr(SUB(a, a))));
    EXPECT_TRUE(v.Finalize(diags));
    EXPECT_FALSE(v.hasAbs());
    EXPECT_FALSE(v.isRelative());

    // abs+(rel-rel)
    v = Value(8, Expr::Ptr(new Expr(ADD(5, SUB(a, a)))));
    EXPECT_TRUE(v.Finalize(diags));
    ASSERT_TRUE(v.hasAbs());
    EXPECT_EQ("5", String::Format(*v.getAbs()));
    EXPECT_FALSE(v.isRelative());

    // (rel1+rel2)-rel2, all external
    v = Value(8, Expr::Ptr(new Expr(SUB(ADD(a, b), b))));
    EXPECT_TRUE(v.Finalize(diags));
    EXPECT_FALSE(v.hasAbs());
    EXPECT_EQ(a, v.getRelative());

    // rel1-rel2 in same section gets left in abs portion
    v = Value(8, Expr::Ptr(new Expr(SUB(c, d))));
    EXPECT_TRUE(v.Finalize(diags));
    ASSERT_TRUE(v.hasAbs());
    EXPECT_EQ("c+(d*-1)", String::Format(*v.getAbs()));
    EXPECT_FALSE(v.isRelative());

    // rel1-rel2 in different sections -> rel and sub portions, no abs
    v = Value(8, Expr::Ptr(new Expr(SUB(d, e))));
    EXPECT_TRUE(v.Finalize(diags));
    EXPECT_FALSE(v.hasAbs());
    EXPECT_EQ(d, v.getRelative());
    EXPECT_EQ(e, v.getSubSymbol());

    // rel1 WRT rel2
    v = Value(8, Expr::Ptr(new Expr(WRT(a, b))));
    EXPECT_TRUE(v.Finalize(diags));
    EXPECT_FALSE(v.hasAbs());
    EXPECT_EQ(a, v.getRelative());
    EXPECT_EQ(b, v.getWRT());

    // rel1 WRT reg
    v = Value(8, Expr::Ptr(new Expr(WRT(a, g))));
    EXPECT_TRUE(v.Finalize(diags));
    ASSERT_TRUE(v.hasAbs());
    EXPECT_EQ("0 WRT g", String::Format(*v.getAbs()));
    EXPECT_EQ(a, v.getRelative());

    // rel1 WRT 5 --> error
    v = Value(8, Expr::Ptr(new Expr(WRT(a, 5))));
    EXPECT_CALL(mock_consumer, DiagId(diag::err_too_complex_expression));
    EXPECT_FALSE(v.Finalize(diags));
    Mock::VerifyAndClear(&mock_consumer);

    // rel1 WRT (5+rel2) --> error
    v = Value(8, Expr::Ptr(new Expr(WRT(a, ADD(5, b)))));
    EXPECT_CALL(mock_consumer, DiagId(diag::err_too_complex_expression));
    EXPECT_FALSE(v.Finalize(diags));
    Mock::VerifyAndClear(&mock_consumer);

    // 5+(rel1 WRT rel2)
    v = Value(8, Expr::Ptr(new Expr(ADD(5, WRT(a, b)))));
    EXPECT_TRUE(v.Finalize(diags));
    ASSERT_TRUE(v.hasAbs());
    EXPECT_EQ("5", String::Format(*v.getAbs()));
    EXPECT_EQ(a, v.getRelative());
    EXPECT_EQ(b, v.getWRT());

    // (5+rel1) WRT rel2
    v = Value(8, Expr::Ptr(new Expr(WRT(ADD(5, a), b))));
    EXPECT_TRUE(v.Finalize(diags));
    ASSERT_TRUE(v.hasAbs());
    EXPECT_EQ("5", String::Format(*v.getAbs()));
    EXPECT_EQ(a, v.getRelative());
    EXPECT_EQ(b, v.getWRT());

    // (rel1 WRT reg) WRT rel2 --> OK
    v = Value(8, Expr::Ptr(new Expr(WRT(ADD(5, a), b))));
    EXPECT_TRUE(v.Finalize(diags));
    ASSERT_TRUE(v.hasAbs());
    EXPECT_EQ("5", String::Format(*v.getAbs()));
    EXPECT_EQ(a, v.getRelative());
    EXPECT_EQ(b, v.getWRT());

    // (rel1 WRT rel2) WRT rel3 --> error
    v = Value(8, Expr::Ptr(new Expr(WRT(WRT(a, b), c))));
    EXPECT_CALL(mock_consumer, DiagId(diag::err_too_complex_expression));
    EXPECT_FALSE(v.Finalize(diags));
    Mock::VerifyAndClear(&mock_consumer);

    // SEG reg1
    v = Value(8, Expr::Ptr(new Expr(SEG(a))));
    EXPECT_TRUE(v.Finalize(diags));
    EXPECT_FALSE(v.hasAbs());
    EXPECT_EQ(a, v.getRelative());
    EXPECT_TRUE(v.isSegOf());

    // SEG 5 --> error
    v = Value(8, Expr::Ptr(new Expr(SEG(5))));
    EXPECT_CALL(mock_consumer, DiagId(diag::err_too_complex_expression));
    EXPECT_FALSE(v.Finalize(diags));
    Mock::VerifyAndClear(&mock_consumer);

    // rel1+SEG rel1 --> error
    v = Value(8, Expr::Ptr(new Expr(ADD(a, SEG(a)))));
    EXPECT_CALL(mock_consumer, DiagId(diag::err_too_complex_expression));
    EXPECT_FALSE(v.Finalize(diags));
    Mock::VerifyAndClear(&mock_consumer);

    // rel1>>5
    v = Value(8, Expr::Ptr(new Expr(SHR(a, 5))));
    EXPECT_TRUE(v.Finalize(diags));
    EXPECT_FALSE(v.hasAbs()) << String::Format(*v.getAbs());
    EXPECT_EQ(a, v.getRelative());
    EXPECT_EQ(5U, v.getRShift());

    // (rel1>>5)>>6 --> left as-is.
    v = Value(8, Expr::Ptr(new Expr(SHR(SHR(a, 5), 6))));
    EXPECT_TRUE(v.Finalize(diags));
    ASSERT_TRUE(v.hasAbs());
    EXPECT_EQ("(a>>5)>>6", String::Format(*v.getAbs()));
    EXPECT_FALSE(v.isRelative());

    // rel1>>reg --> error
    v = Value(8, Expr::Ptr(new Expr(SHR(a, g))));
    EXPECT_CALL(mock_consumer, DiagId(diag::err_too_complex_expression));
    EXPECT_FALSE(v.Finalize(diags));
    Mock::VerifyAndClear(&mock_consumer);

    // rel1+rel1>>5 --> error
    v = Value(8, Expr::Ptr(new Expr(ADD(a, SHR(a, 5)))));
    EXPECT_CALL(mock_consumer, DiagId(diag::err_too_complex_expression));
    EXPECT_FALSE(v.Finalize(diags));
    Mock::VerifyAndClear(&mock_consumer);

    // 5>>rel1 --> left as-is.
    v = Value(8, Expr::Ptr(new Expr(SHR(5, a))));
    EXPECT_TRUE(v.Finalize(diags));
    ASSERT_TRUE(v.hasAbs());
    EXPECT_EQ("5>>a", String::Format(*v.getAbs()));
    EXPECT_FALSE(v.isRelative());
}

TEST_F(ValueTest, Clear)
{
    MockDiagnosticId mock_consumer;
    llvm::IntrusiveRefCntPtr<DiagnosticIDs> diagids(new DiagnosticIDs);
    DiagnosticsEngine diags(diagids, &mock_consumer, false);
    FileSystemOptions opts;
    FileManager fmgr(opts);
    SourceManager smgr(diags, fmgr);
    diags.setSourceManager(&smgr);

    Value v(6, Expr::Ptr(new Expr(WRT(sym1, wrt))));
    EXPECT_TRUE(v.Finalize(diags));
    Bytecode bc;
    Location loc = {&bc, 0};
    v.SubRelative(0, loc);
    EXPECT_FALSE(v.hasAbs());
    EXPECT_EQ(sym1, v.getRelative());
    EXPECT_EQ(wrt, v.getWRT());
    EXPECT_TRUE(v.hasSubRelative());
    v.setSource(SourceLocation::getFromRawEncoding(4));
    InitClear(&v);

    v.Clear();

    EXPECT_FALSE(v.hasAbs());
    EXPECT_FALSE(v.isRelative());
    EXPECT_FALSE(v.isWRT());
    EXPECT_FALSE(v.hasSubRelative());
    EXPECT_FALSE(v.getSource().isValid());
    EXPECT_EQ(0U, v.getNextInsn());
    EXPECT_FALSE(v.isSegOf());
    EXPECT_EQ(0U, v.getRShift());
    EXPECT_FALSE(v.isIPRelative());
    EXPECT_FALSE(v.isJumpTarget());
    EXPECT_FALSE(v.isSectionRelative());
    EXPECT_TRUE(v.isWarnEnabled());
    EXPECT_FALSE(v.isSigned());
    EXPECT_EQ(0U, v.getSize());
}

TEST_F(ValueTest, ClearRelative)
{
    MockDiagnosticId mock_consumer;
    llvm::IntrusiveRefCntPtr<DiagnosticIDs> diagids(new DiagnosticIDs);
    DiagnosticsEngine diags(diagids, &mock_consumer, false);
    FileSystemOptions opts;
    FileManager fmgr(opts);
    SourceManager smgr(diags, fmgr);
    diags.setSourceManager(&smgr);

    Value v(6, Expr::Ptr(new Expr(WRT(sym1, wrt))));
    EXPECT_TRUE(v.Finalize(diags));
    Bytecode bc;
    Location loc = {&bc, 0};
    v.SubRelative(0, loc);
    InitClear(&v);

    v.ClearRelative();

    EXPECT_FALSE(v.hasAbs());
    EXPECT_FALSE(v.isRelative());
    EXPECT_FALSE(v.isWRT());
    EXPECT_FALSE(v.hasSubRelative());
    EXPECT_EQ(3U, v.getNextInsn());
    EXPECT_FALSE(v.isSegOf());
    EXPECT_EQ(0U, v.getRShift());
    EXPECT_FALSE(v.isIPRelative());
    EXPECT_TRUE(v.isJumpTarget());
    EXPECT_FALSE(v.isSectionRelative());
    EXPECT_FALSE(v.isWarnEnabled());
    EXPECT_TRUE(v.isSigned());
    EXPECT_EQ(6U, v.getSize());
}

TEST_F(ValueTest, AddAbsInt)
{
    MockDiagnosticId mock_consumer;
    llvm::IntrusiveRefCntPtr<DiagnosticIDs> diagids(new DiagnosticIDs);
    DiagnosticsEngine diags(diagids, &mock_consumer, false);
    FileSystemOptions opts;
    FileManager fmgr(opts);
    SourceManager smgr(diags, fmgr);
    diags.setSourceManager(&smgr);

    Value v(4);
    EXPECT_FALSE(v.hasAbs());
    // add to an empty abs
    v.AddAbs(6);
    ASSERT_TRUE(v.hasAbs());
    EXPECT_EQ(6, v.getAbs()->getIntNum().getInt());
    // add to an abs with a value
    v.AddAbs(8);
    v.getAbs()->Simplify(diags);
    ASSERT_TRUE(v.hasAbs());
    EXPECT_EQ(14, v.getAbs()->getIntNum().getInt());
}

TEST_F(ValueTest, AddAbsExpr)
{
    MockDiagnosticId mock_consumer;
    llvm::IntrusiveRefCntPtr<DiagnosticIDs> diagids(new DiagnosticIDs);
    DiagnosticsEngine diags(diagids, &mock_consumer, false);
    FileSystemOptions opts;
    FileManager fmgr(opts);
    SourceManager smgr(diags, fmgr);
    diags.setSourceManager(&smgr);

    Value v(4);
    EXPECT_FALSE(v.hasAbs());
    // add to an empty abs
    v.AddAbs(Expr(6));
    ASSERT_TRUE(v.hasAbs());
    v.getAbs()->Simplify(diags);
    EXPECT_EQ(6, v.getAbs()->getIntNum().getInt());
    // add to an abs with a value
    v.AddAbs(Expr(8));
    v.getAbs()->Simplify(diags);
    ASSERT_TRUE(v.hasAbs());
    EXPECT_EQ(14, v.getAbs()->getIntNum().getInt());
}

TEST_F(ValueTest, isRelative)
{
    Value v1(4);
    EXPECT_FALSE(v1.isRelative());
    EXPECT_EQ(SymbolRef(0), v1.getRelative());

    Value v2(4, sym1);
    ASSERT_TRUE(v2.isRelative());
    EXPECT_EQ(sym1, v2.getRelative());
}

TEST_F(ValueTest, isWRT)
{
    MockDiagnosticId mock_consumer;
    llvm::IntrusiveRefCntPtr<DiagnosticIDs> diagids(new DiagnosticIDs);
    DiagnosticsEngine diags(diagids, &mock_consumer, false);
    FileSystemOptions opts;
    FileManager fmgr(opts);
    SourceManager smgr(diags, fmgr);
    diags.setSourceManager(&smgr);

    Value v1(4);
    EXPECT_FALSE(v1.isWRT());
    EXPECT_EQ(SymbolRef(0), v1.getWRT());

    Value v2(6, Expr::Ptr(new Expr(WRT(sym1, wrt))));
    EXPECT_TRUE(v2.Finalize(diags));
    EXPECT_TRUE(v2.isWRT());
    EXPECT_EQ(wrt, v2.getWRT());
}

TEST_F(ValueTest, RSHIFT_MAX)
{
    Value v(4);
    unsigned int last_rshift;
    do {
        last_rshift = v.getRShift();
        setRShift(v, last_rshift+1);
    } while (v.getRShift() != 0 && last_rshift < 60000);   // 60000 is safety
    EXPECT_EQ(Value::RSHIFT_MAX, last_rshift);
}

TEST_F(ValueTest, SubRelative)
{
    Bytecode bc;
    Location loc = {&bc, 0};
    Location loc2;
    Value v(4, sym1);
    EXPECT_EQ(sym1, v.getRelative());
    v.SubRelative(0, loc); // object=0 okay if m_rel set
    EXPECT_EQ(sym1, v.getRelative());
    ASSERT_TRUE(v.getSubLocation(&loc2));
    EXPECT_EQ(loc.bc, loc2.bc);
    EXPECT_EQ(loc.off, loc2.off);

    Object object("x", "y", 0);
    v = Value(4, sym1);
    v.SubRelative(&object, loc);
    EXPECT_EQ(sym1, v.getRelative());   // shouldn't change m_rel
    loc2.bc = 0;
    ASSERT_TRUE(v.getSubLocation(&loc2));
    EXPECT_EQ(loc.bc, loc2.bc);
    EXPECT_EQ(loc.off, loc2.off);

    v = Value(4);
    v.SubRelative(&object, loc);
    EXPECT_EQ(object.getAbsoluteSymbol(), v.getRelative());
    loc2.bc = 0;
    ASSERT_TRUE(v.getSubLocation(&loc2));
    EXPECT_EQ(loc.bc, loc2.bc);
    EXPECT_EQ(loc.off, loc2.off);
}

TEST_F(ValueTest, CalcPCRelSub)
{
    // TODO
}

TEST_F(ValueTest, GetSetSource)
{
    Value v(4);
    EXPECT_FALSE(v.getSource().isValid());
    v.setSource(SourceLocation::getFromRawEncoding(5));
    EXPECT_EQ(5U, v.getSource().getBegin().getRawEncoding());
    EXPECT_EQ(5U, v.getSource().getEnd().getRawEncoding());
}

TEST_F(ValueTest, getIntNum)
{
    MockDiagnosticId mock_consumer;
    llvm::IntrusiveRefCntPtr<DiagnosticIDs> diagids(new DiagnosticIDs);
    DiagnosticsEngine diags(diagids, &mock_consumer, false);
    FileSystemOptions opts;
    FileManager fmgr(opts);
    SourceManager smgr(diags, fmgr);
    diags.setSourceManager(&smgr);

    IntNum intn;
    bool rv;

    // just a size, should be =0
    Value v(4);
    rv = v.getIntNum(&intn, false, diags);
    EXPECT_TRUE(rv);
    EXPECT_EQ(0, intn.getInt());

    // just an integer, should be =int
    v.AddAbs(5);
    rv = v.getIntNum(&intn, false, diags);
    EXPECT_TRUE(rv);
    EXPECT_EQ(5, intn.getInt());

    // with relative portion, not possible (returns false)
    Value v2(6, sym1);
    rv = v2.getIntNum(&intn, false, diags);
    EXPECT_FALSE(rv);

    // TODO: calc_bc_dist-using tests
}

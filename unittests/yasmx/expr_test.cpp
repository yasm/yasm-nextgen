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

#include "llvm/Support/raw_ostream.h"
#include "yasmx/Basic/FileManager.h"
#include "yasmx/Basic/SourceManager.h"
#include "yasmx/Arch.h"
#include "yasmx/Expr.h"

#include "unittests/diag_mock.h"
#include "unittests/unittest_util.h"


namespace yasm
{

class ExprTest : public testing::Test
{
protected:
    static void TransformNeg(Expr& x)
    {
        x.TransformNeg();
        x.Cleanup();
    }

    static void LevelOp(Expr& x,
                        DiagnosticsEngine& diags,
                        bool simplify_reg_mul = true,
                        int loc = -1)
    {
        x.LevelOp(diags, simplify_reg_mul, loc);
    }

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

    const MockRegister a, b, c, d, e, f;
    yasm::Expr x;

    ExprTest()
        : a("a"), b("b"), c("c"), d("d"), e("e"), f("f")
    {
    }
};

// Construction tests
TEST_F(ExprTest, Construct)
{
    Expr e(5);
    EXPECT_EQ("5", String::Format(e));

    Expr e2 = NEG(5);
    EXPECT_EQ("-5", String::Format(e2));

    Expr e3 = MUL(e2, IntNum(6));
    EXPECT_EQ("(-5)*6", String::Format(e3));

    Expr e4 = ADD(e, e3);
    EXPECT_EQ("5+((-5)*6)", String::Format(e4));

    Expr e5(e4);
    EXPECT_EQ("5+((-5)*6)", String::Format(e5));
}

// Expr::Contains() tests
TEST_F(ExprTest, Contains)
{
    x = 5;
    EXPECT_TRUE(x.Contains(ExprTerm::INT));
    EXPECT_FALSE(x.Contains(ExprTerm::FLOAT));

    x = ADD(a, 5);
    EXPECT_TRUE(x.Contains(ExprTerm::INT));
    EXPECT_FALSE(x.Contains(ExprTerm::FLOAT));
    EXPECT_TRUE(x.Contains(ExprTerm::REG));
}

// Expr::TransformNeg() tests
TEST_F(ExprTest, TransformNeg)
{
    x = NEG(ADD(a, b));
    EXPECT_EQ("-(a+b)", String::Format(x));
    ExprTest::TransformNeg(x);
    EXPECT_EQ("(a*-1)+(b*-1)", String::Format(x));

    x = SUB(a, b);
    EXPECT_EQ("a-b", String::Format(x));
    ExprTest::TransformNeg(x);
    EXPECT_EQ("a+(b*-1)", String::Format(x));

    x = NEG(SUB(a, b));
    EXPECT_EQ("-(a-b)", String::Format(x));
    ExprTest::TransformNeg(x);
    EXPECT_EQ("(a*-1)+b", String::Format(x));

    x = SUB(NEG(a), ADD(NEG(b), c));
    EXPECT_EQ("(-a)-((-b)+c)", String::Format(x));
    ExprTest::TransformNeg(x);
    EXPECT_EQ("(a*-1)+(b+(c*-1))", String::Format(x));

    // Negation of misc operators just gets multiplied by -1.
    x = NEG(SEGOFF(a, b));
    EXPECT_EQ("-(a:b)", String::Format(x));
    ExprTest::TransformNeg(x);
    EXPECT_EQ("(a:b)*-1", String::Format(x));

    // Negation of MUL avoids adding another MUL level.
    x = ADD(SUB(a, MUL(b, -1)), NEG(c), d);
    EXPECT_EQ("(a-(b*-1))+(-c)+d", String::Format(x));
    ExprTest::TransformNeg(x);
    EXPECT_EQ("(a+(b*-1*-1))+(c*-1)+d", String::Format(x));

    // Some simple integer negation will be handled here.
    x = NEG(5);
    EXPECT_EQ(2U, x.getTerms().size());
    ExprTest::TransformNeg(x);
    EXPECT_EQ(1U, x.getTerms().size());

    // Of course, it shouldn't affect expressions with no (operator) negation.
    x = ADD(a, MUL(b, -1));
    EXPECT_EQ("a+(b*-1)", String::Format(x));
    ExprTest::TransformNeg(x);
    EXPECT_EQ("a+(b*-1)", String::Format(x));

    // And should gracefully handle IDENTs.
    x = Expr(a);
    EXPECT_EQ("a", String::Format(x));
    ExprTest::TransformNeg(x);
    EXPECT_EQ("a", String::Format(x));
}

// Expr::Simplify() tests
TEST_F(ExprTest, Simplify)
{
    yasmunit::MockDiagnosticConsumer mock_consumer;
    llvm::IntrusiveRefCntPtr<DiagnosticIDs> diagids(new DiagnosticIDs);
    DiagnosticsEngine diags(diagids, &mock_consumer, false);
    FileSystemOptions opts;
    FileManager fmgr(opts);
    SourceManager smgr(diags, fmgr);
    diags.setSourceManager(&smgr);

    x = ADD(a, ADD(ADD(b, c), ADD(ADD(d, e), f)));
    EXPECT_EQ("a+((b+c)+((d+e)+f))", String::Format(x));
    x.Simplify(diags);
    EXPECT_EQ("a+b+c+d+e+f", String::Format(x));

    // Negatives will be transformed to aid in leveling.
    x = SUB(a, ADD(b, ADD(c, d)));
    EXPECT_EQ("a-(b+(c+d))", String::Format(x));
    x.Simplify(diags);
    EXPECT_EQ("a+(b*-1)+(c*-1)+(d*-1)", String::Format(x));

    // Constant folding will also be performed.
    x = MUL(1, MUL(2, ADD(3, 4)));
    EXPECT_EQ("1*(2*(3+4))", String::Format(x));
    x.Simplify(diags);
    EXPECT_EQ("14", String::Format(x));

    // As will identity simplification.
    x = ADD(MUL(5, a, 0), 1);
    EXPECT_EQ("(5*a*0)+1", String::Format(x));
    x.Simplify(diags);
    EXPECT_EQ("1", String::Format(x));

    // We can combine all of the above.
    x = MUL(ADD(5, a, 6), 1);
    EXPECT_EQ("(5+a+6)*1", String::Format(x));
    x.Simplify(diags);
    EXPECT_EQ("a+11", String::Format(x));

    x = ADD(10, NEG(5));
    EXPECT_EQ("10+(-5)", String::Format(x));
    x.Simplify(diags);
    EXPECT_EQ("5", String::Format(x));
}

//
// Expr::LevelOp() tests
//
TEST_F(ExprTest, LevelOpBasic)
{
    yasmunit::MockDiagnosticConsumer mock_consumer;
    llvm::IntrusiveRefCntPtr<DiagnosticIDs> diagids(new DiagnosticIDs);
    DiagnosticsEngine diags(diagids, &mock_consumer, false);
    FileSystemOptions opts;
    FileManager fmgr(opts);
    SourceManager smgr(diags, fmgr);
    diags.setSourceManager(&smgr);

    x = ADD(a, ADD(b, ADD(c, d)));
    EXPECT_EQ("a+(b+(c+d))", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("a+b+c+d", String::Format(x));

    x = ADD(a, SUB(b, ADD(c, d)));
    EXPECT_EQ("a+(b-(c+d))", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("a+(b-(c+d))", String::Format(x));

    // Only one level of leveling is performed.
    x = SUB(a, ADD(b, ADD(c, d)));
    EXPECT_EQ("a-(b+(c+d))", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("a-(b+(c+d))", String::Format(x));

    x = ADD(a, SUB(b, ADD(c, d)), ADD(e, f));
    EXPECT_EQ("a+(b-(c+d))+(e+f)", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("a+(b-(c+d))+e+f", String::Format(x));

    x = ADD(ADD(a, b), ADD(c, d, ADD(e, f)));
    EXPECT_EQ("(a+b)+(c+d+(e+f))", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("a+b+c+d+e+f", String::Format(x));
}

// One-level constant folding will also be performed.
TEST_F(ExprTest, LevelOpConstFold)
{
    yasmunit::MockDiagnosticConsumer mock_consumer;
    llvm::IntrusiveRefCntPtr<DiagnosticIDs> diagids(new DiagnosticIDs);
    DiagnosticsEngine diags(diagids, &mock_consumer, false);
    FileSystemOptions opts;
    FileManager fmgr(opts);
    SourceManager smgr(diags, fmgr);
    diags.setSourceManager(&smgr);

    x = ADD(1, ADD(2, ADD(3, 4)));
    EXPECT_EQ("1+(2+(3+4))", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("10", String::Format(x));

    x = MUL(1, MUL(2, ADD(3, 4)));
    EXPECT_EQ("1*(2*(3+4))", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("2*(3+4)", String::Format(x));

    x = SHR(3, 1);
    EXPECT_EQ("3>>1", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("1", String::Format(x));
}

// Common integer identities will be simplified.
// Some identities can result in deletion of the rest of the expression.
TEST_F(ExprTest, LevelOpIdentities)
{
    yasmunit::MockDiagnosticConsumer mock_consumer;
    llvm::IntrusiveRefCntPtr<DiagnosticIDs> diagids(new DiagnosticIDs);
    DiagnosticsEngine diags(diagids, &mock_consumer, false);
    FileSystemOptions opts;
    FileManager fmgr(opts);
    SourceManager smgr(diags, fmgr);
    diags.setSourceManager(&smgr);

    x = ADD(a, 0);
    EXPECT_EQ("a+0", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("a", String::Format(x));

    // Simplification of 1*REG is affected by simplify_reg_mul
    x = MUL(1, a);
    EXPECT_EQ("1*a", String::Format(x));
    ExprTest::LevelOp(x, diags, false);
    EXPECT_EQ("1*a", String::Format(x));

    // Simplification of 1*REG is affected by simplify_reg_mul
    x = MUL(1, a);
    EXPECT_EQ("1*a", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("a", String::Format(x));

    x = SUB(a, 0);
    EXPECT_EQ("a-0", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("a", String::Format(x));

    x = SUB(0, a);
    EXPECT_EQ("0-a", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("0-a", String::Format(x));

    x = MUL(2, a, 0, 3);
    EXPECT_EQ("2*a*0*3", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("0", String::Format(x));

    x = MUL(ADD(5, a, 6), 0);
    EXPECT_EQ("(5+a+6)*0", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("0", String::Format(x));
}

// SEG of SEG:OFF should be simplified to just the segment portion.
TEST_F(ExprTest, LevelOpSegOfSegoff)
{
    yasmunit::MockDiagnosticConsumer mock_consumer;
    llvm::IntrusiveRefCntPtr<DiagnosticIDs> diagids(new DiagnosticIDs);
    DiagnosticsEngine diags(diagids, &mock_consumer, false);
    FileSystemOptions opts;
    FileManager fmgr(opts);
    SourceManager smgr(diags, fmgr);
    diags.setSourceManager(&smgr);

    x = SEG(SEGOFF(1, 2));
    EXPECT_EQ("SEG (1:2)", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("1", String::Format(x));

    x = SEG(SEGOFF(1, ADD(2, 3)));
    EXPECT_EQ("SEG (1:(2+3))", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("1", String::Format(x));

    x = SEG(SEGOFF(ADD(1, 2), 3));
    EXPECT_EQ("SEG ((1+2):3)", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("1+2", String::Format(x));

    x = SEG(SEGOFF(ADD(1, 2), ADD(3, 4)));
    EXPECT_EQ("SEG ((1+2):(3+4))", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("1+2", String::Format(x));

    // Should only affect SEG of SEG:OFF.
    x = SEG(ADD(1, 2));
    EXPECT_EQ("SEG (1+2)", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("SEG (1+2)", String::Format(x));

    x = SEG(1);
    EXPECT_EQ("SEG 1", String::Format(x));
    ExprTest::LevelOp(x, diags);
    EXPECT_EQ("SEG 1", String::Format(x));
}

} // namespace yasm

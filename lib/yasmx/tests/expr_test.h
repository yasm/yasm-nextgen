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

#include "llvm/Support/raw_ostream.h"
#include "yasmx/Support/Compose.h"
#include "yasmx/Arch.h"
#include "yasmx/Expr.h"

namespace yasm
{

class ExprTest
{
public:
    static void TransformNeg(Expr& x)
    {
        x.TransformNeg();
        x.Cleanup();
    }

    static void LevelOp(Expr& x, bool simplify_reg_mul = true, int loc = -1)
    {
        x.LevelOp(simplify_reg_mul, loc);
    }
};

} // namespace yasm

class ExprTestSuite : public CxxTest::TestSuite
{
public:
    class MockRegister : public yasm::Register
    {
    public:
        MockRegister(const char* name) : m_name(name) {}
        unsigned int getSize() const { return 0; }
        unsigned int getNum() const { return m_name[0]-'a'; }
        void Put(llvm::raw_ostream& os) const { os << m_name; }
        void Write(YAML::Emitter& out) const {}

    private:
        const char* m_name;
    };

    const MockRegister a, b, c, d, e, f;
    yasm::Expr x;

    ExprTestSuite()
        : a("a"), b("b"), c("c"), d("d"), e("e"), f("f")
    {
    }

    // Construction tests
    void testConstruct()
    {
        Expr e(5);
        TS_ASSERT_EQUALS(String::Format(e), "5");

        Expr e2 = NEG(5);
        TS_ASSERT_EQUALS(String::Format(e2), "-5");

        Expr e3 = MUL(e2, IntNum(6));
        TS_ASSERT_EQUALS(String::Format(e3), "(-5)*6");

        Expr e4 = ADD(e, e3);
        TS_ASSERT_EQUALS(String::Format(e4), "5+((-5)*6)");

        Expr e5(e4);
        TS_ASSERT_EQUALS(String::Format(e5), "5+((-5)*6)");
    }

    // Expr::Contains() tests
    void testContains()
    {
        x = 5;
        TS_ASSERT_EQUALS(x.Contains(ExprTerm::INT), true);
        TS_ASSERT_EQUALS(x.Contains(ExprTerm::FLOAT), false);

        x = ADD(a, 5);
        TS_ASSERT_EQUALS(x.Contains(ExprTerm::INT), true);
        TS_ASSERT_EQUALS(x.Contains(ExprTerm::FLOAT), false);
        TS_ASSERT_EQUALS(x.Contains(ExprTerm::REG), true);
    }

    // Expr::TransformNeg() tests
    void testTransformNeg()
    {
        x = NEG(ADD(a, b));
        TS_ASSERT_EQUALS(String::Format(x), "-(a+b)");
        ExprTest::TransformNeg(x);
        TS_ASSERT_EQUALS(String::Format(x), "(a*-1)+(b*-1)");

        x = SUB(a, b);
        TS_ASSERT_EQUALS(String::Format(x), "a-b");
        ExprTest::TransformNeg(x);
        TS_ASSERT_EQUALS(String::Format(x), "a+(b*-1)");

        x = NEG(SUB(a, b));
        TS_ASSERT_EQUALS(String::Format(x), "-(a-b)");
        ExprTest::TransformNeg(x);
        TS_ASSERT_EQUALS(String::Format(x), "(a*-1)+b");

        x = SUB(NEG(a), ADD(NEG(b), c));
        TS_ASSERT_EQUALS(String::Format(x), "(-a)-((-b)+c)");
        ExprTest::TransformNeg(x);
        TS_ASSERT_EQUALS(String::Format(x), "(a*-1)+(b+(c*-1))");

        // Negation of misc operators just gets multiplied by -1.
        x = NEG(SEGOFF(a, b));
        TS_ASSERT_EQUALS(String::Format(x), "-(a:b)");
        ExprTest::TransformNeg(x);
        TS_ASSERT_EQUALS(String::Format(x), "(a:b)*-1");

        // Negation of MUL avoids adding another MUL level.
        x = ADD(SUB(a, MUL(b, -1)), NEG(c), d);
        TS_ASSERT_EQUALS(String::Format(x), "(a-(b*-1))+(-c)+d");
        ExprTest::TransformNeg(x);
        TS_ASSERT_EQUALS(String::Format(x), "(a+(b*-1*-1))+(c*-1)+d");

        // Some simple integer negation will be handled here.
        x = NEG(5);
        TS_ASSERT_EQUALS(x.getTerms().size(), 2U);
        ExprTest::TransformNeg(x);
        TS_ASSERT_EQUALS(x.getTerms().size(), 1U);

        // Of course, it shouldn't affect expressions with no (operator) negation.
        x = ADD(a, MUL(b, -1));
        TS_ASSERT_EQUALS(String::Format(x), "a+(b*-1)");
        ExprTest::TransformNeg(x);
        TS_ASSERT_EQUALS(String::Format(x), "a+(b*-1)");

        // And should gracefully handle IDENTs.
        x = Expr(a);
        TS_ASSERT_EQUALS(String::Format(x), "a");
        ExprTest::TransformNeg(x);
        TS_ASSERT_EQUALS(String::Format(x), "a");
    }

    // Expr::Simplify() tests
    void testSimplify()
    {
        x = ADD(a, ADD(ADD(b, c), ADD(ADD(d, e), f)));
        TS_ASSERT_EQUALS(String::Format(x), "a+((b+c)+((d+e)+f))");
        x.Simplify();
        TS_ASSERT_EQUALS(String::Format(x), "a+b+c+d+e+f");

        // Negatives will be transformed to aid in leveling.
        x = SUB(a, ADD(b, ADD(c, d)));
        TS_ASSERT_EQUALS(String::Format(x), "a-(b+(c+d))");
        x.Simplify();
        TS_ASSERT_EQUALS(String::Format(x), "a+(b*-1)+(c*-1)+(d*-1)");

        // Constant folding will also be performed.
        x = MUL(1, MUL(2, ADD(3, 4)));
        TS_ASSERT_EQUALS(String::Format(x), "1*(2*(3+4))");
        x.Simplify();
        TS_ASSERT_EQUALS(String::Format(x), "14");

        // As will identity simplification.
        x = ADD(MUL(5, a, 0), 1);
        TS_ASSERT_EQUALS(String::Format(x), "(5*a*0)+1");
        x.Simplify();
        TS_ASSERT_EQUALS(String::Format(x), "1");

        // We can combine all of the above.
        x = MUL(ADD(5, a, 6), 1);
        TS_ASSERT_EQUALS(String::Format(x), "(5+a+6)*1");
        x.Simplify();
        TS_ASSERT_EQUALS(String::Format(x), "a+11");

        x = ADD(10, NEG(5));
        TS_ASSERT_EQUALS(String::Format(x), "10+(-5)");
        x.Simplify();
        TS_ASSERT_EQUALS(String::Format(x), "5");
    }

    //
    // Expr::LevelOp() tests
    //
    void testLevelOpBasic()
    {
        x = ADD(a, ADD(b, ADD(c, d)));
        TS_ASSERT_EQUALS(String::Format(x), "a+(b+(c+d))");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "a+b+c+d");

        x = ADD(a, SUB(b, ADD(c, d)));
        TS_ASSERT_EQUALS(String::Format(x), "a+(b-(c+d))");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "a+(b-(c+d))");

        // Only one level of leveling is performed.
        x = SUB(a, ADD(b, ADD(c, d)));
        TS_ASSERT_EQUALS(String::Format(x), "a-(b+(c+d))");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "a-(b+(c+d))");

        x = ADD(a, SUB(b, ADD(c, d)), ADD(e, f));
        TS_ASSERT_EQUALS(String::Format(x), "a+(b-(c+d))+(e+f)");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "a+(b-(c+d))+e+f");

        x = ADD(ADD(a, b), ADD(c, d, ADD(e, f)));
        TS_ASSERT_EQUALS(String::Format(x), "(a+b)+(c+d+(e+f))");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "a+b+c+d+e+f");
    }

    // One-level constant folding will also be performed.
    void testLevelOpConstFold()
    {
        x = ADD(1, ADD(2, ADD(3, 4)));
        TS_ASSERT_EQUALS(String::Format(x), "1+(2+(3+4))");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "10");

        x = MUL(1, MUL(2, ADD(3, 4)));
        TS_ASSERT_EQUALS(String::Format(x), "1*(2*(3+4))");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "2*(3+4)");

        x = SHR(3, 1);
        TS_ASSERT_EQUALS(String::Format(x), "3>>1");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "1");
    }

    // Common integer identities will be simplified.
    // Some identities can result in deletion of the rest of the expression.
    void testLevelOpIdentities()
    {
        x = ADD(a, 0);
        TS_ASSERT_EQUALS(String::Format(x), "a+0");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "a");

        // Simplification of 1*REG is affected by simplify_reg_mul
        x = MUL(1, a);
        TS_ASSERT_EQUALS(String::Format(x), "1*a");
        ExprTest::LevelOp(x, false);
        TS_ASSERT_EQUALS(String::Format(x), "1*a");

        // Simplification of 1*REG is affected by simplify_reg_mul
        x = MUL(1, a);
        TS_ASSERT_EQUALS(String::Format(x), "1*a");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "a");

        x = SUB(a, 0);
        TS_ASSERT_EQUALS(String::Format(x), "a-0");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "a");

        x = SUB(0, a);
        TS_ASSERT_EQUALS(String::Format(x), "0-a");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "0-a");

        x = MUL(2, a, 0, 3);
        TS_ASSERT_EQUALS(String::Format(x), "2*a*0*3");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "0");

        x = MUL(ADD(5, a, 6), 0);
        TS_ASSERT_EQUALS(String::Format(x), "(5+a+6)*0");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "0");
    }

    // SEG of SEG:OFF should be simplified to just the segment portion.
    void testLevelOpSegOfSegoff()
    {
        x = SEG(SEGOFF(1, 2));
        TS_ASSERT_EQUALS(String::Format(x), "SEG (1:2)");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "1");

        x = SEG(SEGOFF(1, ADD(2, 3)));
        TS_ASSERT_EQUALS(String::Format(x), "SEG (1:(2+3))");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "1");

        x = SEG(SEGOFF(ADD(1, 2), 3));
        TS_ASSERT_EQUALS(String::Format(x), "SEG ((1+2):3)");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "1+2");

        x = SEG(SEGOFF(ADD(1, 2), ADD(3, 4)));
        TS_ASSERT_EQUALS(String::Format(x), "SEG ((1+2):(3+4))");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "1+2");

        // Should only affect SEG of SEG:OFF.
        x = SEG(ADD(1, 2));
        TS_ASSERT_EQUALS(String::Format(x), "SEG (1+2)");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "SEG (1+2)");

        x = SEG(1);
        TS_ASSERT_EQUALS(String::Format(x), "SEG 1");
        ExprTest::LevelOp(x);
        TS_ASSERT_EQUALS(String::Format(x), "SEG 1");
    }
};

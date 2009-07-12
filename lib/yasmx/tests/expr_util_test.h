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

#include "yasmx/Support/Compose.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Expr.h"
#include "yasmx/Expr_util.h"
#include "yasmx/Symbol.h"

class EquTestSuite : public CxxTest::TestSuite
{
public:
    void testSingle()
    {
        Symbol a("a");
        a.DefineEqu(Expr(5), 0);
        Expr v = Expr(SymbolRef(&a));
        ExpandEqu(v);
        TS_ASSERT_EQUALS(String::Format(v), "5");
    }

    void testDual()
    {
        Symbol a("a"), b("b");
        a.DefineEqu(Expr(5), 0);
        b.DefineEqu(Expr(4), 0);
        Expr v = MUL(SymbolRef(&a), SymbolRef(&b));
        ExpandEqu(v);
        TS_ASSERT_EQUALS(String::Format(v), "5*4");
    }

    void testNestedSingle()
    {
        Symbol a("a");
        a.DefineEqu(MUL(5, 4), 0);
        Expr v = ADD(SymbolRef(&a), 2);
        ExpandEqu(v);
        TS_ASSERT_EQUALS(String::Format(v), "(5*4)+2");

        Expr v2 = ADD(2, SymbolRef(&a));
        ExpandEqu(v2);
        TS_ASSERT_EQUALS(String::Format(v2), "2+(5*4)");
    }

    void testNestedTwice()
    {
        Symbol a("a");
        a.DefineEqu(MUL(5, 4), 0);
        Expr v = ADD(SymbolRef(&a), SymbolRef(&a));
        ExpandEqu(v);
        TS_ASSERT_EQUALS(String::Format(v), "(5*4)+(5*4)");
    }

    void testDoubleNested()
    {
        Symbol a("a"), b("b");
        a.DefineEqu(MUL(5, 4), 0);
        b.DefineEqu(ADD(SymbolRef(&a), 1), 0);
        Expr v = SUB(SymbolRef(&a), SymbolRef(&b));
        ExpandEqu(v);
        TS_ASSERT_EQUALS(String::Format(v), "(5*4)-((5*4)+1)");
    }

    void testCircular()
    {
        Symbol a("a"), b("b"), c("c");
        a.DefineEqu(ADD(SymbolRef(&b), 1), 0);
        b.DefineEqu(MUL(2, SymbolRef(&c)), 0);
        c.DefineEqu(SUB(SymbolRef(&a), 3), 0);
        Expr v = Expr(SymbolRef(&a));
        TS_ASSERT_THROWS(ExpandEqu(v), yasm::TooComplexError);
    }
};

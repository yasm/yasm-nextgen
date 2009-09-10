//
//  Copyright (C) 2007  Peter Johnson
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

#include <string>

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"
#include "yasmx/Support/ptr_vector.h"
#include "hamt.h"

class HamtTestSuite : public CxxTest::TestSuite
{
public:
    static const int NUM_SYMS = 1000;

    class Symbol
    {
    public:
        Symbol(const std::string& name)
            : m_name(name)
        {
        }

        const std::string& getName() const { return m_name; }

    private:
        std::string m_name;
    };

    class SymGetName
    {
    public:
        const std::string& operator() (const Symbol* sym) const
        { return sym->getName(); }
    };

    typedef yasm::hamt<std::string, Symbol, SymGetName> myhamt;

    class GenSym
    {
    public:
        GenSym(int nsym);
        void InsertCheckNew(myhamt& h);

        typedef stdx::ptr_vector<Symbol> Symbols;
        Symbols syms;

    private:
        stdx::ptr_vector_owner<Symbol> m_syms_owner;
    };

    void testCaseBasic()
    {
        GenSym g(NUM_SYMS);
        myhamt h(false);

        g.InsertCheckNew(h);
    }

    void testCaseFind()
    {
        GenSym g(NUM_SYMS);
        myhamt h(false);

        g.InsertCheckNew(h);

        // find
        for (GenSym::Symbols::iterator i=g.syms.begin(), end=g.syms.end();
             i != end; ++i)
        {
            Symbol* sym = h.Find(i->getName());
            TS_ASSERT(sym == &(*i));
        }
    }

    void testCaseDupInsert()
    {
        GenSym g1(NUM_SYMS);
        GenSym g2(NUM_SYMS);
        myhamt h(false);

        g1.InsertCheckNew(h);

        // duplicate insertion (without replacement)
        for (GenSym::Symbols::iterator i=g1.syms.begin(), end=g1.syms.end(),
             i2=g2.syms.begin(), i2end=g2.syms.end();
             i != end && i2 != i2end; ++i, ++i2)
        {
            Symbol* old = h.Insert(&(*i2));
            TS_ASSERT(old == &(*i));
        }

        // check to make sure the hamt values didn't change
        for (GenSym::Symbols::iterator i=g1.syms.begin(), end=g1.syms.end();
             i != end; ++i)
        {
            Symbol* sym = h.Find(i->getName());
            TS_ASSERT(sym == &(*i));
        }
    }

    void testCaseDupReplace()
    {
        GenSym g1(NUM_SYMS);
        GenSym g2(NUM_SYMS);
        myhamt h(false);

        g1.InsertCheckNew(h);

        // duplicate insertion (with replacement)
        for (GenSym::Symbols::iterator i=g1.syms.begin(), end=g1.syms.end(),
             i2=g2.syms.begin(), i2end=g2.syms.end();
             i != end && i2 != i2end; ++i, ++i2)
        {
            Symbol* old = h.Replace(&(*i2));
            TS_ASSERT(old == &(*i));
        }

        // check to make sure the hamt values changed
        for (GenSym::Symbols::iterator i=g2.syms.begin(), end=g2.syms.end();
             i != end; ++i)
        {
            Symbol* sym = h.Find(i->getName());
            TS_ASSERT(sym == &(*i));
        }
    }
};

HamtTestSuite::GenSym::GenSym(int nsym)
    : m_syms_owner(syms)
{
    for (int i=0; i<nsym; i++)
    {
        llvm::SmallString<128> ss;
        llvm::raw_svector_ostream os(ss);
        os << "sym" << i;
        syms.push_back(new Symbol(os.str()));
    }
}

void
HamtTestSuite::GenSym::InsertCheckNew(myhamt& h)
{
    for (GenSym::Symbols::iterator i=syms.begin(), end=syms.end();
         i != end; ++i)
    {
        Symbol* old = h.Insert(&(*i));
        TS_ASSERT(old == 0);
    }
}


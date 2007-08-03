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
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE linemap_test
#include <boost/test/unit_test.hpp>

#include <string>
#include <iostream>
#include <boost/ptr_container/ptr_vector.hpp>

#include "hamt.h"

static const int NUM_SYMS = 1000;

class Symbol {
public:
    Symbol(const std::string& name);
    ~Symbol();

    std::string get_name() const { return m_name; }

private:
    std::string m_name;
};

Symbol::Symbol(const std::string& name)
    : m_name(name)
{
}

Symbol::~Symbol()
{
}

class SymGetName {
public:
    std::string operator() (const Symbol* sym) const
    { return sym->get_name(); }
};

typedef yasm::hamt<std::string, Symbol, SymGetName> myhamt;

class GenSym {
public:
    GenSym(int nsym);
    void insert_check_new(myhamt& h);

    typedef boost::ptr_vector<Symbol> Symbols;
    Symbols syms;
};

GenSym::GenSym(int nsym)
{
    for (int i=0; i<nsym; i++) {
        std::ostringstream os;
        os << "sym" << i;
        syms.push_back(new Symbol(os.str()));
    }
}

void
GenSym::insert_check_new(myhamt& h)
{
    for (GenSym::Symbols::iterator i=syms.begin(), end=syms.end();
         i != end; ++i) {
        Symbol* old = h.insert(&(*i));
        BOOST_CHECK(old == 0);
    }
}

BOOST_AUTO_TEST_CASE(TestCaseBasic)
{
    GenSym g(NUM_SYMS);
    myhamt h(false);

    g.insert_check_new(h);
}

BOOST_AUTO_TEST_CASE(TestCaseFind)
{
    GenSym g(NUM_SYMS);
    myhamt h(false);

    g.insert_check_new(h);

    // find
    for (GenSym::Symbols::iterator i=g.syms.begin(), end=g.syms.end();
         i != end; ++i) {
        Symbol* sym = h.find(i->get_name());
        BOOST_CHECK(sym == &(*i));
    }
}

BOOST_AUTO_TEST_CASE(TestCaseDupInsert)
{
    GenSym g1(NUM_SYMS);
    GenSym g2(NUM_SYMS);
    myhamt h(false);

    g1.insert_check_new(h);

    // duplicate insertion (without replacement)
    for (GenSym::Symbols::iterator i=g1.syms.begin(), end=g1.syms.end(),
         i2=g2.syms.begin(), i2end=g2.syms.end();
         i != end && i2 != i2end; ++i, ++i2) {
        Symbol* old = h.insert(&(*i2));
        BOOST_CHECK(old == &(*i));
    }

    // check to make sure the hamt values didn't change
    for (GenSym::Symbols::iterator i=g1.syms.begin(), end=g1.syms.end();
         i != end; ++i) {
        Symbol* sym = h.find(i->get_name());
        BOOST_CHECK(sym == &(*i));
    }
}

BOOST_AUTO_TEST_CASE(TestCaseDupReplace)
{
    GenSym g1(NUM_SYMS);
    GenSym g2(NUM_SYMS);
    myhamt h(false);

    g1.insert_check_new(h);

    // duplicate insertion (with replacement)
    for (GenSym::Symbols::iterator i=g1.syms.begin(), end=g1.syms.end(),
         i2=g2.syms.begin(), i2end=g2.syms.end();
         i != end && i2 != i2end; ++i, ++i2) {
        Symbol* old = h.replace(&(*i2));
        BOOST_CHECK(old == &(*i));
    }

    // check to make sure the hamt values changed
    for (GenSym::Symbols::iterator i=g2.syms.begin(), end=g2.syms.end();
         i != end; ++i) {
        Symbol* sym = h.find(i->get_name());
        BOOST_CHECK(sym == &(*i));
    }
}

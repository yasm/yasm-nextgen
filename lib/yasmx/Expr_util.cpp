//
// Expression handling utility functions
//
//  Copyright (C) 2001-2007  Michael Urman, Peter Johnson
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
#include "yasmx/Expr_util.h"

#include "util.h"

#include <algorithm>

#include "yasmx/Expr.h"
#include "yasmx/Symbol.h"


using namespace yasm;

namespace
{

struct SawEqu
{
    yasm::Symbol* sym;
    int depth_delta;
    int end_n;
};

class MatchSawEqu
{
    yasm::Symbol* m_sym;
public:
    MatchSawEqu(yasm::Symbol* sym) : m_sym(sym) {}
    bool operator() (SawEqu& sawequ) { return sawequ.sym == m_sym; }
};

} // anonymous namespace

bool
yasm::ExpandEqu(Expr& e)
{
    if (e.isEmpty())
        return true;
    llvm::SmallVector<SawEqu, 8> seen;
    yasm::ExprTerms& terms = e.getTerms();

    int n = terms.size()-1;
    while (n >= 0)
    {
        ExprTerm* child = &terms[n];
        if (child->isEmpty())
        {
            --n;
            continue;
        }

        while (seen.size() > 0 && seen.back().end_n > n)
            seen.pop_back();

        // Update depth as needed
        int depth_delta = 0;
        if (seen.size() > 0)
            depth_delta = seen.back().depth_delta;
        child->m_depth += depth_delta;

        // Only look at equ's.
        yasm::Symbol* sym;
        const Expr* equ;
        if (!(sym = child->getSymbol()) || !(equ = sym->getEqu()))
        {
            --n;
            continue;
        }

        // Check for circular reference
        if (std::find_if(seen.begin(), seen.end(), MatchSawEqu(sym))
            != seen.end())
            return false;

        // Remember we saw this equ
        SawEqu justsaw = {sym, child->m_depth, n};
        seen.push_back(justsaw);

        // Insert copy of equ value and empty current term.
        terms.insert(terms.begin()+n, equ->getTerms().begin(),
                     equ->getTerms().end());
        n += equ->getTerms().size();
        terms[n].Clear();
        --n;
    }
    return true;
}

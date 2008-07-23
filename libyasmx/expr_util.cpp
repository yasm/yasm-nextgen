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
#include "expr_util.h"

#include "util.h"

#include <algorithm>

#include "errwarn.h"
#include "expr.h"
#include "symbol.h"


namespace
{

using yasm::Expr;

void
do_expand_equ(Expr* expr, std::vector<const Expr*>& seen)
{
    for (yasm::ExprTerms::iterator i=expr->get_terms().begin(),
         end=expr->get_terms().end(); i != end; ++i)
    {
        // Expand equ's.
        yasm::Symbol* sym;
        const Expr* equ;
        if ((sym = i->get_sym()) && (equ = sym->get_equ()))
        {
            // Check for circular reference
            if (std::find(seen.begin(), seen.end(), equ) != seen.end())
                throw yasm::TooComplexError(N_("circular reference detected"));

            Expr* newe = equ->clone();
            *i = newe;

            // Remember we saw this equ and recurse
            seen.push_back(equ);
            do_expand_equ(newe, seen);
            seen.pop_back();
        }
        else if (Expr* e = i->get_expr())
            do_expand_equ(e, seen);     // Recurse
    }
}

} // anonymous namespace

namespace yasm
{

void
expand_equ(Expr* expr)
{
    std::vector<const Expr*> seen;
    do_expand_equ(expr, seen);
}

} // namespace yasm

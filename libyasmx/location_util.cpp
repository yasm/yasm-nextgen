//
// YASM location utility functions implementation.
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
#include "location_util.h"

#include "bytecode.h"
#include "expr.h"
#include "intnum.h"
#include "symbol.h"


namespace yasm
{

// Transforms instances of Symbol-Symbol [Symbol+(-1*Symbol)] into single
// ExprTerms if possible.  Uses a simple n^2 algorithm because n is usually
// quite small.  Also works for loc-loc (or Symbol-loc, loc-Symbol).
static void
xform_dist_base(Expr* e, FUNCTION::function<bool (ExprTerm& term,
                                                  Location loc1,
                                                  Location loc2)> func)
{
    // Handle Symbol-Symbol in ADD exprs by looking for (-1*Symbol) and
    // Symbol term pairs (where both Symbols are in the same segment).
    if (!e->is_op(Op::ADD))
        return;

    ExprTerms& terms = e->get_terms();
    for (ExprTerms::iterator i=terms.begin(), end=terms.end();
         i != end; ++i)
    {
        // First look for an (-1*Symbol) term
        Expr* sube = i->get_expr();
        if (!sube)
            continue;
        ExprTerms& subterms = sube->get_terms();
        if (!sube->is_op(Op::MUL) || subterms.size() != 2)
            continue;

        IntNum* intn;
        Symbol* sym = 0;
        Location loc;

        if ((intn = subterms[0].get_int()))
        {
            if ((sym = subterms[1].get_sym()))
                ;
            else if (Location* locp = subterms[1].get_loc())
                loc = *locp;
            else
                continue;
        }
        else if ((intn = subterms[1].get_int()))
        {
            if ((sym = subterms[0].get_sym()))
                ;
            else if (Location* locp = subterms[0].get_loc())
                loc = *locp;
            else
                continue;
        }
        else
            continue;

        if (!intn->is_neg1())
            continue;

        if (sym && !sym->get_label(&loc))
            continue;
        BytecodeContainer* container = loc.bc->get_container();

        // Now look for a Symbol term in the same segment
        for (ExprTerms::iterator j=terms.begin(), end=terms.end();
             j != end; ++j)
        {
            Symbol* sym2;
            Location loc2;

            if ((sym2 = j->get_sym()) && sym2->get_label(&loc2))
                ;
            else if (Location* loc2p = j->get_loc())
                loc2 = *loc2p;
            else
                continue;

            if (container != loc2.bc->get_container())
                continue;

            if (func(*j, loc, loc2))
            {
                // Delete the matching (-1*Symbol) term
                i->release();
                break;  // stop looking for matching Symbol term
            }
        }
    }

    // Clean up any deleted (NONE) terms
    ExprTerms::iterator erasefrom =
        std::remove_if(terms.begin(), terms.end(),
                       BIND::bind(&ExprTerm::is_type, _1, ExprTerm::NONE));
    terms.erase(erasefrom, terms.end());
    ExprTerms(terms).swap(terms);   // trim capacity
}

static inline bool
calc_dist_cb(ExprTerm& term, Location loc, Location loc2)
{
    IntNum dist;
    if (!calc_dist(loc, loc2, &dist))
        return false;
    // Change the term to an integer
    term = dist;
    return true;
}

void
xform_calc_dist(Expr* e)
{
    xform_dist_base(e, &calc_dist_cb);
}

static inline bool
calc_dist_no_bc_cb(ExprTerm& term, Location loc, Location loc2)
{
    IntNum dist;
    if (!calc_dist_no_bc(loc, loc2, &dist))
        return false;
    // Change the term to an integer
    term = dist;
    return true;
}

void
xform_calc_dist_no_bc(Expr* e)
{
    xform_dist_base(e, &calc_dist_no_bc_cb);
}

static inline bool
subst_dist_cb(ExprTerm& term, Location loc, Location loc2,
              unsigned int* subst,
              FUNCTION::function<void (unsigned int subst,
                                       Location loc,
                                       Location loc2)> func)
{
    // Call higher-level callback
    func(*subst, loc, loc2);
    // Change the term to an subst
    term = ExprTerm(ExprTerm::Subst(*subst));
    *subst = *subst + 1;
    return true;
}

static inline void
xform_subst_dist(Expr* e, unsigned int* subst,
                 FUNCTION::function<void (unsigned int subst,
                                          Location loc,
                                          Location loc2)> func)
{
    xform_dist_base(e, BIND::bind(&subst_dist_cb, _1, _2, _3, subst, func));
}

int
subst_dist(Expr* e,
           FUNCTION::function<void (unsigned int subst, Location loc,
                                    Location loc2)> func)
{
    unsigned int subst = 0;
    e->level_tree(true, true, true,
                  BIND::bind(&xform_subst_dist, _1, &subst, func));
    return subst;
}

} // namespace yasm

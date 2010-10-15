#ifndef YASM_LOCATION_UTIL_H
#define YASM_LOCATION_UTIL_H
///
/// @file
/// @brief Location utility functions.
///
/// @license
///  Copyright (C) 2007  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
///  - Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
///  - Redistributions in binary form must reproduce the above copyright
///    notice, this list of conditions and the following disclaimer in the
///    documentation and/or other materials provided with the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
/// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
/// @endlicense
///
#include "yasmx/Config/functional.h"

#include "yasmx/Location.h"


class Diagnostic;
class Expr;
class ExprTerm;

/// Simplify instances of Symbol-Symbol [Symbol+(-1*Symbol)] in an expression
/// into integers if possible by calling calc_dist().
/// @param e            expression
/// @param diags        diagnostic reporting
/// @warning Only valid /after/ optimization.
void SimplifyCalcDist(Expr& e, Diagnostic& diags);

/// Simplify instances of Symbol-Symbol [Symbol+(-1*Symbol)] in an expression
/// into integers if possible by calling CalcDistNoBC().
/// Unlike SimplifyCalcDist(), this only calculates the distance if both
/// locations/symbols are within the same bytecode, so it's safe to call
/// prior to optimization being performed.
/// @param e            expression
/// @param diags        diagnostic reporting
void SimplifyCalcDistNoBC(Expr& e, Diagnostic& diags);

int SubstDist(Expr& e, Diagnostic& diags,
              const TR1::function<void (unsigned int subst,
                                        Location loc,
                                        Location loc2)>& func);

/// Try to evaluate an expression down to a single integer or float term.
/// Unlike SimplifyCalcDist(), does not modify the expression.
/// If the expression cannot be evaluated to int or float, false is returned.
/// @note If valueloc is true, replaces symbols/locations with bytecode offsets
///       without checking whether such combinations make sense (e.g. are
///       within the same section).
///       If you need such checking, use SimplifyCalcDist() instead.
/// @param e            expression
/// @param diags        diagnostic reporting
/// @param result       result expression term (output)
/// @param subst        Substitution terms ([0]=subst 0, [1]=subst 1, etc).
///                     Substitutions must be integer or float terms.
///                     If null, any substitution in the expr will result in
///                     evaluation failure and a false return
/// @param nsubst       size of substitution terms array
/// @param valueloc     replace symbols and locations with their values
/// @param zeroreg      replace registers with zero?
/// @return True if successful.
bool Evaluate(const Expr& e,
              Diagnostic& diags,
              ExprTerm* result,
              const ExprTerm* subst,
              unsigned int nsubst,
              bool valueloc=true,
              bool zeroreg=false);

inline bool
Evaluate(const Expr& e,
         Diagnostic& diags,
         ExprTerm* result,
         bool valueloc=true,
         bool zeroreg=false)
{
    return Evaluate(e, diags, result, 0, 0, valueloc, zeroreg);
}

#endif

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
#include "export.h"
#include "functional.h"
#include "location.h"


namespace yasm
{

class Expr;

/// Simplify instances of Symbol-Symbol [Symbol+(-1*Symbol)] in an expression
/// into integers if possible by calling calc_dist().
/// @param e            expression
/// @warning Only valid /after/ optimization.
YASM_LIB_EXPORT
void simplify_calc_dist(Expr& e);

/// Simplify instances of Symbol-Symbol [Symbol+(-1*Symbol)] in an expression
/// into integers if possible by calling calc_dist_no_bc().
/// Unlike simplify_calc_dist(), this only calculates the distance if both
/// locations/symbols are within the same bytecode, so it's safe to call
/// prior to optimization being performed.
/// @param e            expression
YASM_LIB_EXPORT
void simplify_calc_dist_no_bc(Expr& e);

YASM_LIB_EXPORT
int subst_dist(Expr& e,
               const FUNCTION::function<void (unsigned int subst,
                                              Location loc,
                                              Location loc2)>& func);

} // namespace yasm

#endif

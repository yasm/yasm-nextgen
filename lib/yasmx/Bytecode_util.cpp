///
/// @file libyasm/bytecode_util.cpp
/// @brief YASM bytecode utility functions implementation.
///
/// @license
///  Copyright (C) 2001-2008  Peter Johnson
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
#include "yasmx/Bytecode_util.h"

#include "yasmx/Support/errwarn.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Errwarns.h"

namespace yasm
{

void
finalize(Bytecode& bc, Errwarns& errwarns)
{
    try
    {
        bc.finalize();
    }
    catch (Error& err)
    {
        errwarns.propagate(bc.get_line(), err);
    }
    errwarns.propagate(bc.get_line());  // propagate warnings
}

void
calc_len(Bytecode& bc,
         const Bytecode::AddSpanFunc& add_span,
         Errwarns& errwarns)
{
    try
    {
        bc.calc_len(add_span);
    }
    catch (Error& err)
    {
        errwarns.propagate(bc.get_line(), err);
    }
    errwarns.propagate(bc.get_line());  // propagate warnings
}

bool
expand(Bytecode &bc,
       int span,
       long old_val,
       long new_val,
       /*@out@*/ long* neg_thres,
       /*@out@*/ long* pos_thres,
       Errwarns& errwarns)
{
    bool retval = false;
    try
    {
        retval = bc.expand(span, old_val, new_val, neg_thres, pos_thres);
    }
    catch (Error& err)
    {
        errwarns.propagate(bc.get_line(), err);
    }
    errwarns.propagate(bc.get_line());  // propagate warnings
    return retval;
}

unsigned long
update_offset(Bytecode& bc, unsigned long offset, Errwarns& errwarns)
{
    unsigned long retval;
    try
    {
        retval = bc.update_offset(offset);
    }
    catch (Error& err)
    {
        errwarns.propagate(bc.get_line(), err);
        retval = bc.next_offset();
    }
    errwarns.propagate(bc.get_line());  // propagate warnings
    return retval;
}

} // namespace yasm

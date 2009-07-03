#ifndef YASM_BYTECODE_UTIL_H
#define YASM_BYTECODE_UTIL_H
///
/// @file
/// @brief Bytecode utility functions.
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
#include "yasmx/Config/export.h"

#include "yasmx/Bytecode.h"


namespace yasm
{

class Errwarns;

/// Finalize a bytecode after parsing (wrapping Bytecode::finalize()).
/// Catches yasm errors and saves based on bytecode line number.
/// @param bc           bytecode
/// @param errwarns     error/warning set
YASM_LIB_EXPORT
void Finalize(Bytecode& bc, Errwarns& errwarns);

/// Resolve EQUs in a bytecode and calculate its minimum size
/// (wrapping Bytecode::calc_len()).
/// Catches yasm errors and saves based on bytecode line number.
/// @param bc           bytecode
/// @param add_span     function to call to add a span
/// @param errwarns     error/warning set
YASM_LIB_EXPORT
void CalcLen(Bytecode& bc,
             const Bytecode::AddSpanFunc& add_span,
             Errwarns& errwarns);

/// Recalculate a bytecode's length based on an expanded span length
/// (wrapping Bytecode::expand()).
/// Catches yasm errors and saves based on bytecode line number.
/// @param bc           bytecode
/// @param span         span ID (as given to yasm_bc_add_span_func in
///                     yasm_bc_calc_len)
/// @param old_val      previous span value
/// @param new_val      new span value
/// @param neg_thres    negative threshold for long/short decision
///                     (returned)
/// @param pos_thres    positive threshold for long/short decision
///                     (returned)
/// @param errwarns     error/warning set
/// @return False if bc no longer dependent on this span's length,
///         or true if bc size may increase for this span further
///         based on the new negative and positive thresholds returned.
YASM_LIB_EXPORT
bool Expand(Bytecode& bc,
            int span,
            long old_val,
            long new_val,
            /*@out@*/ long* neg_thres,
            /*@out@*/ long* pos_thres,
            Errwarns& errwarns);

/// Updates bytecode offset (wrapping Bytecode::update_offset()).
/// @param bc           bytecode
/// @param offset       offset to set this bytecode to
/// @param errwarns     error/warning set
/// @return Offset of next bytecode.
YASM_LIB_EXPORT
unsigned long UpdateOffset(Bytecode& bc,
                           unsigned long offset,
                           Errwarns& errwarns);

} // namespace yasm

#endif

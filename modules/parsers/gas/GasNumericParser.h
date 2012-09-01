#ifndef YASM_GASNUMERICPARSER_H
#define YASM_GASNUMERICPARSER_H
//
// GAS-compatible numeric literal parser
//
//  Copyright (C) 2009-2010  Peter Johnson
//
// Based on the LLVM Compiler Infrastructure
// (distributed under the University of Illinois Open Source License.
// See Copying/LLVM.txt for details).
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
#include "yasmx/Parse/NumericParser.h"


namespace yasm
{
class Preprocessor;
class SourceLocation;

namespace parser
{

/// This performs strict semantic analysis of the content of a ppnumber,
/// classifying it as either integer, floating, or erroneous, determines the
/// radix of the value and can convert it to a useful value.
class YASM_STD_EXPORT GasNumericParser : public NumericParser
{
public:
    /// @param force_float  If true, always treat as decimal float;
    ///                     0[letter] prefix is optional
    GasNumericParser(StringRef str,
                     SourceLocation loc,
                     Preprocessor& pp,
                     bool force_float = false);
    virtual ~GasNumericParser();

    //virtual bool getIntegerValue(IntNum* val);
    //virtual llvm::APFloat getFloatValue(const llvm::fltSemantics& format,
    //                                    bool* is_exact = 0);
};

}} // namespace yasm::parser

#endif

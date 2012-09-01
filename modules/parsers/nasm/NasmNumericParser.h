#ifndef YASM_NASMNUMERICPARSER_H
#define YASM_NASMNUMERICPARSER_H
//
// NASM-compatible numeric literal parser
//
//  Copyright (C) 2009  Peter Johnson
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
class YASM_STD_EXPORT NasmNumericParser : public NumericParser
{
public:
    NasmNumericParser(StringRef str,
                      SourceLocation loc,
                      Preprocessor& pp);
    virtual ~NasmNumericParser();

    //virtual bool getIntegerValue(IntNum* val);
    virtual llvm::APFloat getFloatValue(const llvm::fltSemantics& format,
                                        bool* is_exact = 0);

private:
    /// Read and skip over any hex digits, up to End.
    /// Return a pointer to the first non-hex digit or End.
    const char* SkipHexDigits(const char* ptr)
    {
        while (ptr != m_digits_end && (isxdigit(*ptr) || *ptr == '_'))
            ptr++;
        return ptr;
    }
  
    /// Read and skip over any octal digits, up to End.
    /// Return a pointer to the first non-hex digit or End.
    const char* SkipOctalDigits(const char* ptr)
    {
        while (ptr != m_digits_end &&
               ((*ptr >= '0' && *ptr <= '7') || *ptr == '_'))
            ptr++;
        return ptr;
    }
  
    /// Read and skip over any decimal digits, up to End.
    /// Return a pointer to the first non-hex digit or End.
    const char* SkipDigits(const char* ptr)
    {
        while (ptr != m_digits_end && (isdigit(*ptr) || *ptr == '_'))
            ptr++;
        return ptr;
    }
  
    /// Read and skip over any binary digits, up to End.
    /// Return a pointer to the first non-binary digit or End.
    const char* SkipBinaryDigits(const char* ptr)
    {
        while (ptr != m_digits_end &&
               (*ptr == '0' || *ptr == '1' || *ptr == '_'))
            ptr++;
        return ptr;
    }
};

}} // namespace yasm::parser

#endif

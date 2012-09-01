#ifndef YASM_PARSE_NUMERICPARSER_H
#define YASM_PARSE_NUMERICPARSER_H
//
// Numeric literal parser base class
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
#include <cctype>

#include "yasmx/Basic/LLVM.h"
#include "yasmx/Config/export.h"


namespace llvm
{
class APFloat;
struct fltSemantics;
}

namespace yasm
{

class IntNum;

/// This performs strict semantic analysis of the content of a ppnumber,
/// classifying it as either integer, floating, or erroneous, determines the
/// radix of the value and can convert it to a useful value.
class YASM_LIB_EXPORT NumericParser
{
public:
    NumericParser(StringRef str);
    virtual ~NumericParser();

    bool hadError() const { return m_had_error; }
    bool isInteger() const { return !m_is_float; }
    bool isFloat() const { return m_is_float; }

    unsigned int getRadix() const { return m_radix; }

    /// Convert this numeric literal value to an IntNum.
    /// If there is an overflow (i.e., if the unsigned
    /// value read is larger than the IntNum's bits will hold), set val to
    /// the low bits of the result and return true.  Otherwise, return false.
    virtual bool getIntegerValue(IntNum* val);

    /// Convert this numeric literal to a floating value, using
    /// the specified APFloat fltSemantics (specifying float, double, etc).
    /// The optional bool is_exact (passed-by-pointer) has its value
    /// set to true if the returned APFloat can represent the number in the
    /// literal exactly, and false otherwise.
    virtual llvm::APFloat getFloatValue(const llvm::fltSemantics& format,
                                        bool* is_exact = 0);

protected:
    const char* m_digits_begin;
    const char* m_digits_end;

    unsigned int m_radix;

    bool m_is_float;
    bool m_had_error;

    /// Read and skip over any hex digits, up to End.
    /// Return a pointer to the first non-hex digit or End.
    const char* SkipHexDigits(const char* ptr)
    {
        while (ptr != m_digits_end && isxdigit(*ptr))
            ptr++;
        return ptr;
    }
  
    /// Read and skip over any octal digits, up to End.
    /// Return a pointer to the first non-hex digit or End.
    const char* SkipOctalDigits(const char* ptr)
    {
        while (ptr != m_digits_end && (*ptr >= '0' && *ptr <= '7'))
            ptr++;
        return ptr;
    }
  
    /// Read and skip over any decimal digits, up to End.
    /// Return a pointer to the first non-hex digit or End.
    const char* SkipDigits(const char* ptr)
    {
        while (ptr != m_digits_end && isdigit(*ptr))
            ptr++;
        return ptr;
    }
  
    /// Read and skip over any binary digits, up to End.
    /// Return a pointer to the first non-binary digit or End.
    const char* SkipBinaryDigits(const char* ptr)
    {
        while (ptr != m_digits_end && (*ptr == '0' || *ptr == '1'))
            ptr++;
        return ptr;
    }
};

} // namespace yasm

#endif

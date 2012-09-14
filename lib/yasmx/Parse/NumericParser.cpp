//
// Numeric literal parser
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

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "yasmx/IntNum.h"


using namespace yasm;
using llvm::APFloat;

NumericParser::NumericParser(StringRef str)
    : m_digits_begin(str.begin())
    , m_digits_end(str.end())
    , m_is_float(false)
    , m_had_error(false)
{
}

NumericParser::~NumericParser()
{
}

bool
NumericParser::getIntegerValue(IntNum* val)
{
    if ((m_digits_end-m_digits_begin) == 0)
    {
        val->Zero();
        return false;
    }
    return val->setStr(StringRef(m_digits_begin, m_digits_end-m_digits_begin),
                       m_radix);
}

APFloat
NumericParser::getFloatValue(const llvm::fltSemantics& format,
                             bool* is_exact)
{
    SmallVector<char, 256> float_chars;
    for (const char* ch = m_digits_begin; ch < m_digits_end; ++ch)
        float_chars.push_back(*ch);

    float_chars.push_back('\0');

    APFloat val(format, APFloat::fcZero, false);

    // avoid assert in APFloat on empty string
    if (float_chars.size() == 1)
    {
        if (is_exact)
            *is_exact = true;
        return val;
    }

    APFloat::opStatus status =
        val.convertFromString(&float_chars[0], APFloat::rmNearestTiesToEven);

    if (is_exact)
        *is_exact = (status == APFloat::opOK);

    return val;
}

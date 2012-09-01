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
#include "GasNumericParser.h"

#include <string>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/SmallVector.h"
#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Parse/Preprocessor.h"
#include "yasmx/IntNum.h"


using namespace yasm;
using namespace yasm::parser;

/// decimal integer: [1-9] [0-9]*
/// binary integer: "0" [bB] [01]+
/// octal integer: "0" [0-7]*
/// hex integer: "0" [xX] [0-9a-fA-F]+
///
/// float: "0" [a-zA-Z except bB or xX]
///        [-+]? [0-9]* ([.] [0-9]*)? ([eE] [-+]? [0-9]+)?
///
GasNumericParser::GasNumericParser(StringRef str,
                                   SourceLocation loc,
                                   Preprocessor& pp,
                                   bool force_float)
    : NumericParser(str)
{
    // This routine assumes that the range begin/end matches the regex for
    // integer and FP constants, and assumes that the byte at "*end" is both
    // valid and not part of the regex.  Because of this, it doesn't have to
    // check for 'overscan' in various places.
    assert(!isalnum(*m_digits_end) && *m_digits_end != '.' &&
           "Lexer didn't maximally munch?");

    const char* s = str.begin();

    // Look for key radix prefixes
    if (force_float)
    {
        // forced decimal float; skip the prefix if present
        m_radix = 10;
        m_is_float = true;
        if (*s == '0' && isalpha(s[1]))
            s += 2;
    }
    else if (*s == '0' && (s[1] == 'x' || s[1] == 'X'))
    {
        m_radix = 16;
        s += 2;
    }
    else if (*s == '0' && (s[1] == 'b' || s[1] == 'B'))
    {
        m_radix = 2;
        s += 2;
    }
    else if (*s == '0' && isalpha(s[1]))
    {
        // it's a decimal float; skip the prefix
        m_radix = 10;
        s += 2;
        m_is_float = true;
    }
    else if (*s == '0')
    {
        // It's an octal integer
        m_radix = 8;
    }
    else
    {
        // Otherwise it's a decimal
        m_radix = 10;
    }

    m_digits_begin = s;

    switch (m_radix)
    {
        case 2:     s = SkipBinaryDigits(s); break;
        case 8:     s = SkipOctalDigits(s); break;
        case 10:    s = SkipDigits(s); break;
        case 16:    s = SkipHexDigits(s); break;
    }

    if (s == m_digits_end)
    {
        // Done.
    }
    else if (isxdigit(*s) && (!m_is_float || (*s != 'e' && *s != 'E')))
    {
        unsigned int err;
        switch (m_radix)
        {
            case 2: err = diag::err_invalid_binary_digit; break;
            case 8: err = diag::err_invalid_octal_digit; break;
            case 10: err = diag::err_invalid_decimal_digit; break;
            case 16:
            default:
                assert(false && "unexpected radix");
                err = diag::err_invalid_decimal_digit;
                break;
        }
        pp.Diag(pp.AdvanceToTokenCharacter(loc, s-str.begin()), err)
            << std::string(s, s+1);
        m_had_error = true;
        return;
    }
    else if (m_is_float)
    {
        if (*s == '-' || *s == '+')
        {
            ++s;
            s = SkipDigits(s);
        }
        if (*s == '.')
        {
            ++s;
            s = SkipDigits(s);
        }

        if (*s == 'e' || *s == 'E')
        {
            // Float exponent
            const char* exponent = s;
            ++s;
            if (*s == '+' || *s == '-') // sign
                ++s;
            const char* first_non_digit = SkipDigits(s);
            if (first_non_digit == s)
            {
                pp.Diag(pp.AdvanceToTokenCharacter(loc, exponent-str.begin()),
                        diag::err_exponent_has_no_digits);
                m_had_error = true;
                return;
            }
            s = first_non_digit;
        }
    }

    // Report an error if there are any.
    if (s != m_digits_end)
    {
        pp.Diag(pp.AdvanceToTokenCharacter(loc, s-str.begin()),
                m_is_float ? diag::err_invalid_suffix_float_constant :
                             diag::err_invalid_suffix_integer_constant)
            << std::string(s, str.end());
        m_had_error = true;
        return;
    }
}

GasNumericParser::~GasNumericParser()
{
}

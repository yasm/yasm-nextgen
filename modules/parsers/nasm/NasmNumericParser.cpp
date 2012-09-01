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
#include "NasmNumericParser.h"

#include <string>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/SmallVector.h"
#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Parse/Preprocessor.h"
#include "yasmx/IntNum.h"


using namespace yasm;
using namespace yasm::parser;
using llvm::APFloat;

/// decimal integer: [0-9] [0-9_]*
/// binary integer: [01] [01_]* [bB]
/// binary integer: "0b" [01_]+
/// octal integer: [0-7] [0-7_]* [qQoO]
/// hex integer: [0-9] [0-9a-fA-F_]* [hH]
/// hex integer: [$] [0-9] [0-9a-fA-F_]*
/// hex integer: "0x" [0-9a-fA-F_]+
///
/// decimal float: [0-9]+ [.] [0-9]* ([eE] [-+]? [0-9]+)?
/// decimal float: [0-9]+ [eE] [-+]? [0-9]+
/// hex float: "0x" [0-9a-fA-F_]* [.] [0-9a-fA-F]* ([pP] [-+]? [0-9]+)?
/// hex float: "0x" [0-9a-fA-F_]+ [pP] [-+]? [0-9]+
///
NasmNumericParser::NasmNumericParser(StringRef str,
                                     SourceLocation loc,
                                     Preprocessor& pp)
    : NumericParser(str)
{
    // This routine assumes that the range begin/end matches the regex for
    // integer and FP constants, and assumes that the byte at "*end" is both
    // valid and not part of the regex.  Because of this, it doesn't have to
    // check for 'overscan' in various places.
    assert(!isalnum(*m_digits_end) && *m_digits_end != '.' &&
           *m_digits_end != '_' && "Lexer didn't maximally munch?");

    const char* s = str.begin();
    bool float_ok = false;

    // Look for key radix flags (prefixes and suffixes)
    if (*s == '$')
    {
        m_radix = 16;
        ++s;
    }
    else if (m_digits_end[-1] == 'b' || m_digits_end[-1] == 'B')
    {
        m_radix = 2;
        --m_digits_end;
    }
    else if (m_digits_end[-1] == 'q' || m_digits_end[-1] == 'Q' ||
             m_digits_end[-1] == 'o' || m_digits_end[-1] == 'O')
    {
        m_radix = 8;
        --m_digits_end;
    }
    else if (m_digits_end[-1] == 'h' || m_digits_end[-1] == 'H')
    {
        m_radix = 16;
        --m_digits_end;
    }
    else if (*s == '0' && (s[1] == 'x' || s[1] == 'X') &&
             (isxdigit(s[2]) || s[2] == '.'))
    {
        m_radix = 16;
        float_ok = true;    // C99-style hex floating point
        s += 2;
    }
    else if (*s == '0' && (s[1] == 'b' || s[1] == 'B') &&
             (s[2] == '0' || s[2] == '1'))
    {
        m_radix = 2;
        s += 2;
    }
    else
    {
        // Otherwise it's a decimal or float
        m_radix = 10;
        float_ok = true;
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
    else if (isxdigit(*s) && (!float_ok || (*s != 'e' && *s != 'E')))
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
    else if (*s == '.' && float_ok)
    {
        ++s;
        m_is_float = true;
        if (m_radix == 16)
            s = SkipHexDigits(s);
        else
            s = SkipDigits(s);
    }

    if (float_ok &&
        ((m_radix == 10 && (*s == 'e' || *s == 'E')) ||
         (m_radix == 16 && (*s == 'p' || *s == 'P'))))
    {
        // Float exponent
        const char* exponent = s;
        ++s;
        m_is_float = true;
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

NasmNumericParser::~NasmNumericParser()
{
}

APFloat
NasmNumericParser::getFloatValue(const llvm::fltSemantics& format,
                                 bool* is_exact)
{
    SmallVector<char, 256> float_chars;
    for (const char* ch = m_digits_begin; ch < m_digits_end; ++ch)
    {
        if (*ch != '_')
            float_chars.push_back(*ch);
    }

    float_chars.push_back('\0');

    APFloat val(format, APFloat::fcZero, false);
  
    APFloat::opStatus status =
        val.convertFromString(&float_chars[0], APFloat::rmNearestTiesToEven);

    if (is_exact)
        *is_exact = (status == APFloat::opOK);

    return val;
}

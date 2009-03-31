#ifndef YASM_INTNUM_IOMANIP_H
#define YASM_INTNUM_IOMANIP_H
///
/// @file
/// @brief Big integer number stream formatting manipulators.
///
/// @license
///  Copyright (C) 2008  Peter Johnson
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
#include <ostream>

#include "Config/export.h"


namespace yasm
{

class IntNum;

class set_intnum_bits
{
public:
    set_intnum_bits(int bits) : m_bits(bits) {}
    void operator() (std::ios_base& s) const { s.iword(m_idx) = m_bits; }

private:
    static YASM_LIB_EXPORT const int m_idx;
    int m_bits;

    friend YASM_LIB_EXPORT
    std::ostream& operator<< (std::ostream& os, const IntNum& intn);
};

inline std::ostream&
operator<< (std::ostream& s, const set_intnum_bits& setter)
{
    setter(s);
    return s;
}

} // namespace yasm

#endif

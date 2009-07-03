#ifndef YASM_BITCOUNT_H
#define YASM_BITCOUNT_H
///
/// @file
/// @brief Bit-counting utility functions.
///
/// @license
///  Copyright (C) 2001-2007  Peter Johnson
///
/// Based on the paper "Ideal Hash Tries" by Phil Bagwell [2000].
/// One algorithmic change from that described in the paper: we use the LSB's
/// of the key to index the root table and move upward in the key rather than
/// use the MSBs as described in the paper.  The LSBs have more entropy.
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
/// 1. Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
/// 2. Redistributions in binary form must reproduce the above copyright
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

namespace yasm
{

// Bit-counting: used primarily by HAMT but also in a few other places.
inline unsigned long
BC_TWO(int c)
{
    return (0x1ul << c);
}

inline unsigned long
BC_MSK(int c)
{
    return (static_cast<unsigned long>(-1) / (BC_TWO(BC_TWO(c)) + 1ul));
}

inline void
BC_COUNT(unsigned long& x, int c)
{
    x = (x & BC_MSK(c)) + ((x >> BC_TWO(c)) & BC_MSK(c));
}

inline unsigned long
BitCount(unsigned long s)
{
    unsigned long d=s;
    BC_COUNT(d, 0);
    BC_COUNT(d, 1);
    BC_COUNT(d, 2);
    BC_COUNT(d, 3);
    BC_COUNT(d, 4);
    return d;
}

/// Determine if a value is exactly a power of 2.  Zero is treated as a power
/// of two.
/// @param x    value
/// @return Nonzero if x is a power of 2.
inline bool
isExp2(unsigned long x)
{
    return ((x & (x - 1)) == 0);
}

} // namespace yasm

#endif

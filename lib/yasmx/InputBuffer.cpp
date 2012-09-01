///
/// @file
/// @brief Input buffer implementation.
///
/// @license
///  Copyright (C) 2008-2009  Peter Johnson
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
#include "yasmx/InputBuffer.h"

#include <cassert>

#include "llvm/ADT/SmallVector.h"
#include "yasmx/IntNum.h"


using namespace yasm;

static inline uint64_t
ReadU64I(InputBuffer& input)
{
    uint64_t low, high;
    if (input.isBigEndian())
    {
        high = ReadU32(input);
        low = ReadU32(input);
    }
    else
    {
        low = ReadU32(input);
        high = ReadU32(input);
    }
    return ((high << 32) | low);
}

static IntNum
ReadN(InputBuffer& input, int n, bool sign)
{
    assert((n & 7) == 0 && "n must be a multiple of 8");
    assert(n <= IntNum::BITVECT_NATIVE_SIZE && "too large for internal format");
    assert(n > 0 && "can't read 0 bits");

    // optimize some fixed cases
    if (sign)
    {
        switch (n)
        {
            case 8: return ReadS8(input);
            case 16: return ReadS16(input);
            case 32: return ReadS32(input);
            default: break;
        }
    }
    else
    {
        switch (n)
        {
            case 8: return ReadU8(input);
            case 16: return ReadU16(input);
            case 32: return ReadU32(input);
            default: break;
        }
    }

    // Read the buffer into an array of words
    unsigned int nwords = (n+63)/64;
    SmallVector<uint64_t, 4> words(nwords);

    if (input.isBigEndian())
    {
        // start with bytes of most significant word
        int i = n;
        if ((n & 63) != 0)
        {
            int wend = n & ~63;
            uint64_t last = words[nwords-1];
            for (; i>=wend; i-=8)
            {
                last |= ReadU8(input);
                last <<= 8;
            }
        }
        // rest (if any) is whole words
        unsigned int w = nwords-2;
        for (; i>=64; i-=64, --w)
            words[w] = ReadU64I(input);
    }
    else
    {
        // whole words first
        unsigned int w = 0;
        int i = 0;
        for (; i<=n-64; i+=64, ++w)
            words[w] = ReadU64I(input);
        // finish with bytes
        if (i < n)
        {
            uint64_t last = 0;
            for (; i<n; i+=8)
            {
                last |= static_cast<uint64_t>(ReadU8(input)) << i;
            }
            words[w] = last;
        }
    }

    llvm::APInt val(n, nwords, &words[0]);
    // Zero extend if needed to make positive number
    if (!sign && val.isNegative())
        val = val.zext(n+1);

    // Convert to intnum
    IntNum intn;
    intn.setBV(val);
    return intn;
}

IntNum
yasm::ReadUnsigned(InputBuffer& input, int n)
{
    return ReadN(input, n, false);
}

IntNum
yasm::ReadSigned(InputBuffer& input, int n)
{
    return ReadN(input, n, true);
}

IntNum
yasm::ReadU64(InputBuffer& input)
{
    return ReadN(input, 64, false);
}

IntNum
yasm::ReadS64(InputBuffer& input)
{
    return ReadN(input, 64, true);
}

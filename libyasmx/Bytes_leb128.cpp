//
// LEB128 implementation.
//
//  Copyright (C) 2001-2008  Peter Johnson
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
#include "yasmx/Bytes_leb128.h"

#include <util.h>

#include "yasmx/Support/errwarn.h"
#include "yasmx/Bytes.h"
#include "yasmx/IntNum.h"


namespace yasm
{

static BitVector::scoped_wordptr staticbv(IntNum::BITVECT_NATIVE_SIZE);

static unsigned long
write_leb128(Bytes& bytes, BitVector::wordptr val, bool sign)
{
    unsigned long size;
    if (sign)
    {
        // Signed mode
        if (BitVector::msb_(val))
        {
            // Negative
            BitVector::Negate(staticbv, val);
            size = BitVector::Set_Max(staticbv)+2;
        }
        else
        {
            // Positive
            size = BitVector::Set_Max(val)+2;
        }
    }
    else
    {
        // Unsigned mode
        size = BitVector::Set_Max(val)+1;
    }

    // Positive/Unsigned write
    Bytes::size_type orig_size = bytes.size();
    unsigned long i = 0;
    for (; i<size-7; i += 7)
        bytes.push_back(static_cast<unsigned char>(
            BitVector::Chunk_Read(val, 7, i) | 0x80));
    // last byte does not have MSB set
    bytes.push_back(static_cast<unsigned char>(
        BitVector::Chunk_Read(val, 7, i)));
    return static_cast<unsigned long>(bytes.size()-orig_size);
}

static unsigned long
size_leb128(BitVector::wordptr val, bool sign)
{
    if (sign)
    {
        // Signed mode
        if (BitVector::msb_(val))
        {
            // Negative
            BitVector::Negate(staticbv, val);
            return (BitVector::Set_Max(staticbv)+8)/7;
        }
        else
        {
            // Positive
            return (BitVector::Set_Max(val)+8)/7;
        }
    }
    else
    {
        // Unsigned mode
        return (BitVector::Set_Max(val)+7)/7;
    }
}

unsigned long
write_leb128(Bytes& bytes, const IntNum& intn, bool sign)
{
    // Shortcut 0
    if (intn.is_zero())
    {
        bytes.push_back(0);
        return 1;
    }

    return write_leb128(bytes, intn.get_bv(staticbv), sign);
}

unsigned long
size_leb128(const IntNum& intn, bool sign)
{
    // Shortcut 0
    if (intn.is_zero())
        return 1;

    return size_leb128(intn.get_bv(staticbv), sign);
}

IntNum
read_leb128(Bytes& bytes, bool sign, /*@out@*/ unsigned long* size)
{
    const unsigned char* ptr;
    unsigned long n = 0, i = 0;

    BitVector::Empty(staticbv);
    for (;;)
    {
        ptr = bytes.read(1);
        BitVector::Chunk_Store(staticbv, 7, i, *ptr);
        ++n;
        i += 7;
        if ((*ptr & 0x80) != 0x80)
            break;
    }

    if (size)
        *size = n;

    if (i > IntNum::BITVECT_NATIVE_SIZE)
        throw OverflowError(
            N_("Numeric constant too large for internal format"));

    if (sign && (*ptr & 0x40) == 0x40)
        BitVector::Interval_Fill(staticbv, i, IntNum::BITVECT_NATIVE_SIZE-1);

    IntNum intn;
    intn.set_bv(staticbv);
    return intn;
}

unsigned long
write_sleb128(Bytes& bytes, long v)
{
    // Shortcut 0
    if (v == 0)
    {
        bytes.push_back(0);
        return 1;
    }

    BitVector::wordptr val = staticbv;

    BitVector::Empty(val);
    if (v >= 0)
        BitVector::Chunk_Store(val, 32, 0, static_cast<unsigned long>(v));
    else
    {
        BitVector::Chunk_Store(val, 32, 0, static_cast<unsigned long>(-v));
        BitVector::Negate(val, val);
    }
    return write_leb128(bytes, val, true);
}

unsigned long
size_sleb128(long v)
{
    if (v == 0)
        return 1;

    BitVector::wordptr val = staticbv;

    BitVector::Empty(val);
    if (v >= 0)
        BitVector::Chunk_Store(val, 32, 0, static_cast<unsigned long>(v));
    else
    {
        BitVector::Chunk_Store(val, 32, 0, static_cast<unsigned long>(-v));
        BitVector::Negate(val, val);
    }
    return size_leb128(val, true);
}

unsigned long
write_uleb128(Bytes& bytes, unsigned long v)
{
    // Shortcut 0
    if (v == 0)
    {
        bytes.push_back(0);
        return 1;
    }

    BitVector::wordptr val = staticbv;

    BitVector::Empty(val);
    BitVector::Chunk_Store(val, 32, 0, v);
    return write_leb128(bytes, val, false);
}

unsigned long
size_uleb128(unsigned long v)
{
    if (v == 0)
        return 1;

    BitVector::wordptr val = staticbv;

    BitVector::Empty(val);
    BitVector::Chunk_Store(val, 32, 0, v);
    return size_leb128(val, false);
}

} // namespace yasm

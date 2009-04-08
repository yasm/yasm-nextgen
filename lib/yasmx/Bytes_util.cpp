///
/// @file
/// @brief Bytes utility functions implementation.
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
#include "yasmx/Bytes_util.h"

#include <cassert>

#include "yasmx/IntNum.h"


namespace yasm
{

static BitVector::scoped_wordptr staticbv(IntNum::BITVECT_NATIVE_SIZE);

void
write_8(Bytes& bytes, const IntNum& intn)
{
    bytes.push_back(static_cast<unsigned char>(
        BitVector::Chunk_Read(intn.get_bv(staticbv), 8, 0)));
}

void
write_16(Bytes& bytes, const IntNum& intn)
{
    BitVector::wordptr bv = intn.get_bv(staticbv);
    if (bytes.is_bigendian())
    {
        bytes.push_back(static_cast<unsigned char>(
            BitVector::Chunk_Read(bv, 8, 8)));
        bytes.push_back(static_cast<unsigned char>(
            BitVector::Chunk_Read(bv, 8, 0)));
    }
    else
    {
        bytes.push_back(static_cast<unsigned char>(
            BitVector::Chunk_Read(bv, 8, 0)));
        bytes.push_back(static_cast<unsigned char>(
            BitVector::Chunk_Read(bv, 8, 8)));
    }
}

void
write_32(Bytes& bytes, const IntNum& intn)
{
    BitVector::wordptr bv = intn.get_bv(staticbv);
    if (bytes.is_bigendian())
    {
        for (int i=32-8; i>=0; i-=8)
            bytes.push_back(static_cast<unsigned char>(
                BitVector::Chunk_Read(bv, 8, i)));
    }
    else
    {
        for (int i=0; i<32; i+=8)
            bytes.push_back(static_cast<unsigned char>(
                BitVector::Chunk_Read(bv, 8, i)));
    }
}

void
write_64(Bytes& bytes, const IntNum& intn)
{
    BitVector::wordptr bv = intn.get_bv(staticbv);
    if (bytes.is_bigendian())
    {
        for (int i=64-8; i>=0; i-=8)
            bytes.push_back(static_cast<unsigned char>(
                BitVector::Chunk_Read(bv, 8, i)));
    }
    else
    {
        for (int i=0; i<64; i+=8)
            bytes.push_back(static_cast<unsigned char>(
                BitVector::Chunk_Read(bv, 8, i)));
    }
}

void
write_n(Bytes& bytes, const IntNum& intn, int n)
{
    assert((n & 7) == 0 && "n must be a multiple of 8");
    BitVector::wordptr bv = intn.get_bv(staticbv);
    if (bytes.is_bigendian())
    {
        for (int i=n-8; i>=0; i-=8)
            bytes.push_back(static_cast<unsigned char>(
                BitVector::Chunk_Read(bv, 8, i)));
    }
    else
    {
        for (int i=0; i<n; i+=8)
            bytes.push_back(static_cast<unsigned char>(
                BitVector::Chunk_Read(bv, 8, i)));
    }
}

void
write_n(Bytes& bytes, unsigned long val, int n)
{
    assert((n & 7) == 0 && "n must be a multiple of 8");
    if (bytes.is_bigendian())
    {
        for (int i=n-8; i>=0; i-=8)
            bytes.push_back(static_cast<unsigned char>((val >> i) & 0xFF));
    }
    else
    {
        for (int i=0; i<n; i+=8)
            bytes.push_back(static_cast<unsigned char>((val >> i) & 0xFF));
    }
}

static IntNum
read_n(Bytes& bytes, int n, bool sign)
{
    assert((n & 7) == 0 && "n must be a multiple of 8");
    assert(n <= IntNum::BITVECT_NATIVE_SIZE && "too large for internal format");
    assert(n > 0 && "can't read 0 bits");

    // Read the buffer into a bitvect
    const unsigned char* ptr;
    BitVector::Empty(staticbv);
    if (bytes.is_bigendian())
    {
        for (int i=n-8; i>=0; i-=8)
        {
            ptr = bytes.read(1);
            BitVector::Chunk_Store(staticbv, 8, i, *ptr);
        }
    }
    else
    {
        for (int i=0; i<n; i+=8)
        {
            ptr = bytes.read(1);
            BitVector::Chunk_Store(staticbv, 8, i, *ptr);
        }
    }

    // Sign extend if needed
    if (n < IntNum::BITVECT_NATIVE_SIZE && sign && (*ptr & 0x80) == 0x80)
        BitVector::Interval_Fill(staticbv, n, IntNum::BITVECT_NATIVE_SIZE-1);

    IntNum intn;
    intn.set_bv(staticbv);
    return intn;
}

IntNum
read_un(Bytes& bytes, int n)
{
    return read_n(bytes, n, false);
}

IntNum
read_sn(Bytes& bytes, int n)
{
    return read_n(bytes, n, true);
}

IntNum
read_u64(Bytes& bytes)
{
    return read_n(bytes, 64, false);
}

IntNum
read_s64(Bytes& bytes)
{
    return read_n(bytes, 64, true);
}

} // namespace yasm

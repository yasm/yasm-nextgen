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
#include "bytes_util.h"

#include "intnum.h"


namespace yasm
{

static BitVector::scoped_wordptr staticbv(IntNum::BITVECT_NATIVE_SIZE);

void
write_8(Bytes& bytes, const IntNum& intn)
{
    bytes.push_back(static_cast<unsigned char>(
        BitVector::Chunk_Read(intn.to_bv(staticbv), 8, 0)));
}

void
write_16(Bytes& bytes, const IntNum& intn)
{
    BitVector::wordptr bv = intn.to_bv(staticbv);
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
    BitVector::wordptr bv = intn.to_bv(staticbv);
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
    BitVector::wordptr bv = intn.to_bv(staticbv);
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

IntNum
read_u64(Bytes& bytes)
{
    const unsigned char* ptr = bytes.read(8);
    return IntNum(ptr, false, 4, bytes.is_bigendian());
}

} // namespace yasm

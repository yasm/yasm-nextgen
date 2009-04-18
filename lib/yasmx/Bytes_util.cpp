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

#include <util.h>

#include <cassert>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "yasmx/Support/Compose.h"
#include "yasmx/Support/errwarn.h"
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

bool
ok_size(const llvm::APInt& intn,
        unsigned int size,
        unsigned int rshift,
        int rangetype)
{
    unsigned int intn_size;
    switch (rangetype)
    {
        case 0:
            intn_size = intn.getActiveBits();
            break;
        case 1:
            intn_size = intn.getMinSignedBits();
            break;
        case 2:
            if (intn.isNegative())
                intn_size = intn.getMinSignedBits();
            else
                intn_size = intn.getActiveBits();
            break;
        default:
            assert(false && "invalid range type");
            return false;
    }
    return (intn_size <= (size-rshift));
}

void
overwrite(Bytes& bytes,
          const llvm::APInt& intn,
          unsigned int size,
          int shift,
          bool bigendian,
          int warn)
{
    // Split shift into left (shift) and right (rshift) components.
    unsigned int rshift = 0;
    if (shift < 0)
    {
        rshift = static_cast<unsigned int>(-shift);
        shift = 0;
    }

    // General size warnings
    if (warn<0 && !ok_size(intn, size, rshift, 1))
        warn_set(WARN_GENERAL,
                 String::compose(N_("value does not fit in signed %1 bit field"),
                                 size));
    if (warn>0 && !ok_size(intn, size, rshift, 2))
        warn_set(WARN_GENERAL,
                 String::compose(N_("value does not fit in %1 bit field"),
                                 size));

    // Check low bits if right shifting and warnings enabled
    if (warn && rshift > 0)
    {
        if (intn.countTrailingZeros() < rshift)
            warn_set(WARN_GENERAL,
                     N_("misaligned value, truncating to boundary"));
    }

    // Make a working copy of the right-shifted value
    llvm::APInt work = intn.ashr(rshift);

    // Sign extend (or truncate) to correct size
    work.sextOrTrunc(size);

    // Shortcut easy case
    if (shift == 0 && bytes.size()*8 == size)
    {
        assert(!bigendian && "big endian not yet supported");
        work.toOctetsLE(&bytes[0], bytes.size());
        return;
    }

    assert(false && "not yet implemented");
}

void
overwrite(Bytes& bytes,
          const llvm::APFloat& flt,
          unsigned int size,
          int shift,
          bool bigendian,
          int warn)
{
    const llvm::fltSemantics* semantics;
    switch (size)
    {
        case 32:
            semantics = &llvm::APFloat::IEEEsingle;
            break;
        case 64:
            semantics = &llvm::APFloat::IEEEdouble;
            break;
        case 80:
            semantics = &llvm::APFloat::x87DoubleExtended;
            break;
        default:
            assert(false && "invalid floating point constant size");
            return;
    }

    llvm::APFloat fltcopy = flt;
    bool lost_info = false;
    llvm::APFloat::opStatus status =
        fltcopy.convert(*semantics, llvm::APFloat::rmNearestTiesToEven,
                        &lost_info);
    if (warn)
    {
        switch (status)
        {
            case llvm::APFloat::opOverflow:
                warn_set(WARN_GENERAL,
                         N_("overflow in floating point expression"));
                break;
            case llvm::APFloat::opUnderflow:
                warn_set(WARN_GENERAL,
                         N_("underflow in floating point expression"));
                break;
            default:
                break;
        }
    }

    // use APInt function to actually output
    llvm::APInt fltbits = fltcopy.bitcastToAPInt();
    assert(fltbits.getBitWidth() == size && "bad float to bits conversion");
    overwrite(bytes, fltbits, size, shift, bigendian, warn);
}

} // namespace yasm

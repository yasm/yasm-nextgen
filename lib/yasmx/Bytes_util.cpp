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
#include "llvm/ADT/SmallVector.h"
#include "yasmx/Support/Compose.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/IntNum.h"


namespace yasm
{

static llvm::APInt staticbv(IntNum::BITVECT_NATIVE_SIZE, 0);

void
write_8(Bytes& bytes, const IntNum& intn)
{
    bytes.push_back(static_cast<unsigned char>(intn.extract(8, 0)));
}

void
write_16(Bytes& bytes, const IntNum& intn)
{
    write_16(bytes, static_cast<unsigned int>(intn.extract(16, 0)));
}

void
write_32(Bytes& bytes, const IntNum& intn)
{
    write_32(bytes, intn.extract(32, 0));
}

void
write_64(Bytes& bytes, const IntNum& intn)
{
    unsigned long low = intn.extract(32, 0);
    unsigned long high = intn.extract(32, 32);
    if (bytes.is_bigendian())
    {
        write_32(bytes, high);
        write_32(bytes, low);
    }
    else
    {
        write_32(bytes, low);
        write_32(bytes, high);
    }
}

static inline void
write_64i(Bytes& bytes, uint64_t val)
{
    unsigned long low = static_cast<unsigned long>(val);
    unsigned long high = static_cast<unsigned long>(val >> 32);
    if (bytes.is_bigendian())
    {
        write_32(bytes, high);
        write_32(bytes, low);
    }
    else
    {
        write_32(bytes, low);
        write_32(bytes, high);
    }
}

static inline uint64_t
read_u64i(Bytes& bytes)
{
    uint64_t low, high;
    if (bytes.is_bigendian())
    {
        high = read_u32(bytes);
        low = read_u32(bytes);
    }
    else
    {
        low = read_u32(bytes);
        high = read_u32(bytes);
    }
    return ((high << 32) | low);
}

void
write_n(Bytes& bytes, const IntNum& intn, int n)
{
    assert((n & 7) == 0 && "n must be a multiple of 8");

    // optimize some fixed cases
    switch (n)
    {
        case 8: write_8(bytes, intn); return;
        case 16: write_16(bytes, intn); return;
        case 32: write_32(bytes, intn); return;
        case 64: write_64(bytes, intn); return;
        default: break;
    }

    // harder cases
    const llvm::APInt* bv = intn.get_bv(&staticbv);
    const uint64_t* words = bv->getRawData();
    unsigned int nwords = bv->getNumWords();
    llvm::APInt tmp;    // must be here so it stays in scope
    if (bv->getBitWidth() < static_cast<unsigned int>(n*8))
    {
        tmp = *bv;
        tmp.sext(n*8);
        words = tmp.getRawData();
        nwords = tmp.getNumWords();
    }
    if (bytes.is_bigendian())
    {
        // start with bytes of most significant word
        int i = n;
        if ((n & 63) != 0)
        {
            int wend = n & ~63;
            uint64_t last = words[nwords-1];
            for (; i>=wend; i-=8)
            {
                bytes.push_back(static_cast<unsigned char>
                                ((last >> (i-wend)) & 0xFF));
            }
        }
        // rest (if any) is whole words
        unsigned int w = nwords-2;
        for (; i>=64; i-=64, --w)
            write_64i(bytes, words[w]);
    }
    else
    {
        // whole words first
        unsigned int w = 0;
        int i = 0;
        for (; i<=n-64; i+=64, ++w)
            write_64i(bytes, words[w]);
        // finish with bytes
        if (i < n)
        {
            uint64_t last = words[w];
            for (; i<n; i+=8)
            {
                bytes.push_back(static_cast<unsigned char>(last & 0xFF));
                last >>= 8;
            }
        }
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

    // optimize some fixed cases
    if (sign)
    {
        switch (n)
        {
            case 8: return read_s8(bytes);
            case 16: return read_s16(bytes);
            case 32: return read_s32(bytes);
            case 64: return read_s64(bytes);
            default: break;
        }
    }
    else
    {
        switch (n)
        {
            case 8: return read_u8(bytes);
            case 16: return read_u16(bytes);
            case 32: return read_u32(bytes);
            case 64: return read_u64(bytes);
            default: break;
        }
    }

    // Read the buffer into an array of words
    unsigned int nwords = (n+63)/64;
    llvm::SmallVector<uint64_t, 4> words(nwords);

    if (bytes.is_bigendian())
    {
        // start with bytes of most significant word
        int i = n;
        if ((n & 63) != 0)
        {
            int wend = n & ~63;
            uint64_t last = words[nwords-1];
            for (; i>=wend; i-=8)
            {
                last |= read_u8(bytes);
                last <<= 8;
            }
        }
        // rest (if any) is whole words
        unsigned int w = nwords-2;
        for (; i>=64; i-=64, --w)
            words[w] = read_u64i(bytes);
    }
    else
    {
        // whole words first
        unsigned int w = 0;
        int i = 0;
        for (; i<=n-64; i+=64, ++w)
            words[w] = read_u64i(bytes);
        // finish with bytes
        if (i < n)
        {
            uint64_t last = 0;
            for (; i<n; i+=8)
            {
                last |= static_cast<uint64_t>(read_u8(bytes)) << i;
            }
            words[w] = last;
        }
    }

    llvm::APInt val(n, nwords, &words[0]);
    // Zero extend if needed to make positive number
    if (!sign && val.isNegative())
        val.zext(n+1);

    // Convert to intnum
    IntNum intn;
    intn.set_bv(val);
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
#if 0
    if (bigendian)
    {
        // TODO
        assert(false && "big endian not implemented");
    }
    else
        BitVector::Block_Store(op1, ptr, static_cast<N_int>(destsize));

    // If not already a bitvect, convert value to be written to a bitvect
    wordptr op2 = get_bv(op2static);

    // Check low bits if right shifting and warnings enabled
    if (warn && rshift > 0)
    {
        BitVector::Copy(conv_bv, op2);
        BitVector::Move_Left(conv_bv,
                             static_cast<N_int>(BITVECT_NATIVE_SIZE-rshift));
        if (!BitVector::is_empty(conv_bv))
            warn_set(WARN_GENERAL,
                     N_("misaligned value, truncating to boundary"));
    }

    // Shift right if needed
    if (rshift > 0)
    {
        bool carry_in = BitVector::msb_(op2);
        while (rshift-- > 0)
            BitVector::shift_right(op2, carry_in);
    }

    // Write the new value into the destination bitvect
    BitVector::Interval_Copy(op1, op2, static_cast<unsigned int>(shift), 0,
                             static_cast<N_int>(size));

    // Write out the new data
    unsigned int len;
    unsigned char* buf = BitVector::Block_Read(op1, &len);
    if (bigendian)
    {
        // TODO
        assert(false && "big endian not implemented");
    }
    else
        std::memcpy(ptr, buf, destsize);
    free(buf);
#endif
}

void
overwrite(Bytes& bytes,
          const IntNum& intn,
          unsigned int size,
          int shift,
          bool bigendian,
          int warn)
{
    int destsize = bytes.size();

    // Split shift into left (shift) and right (rshift) components.
    unsigned int rshift = 0;
    if (shift < 0)
    {
        rshift = static_cast<unsigned int>(-shift);
        shift = 0;
    }

    // General size warnings
    if (warn<0 && !intn.ok_size(size, rshift, 1))
        warn_set(WARN_GENERAL,
                 String::compose(N_("value does not fit in signed %1 bit field"),
                                size));
    if (warn>0 && !intn.ok_size(size, rshift, 2))
        warn_set(WARN_GENERAL,
                 String::compose(N_("value does not fit in %1 bit field"),
                                 size));

    // Non-bigval (for speed)
    if (intn.is_int())
    {
        long v = intn.get_int();

        // Check low bits if right shifting and warnings enabled
        if (warn && rshift > 0)
        {
            long mask = (1L<<rshift)-1;
            if (v & mask)
                warn_set(WARN_GENERAL,
                         N_("misaligned value, truncating to boundary"));
        }

        // Shift right if needed
        v >>= rshift;
        rshift = 0;

        // Write out the new data, 8 bits at a time.
        assert(!bigendian && "big endian not implemented");
        for (int i = 0, sz = size; i < destsize && sz > 0; ++i)
        {
            // handle left shift past whole bytes
            if (shift >= 8)
            {
                shift -= 8;
                continue;
            }

            // Handle first chunk specially for left shifted values
            if (shift > 0 && sz == static_cast<int>(size))
            {
                unsigned char chunk =
                    ((static_cast<unsigned char>(v) & 0xff) << shift) & 0xff;
                unsigned char mask = ~((1U<<shift)-1);  // keep MSBs

                // write appropriate bits
                bytes[i] &= ~mask;
                bytes[i] |= chunk & mask;

                v >>= (8-shift);
                sz -= (8-shift);
            }
            else
            {
                unsigned char chunk = static_cast<unsigned char>(v) & 0xff;
                unsigned char mask = 0xff;
                // for last chunk, need to keep least significant bits
                if (sz < 8)
                    mask = (1U<<sz)-1;

                // write appropriate bits
                bytes[i] &= ~mask;
                bytes[i] |= chunk & mask;

                v >>= 8;
                sz -= 8;
            }
        }
    }
    else
        overwrite(bytes, *intn.get_bv(&staticbv), size, shift, bigendian, warn);
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

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


using namespace yasm;

static llvm::APInt staticbv(IntNum::BITVECT_NATIVE_SIZE, 0);

void
yasm::Write8(Bytes& bytes, const IntNum& intn)
{
    bytes.push_back(static_cast<unsigned char>(intn.Extract(8, 0)));
}

void
yasm::Write16(Bytes& bytes, const IntNum& intn)
{
    Write16(bytes, static_cast<unsigned int>(intn.Extract(16, 0)));
}

void
yasm::Write32(Bytes& bytes, const IntNum& intn)
{
    Write32(bytes, intn.Extract(32, 0));
}

void
yasm::Write64(Bytes& bytes, const IntNum& intn)
{
    unsigned long low = intn.Extract(32, 0);
    unsigned long high = intn.Extract(32, 32);
    if (bytes.isBigEndian())
    {
        Write32(bytes, high);
        Write32(bytes, low);
    }
    else
    {
        Write32(bytes, low);
        Write32(bytes, high);
    }
}

static inline void
Write64I(Bytes& bytes, uint64_t val)
{
    unsigned long low = static_cast<unsigned long>(val);
    unsigned long high = static_cast<unsigned long>(val >> 32);
    if (bytes.isBigEndian())
    {
        Write32(bytes, high);
        Write32(bytes, low);
    }
    else
    {
        Write32(bytes, low);
        Write32(bytes, high);
    }
}

void
yasm::WriteN(Bytes& bytes, const IntNum& intn, int n)
{
    assert((n & 7) == 0 && "n must be a multiple of 8");

    // optimize some fixed cases
    switch (n)
    {
        case 8: Write8(bytes, intn); return;
        case 16: Write16(bytes, intn); return;
        case 32: Write32(bytes, intn); return;
        case 64: Write64(bytes, intn); return;
        default: break;
    }

    // harder cases
    const llvm::APInt* bv = intn.getBV(&staticbv);
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
    if (bytes.isBigEndian())
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
            Write64I(bytes, words[w]);
    }
    else
    {
        // whole words first
        unsigned int w = 0;
        int i = 0;
        for (; i<=n-64; i+=64, ++w)
            Write64I(bytes, words[w]);
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
yasm::WriteN(Bytes& bytes, unsigned long val, int n)
{
    assert((n & 7) == 0 && "n must be a multiple of 8");
    if (bytes.isBigEndian())
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

void
yasm::Overwrite(Bytes& bytes,
                const llvm::APInt& intn,
                unsigned int size,
                int shift,
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
    if (warn<0 && !isOkSize(intn, size, rshift, 1))
        setWarn(WARN_GENERAL,
                String::Compose(N_("value does not fit in signed %1 bit field"),
                                size));
    if (warn>0 && !isOkSize(intn, size, rshift, 2))
        setWarn(WARN_GENERAL,
                String::Compose(N_("value does not fit in %1 bit field"),
                                size));

    // Check low bits if right shifting and warnings enabled
    if (warn && rshift > 0)
    {
        if (intn.countTrailingZeros() < rshift)
            setWarn(WARN_GENERAL,
                    N_("misaligned value, truncating to boundary"));
    }

    // Make a working copy of the right-shifted value
    llvm::APInt work = intn.ashr(rshift);

    // Sign extend (or truncate) to correct size
    work.sextOrTrunc(size);

    // Shortcut easy case
    if (shift == 0 && bytes.size()*8 == size)
    {
        assert(bytes.isLittleEndian() && "big endian not yet supported");

        const uint64_t* words = work.getRawData();

        // whole words first
        unsigned int w = 0;
        int i = 0, o = 0;
        int n = static_cast<int>(size);
        for (; i<=n-64; i+=64, ++w)
        {
            uint64_t wrd = words[w];
            for (int j=0; j<8; ++j)
            {
                bytes[o++] = static_cast<unsigned char>(wrd) & 0xFF;
                wrd >>= 8;
            }
        }

        // finish with bytes
        if (i < n)
        {
            uint64_t last = words[w];
            for (; i<n; i+=8)
            {
                bytes[o++] = static_cast<unsigned char>(last) & 0xFF;
                last >>= 8;
            }
        }
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
yasm::Overwrite(Bytes& bytes,
                const IntNum& intn,
                unsigned int size,
                int shift,
                int warn)
{
    // Handle bigval specially
    if (!intn.isInt())
    {
        Overwrite(bytes, *intn.getBV(&staticbv), size, shift, warn);
        return;
    }

    int destsize = bytes.size();

    // Split shift into left (shift) and right (rshift) components.
    unsigned int rshift = 0;
    if (shift < 0)
    {
        rshift = static_cast<unsigned int>(-shift);
        shift = 0;
    }

    // General size warnings
    if (warn<0 && !intn.isOkSize(size, rshift, 1))
        setWarn(WARN_GENERAL,
                String::Compose(N_("value does not fit in signed %1 bit field"),
                                size));
    if (warn>0 && !intn.isOkSize(size, rshift, 2))
        setWarn(WARN_GENERAL,
                String::Compose(N_("value does not fit in %1 bit field"),
                                size));

    long v = intn.getInt();

    // Check low bits if right shifting and warnings enabled
    if (warn && rshift > 0)
    {
        long mask = (1L<<rshift)-1;
        if (v & mask)
            setWarn(WARN_GENERAL,
                    N_("misaligned value, truncating to boundary"));
    }

    // Shift right if needed
    v >>= rshift;
    rshift = 0;

    // Write out the new data, 8 bits at a time.
    assert(bytes.isLittleEndian() && "big endian not implemented");
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

void
yasm::Overwrite(Bytes& bytes,
                const llvm::APFloat& flt,
                unsigned int size,
                int shift,
                int warn)
{
    const llvm::fltSemantics* semantics;
    switch (size)
    {
        case 16:
            semantics = &llvm::APFloat::IEEEhalf;
            break;
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
                setWarn(WARN_GENERAL,
                        N_("overflow in floating point expression"));
                break;
            case llvm::APFloat::opUnderflow:
                setWarn(WARN_GENERAL,
                        N_("underflow in floating point expression"));
                break;
            default:
                break;
        }
    }

    // use APInt function to actually output
    llvm::APInt fltbits = fltcopy.bitcastToAPInt();
    assert(fltbits.getBitWidth() == size && "bad float to bits conversion");
    Overwrite(bytes, fltbits, size, shift, warn);
}

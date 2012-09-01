///
/// @file
/// @brief Numeric output implementation.
///
/// @license
///  Copyright (C) 2008-2010  Peter Johnson
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
#include "yasmx/NumericOutput.h"

#include <cassert>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Bytes.h"
#include "yasmx/IntNum.h"


using namespace yasm;
using llvm::APInt;
using llvm::APFloat;

static APInt staticbv(IntNum::BITVECT_NATIVE_SIZE, 0);

NumericOutput::NumericOutput(Bytes& bytes)
    : m_bytes(bytes)
    , m_size(0)
    , m_shift(0)
    , m_rshift(0)
    , m_sign(false)
    , m_warns_enabled(false)
    , m_warns(0)
{
}

void
NumericOutput::EmitWarnings(DiagnosticsEngine& diags) const
{
    if (m_sign && (m_warns & INT_OVERFLOW) != 0)
        diags.Report(m_source, diag::warn_signed_overflow) << m_size;
    if (!m_sign && (m_warns & INT_OVERFLOW) != 0)
        diags.Report(m_source, diag::warn_unsigned_overflow) << m_size;
    if ((m_warns & FP_UNDERFLOW) != 0)
        diags.Report(m_source, diag::warn_float_underflow);
    if ((m_warns & FP_OVERFLOW) != 0)
        diags.Report(m_source, diag::warn_float_overflow);
    if ((m_warns & TRUNCATED) != 0)
        diags.Report(m_source, diag::warn_truncated) << m_rshift;
}

void
NumericOutput::OutputInteger(const APInt& intn)
{
    // General size warnings
    if (m_warns_enabled && m_sign && !isOkSize(intn, m_size, m_rshift, 1))
        m_warns |= INT_OVERFLOW;
    if (m_warns_enabled && !m_sign && !isOkSize(intn, m_size, m_rshift, 2))
        m_warns |= INT_OVERFLOW;

    // Check low bits if right shifting
    if (m_warns_enabled && m_rshift > 0 && intn.countTrailingZeros() < m_rshift)
        m_warns |= TRUNCATED;

    // Make a working copy of the right-shifted value
    APInt work = intn.ashr(m_rshift);

    // Sign extend (or truncate) to correct size
    work = work.sextOrTrunc(m_size);

    // Shortcut easy case
    if (m_shift == 0 && m_bytes.size()*8 == m_size)
    {
        assert(m_bytes.isLittleEndian() && "big endian not yet supported");

        const uint64_t* words = work.getRawData();

        // whole words first
        unsigned int w = 0;
        int i = 0, o = 0;
        int n = static_cast<int>(m_size);
        for (; i<=n-64; i+=64, ++w)
        {
            uint64_t wrd = words[w];
            for (int j=0; j<8; ++j)
            {
                m_bytes[o++] = static_cast<unsigned char>(wrd) & 0xFF;
                wrd >>= 8;
            }
        }

        // finish with bytes
        if (i < n)
        {
            uint64_t last = words[w];
            for (; i<n; i+=8)
            {
                m_bytes[o++] = static_cast<unsigned char>(last) & 0xFF;
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
NumericOutput::OutputInteger(const IntNum& intn)
{
    // Handle bigval specially
    if (!intn.isInt())
        return OutputInteger(*intn.getBV(&staticbv));

    int destsize = m_bytes.size();

    // General size warnings
    if (m_warns_enabled && m_sign && !intn.isOkSize(m_size, m_rshift, 1))
        m_warns |= INT_OVERFLOW;
    if (m_warns_enabled && !m_sign && !intn.isOkSize(m_size, m_rshift, 2))
        m_warns |= INT_OVERFLOW;

    long v = intn.getInt();

    // Check low bits if right shifting and warnings enabled
    if (m_warns_enabled && m_rshift > 0 && (v & ((1L<<m_rshift)-1)) != 0)
        m_warns |= TRUNCATED;

    // Shift right if needed
    v >>= m_rshift;

    // Write out the new data, 8 bits at a time.
    assert(m_bytes.isLittleEndian() && "big endian not implemented");
    for (int i = 0, sz = m_size; i < destsize && sz > 0; ++i)
    {
        // handle left shift past whole bytes
        if (m_shift >= 8)
        {
            m_shift -= 8;
            continue;
        }

        // Handle first chunk specially for left shifted values
        if (m_shift > 0 && sz == static_cast<int>(m_size))
        {
            unsigned char chunk =
                ((static_cast<unsigned char>(v) & 0xff) << m_shift) & 0xff;
            unsigned char mask = ~((1U<<m_shift)-1);    // keep MSBs

            // write appropriate bits
            m_bytes[i] &= ~mask;
            m_bytes[i] |= chunk & mask;

            v >>= (8-m_shift);
            sz -= (8-m_shift);
        }
        else
        {
            unsigned char chunk = static_cast<unsigned char>(v) & 0xff;
            unsigned char mask = 0xff;
            // for last chunk, need to keep least significant bits
            if (sz < 8)
                mask = (1U<<sz)-1;

            // write appropriate bits
            m_bytes[i] &= ~mask;
            m_bytes[i] |= chunk & mask;

            v >>= 8;
            sz -= 8;
        }
    }
}

void
NumericOutput::OutputFloat(const APFloat& flt)
{
    const llvm::fltSemantics* semantics;
    switch (m_size)
    {
        case 16: semantics = &APFloat::IEEEhalf; break;
        case 32: semantics = &APFloat::IEEEsingle; break;
        case 64: semantics = &APFloat::IEEEdouble; break;
        case 80: semantics = &APFloat::x87DoubleExtended; break;
        default:
            assert(false && "invalid floating point constant size");
            return;
    }

    APFloat fltcopy = flt;
    bool lost_info = false;
    APFloat::opStatus status =
        fltcopy.convert(*semantics, APFloat::rmNearestTiesToEven,
                        &lost_info);
    if (m_warns_enabled)
    {
        switch (status)
        {
            case APFloat::opOverflow:   m_warns |= FP_OVERFLOW; break;
            case APFloat::opUnderflow:  m_warns |= FP_UNDERFLOW; break;
            default: break;
        }
    }

    // use APInt function to actually output
    APInt fltbits = fltcopy.bitcastToAPInt();
    assert(fltbits.getBitWidth() == m_size && "bad float to bits conversion");
    OutputInteger(fltbits);
}

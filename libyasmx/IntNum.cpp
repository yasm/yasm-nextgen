//
// Integer number functions.
//
//  Copyright (C) 2001-2007  Peter Johnson
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
#include "yasmx/IntNum.h"
#include "yasmx/IntNum_iomanip.h"

#include "util.h"

#include <cctype>
#include <cstring>
#include <limits>

#include "yasmx/Support/BitVector.h"
#include "yasmx/Support/Compose.h"
#include "yasmx/errwarn.h"


using BitVector::wordptr;
using BitVector::N_int;

#define LONG_BITS \
    static_cast<unsigned int>(std::numeric_limits<long>::digits)
#define ULONG_BITS \
    static_cast<unsigned int>(std::numeric_limits<unsigned long>::digits)

namespace yasm
{

YASM_LIB_EXPORT const int set_intnum_bits::m_idx = std::ios_base::xalloc();

/// Static bitvect used for conversions.
static BitVector::scoped_wordptr conv_bv(IntNum::BITVECT_NATIVE_SIZE);

/// Static bitvects used for computation.
static BitVector::scoped_wordptr result(IntNum::BITVECT_NATIVE_SIZE);
static BitVector::scoped_wordptr spare(IntNum::BITVECT_NATIVE_SIZE);
static BitVector::scoped_wordptr op1static(IntNum::BITVECT_NATIVE_SIZE);
static BitVector::scoped_wordptr op2static(IntNum::BITVECT_NATIVE_SIZE);

/// Static bitvect decimal conversion.
static BitVector::from_Dec_static my_from_Dec(IntNum::BITVECT_NATIVE_SIZE);

void
IntNum::from_bv(wordptr bv)
{
    if (BitVector::Set_Max(bv) < LONG_BITS)
    {
        m_type = INTNUM_L;
        m_val.l = static_cast<long>(BitVector::Chunk_Read(bv, LONG_BITS, 0));
    }
    else if (BitVector::msb_(bv))
    {
        // Negative, negate and see if we'll fit into a long.
        BitVector::Negate(bv, bv);
        if (BitVector::Set_Max(bv) >= LONG_BITS)
        {
            // too negative
            BitVector::Negate(bv, bv);
            m_type = INTNUM_BV;
            m_val.bv = BitVector::Clone(bv);
        }
        else
        {
            unsigned long ul = BitVector::Chunk_Read(bv, LONG_BITS, 0);
            m_type = INTNUM_L;
            m_val.l = -static_cast<long>(ul);
        }
    }
    else
    {
        m_type = INTNUM_BV;
        m_val.bv = BitVector::Clone(bv);
    }
}

wordptr
IntNum::to_bv(/*@returned@*/ wordptr bv) const
{
    if (m_type == INTNUM_BV)
        return m_val.bv;

    BitVector::Empty(bv);
    if (m_val.l >= 0)
        BitVector::Chunk_Store(bv, LONG_BITS, 0,
                               static_cast<unsigned long>(m_val.l));
    else
    {
        BitVector::Chunk_Store(bv, LONG_BITS, 0,
                               static_cast<unsigned long>(-m_val.l));
        BitVector::Negate(bv, bv);
    }
    return bv;
}

IntNum::IntNum(char* str, int base)
{
    BitVector::ErrCode err;
    const char* errstr;

    switch (base)
    {
        case 2:
            err = BitVector::from_Bin(conv_bv,
                                      reinterpret_cast<unsigned char *>(str));
            errstr = N_("invalid binary literal");
            break;
        case 8:
            err = BitVector::from_Oct(conv_bv,
                                      reinterpret_cast<unsigned char *>(str));
            errstr = N_("invalid octal literal");
            break;
        case 10:
            err = my_from_Dec(conv_bv, reinterpret_cast<unsigned char *>(str));
            errstr = N_("invalid decimal literal");
            break;
        case 16:
            err = BitVector::from_Hex(conv_bv,
                                      reinterpret_cast<unsigned char *>(str));
            errstr = N_("invalid hex literal");
            break;
        default:
            throw ValueError(N_("invalid base"));
    }

    switch (err)
    {
        case BitVector::ErrCode_Pars:
            throw ValueError(errstr);
        case BitVector::ErrCode_Ovfl:
            throw OverflowError(
                N_("Numeric constant too large for internal format"));
        default:
            break;
    }
    from_bv(conv_bv);
}

IntNum::IntNum(const unsigned char* ptr, bool sign, unsigned long& size)
{
    const unsigned char* ptr_orig = ptr;
    unsigned long i = 0;

    BitVector::Empty(conv_bv);
    for (;;)
    {
        BitVector::Chunk_Store(conv_bv, 7, i, *ptr);
        i += 7;
        if ((*ptr & 0x80) != 0x80)
            break;
        ptr++;
    }

    size = static_cast<unsigned long>(ptr-ptr_orig)+1;

    if (i > BITVECT_NATIVE_SIZE)
        throw OverflowError(
            N_("Numeric constant too large for internal format"));
    else if (sign && (*ptr & 0x40) == 0x40)
        BitVector::Interval_Fill(conv_bv, i, BITVECT_NATIVE_SIZE-1);

    from_bv(conv_bv);
}

IntNum::IntNum(const unsigned char* ptr,
               bool sign,
               unsigned int srcsize,
               bool bigendian)
{
    if (srcsize*8 > BITVECT_NATIVE_SIZE)
        throw OverflowError(
            N_("Numeric constant too large for internal format"));

    // Read the buffer into a bitvect
    unsigned long i = 0;
    BitVector::Empty(conv_bv);
    if (bigendian)
    {
        for (i = 0; i < srcsize; i++)
            BitVector::Chunk_Store(conv_bv, 8, (srcsize-1-i)*8, ptr[i]);
    }
    else
    {
        for (i = 0; i < srcsize; i++)
            BitVector::Chunk_Store(conv_bv, 8, i*8, ptr[i]);
    }

    // Sign extend if needed
    if (srcsize*8 < BITVECT_NATIVE_SIZE && sign && (ptr[i] & 0x80) == 0x80)
        BitVector::Interval_Fill(conv_bv, i*8, BITVECT_NATIVE_SIZE-1);

    from_bv(conv_bv);
}

IntNum::IntNum(const IntNum& rhs)
{
    m_type = rhs.m_type;
    if (rhs.m_type == INTNUM_BV)
        m_val.bv = BitVector::Clone(rhs.m_val.bv);
    else
        m_val.l = rhs.m_val.l;
}

void
IntNum::swap(IntNum& oth)
{
    std::swap(static_cast<IntNumData&>(*this), static_cast<IntNumData&>(oth));
}

// Speedup function for non-bitvect calculations.
// Always makes conservative assumptions; we fall back to bitvect if this
// function returns false.
static bool
calc_long(Op::Op op, long* lhs, long rhs)
{
    switch (op)
    {
        case Op::ADD:
            if (*lhs >= LONG_MAX/2 || *lhs <= LONG_MIN/2 ||
                rhs >= LONG_MAX/2 || rhs <= LONG_MIN/2)
                return false;
            *lhs += rhs;
            break;
        case Op::SUB:
            if (*lhs >= LONG_MAX/2 || *lhs <= LONG_MIN/2 ||
                rhs >= LONG_MAX/2 || rhs <= LONG_MIN/2)
                return false;
            *lhs -= rhs;
            break;
        case Op::MUL:
            // half range
            if (*lhs > -(1L<<(LONG_BITS/2)) && *lhs < (1L<<(LONG_BITS/2)))
            {
                if (rhs <= -(1L<<(LONG_BITS/2)) || rhs >= (1L<<(LONG_BITS/2)))
                    return false;
                *lhs *= rhs;
                break;
            }
            // maybe someday?
            return false;
        case Op::DIV:
            // TODO: make sure lhs and rhs are unsigned
        case Op::SIGNDIV:
            if (rhs == 0)
                throw ZeroDivisionError(N_("divide by zero"));
            *lhs /= rhs;
            break;
        case Op::MOD:
            // TODO: make sure lhs and rhs are unsigned
        case Op::SIGNMOD:
            if (rhs == 0)
                throw ZeroDivisionError(N_("divide by zero"));
            *lhs %= rhs;
            break;
        case Op::NEG:
            *lhs = -(*lhs);
            break;
        case Op::NOT:
            *lhs = ~(*lhs);
            break;
        case Op::OR:
            *lhs |= rhs;
            break;
        case Op::AND:
            *lhs &= rhs;
            break;
        case Op::XOR:
            *lhs ^= rhs;
            break;
        case Op::XNOR:
            *lhs = ~(*lhs ^ rhs);
            break;
        case Op::NOR:
            *lhs = ~(*lhs | rhs);
            break;
        case Op::SHL:
            // maybe someday?
            return false;
        case Op::SHR:
            *lhs >>= rhs;
            break;
        case Op::LOR:
            *lhs = (*lhs || rhs);
            break;
        case Op::LAND:
            *lhs = (*lhs && rhs);
            break;
        case Op::LNOT:
            *lhs = !*lhs;
            break;
        case Op::LXOR:
            *lhs = (!!(*lhs) ^ !!rhs);
            break;
        case Op::LXNOR:
            *lhs = !(!!(*lhs) ^ !!rhs);
            break;
        case Op::LNOR:
            *lhs = !(*lhs || rhs);
            break;
        case Op::EQ:
            *lhs = (*lhs == rhs);
            break;
        case Op::LT:
            *lhs = (*lhs < rhs);
            break;
        case Op::GT:
            *lhs = (*lhs > rhs);
            break;
        case Op::LE:
            *lhs = (*lhs <= rhs);
            break;
        case Op::GE:
            *lhs = (*lhs >= rhs);
            break;
        case Op::NE:
            *lhs = (*lhs != rhs);
            break;
        case Op::IDENT:
            break;
        default:
            return false;
    }
    return true;
}

/*@-nullderef -nullpass -branchstate@*/
void
IntNum::calc(Op::Op op, const IntNum* operand)
{
    if (!operand && op != Op::NEG && op != Op::NOT && op != Op::LNOT)
        throw ArithmeticError(N_("operation needs an operand"));

    if (m_type == INTNUM_L && (!operand || operand->m_type == INTNUM_L))
    {
        if (calc_long(op, &m_val.l, operand ? operand->m_val.l : 0))
            return;
    }

    // Always do computations with in full bit vector.
    // Bit vector results must be calculated through intermediate storage.
    wordptr op1 = to_bv(op1static);
    wordptr op2 = 0;
    if (operand)
        op2 = operand->to_bv(op2static);

    // A operation does a bitvector computation if result is allocated.
    switch (op)
    {
        case Op::ADD:
        {
            bool carry = false;
            BitVector::add(result, op1, op2, &carry);
            break;
        }
        case Op::SUB:
        {
            bool carry = false;
            BitVector::sub(result, op1, op2, &carry);
            break;
        }
        case Op::MUL:
            BitVector::Multiply(result, op1, op2);
            break;
        case Op::DIV:
            // TODO: make sure op1 and op2 are unsigned
            if (BitVector::is_empty(op2))
                throw ZeroDivisionError(N_("divide by zero"));
            else
                BitVector::Divide(result, op1, op2, spare);
            break;
        case Op::SIGNDIV:
            if (BitVector::is_empty(op2))
                throw ZeroDivisionError(N_("divide by zero"));
            else
                BitVector::Divide(result, op1, op2, spare);
            break;
        case Op::MOD:
            // TODO: make sure op1 and op2 are unsigned
            if (BitVector::is_empty(op2))
                throw ZeroDivisionError(N_("divide by zero"));
            else
                BitVector::Divide(spare, op1, op2, result);
            break;
        case Op::SIGNMOD:
            if (BitVector::is_empty(op2))
                throw ZeroDivisionError(N_("divide by zero"));
            else
                BitVector::Divide(spare, op1, op2, result);
            break;
        case Op::NEG:
            BitVector::Negate(result, op1);
            break;
        case Op::NOT:
            BitVector::Set_Complement(result, op1);
            break;
        case Op::OR:
            BitVector::Set_Union(result, op1, op2);
            break;
        case Op::AND:
            BitVector::Set_Intersection(result, op1, op2);
            break;
        case Op::XOR:
            BitVector::Set_ExclusiveOr(result, op1, op2);
            break;
        case Op::XNOR:
            BitVector::Set_ExclusiveOr(result, op1, op2);
            BitVector::Set_Complement(result, result);
            break;
        case Op::NOR:
            BitVector::Set_Union(result, op1, op2);
            BitVector::Set_Complement(result, result);
            break;
        case Op::SHL:
            if (operand->m_type == INTNUM_L && operand->m_val.l >= 0)
            {
                BitVector::Copy(result, op1);
                BitVector::Move_Left(result,
                                     static_cast<N_int>(operand->m_val.l));
            }
            else    // don't even bother, just zero result
                BitVector::Empty(result);
            break;
        case Op::SHR:
            if (operand->m_type == INTNUM_L && operand->m_val.l >= 0)
            {
                BitVector::Copy(result, op1);
                bool carry = BitVector::msb_(op1);
                N_int count = static_cast<N_int>(operand->m_val.l);
                while (count-- > 0)
                    BitVector::shift_right(result, carry);
            }
            else    // don't even bother, just zero result
                BitVector::Empty(result);
            break;
        case Op::LOR:
            BitVector::Empty(result);
            BitVector::LSB(result, !BitVector::is_empty(op1) ||
                           !BitVector::is_empty(op2));
            break;
        case Op::LAND:
            BitVector::Empty(result);
            BitVector::LSB(result, !BitVector::is_empty(op1) &&
                           !BitVector::is_empty(op2));
            break;
        case Op::LNOT:
            BitVector::Empty(result);
            BitVector::LSB(result, BitVector::is_empty(op1));
            break;
        case Op::LXOR:
            BitVector::Empty(result);
            BitVector::LSB(result, !BitVector::is_empty(op1) ^
                           !BitVector::is_empty(op2));
            break;
        case Op::LXNOR:
            BitVector::Empty(result);
            BitVector::LSB(result, !(!BitVector::is_empty(op1) ^
                           !BitVector::is_empty(op2)));
            break;
        case Op::LNOR:
            BitVector::Empty(result);
            BitVector::LSB(result, !(!BitVector::is_empty(op1) ||
                           !BitVector::is_empty(op2)));
            break;
        case Op::EQ:
            BitVector::Empty(result);
            BitVector::LSB(result, BitVector::equal(op1, op2));
            break;
        case Op::LT:
            BitVector::Empty(result);
            BitVector::LSB(result, BitVector::Compare(op1, op2) < 0);
            break;
        case Op::GT:
            BitVector::Empty(result);
            BitVector::LSB(result, BitVector::Compare(op1, op2) > 0);
            break;
        case Op::LE:
            BitVector::Empty(result);
            BitVector::LSB(result, BitVector::Compare(op1, op2) <= 0);
            break;
        case Op::GE:
            BitVector::Empty(result);
            BitVector::LSB(result, BitVector::Compare(op1, op2) >= 0);
            break;
        case Op::NE:
            BitVector::Empty(result);
            BitVector::LSB(result, !BitVector::equal(op1, op2));
            break;
        case Op::SEG:
            throw ArithmeticError(String::compose(N_("invalid use of '%1'"),
                                                  "SEG"));
            break;
        case Op::WRT:
            throw ArithmeticError(String::compose(N_("invalid use of '%1'"),
                                                  "WRT"));
            break;
        case Op::SEGOFF:
            throw ArithmeticError(String::compose(N_("invalid use of '%1'"),
                                                  ":"));
            break;
        case Op::IDENT:
            if (result)
                BitVector::Copy(result, op1);
            break;
        default:
            throw ArithmeticError(
                N_("invalid operation in intnum calculation"));
    }

    // Try to fit the result into long if possible
    if (m_type == INTNUM_BV)
        BitVector::Destroy(m_val.bv);
    from_bv(result);
}
/*@=nullderef =nullpass =branchstate@*/

void
IntNum::set(unsigned long val)
{
    if (val > LONG_MAX)
    {
        if (m_type != INTNUM_BV)
        {
            m_val.bv = BitVector::Create(BITVECT_NATIVE_SIZE, true);
            m_type = INTNUM_BV;
        }
        BitVector::Chunk_Store(m_val.bv, ULONG_BITS, 0, val);
    }
    else
    {
        if (m_type == INTNUM_BV)
        {
            BitVector::Destroy(m_val.bv);
            m_type = INTNUM_L;
        }
        m_val.l = static_cast<long>(val);
    }
}

int
IntNum::sign() const
{
    if (m_type == INTNUM_L)
    {
        if (m_val.l == 0)
            return 0;
        else if (m_val.l < 0)
            return -1;
        else
            return 1;
    }
    else
        return BitVector::Sign(m_val.bv);
}

unsigned long
IntNum::get_uint() const
{
    if (m_type == INTNUM_L)
    {
        if (m_val.l < 0)
            return 0;
        return static_cast<unsigned long>(m_val.l);
    }

    // Handle bitvect
    if (BitVector::msb_(m_val.bv))
        return 0;
    if (BitVector::Set_Max(m_val.bv) > ULONG_BITS)
        return ULONG_MAX;
    return BitVector::Chunk_Read(m_val.bv, ULONG_BITS, 0);
}

long
IntNum::get_int() const
{
    if (m_type == INTNUM_L)
        return m_val.l;

    // Handle bitvect
    if (BitVector::msb_(m_val.bv))
    {
        // it's negative: negate the bitvector to get a positive
        // number, then negate the positive number.
        unsigned long ul;

        BitVector::Negate(conv_bv, m_val.bv);
        if (BitVector::Set_Max(conv_bv) >= LONG_BITS)
        {
            // too negative
            return LONG_MIN;
        }
        ul = BitVector::Chunk_Read(conv_bv, LONG_BITS, 0);
        // check for too negative
        return -static_cast<long>(ul);
    }

    // it's positive, and since it's a BV, it must be >0x7FFFFFFF
    return LONG_MAX;
}

void
IntNum::get_sized(unsigned char* ptr,
                  unsigned int destsize,
                  unsigned int valsize,
                  int shift,
                  bool bigendian,
                  int warn) const
{
    // Currently don't support destinations larger than our native size
    assert(destsize*8 <= BITVECT_NATIVE_SIZE && "destination too large");

    // Split shift into left (shift) and right (rshift) components.
    unsigned int rshift = 0;
    if (shift < 0)
    {
        rshift = static_cast<unsigned int>(-shift);
        shift = 0;
    }

    // General size warnings
    if (warn<0 && !ok_size(valsize, rshift, 1))
        warn_set(WARN_GENERAL,
                 String::compose(N_("value does not fit in signed %1 bit field"),
                                valsize));
    if (warn>0 && !ok_size(valsize, rshift, 2))
        warn_set(WARN_GENERAL,
                 String::compose(N_("value does not fit in %1 bit field"),
                                 valsize));

    // Non-bitvect (for speed)
    if (m_type == INTNUM_L)
    {
        long v = m_val.l;

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
        for (int i = 0, sz = valsize;
             i < static_cast<int>(destsize) && sz > 0; ++i)
        {
            // handle left shift past whole bytes
            if (shift >= 8)
            {
                shift -= 8;
                continue;
            }

            // Handle first chunk specially for left shifted values
            if (shift > 0 && sz == static_cast<int>(valsize))
            {
                unsigned char chunk =
                    ((static_cast<unsigned char>(v) & 0xff) << shift) & 0xff;
                unsigned char mask = ~((1U<<shift)-1);  // keep MSBs

                // write appropriate bits
                ptr[i] &= ~mask;
                ptr[i] |= chunk & mask;

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
                ptr[i] &= ~mask;
                ptr[i] |= chunk & mask;

                v >>= 8;
                sz -= 8;
            }
        }

        return;
    }

    // Read the original data into a bitvect
    wordptr op1 = op1static;

    if (bigendian)
    {
        // TODO
        assert(false && "big endian not implemented");
    }
    else
        BitVector::Block_Store(op1, ptr, static_cast<N_int>(destsize));

    // If not already a bitvect, convert value to be written to a bitvect
    wordptr op2 = to_bv(op2static);

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
                             static_cast<N_int>(valsize));

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
}

bool
IntNum::ok_size(unsigned int size, unsigned int rshift, int rangetype) const
{
    // Non-bitvect (for speed)
    if (m_type == INTNUM_L)
    {
        long v = m_val.l;
        v >>= rshift;
        switch (rangetype)
        {
            case 0:
                if (v < 0)
                    return false;
                if (size >= LONG_BITS)
                    return true;
                return (v < (1L<<size));
            case 1:
                if (v < 0)
                {
                    if (size >= LONG_BITS+1)
                        return true;
                    v = 0-v;
                    return (v <= (1L<<(size-1)));
                }
                if (size >= LONG_BITS+1)
                    return true;
                return (v < (1L<<(size-1)));
            case 2:
                if (v < 0)
                {
                    if (size >= LONG_BITS+1)
                        return true;
                    v = 0-v;
                    return (v <= (1L<<(size-1)));
                }
                if (size >= LONG_BITS)
                    return true;
                return (v < (1L<<size));
            default:
                assert(false);
        }
    }

    // Handle bitvect
    wordptr val;
    if (rshift > 0)
    {
        val = conv_bv;
        BitVector::Copy(val, m_val.bv);
    }
    else
        val = m_val.bv;

    if (size >= BITVECT_NATIVE_SIZE)
        return 1;

    if (rshift > 0)
    {
        bool carry_in = BitVector::msb_(val);
        while (rshift-- > 0)
            BitVector::shift_right(val, carry_in);
    }

    if (rangetype > 0)
    {
        if (BitVector::msb_(val))
        {
            // it's negative
            BitVector::Negate(conv_bv, val);
            BitVector::dec(conv_bv, conv_bv);
            return (BitVector::Set_Max(conv_bv) < static_cast<long>(size)-1);
        }

        if (rangetype == 1)
            size--;
    }
    return (BitVector::Set_Max(val) < static_cast<long>(size));
}

bool
IntNum::in_range(long low, long high) const
{
    // If not already a bitvect, convert value to be written to a bitvect
    wordptr val = to_bv(result);

    // Convert high and low to bitvects
    wordptr lval = op1static;
    BitVector::Empty(lval);
    if (low >= 0)
        BitVector::Chunk_Store(lval, LONG_BITS, 0,
                               static_cast<unsigned long>(low));
    else
    {
        BitVector::Chunk_Store(lval, LONG_BITS, 0,
                               static_cast<unsigned long>(-low));
        BitVector::Negate(lval, lval);
    }

    wordptr hval = op2static;
    BitVector::Empty(hval);
    if (high >= 0)
        BitVector::Chunk_Store(hval, LONG_BITS, 0,
                               static_cast<unsigned long>(high));
    else
    {
        BitVector::Chunk_Store(hval, LONG_BITS, 0,
                               static_cast<unsigned long>(-high));
        BitVector::Negate(hval, hval);
    }

    // Compare!
    return (BitVector::Compare(val, lval) >= 0
            && BitVector::Compare(val, hval) <= 0);
}

IntNum&
IntNum::operator++()
{
    if (m_type == INTNUM_L && m_val.l < LONG_MAX)
        ++m_val.l;
    else
    {
        if (m_type == INTNUM_L)
        {
            m_val.bv = to_bv(BitVector::Create(BITVECT_NATIVE_SIZE, false));
            m_type = INTNUM_BV;
        }
        BitVector::increment(m_val.bv);
    }
    return *this;
}

IntNum&
IntNum::operator--()
{
    if (m_type == INTNUM_L && m_val.l > LONG_MIN)
        --m_val.l;
    else
    {
        if (m_type == INTNUM_L)
        {
            m_val.bv = to_bv(BitVector::Create(BITVECT_NATIVE_SIZE, false));
            m_type = INTNUM_BV;
        }
        BitVector::decrement(m_val.bv);
    }
    return *this;
}

int
compare(const IntNum& lhs, const IntNum& rhs)
{
    if (lhs.m_type == IntNum::INTNUM_L && rhs.m_type == IntNum::INTNUM_L)
    {
        if (lhs.m_val.l < rhs.m_val.l)
            return -1;
        if (lhs.m_val.l > rhs.m_val.l)
            return 1;
        return 0;
    }

    wordptr op1 = lhs.to_bv(op1static);
    wordptr op2 = rhs.to_bv(op2static);
    return BitVector::Compare(op1, op2);
}

bool
operator==(const IntNum& lhs, const IntNum& rhs)
{
    if (lhs.m_type == IntNum::INTNUM_L && rhs.m_type == IntNum::INTNUM_L)
        return lhs.m_val.l == rhs.m_val.l;

    wordptr op1 = lhs.to_bv(op1static);
    wordptr op2 = rhs.to_bv(op2static);
    return BitVector::equal(op1, op2);
}

bool
operator<(const IntNum& lhs, const IntNum& rhs)
{
    if (lhs.m_type == IntNum::INTNUM_L && rhs.m_type == IntNum::INTNUM_L)
        return lhs.m_val.l < rhs.m_val.l;

    wordptr op1 = lhs.to_bv(op1static);
    wordptr op2 = rhs.to_bv(op2static);
    return BitVector::Compare(op1, op2) < 0;
}

bool
operator>(const IntNum& lhs, const IntNum& rhs)
{
    if (lhs.m_type == IntNum::INTNUM_L && rhs.m_type == IntNum::INTNUM_L)
        return lhs.m_val.l > rhs.m_val.l;

    wordptr op1 = lhs.to_bv(op1static);
    wordptr op2 = rhs.to_bv(op2static);
    return BitVector::Compare(op1, op2) > 0;
}

char*
IntNum::get_str() const
{
    if (m_type == INTNUM_BV)
        return reinterpret_cast<char*>(BitVector::to_Dec(m_val.bv));

    char* s = static_cast<char*>(malloc(16));
    sprintf(s, "%ld", m_val.l);
    return s;
}

std::ostream&
operator<< (std::ostream& os, const IntNum& intn)
{
    wordptr bv = intn.to_bv(conv_bv);

    BitVector::N_word bits =
        static_cast<BitVector::N_word>(os.iword(set_intnum_bits::m_idx));

    std::ios_base::fmtflags ff = os.flags();
    if ((ff & os.showpos) && (ff & os.dec) && BitVector::Sign(bv) >= 0)
        os << '+';

    unsigned char* s;
    if (ff & os.oct)
    {
        if (ff & os.showbase)
            os << '0';
        s = BitVector::to_Oct(bv, bits);
    }
    else if (ff & os.hex)
    {
        if (ff & os.showbase)
        {
            if (ff & os.uppercase)
                os << "0X";
            else
                os << "0x";
        }
        s = BitVector::to_Hex(bv, (ff & os.uppercase) != 0, bits);
    }
    else
        s = BitVector::to_Dec(bv);
    os << s;
    free(s);
    return os;
}

} // namespace yasm

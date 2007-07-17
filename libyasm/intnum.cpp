/*
 * Integer number functions.
 *
 *  Copyright (C) 2001-2007  Peter Johnson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "util.h"

#include <cctype>
#include <limits.h>

#include "bitvect.h"

#include "errwarn.h"
#include "intnum.h"

using BitVector::wordptr;
using BitVector::N_int;

namespace yasm {

/* "Native" "word" size for intnum calculations. */
static const unsigned int BITVECT_NATIVE_SIZE = 128;

class IntNumManager {
public:
    static IntNumManager & instance()
    {
        static IntNumManager inst;
        return inst;
    }

    /* static bitvect used for conversions */
    /*@only@*/ wordptr conv_bv;

    /* static bitvects used for computation */
    /*@only@*/ wordptr result, spare, op1static, op2static;

    /*@only@*/ BitVector::from_Dec_static_data *from_dec_data;

private:
    IntNumManager();
    IntNumManager(const IntNumManager &) {}
    IntNumManager & operator= (const IntNumManager &) { return *this; }
    ~IntNumManager();
};

IntNumManager::IntNumManager()
{
    BitVector::Boot();
    conv_bv = BitVector::Create(BITVECT_NATIVE_SIZE, false);
    result = BitVector::Create(BITVECT_NATIVE_SIZE, false);
    spare = BitVector::Create(BITVECT_NATIVE_SIZE, false);
    op1static = BitVector::Create(BITVECT_NATIVE_SIZE, false);
    op2static = BitVector::Create(BITVECT_NATIVE_SIZE, false);
    from_dec_data = BitVector::from_Dec_static_Boot(BITVECT_NATIVE_SIZE);
}

IntNumManager::~IntNumManager()
{
    BitVector::from_Dec_static_Shutdown(from_dec_data);
    BitVector::Destroy(op2static);
    BitVector::Destroy(op1static);
    BitVector::Destroy(spare);
    BitVector::Destroy(result);
    BitVector::Destroy(conv_bv);
    BitVector::Shutdown();
}

IntNum::IntNum(char *str, int base)
{
    IntNumManager &manager = IntNumManager::instance();
    wordptr conv_bv = manager.conv_bv;
    BitVector::ErrCode err;
    const char *errstr;

    switch (base) {
        case 2:
            err = BitVector::from_Bin(conv_bv, (unsigned char *)str);
            errstr = N_("invalid binary literal");
            break;
        case 8:
            err = BitVector::from_Oct(conv_bv, (unsigned char *)str);
            errstr = N_("invalid octal literal");
            break;
        case 10:
            err = BitVector::from_Dec_static(manager.from_dec_data, conv_bv,
                                            (unsigned char *)str);
            errstr = N_("invalid decimal literal");
            break;
        case 16:
            err = BitVector::from_Hex(conv_bv, (unsigned char *)str);
            errstr = N_("invalid hex literal");
            break;
        default:
            error_set(ERROR_VALUE, N_("invalid base"));
            m_type = INTNUM_UL;
            m_val.ul = 0;
            return;
    }
    
    switch (err) {
        case BitVector::ErrCode_Pars:
            error_set(ERROR_VALUE, errstr);
            break;
        case BitVector::ErrCode_Ovfl:
            error_set(ERROR_OVERFLOW,
                      N_("Numeric constant too large for internal format"));
            break;
        default:
            break;
    }
    if (BitVector::Set_Max(conv_bv) < 32) {
        m_type = INTNUM_UL;
        m_val.ul = BitVector::Chunk_Read(conv_bv, 32, 0);
    } else {
        m_type = INTNUM_BV;
        m_val.bv = BitVector::Clone(conv_bv);
    }
}
#if 0
/*@-usedef -compdef -uniondef@*/
yasm_intnum *
yasm_intnum_create_charconst_nasm(const char *str)
{
    yasm_intnum *intn = yasm_xmalloc(sizeof(yasm_intnum));
    size_t len = strlen(str);

    if(len*8 > BITVECT_NATIVE_SIZE)
        yasm_error_set(YASM_ERROR_OVERFLOW,
                       N_("Character constant too large for internal format"));

    if (len > 4) {
        BitVector::Empty(conv_bv);
        m_type = INTNUM_BV;
    } else {
        m_val.ul = 0;
        m_type = INTNUM_UL;
    }

    switch (len) {
        case 4:
            m_val.ul |= ((unsigned long)str[3]) & 0xff;
            m_val.ul <<= 8;
            /*@fallthrough@*/
        case 3:
            m_val.ul |= ((unsigned long)str[2]) & 0xff;
            m_val.ul <<= 8;
            /*@fallthrough@*/
        case 2:
            m_val.ul |= ((unsigned long)str[1]) & 0xff;
            m_val.ul <<= 8;
            /*@fallthrough@*/
        case 1:
            m_val.ul |= ((unsigned long)str[0]) & 0xff;
        case 0:
            break;
        default:
            /* >32 bit conversion */
            while (len) {
                BitVector::Move_Left(conv_bv, 8);
                BitVector::Chunk_Store(conv_bv, 8, 0,
                                      (unsigned long)str[--len]);
            }
            m_val.bv = BitVector::Clone(conv_bv);
    }

    return intn;
}
/*@=usedef =compdef =uniondef@*/
#endif

IntNum::IntNum(long i)
{
    /* positive numbers can go in as uint */
    if (i >= 0) {
        m_type = INTNUM_UL;
        m_val.ul = (unsigned long)i;
        return;
    }

    IntNumManager &manager = IntNumManager::instance();
    wordptr conv_bv = manager.conv_bv;

    BitVector::Empty(conv_bv);
    BitVector::Chunk_Store(conv_bv, 32, 0, (unsigned long)(-i));
    BitVector::Negate(conv_bv, conv_bv);

    m_type = INTNUM_BV;
    m_val.bv = BitVector::Clone(conv_bv);
}

IntNum::IntNum(int i)
{
    /* positive numbers can go in as uint */
    if (i >= 0) {
        m_type = INTNUM_UL;
        m_val.ul = (unsigned long)i;
        return;
    }

    IntNumManager &manager = IntNumManager::instance();
    wordptr conv_bv = manager.conv_bv;

    BitVector::Empty(conv_bv);
    BitVector::Chunk_Store(conv_bv, 32, 0, (unsigned long)(-i));
    BitVector::Negate(conv_bv, conv_bv);

    m_type = INTNUM_BV;
    m_val.bv = BitVector::Clone(conv_bv);
}

IntNum::IntNum(const unsigned char *ptr, bool sign, unsigned long &size)
{
    IntNumManager &manager = IntNumManager::instance();
    wordptr conv_bv = manager.conv_bv;
    const unsigned char *ptr_orig = ptr;
    unsigned long i = 0;

    BitVector::Empty(conv_bv);
    for (;;) {
        BitVector::Chunk_Store(conv_bv, 7, i, *ptr);
        i += 7;
        if ((*ptr & 0x80) != 0x80)
            break;
        ptr++;
    }

    size = (unsigned long)(ptr-ptr_orig)+1;

    if (i > BITVECT_NATIVE_SIZE)
        error_set(ERROR_OVERFLOW,
                  N_("Numeric constant too large for internal format"));
    else if (sign && (*ptr & 0x40) == 0x40)
        BitVector::Interval_Fill(conv_bv, i, BITVECT_NATIVE_SIZE-1);

    if (BitVector::Set_Max(conv_bv) < 32) {
        m_type = INTNUM_UL;
        m_val.ul = BitVector::Chunk_Read(conv_bv, 32, 0);
    } else {
        m_type = INTNUM_BV;
        m_val.bv = BitVector::Clone(conv_bv);
    }
}

IntNum::IntNum(const unsigned char *ptr, bool sign, size_t srcsize, bool bigendian)
{
    IntNumManager &manager = IntNumManager::instance();
    wordptr conv_bv = manager.conv_bv;
    unsigned long i = 0;

    if (srcsize*8 > BITVECT_NATIVE_SIZE)
        error_set(ERROR_OVERFLOW,
                  N_("Numeric constant too large for internal format"));

    /* Read the buffer into a bitvect */
    BitVector::Empty(conv_bv);
    if (bigendian) {
        /* TODO */
        internal_error(N_("big endian not implemented"));
    } else {
        for (i = 0; i < srcsize; i++)
            BitVector::Chunk_Store(conv_bv, 8, i*8, ptr[i]);
    }

    /* Sign extend if needed */
    if (srcsize*8 < BITVECT_NATIVE_SIZE && sign && (ptr[i] & 0x80) == 0x80)
        BitVector::Interval_Fill(conv_bv, i*8, BITVECT_NATIVE_SIZE-1);

    if (BitVector::Set_Max(conv_bv) < 32) {
        m_type = INTNUM_UL;
        m_val.ul = BitVector::Chunk_Read(conv_bv, 32, 0);
    } else {
        m_type = INTNUM_BV;
        m_val.bv = BitVector::Clone(conv_bv);
    }
}

IntNum::IntNum(const IntNum &rhs)
    : m_type (rhs.m_type)
{
    switch (rhs.m_type) {
        case INTNUM_UL:
            m_val.ul = rhs.m_val.ul;
            break;
        case INTNUM_BV:
            m_val.bv = BitVector::Clone(rhs.m_val.bv);
            break;
    }
}

/*@-nullderef -nullpass -branchstate@*/
bool
IntNum::calc(Expr::Op op, const IntNum *operand)
{
    IntNumManager &manager = IntNumManager::instance();
    wordptr result = manager.result;
    wordptr spare = manager.spare;
    bool carry = false;
    wordptr op1, op2 = NULL;
    N_int count;

    /* Always do computations with in full bit vector.
     * Bit vector results must be calculated through intermediate storage.
     */
    if (m_type == INTNUM_BV)
        op1 = m_val.bv;
    else {
        op1 = manager.op1static;
        BitVector::Empty(op1);
        BitVector::Chunk_Store(op1, 32, 0, m_val.ul);
    }

    if (operand) {
        if (operand->m_type == INTNUM_BV)
            op2 = operand->m_val.bv;
        else {
            op2 = manager.op2static;
            BitVector::Empty(op2);
            BitVector::Chunk_Store(op2, 32, 0, operand->m_val.ul);
        }
    }

    if (!operand && op != Expr::NEG && op != Expr::NOT && op != Expr::LNOT) {
        error_set(ERROR_ARITHMETIC, N_("operation needs an operand"));
        BitVector::Empty(result);
        return true;
    }

    /* A operation does a bitvector computation if result is allocated. */
    switch (op) {
        case Expr::ADD:
            BitVector::add(result, op1, op2, &carry);
            break;
        case Expr::SUB:
            BitVector::sub(result, op1, op2, &carry);
            break;
        case Expr::MUL:
            BitVector::Multiply(result, op1, op2);
            break;
        case Expr::DIV:
            /* TODO: make sure op1 and op2 are unsigned */
            if (BitVector::is_empty(op2)) {
                error_set(ERROR_ZERO_DIVISION, N_("divide by zero"));
                BitVector::Empty(result);
                return true;
            } else
                BitVector::Divide(result, op1, op2, spare);
            break;
        case Expr::SIGNDIV:
            if (BitVector::is_empty(op2)) {
                error_set(ERROR_ZERO_DIVISION, N_("divide by zero"));
                BitVector::Empty(result);
                return true;
            } else
                BitVector::Divide(result, op1, op2, spare);
            break;
        case Expr::MOD:
            /* TODO: make sure op1 and op2 are unsigned */
            if (BitVector::is_empty(op2)) {
                error_set(ERROR_ZERO_DIVISION, N_("divide by zero"));
                BitVector::Empty(result);
                return true;
            } else
                BitVector::Divide(spare, op1, op2, result);
            break;
        case Expr::SIGNMOD:
            if (BitVector::is_empty(op2)) {
                error_set(ERROR_ZERO_DIVISION, N_("divide by zero"));
                BitVector::Empty(result);
                return true;
            } else
                BitVector::Divide(spare, op1, op2, result);
            break;
        case Expr::NEG:
            BitVector::Negate(result, op1);
            break;
        case Expr::NOT:
            BitVector::Set_Complement(result, op1);
            break;
        case Expr::OR:
            BitVector::Set_Union(result, op1, op2);
            break;
        case Expr::AND:
            BitVector::Set_Intersection(result, op1, op2);
            break;
        case Expr::XOR:
            BitVector::Set_ExclusiveOr(result, op1, op2);
            break;
        case Expr::XNOR:
            BitVector::Set_ExclusiveOr(result, op1, op2);
            BitVector::Set_Complement(result, result);
            break;
        case Expr::NOR:
            BitVector::Set_Union(result, op1, op2);
            BitVector::Set_Complement(result, result);
            break;
        case Expr::SHL:
            if (operand->m_type == INTNUM_UL) {
                BitVector::Copy(result, op1);
                BitVector::Move_Left(result, (N_int)operand->m_val.ul);
            } else      /* don't even bother, just zero result */
                BitVector::Empty(result);
            break;
        case Expr::SHR:
            if (operand->m_type == INTNUM_UL) {
                BitVector::Copy(result, op1);
                carry = BitVector::msb_(op1);
                count = (N_int)operand->m_val.ul;
                while (count-- > 0)
                    BitVector::shift_right(result, carry);
            } else      /* don't even bother, just zero result */
                BitVector::Empty(result);
            break;
        case Expr::LOR:
            BitVector::Empty(result);
            BitVector::LSB(result, !BitVector::is_empty(op1) ||
                           !BitVector::is_empty(op2));
            break;
        case Expr::LAND:
            BitVector::Empty(result);
            BitVector::LSB(result, !BitVector::is_empty(op1) &&
                           !BitVector::is_empty(op2));
            break;
        case Expr::LNOT:
            BitVector::Empty(result);
            BitVector::LSB(result, BitVector::is_empty(op1));
            break;
        case Expr::LXOR:
            BitVector::Empty(result);
            BitVector::LSB(result, !BitVector::is_empty(op1) ^
                           !BitVector::is_empty(op2));
            break;
        case Expr::LXNOR:
            BitVector::Empty(result);
            BitVector::LSB(result, !(!BitVector::is_empty(op1) ^
                           !BitVector::is_empty(op2)));
            break;
        case Expr::LNOR:
            BitVector::Empty(result);
            BitVector::LSB(result, !(!BitVector::is_empty(op1) ||
                           !BitVector::is_empty(op2)));
            break;
        case Expr::EQ:
            BitVector::Empty(result);
            BitVector::LSB(result, BitVector::equal(op1, op2));
            break;
        case Expr::LT:
            BitVector::Empty(result);
            BitVector::LSB(result, BitVector::Compare(op1, op2) < 0);
            break;
        case Expr::GT:
            BitVector::Empty(result);
            BitVector::LSB(result, BitVector::Compare(op1, op2) > 0);
            break;
        case Expr::LE:
            BitVector::Empty(result);
            BitVector::LSB(result, BitVector::Compare(op1, op2) <= 0);
            break;
        case Expr::GE:
            BitVector::Empty(result);
            BitVector::LSB(result, BitVector::Compare(op1, op2) >= 0);
            break;
        case Expr::NE:
            BitVector::Empty(result);
            BitVector::LSB(result, !BitVector::equal(op1, op2));
            break;
        case Expr::SEG:
            error_set(ERROR_ARITHMETIC, N_("invalid use of '%s'"), "SEG");
            break;
        case Expr::WRT:
            error_set(ERROR_ARITHMETIC, N_("invalid use of '%s'"), "WRT");
            break;
        case Expr::SEGOFF:
            error_set(ERROR_ARITHMETIC, N_("invalid use of '%s'"), ":");
            break;
        case Expr::IDENT:
            if (result)
                BitVector::Copy(result, op1);
            break;
        default:
            error_set(ERROR_ARITHMETIC, N_("invalid operation in intnum calculation"));
            BitVector::Empty(result);
            return true;
    }

    /* Try to fit the result into 32 bits if possible */
    if (BitVector::Set_Max(result) < 32) {
        if (m_type == INTNUM_BV) {
            BitVector::Destroy(m_val.bv);
            m_type = INTNUM_UL;
        }
        m_val.ul = BitVector::Chunk_Read(result, 32, 0);
    } else {
        if (m_type == INTNUM_BV) {
            BitVector::Copy(m_val.bv, result);
        } else {
            m_type = INTNUM_BV;
            m_val.bv = BitVector::Clone(result);
        }
    }
    return false;
}
/*@=nullderef =nullpass =branchstate@*/

void
IntNum::set(long val)
{
    /* positive numbers can go through the uint() function */
    if (val >= 0) {
        set((unsigned long)val);
        return;
    }

    IntNumManager &manager = IntNumManager::instance();
    wordptr conv_bv = manager.conv_bv;

    BitVector::Empty(conv_bv);
    BitVector::Chunk_Store(conv_bv, 32, 0, (unsigned long)(-val));
    BitVector::Negate(conv_bv, conv_bv);

    if (m_type == INTNUM_BV)
        BitVector::Copy(m_val.bv, conv_bv);
    else {
        m_val.bv = BitVector::Clone(conv_bv);
        m_type = INTNUM_BV;
    }
}

long
IntNum::get_int() const
{
    switch (m_type) {
        case INTNUM_UL:
            /* unsigned long values are always positive; max out if needed */
            return (m_val.ul & 0x80000000) ? LONG_MAX : (long)m_val.ul;
        case INTNUM_BV:
            if (BitVector::msb_(m_val.bv)) {
                /* it's negative: negate the bitvector to get a positive
                 * number, then negate the positive number.
                 */
                IntNumManager &manager = IntNumManager::instance();
                wordptr conv_bv = manager.conv_bv;
                unsigned long ul;

                BitVector::Negate(conv_bv, m_val.bv);
                if (BitVector::Set_Max(conv_bv) >= 32) {
                    /* too negative */
                    return LONG_MIN;
                }
                ul = BitVector::Chunk_Read(conv_bv, 32, 0);
                /* check for too negative */
                return (ul & 0x80000000) ? LONG_MIN : -((long)ul);
            }

            /* it's positive, and since it's a BV, it must be >0x7FFFFFFF */
            return LONG_MAX;
        default:
            internal_error(N_("unknown intnum type"));
            /*@notreached@*/
            return 0;
    }
}

void
IntNum::get_sized(unsigned char *ptr, size_t destsize, size_t valsize, int shift,
                  bool bigendian, int warn) const
{
    IntNumManager &manager = IntNumManager::instance();
    wordptr conv_bv = manager.conv_bv;
    wordptr op1 = manager.op1static, op2;
    unsigned char *buf;
    unsigned int len;
    size_t rshift = shift < 0 ? (size_t)(-shift) : 0;
    int carry_in;

    /* Currently don't support destinations larger than our native size */
    if (destsize*8 > BITVECT_NATIVE_SIZE)
        internal_error(N_("destination too large"));

    /* General size warnings */
    if (warn<0 && !ok_size(valsize, rshift, 1))
        warn_set(WARN_GENERAL, N_("value does not fit in signed %d bit field"),
                 valsize);
    if (warn>0 && !ok_size(valsize, rshift, 2))
        warn_set(WARN_GENERAL, N_("value does not fit in %d bit field"), valsize);

    /* Read the original data into a bitvect */
    if (bigendian) {
        /* TODO */
        internal_error(N_("big endian not implemented"));
    } else
        BitVector::Block_Store(op1, ptr, (N_int)destsize);

    /* If not already a bitvect, convert value to be written to a bitvect */
    if (m_type == INTNUM_BV)
        op2 = m_val.bv;
    else {
        op2 = manager.op2static;
        BitVector::Empty(op2);
        BitVector::Chunk_Store(op2, 32, 0, m_val.ul);
    }

    /* Check low bits if right shifting and warnings enabled */
    if (warn && rshift > 0) {
        BitVector::Copy(conv_bv, op2);
        BitVector::Move_Left(conv_bv, (N_int)(BITVECT_NATIVE_SIZE-rshift));
        if (!BitVector::is_empty(conv_bv))
            warn_set(WARN_GENERAL, N_("misaligned value, truncating to boundary"));
    }

    /* Shift right if needed */
    if (rshift > 0) {
        carry_in = BitVector::msb_(op2);
        while (rshift-- > 0)
            BitVector::shift_right(op2, carry_in);
        shift = 0;
    }

    /* Write the new value into the destination bitvect */
    BitVector::Interval_Copy(op1, op2, (unsigned int)shift, 0, (N_int)valsize);

    /* Write out the new data */
    buf = BitVector::Block_Read(op1, &len);
    if (bigendian) {
        /* TODO */
        internal_error(N_("big endian not implemented"));
    } else
        memcpy(ptr, buf, destsize);
    free(buf);
}

bool
IntNum::ok_size(size_t size, size_t rshift, int rangetype) const
{
    IntNumManager &manager = IntNumManager::instance();
    wordptr conv_bv = manager.conv_bv;
    wordptr val;

    /* If not already a bitvect, convert value to a bitvect */
    if (m_type == INTNUM_BV) {
        if (rshift > 0) {
            val = conv_bv;
            BitVector::Copy(val, m_val.bv);
        } else
            val = m_val.bv;
    } else {
        val = conv_bv;
        BitVector::Empty(val);
        BitVector::Chunk_Store(val, 32, 0, m_val.ul);
    }

    if (size >= BITVECT_NATIVE_SIZE)
        return 1;

    if (rshift > 0) {
        int carry_in = BitVector::msb_(val);
        while (rshift-- > 0)
            BitVector::shift_right(val, carry_in);
    }

    if (rangetype > 0) {
        if (BitVector::msb_(val)) {
            /* it's negative */
            int retval;

            BitVector::Negate(conv_bv, val);
            BitVector::dec(conv_bv, conv_bv);
            retval = BitVector::Set_Max(conv_bv) < (long)size-1;

            return retval;
        }
        
        if (rangetype == 1)
            size--;
    }
    return (BitVector::Set_Max(val) < (long)size);
}

bool
IntNum::in_range(long low, long high) const
{
    IntNumManager &manager = IntNumManager::instance();
    wordptr val = manager.result;
    wordptr lval = manager.op1static;
    wordptr hval = manager.op2static;

    /* If not already a bitvect, convert value to be written to a bitvect */
    if (m_type == INTNUM_BV)
        val = m_val.bv;
    else {
        BitVector::Empty(val);
        BitVector::Chunk_Store(val, 32, 0, m_val.ul);
    }

    /* Convert high and low to bitvects */
    BitVector::Empty(lval);
    if (low >= 0)
        BitVector::Chunk_Store(lval, 32, 0, (unsigned long)low);
    else {
        BitVector::Chunk_Store(lval, 32, 0, (unsigned long)(-low));
        BitVector::Negate(lval, lval);
    }

    BitVector::Empty(hval);
    if (high >= 0)
        BitVector::Chunk_Store(hval, 32, 0, (unsigned long)high);
    else {
        BitVector::Chunk_Store(hval, 32, 0, (unsigned long)(-high));
        BitVector::Negate(hval, hval);
    }

    /* Compare! */
    return (BitVector::Compare(val, lval) >= 0
            && BitVector::Compare(val, hval) <= 0);
}

static unsigned long
get_leb128(wordptr val, unsigned char *ptr, bool sign)
{
    unsigned long i, size;
    unsigned char *ptr_orig = ptr;

    if (sign) {
        /* Signed mode */
        if (BitVector::msb_(val)) {
            /* Negative */
            IntNumManager &manager = IntNumManager::instance();
            wordptr conv_bv = manager.conv_bv;
            BitVector::Negate(conv_bv, val);
            size = BitVector::Set_Max(conv_bv)+2;
        } else {
            /* Positive */
            size = BitVector::Set_Max(val)+2;
        }
    } else {
        /* Unsigned mode */
        size = BitVector::Set_Max(val)+1;
    }

    /* Positive/Unsigned write */
    for (i=0; i<size; i += 7) {
        *ptr = (unsigned char)BitVector::Chunk_Read(val, 7, i);
        *ptr |= 0x80;
        ptr++;
    }
    *(ptr-1) &= 0x7F;   /* Clear MSB of last byte */
    return (unsigned long)(ptr-ptr_orig);
}

static unsigned long
size_leb128(wordptr val, bool sign)
{
    if (sign) {
        /* Signed mode */
        if (BitVector::msb_(val)) {
            /* Negative */
            IntNumManager &manager = IntNumManager::instance();
            wordptr conv_bv = manager.conv_bv;
            BitVector::Negate(conv_bv, val);
            return (BitVector::Set_Max(conv_bv)+8)/7;
        } else {
            /* Positive */
            return (BitVector::Set_Max(val)+8)/7;
        }
    } else {
        /* Unsigned mode */
        return (BitVector::Set_Max(val)+7)/7;
    }
}

unsigned long
IntNum::get_leb128(unsigned char *ptr, bool sign) const
{
    IntNumManager &manager = IntNumManager::instance();
    wordptr val = manager.op1static;

    /* Shortcut 0 */
    if (m_type == INTNUM_UL && m_val.ul == 0) {
        *ptr = 0;
        return 1;
    }

    /* If not already a bitvect, convert value to be written to a bitvect */
    if (m_type == INTNUM_BV)
        val = m_val.bv;
    else {
        BitVector::Empty(val);
        BitVector::Chunk_Store(val, 32, 0, m_val.ul);
    }

    return ::yasm::get_leb128(val, ptr, sign);
}

unsigned long
IntNum::size_leb128(bool sign) const
{
    IntNumManager &manager = IntNumManager::instance();
    wordptr val = manager.op1static;

    /* Shortcut 0 */
    if (m_type == INTNUM_UL && m_val.ul == 0) {
        return 1;
    }

    /* If not already a bitvect, convert value to a bitvect */
    if (m_type == INTNUM_BV)
        val = m_val.bv;
    else {
        BitVector::Empty(val);
        BitVector::Chunk_Store(val, 32, 0, m_val.ul);
    }

    return ::yasm::size_leb128(val, sign);
}

unsigned long
get_sleb128(long v, unsigned char *ptr)
{
    IntNumManager &manager = IntNumManager::instance();
    wordptr val = manager.op1static;

    /* Shortcut 0 */
    if (v == 0) {
        *ptr = 0;
        return 1;
    }

    BitVector::Empty(val);
    if (v >= 0)
        BitVector::Chunk_Store(val, 32, 0, (unsigned long)v);
    else {
        BitVector::Chunk_Store(val, 32, 0, (unsigned long)(-v));
        BitVector::Negate(val, val);
    }
    return get_leb128(val, ptr, 1);
}

unsigned long
size_sleb128(long v)
{
    IntNumManager &manager = IntNumManager::instance();
    wordptr val = manager.op1static;

    if (v == 0)
        return 1;

    BitVector::Empty(val);
    if (v >= 0)
        BitVector::Chunk_Store(val, 32, 0, (unsigned long)v);
    else {
        BitVector::Chunk_Store(val, 32, 0, (unsigned long)(-v));
        BitVector::Negate(val, val);
    }
    return size_leb128(val, 1);
}

unsigned long
get_uleb128(unsigned long v, unsigned char *ptr)
{
    IntNumManager &manager = IntNumManager::instance();
    wordptr val = manager.op1static;

    /* Shortcut 0 */
    if (v == 0) {
        *ptr = 0;
        return 1;
    }

    BitVector::Empty(val);
    BitVector::Chunk_Store(val, 32, 0, v);
    return get_leb128(val, ptr, 0);
}

unsigned long
size_uleb128(unsigned long v)
{
    IntNumManager &manager = IntNumManager::instance();
    wordptr val = manager.op1static;

    if (v == 0)
        return 1;

    BitVector::Empty(val);
    BitVector::Chunk_Store(val, 32, 0, v);
    return size_leb128(val, 0);
}

char *
IntNum::get_str() const
{
    char *s;
    switch (m_type) {
        case INTNUM_UL:
            s = (char *)malloc(16);
            sprintf(s, "%lu", m_val.ul);
            return s;
            break;
        case INTNUM_BV:
            return (char *)BitVector::to_Dec(m_val.bv);
            break;
    }
    /*@notreached@*/
    return NULL;
}

std::ostream& operator<< (std::ostream &os, const IntNum &intn)
{
    switch (intn.m_type) {
        case IntNum::INTNUM_UL:
            os << intn.m_val.ul;
            break;
        case IntNum::INTNUM_BV:
        {
            unsigned char *s = BitVector::to_Dec(intn.m_val.bv);
            os << s;
            free(s);
            break;
        }
    }
    return os;
}

} // namespace yasm

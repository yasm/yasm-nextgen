//
// Floating point number functions.
//
//  Copyright (C) 2001-2007  Peter Johnson
//
//  Based on public-domain x86 assembly code by Randall Hyde (8/28/91).
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
#include "floatnum.h"

#include "util.h"

#include <cstring>
#include <iomanip>
#include <ostream>

#include "bitvect.h"
#include "errwarn.h"


using BitVector::charptr;
using BitVector::wordptr;
using BitVector::N_char;
using BitVector::N_int;
using BitVector::N_long;

namespace yasm
{

// constants describing parameters of internal floating point format
static const unsigned int MANT_BITS = 80;
static const unsigned int MANT_BYTES = 10;
static const unsigned int MANT_SIGDIGITS = 24;
static const unsigned short EXP_BIAS = 0x7FFF;
static const unsigned short EXP_INF = 0xFFFF;
static const unsigned short EXP_MAX = 0xFFFE;
static const unsigned short EXP_MIN = 1;
static const unsigned short EXP_ZERO = 0;

// Flag settings for flags field
static const unsigned int FLAG_ISZERO = 1<<0;

class FloatNumManager
{
private:
    /// "Source" for POT_Entry.
    struct POT_Entry_Source
    {
        unsigned char mantissa[MANT_BYTES]; ///< Little endian mantissa
        unsigned short exponent;            ///< Bias 32767 exponent
    };

public:
    static FloatNumManager& instance()
    {
        static FloatNumManager inst;
        return inst;
    }

    class POT_Entry
    {
    public:
        ~POT_Entry() { delete flt; }

        FloatNum* flt;
        int dec_exponent;
    };

    // Power of ten tables used by the floating point I/O routines.
    // The POT_Table? arrays are built from the POT_Table?_Source arrays at
    // runtime by POT_Table_Init().

    /// This table contains the powers of ten raised to negative powers of two:
    ///
    /// entry[12-n] = 10 ** (-2 ** n) for 0 <= n <= 12.
    /// entry[13] = 1.0
    POT_Entry* POT_TableN;

    /// This table contains the powers of ten raised to positive powers of two:
    ///
    /// entry[12-n] = 10 ** (2 ** n) for 0 <= n <= 12.
    /// entry[13] = 1.0
    /// entry[-1] = entry[0];
    ///
    /// There is a -1 entry since it is possible for the algorithm to back up
    /// before the table.  This -1 entry is created at runtime by duplicating
    /// the 0 entry.
    POT_Entry* POT_TableP;
private:
    FloatNumManager();
    FloatNumManager(const FloatNumManager &) {}
    FloatNumManager & operator= (const FloatNumManager &) { return *this; }
    ~FloatNumManager();

    static const POT_Entry_Source POT_TableN_Source[];
    static const POT_Entry_Source POT_TableP_Source[];
};

const FloatNumManager::POT_Entry_Source
FloatNumManager::POT_TableN_Source[] =
{
    {{0xe3,0x2d,0xde,0x9f,0xce,0xd2,0xc8,0x04,0xdd,0xa6},0x4ad8}, // 1e-4096
    {{0x25,0x49,0xe4,0x2d,0x36,0x34,0x4f,0x53,0xae,0xce},0x656b}, // 1e-2048
    {{0xa6,0x87,0xbd,0xc0,0x57,0xda,0xa5,0x82,0xa6,0xa2},0x72b5}, // 1e-1024
    {{0x33,0x71,0x1c,0xd2,0x23,0xdb,0x32,0xee,0x49,0x90},0x795a}, // 1e-512
    {{0x91,0xfa,0x39,0x19,0x7a,0x63,0x25,0x43,0x31,0xc0},0x7cac}, // 1e-256
    {{0x7d,0xac,0xa0,0xe4,0xbc,0x64,0x7c,0x46,0xd0,0xdd},0x7e55}, // 1e-128
    {{0x24,0x3f,0xa5,0xe9,0x39,0xa5,0x27,0xea,0x7f,0xa8},0x7f2a}, // 1e-64
    {{0xde,0x67,0xba,0x94,0x39,0x45,0xad,0x1e,0xb1,0xcf},0x7f94}, // 1e-32
    {{0x2f,0x4c,0x5b,0xe1,0x4d,0xc4,0xbe,0x94,0x95,0xe6},0x7fc9}, // 1e-16
    {{0xc2,0xfd,0xfc,0xce,0x61,0x84,0x11,0x77,0xcc,0xab},0x7fe4}, // 1e-8
    {{0xc3,0xd3,0x2b,0x65,0x19,0xe2,0x58,0x17,0xb7,0xd1},0x7ff1}, // 1e-4
    {{0x71,0x3d,0x0a,0xd7,0xa3,0x70,0x3d,0x0a,0xd7,0xa3},0x7ff8}, // 1e-2
    {{0xcd,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc},0x7ffb}, // 1e-1
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80},0x7fff}, // 1e-0
};

const FloatNumManager::POT_Entry_Source
FloatNumManager::POT_TableP_Source[] =
{
    {{0x4c,0xc9,0x9a,0x97,0x20,0x8a,0x02,0x52,0x60,0xc4},0xb525}, // 1e+4096
    {{0x4d,0xa7,0xe4,0x5d,0x3d,0xc5,0x5d,0x3b,0x8b,0x9e},0x9a92}, // 1e+2048
    {{0x0d,0x65,0x17,0x0c,0x75,0x81,0x86,0x75,0x76,0xc9},0x8d48}, // 1e+1024
    {{0x65,0xcc,0xc6,0x91,0x0e,0xa6,0xae,0xa0,0x19,0xe3},0x86a3}, // 1e+512
    {{0xbc,0xdd,0x8d,0xde,0xf9,0x9d,0xfb,0xeb,0x7e,0xaa},0x8351}, // 1e+256
    {{0x6f,0xc6,0xdf,0x8c,0xe9,0x80,0xc9,0x47,0xba,0x93},0x81a8}, // 1e+128
    {{0xbf,0x3c,0xd5,0xa6,0xcf,0xff,0x49,0x1f,0x78,0xc2},0x80d3}, // 1e+64
    {{0x20,0xf0,0x9d,0xb5,0x70,0x2b,0xa8,0xad,0xc5,0x9d},0x8069}, // 1e+32
    {{0x00,0x00,0x00,0x00,0x00,0x04,0xbf,0xc9,0x1b,0x8e},0x8034}, // 1e+16
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0xbc,0xbe},0x8019}, // 1e+8
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x9c},0x800c}, // 1e+4
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xc8},0x8005}, // 1e+2
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xa0},0x8002}, // 1e+1
    {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80},0x7fff}, // 1e+0
};


FloatNum::FloatNum(const unsigned char* mantissa, unsigned short exponent)
    : m_exponent(exponent), m_sign(0), m_flags(0)
{
    m_mantissa = BitVector::Create(MANT_BITS, false);
    BitVector::Block_Store(m_mantissa, const_cast<N_char*>(mantissa),
                           MANT_BYTES);
}

FloatNumManager::FloatNumManager()
{
    int dec_exp = 1;
    int i;

    // Allocate space for two POT tables
    POT_TableN = new POT_Entry[14];
    POT_TableP = new POT_Entry[15]; // note 1 extra for -1

    // Initialize entry[0..12]
    for (i=12; i>=0; i--)
    {
        POT_TableN[i].flt = new FloatNum(POT_TableN_Source[i].mantissa,
                                         POT_TableN_Source[i].exponent);
        POT_TableN[i].dec_exponent = 0-dec_exp;
        POT_TableP[i+1].flt = new FloatNum(POT_TableP_Source[i].mantissa,
                                           POT_TableP_Source[i].exponent);
        POT_TableP[i+1].dec_exponent = dec_exp;
        dec_exp *= 2;       // Update decimal exponent
    }

    // Initialize entry[13]
    POT_TableN[13].flt = new FloatNum(POT_TableN_Source[13].mantissa,
                                      POT_TableN_Source[13].exponent);
    POT_TableN[13].dec_exponent = 0;
    POT_TableP[14].flt = new FloatNum(POT_TableP_Source[13].mantissa,
                                      POT_TableP_Source[13].exponent);
    POT_TableP[14].dec_exponent = 0;

    // Initialize entry[-1] for POT_TableP
    POT_TableP[0].flt = new FloatNum(POT_TableP_Source[0].mantissa,
                                     POT_TableP_Source[0].exponent);
    POT_TableP[0].dec_exponent = 4096;

    // Offset POT_TableP so that [0] becomes [-1]
    POT_TableP++;
}

FloatNumManager::~FloatNumManager()
{
    // Un-offset POT_TableP
    POT_TableP--;

    delete[] POT_TableN;
    delete[] POT_TableP;
}

void
FloatNum::normalize()
{
    if (BitVector::is_empty(m_mantissa))
    {
        m_exponent = 0;
        return;
    }

    // Look for the highest set bit, shift to make it the MSB, and adjust
    // exponent.  Don't let exponent go negative.
    long norm_amt = (MANT_BITS-1)-BitVector::Set_Max(m_mantissa);
    if (norm_amt > static_cast<long>(m_exponent))
        norm_amt = static_cast<long>(m_exponent);
    BitVector::Move_Left(m_mantissa, static_cast<N_int>(norm_amt));
    m_exponent -= static_cast<unsigned short>(norm_amt);
}

void
FloatNum::mul(const FloatNum* op)
{
    // Compute the new sign
    m_sign ^= op->m_sign;

    // Check for multiply by 0
    if (BitVector::is_empty(m_mantissa) ||
        BitVector::is_empty(op->m_mantissa))
    {
        BitVector::Empty(m_mantissa);
        m_exponent = EXP_ZERO;
        return;
    }

    // Add exponents, checking for overflow/underflow.
    long expon;
    expon = (static_cast<int>(m_exponent)-EXP_BIAS) +
        (static_cast<int>(op->m_exponent)-EXP_BIAS);
    expon += EXP_BIAS;
    if (expon > EXP_MAX)
    {
        // Overflow; return infinity.
        BitVector::Empty(m_mantissa);
        m_exponent = EXP_INF;
        return;
    }
    else if (expon < EXP_MIN)
    {
        // Underflow; return zero.
        BitVector::Empty(m_mantissa);
        m_exponent = EXP_ZERO;
        return;
    }

    // Add one to the final exponent, as the multiply shifts one extra time.
    m_exponent = static_cast<unsigned short>(expon+1);

    // Allocate space for the multiply result
    wordptr product =
        BitVector::Create(static_cast<N_int>((MANT_BITS+1)*2), false);

    // Allocate 1-bit-longer fields to force the operands to be unsigned
    wordptr op1 = BitVector::Create(static_cast<N_int>(MANT_BITS+1), false);
    wordptr op2 = BitVector::Create(static_cast<N_int>(MANT_BITS+1), false);

    // Make the operands unsigned after copying from original operands
    BitVector::Copy(op1, m_mantissa);
    BitVector::MSB(op1, false);
    BitVector::Copy(op2, op->m_mantissa);
    BitVector::MSB(op2, false);

    // Compute the product of the mantissas
    BitVector::Multiply(product, op1, op2);

    // Normalize the product.  Note: we know the product is non-zero because
    // both of the original operands were non-zero.
    //
    // Look for the highest set bit, shift to make it the MSB, and adjust
    // exponent.  Don't let exponent go negative.
    long norm_amt = (MANT_BITS*2-1)-BitVector::Set_Max(product);
    if (norm_amt > static_cast<long>(m_exponent))
        norm_amt = static_cast<long>(m_exponent);
    BitVector::Move_Left(product, static_cast<N_int>(norm_amt));
    m_exponent -= static_cast<unsigned short>(norm_amt);

    // Store the highest bits of the result
    BitVector::Interval_Copy(m_mantissa, product, 0, MANT_BITS, MANT_BITS);

    // Free allocated variables
    BitVector::Destroy(product);
    BitVector::Destroy(op1);
    BitVector::Destroy(op2);
}

FloatNum::FloatNum(const char *str)
{
    FloatNumManager& manager = FloatNumManager::instance();
    bool carry;

    m_mantissa = BitVector::Create(MANT_BITS, true);

    // allocate and initialize calculation variables
    wordptr operand[2];
    operand[0] = BitVector::Create(MANT_BITS, true);
    operand[1] = BitVector::Create(MANT_BITS, true);
    int dec_exponent = 0;               // decimal (powers of 10) exponent
    unsigned int sig_digits = 0;
    int decimal_pt = 1;

    // set initial flags to 0
    m_flags = 0;

    // check for + or - character and skip
    if (*str == '-')
    {
        m_sign = 1;
        str++;
    }
    else if (*str == '+')
    {
        m_sign = 0;
        str++;
    }
    else
        m_sign = 0;

    // eliminate any leading zeros (which do not count as significant digits)
    while (*str == '0')
        str++;

    // When we reach the end of the leading zeros, first check for a decimal
    // point.  If the number is of the form "0---0.0000" we need to get rid
    // of the zeros after the decimal point and not count them as significant
    // digits.
    if (*str == '.')
    {
        str++;
        while (*str == '0')
        {
            str++;
            dec_exponent--;
        }
    }
    else
    {
        // The number is of the form "yyy.xxxx" (where y <> 0).
        while (isdigit(*str))
        {
            // See if we've processed more than the max significant digits:
            if (sig_digits < MANT_SIGDIGITS)
            {
                // Multiply mantissa by 10 [x = (x<<1)+(x<<3)]
                BitVector::shift_left(m_mantissa, false);
                BitVector::Copy(operand[0], m_mantissa);
                BitVector::Move_Left(m_mantissa, 2);
                carry = false;
                BitVector::add(operand[1], operand[0], m_mantissa, &carry);

                // Add in current digit
                BitVector::Empty(operand[0]);
                BitVector::Chunk_Store(operand[0], 4, 0,
                                       static_cast<N_long>(*str-'0'));
                carry = false;
                BitVector::add(m_mantissa, operand[1], operand[0], &carry);
            }
            else
            {
                // Can't integrate more digits with mantissa, so instead just
                // raise by a power of ten.
                dec_exponent++;
            }
            sig_digits++;
            str++;
        }

        if (*str == '.')
            str++;
        else
            decimal_pt = 0;
    }

    if (decimal_pt)
    {
        // Process the digits to the right of the decimal point.
        while (isdigit(*str))
        {
            // See if we've processed more than 19 significant digits:
            if (sig_digits < 19)
            {
                // Raise by a power of ten
                dec_exponent--;

                // Multiply mantissa by 10 [x = (x<<1)+(x<<3)]
                BitVector::shift_left(m_mantissa, 0);
                BitVector::Copy(operand[0], m_mantissa);
                BitVector::Move_Left(m_mantissa, 2);
                carry = false;
                BitVector::add(operand[1], operand[0], m_mantissa, &carry);

                // Add in current digit
                BitVector::Empty(operand[0]);
                BitVector::Chunk_Store(operand[0], 4, 0,
                                       static_cast<N_long>(*str-'0'));
                carry = false;
                BitVector::add(m_mantissa, operand[1], operand[0], &carry);
            }
            sig_digits++;
            str++;
        }
    }

    if (*str == 'e' || *str == 'E')
    {
        str++;
        // We just saw the "E" character, now read in the exponent value and
        // add it into dec_exponent.
        int dec_exp_add = 0;
        sscanf(str, "%d", &dec_exp_add);
        dec_exponent += dec_exp_add;
    }

    // Free calculation variables.
    BitVector::Destroy(operand[1]);
    BitVector::Destroy(operand[0]);

    // Normalize the number, checking for 0 first.
    if (BitVector::is_empty(m_mantissa))
    {
        // Mantissa is 0, zero exponent too.
        m_exponent = 0;
        // Set zero flag so output functions don't see 0 value as underflow.
        m_flags |= FLAG_ISZERO;
        // Return 0 value.
        return;
    }
    // Exponent if already norm.
    m_exponent = static_cast<unsigned short>(0x7FFF+(MANT_BITS-1));
    normalize();

    // The number is normalized.  Now multiply by 10 the number of times
    // specified in DecExponent.  This uses the power of ten tables to speed
    // up this operation (and make it more accurate).
    if (dec_exponent > 0)
    {
        int POT_index = 0;
        // Until we hit 1.0 or finish exponent or overflow
        while ((POT_index < 14) && (dec_exponent != 0) &&
               (m_exponent != EXP_INF))
        {
            // Find the first power of ten in the table which is just less
            // than the exponent.
            while (dec_exponent < manager.POT_TableP[POT_index].dec_exponent)
                POT_index++;

            if (POT_index < 14)
            {
                // Subtract out what we're multiplying in from exponent
                dec_exponent -= manager.POT_TableP[POT_index].dec_exponent;

                // Multiply by current power of 10
                mul(manager.POT_TableP[POT_index].flt);
            }
        }
    }
    else if (dec_exponent < 0)
    {
        int POT_index = 0;
        // Until we hit 1.0 or finish exponent or underflow
        while ((POT_index < 14) && (dec_exponent != 0) &&
               (m_exponent != EXP_ZERO))
        {
            // Find the first power of ten in the table which is just less
            // than the exponent.
            while (dec_exponent > manager.POT_TableN[POT_index].dec_exponent)
                POT_index++;

            if (POT_index < 14)
            {
                // Subtract out what we're multiplying in from exponent
                dec_exponent -= manager.POT_TableN[POT_index].dec_exponent;

                // Multiply by current power of 10
                mul(manager.POT_TableN[POT_index].flt);
            }
        }
    }

    // Round the result. (Don't round underflow or overflow).  Also don't
    // increment if this would cause the mantissa to wrap.
    if ((m_exponent != EXP_INF) && (m_exponent != EXP_ZERO) &&
        !BitVector::is_full(m_mantissa))
        BitVector::increment(m_mantissa);
}

FloatNum::FloatNum(const FloatNum& flt)
    : m_exponent(flt.m_exponent),
      m_sign(flt.m_sign),
      m_flags(flt.m_flags)
{
    m_mantissa = BitVector::Clone(flt.m_mantissa);
}

void
FloatNum::swap(FloatNum& oth)
{
    std::swap(m_mantissa, oth.m_mantissa);
    std::swap(m_exponent, oth.m_exponent);
    std::swap(m_sign, oth.m_sign);
    std::swap(m_flags, oth.m_flags);
}

void
FloatNum::calc(Op::Op op, /*@unused@*/ const FloatNum* operand)
{
    if (op != Op::NEG)
        throw FloatingPointError(
            N_("Unsupported floating-point arithmetic operation"));
    m_sign ^= 1;
}

int
FloatNum::get_int(unsigned long& ret_val) const
{
    unsigned char t[4];

    if (get_sized(t, 4, 32, 0, false, 0))
    {
        ret_val = 0xDEADBEEFUL;     // Obviously incorrect return value
        return 1;
    }

    ret_val = static_cast<unsigned long>(t[0] & 0xFF);
    ret_val |= static_cast<unsigned long>((t[1] & 0xFF) << 8);
    ret_val |= static_cast<unsigned long>((t[2] & 0xFF) << 16);
    ret_val |= static_cast<unsigned long>((t[3] & 0xFF) << 24);
    return 0;
}

int
FloatNum::get_common(/*@out@*/ unsigned char* ptr,
                     N_int byte_size,
                     N_int mant_bits,
                     bool implicit1,
                     N_int exp_bits) const
{
    long exponent = static_cast<long>(m_exponent);
    bool overflow = false, underflow = false;
    int retval = 0;
    const long exp_bias = (1<<(exp_bits-1))-1;
    const long exp_inf = (1<<exp_bits)-1;

    wordptr output = BitVector::Create(byte_size*8, true);

    // copy mantissa
    BitVector::Interval_Copy(output, m_mantissa, 0,
        static_cast<N_int>((MANT_BITS-implicit1)-mant_bits), mant_bits);

    // round mantissa
    if (BitVector::bit_test(m_mantissa, (MANT_BITS-implicit1)-(mant_bits+1)))
        BitVector::increment(output);

    if (BitVector::bit_test(output, mant_bits))
    {
        // overflowed, so zero mantissa (and set explicit bit if necessary)
        BitVector::Empty(output);
        BitVector::Bit_Copy(output, mant_bits-1, !implicit1);
        // and up the exponent (checking for overflow)
        if (exponent+1 >= EXP_INF)
            overflow = true;
        else
            exponent++;
    }

    // adjust the exponent to the output bias, checking for overflow
    exponent -= EXP_BIAS-exp_bias;
    if (exponent >= exp_inf)
        overflow = true;
    else if (exponent <= 0)
        underflow = true;

    // underflow and overflow both set!?
    if (underflow && overflow)
        throw InternalError(N_("Both underflow and overflow set"));

    // check for underflow or overflow and set up appropriate output
    if (underflow)
    {
        BitVector::Empty(output);
        exponent = 0;
        if (!(m_flags & FLAG_ISZERO))
            retval = -1;
    }
    else if (overflow)
    {
        BitVector::Empty(output);
        exponent = exp_inf;
        retval = 1;
    }

    // move exponent into place
    BitVector::Chunk_Store(output, exp_bits, mant_bits,
                           static_cast<N_long>(exponent));

    // merge in sign bit
    BitVector::Bit_Copy(output, byte_size*8-1, m_sign);

    // get little-endian bytes
    unsigned int len;
    charptr buf = BitVector::Block_Read(output, &len);
    if (len < byte_size)
        throw InternalError(
            N_("Byte length of BitVector does not match bit length"));

    // copy to output
    std::memcpy(ptr, buf, byte_size*sizeof(unsigned char));

    // free allocated resources
    BitVector::Dispose(buf);

    BitVector::Destroy(output);

    return retval;
}

// IEEE-754 (Intel) "single precision" format:
// 32 bits:
// Bit 31    Bit 22              Bit 0
// |         |                       |
// seeeeeee emmmmmmm mmmmmmmm mmmmmmmm
//
// e = bias 127 exponent
// s = sign bit
// m = mantissa bits, bit 23 is an implied one bit.
//
// IEEE-754 (Intel) "double precision" format:
// 64 bits:
// bit 63       bit 51                                               bit 0
// |            |                                                        |
// seeeeeee eeeemmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm mmmmmmmm
//
// e = bias 1023 exponent.
// s = sign bit.
// m = mantissa bits.  Bit 52 is an implied one bit.
//
// IEEE-754 (Intel) "extended precision" format:
// 80 bits:
// bit 79            bit 63                           bit 0
// |                 |                                    |
// seeeeeee eeeeeeee mmmmmmmm m...m m...m m...m m...m m...m
//
// e = bias 16383 exponent
// m = 64 bit mantissa with NO implied bit!
// s = sign (for mantissa)

int
FloatNum::get_sized(unsigned char *ptr, size_t destsize, size_t valsize,
                    size_t shift, bool bigendian, int warn) const
{
    int retval;

    if (destsize*8 != valsize || shift>0 || bigendian)
    {
        // TODO
        throw InternalError(N_("unsupported floatnum functionality"));
    }

    switch (destsize)
    {
        case 4:
            retval = get_common(ptr, 4, 23, true, 8);
            break;
        case 8:
            retval = get_common(ptr, 8, 52, true, 11);
            break;
        case 10:
            retval = get_common(ptr, 10, 64, false, 15);
            break;
        default:
            throw InternalError(N_("Invalid float conversion size"));
            /*@notreached@*/
            return 1;
    }

    if (warn)
    {
        if (retval < 0)
            warn_set(WARN_GENERAL,
                     N_("underflow in floating point expression"));
        else if (retval > 0)
            warn_set(WARN_GENERAL,
                     N_("overflow in floating point expression"));
    }
    return retval;
}

bool
FloatNum::is_valid_size(size_t size) const
{
    switch (size)
    {
        case 32:
        case 64:
        case 80:
            return 1;
        default:
            return 0;
    }
}

std::ostream&
operator<< (std::ostream& os, const FloatNum& flt)
{
    unsigned char out[10];

    std::ios_base::fmtflags origff = os.flags();

    // Internal format
    unsigned char* str = BitVector::to_Hex(flt.m_mantissa, false);
    os << (flt.m_sign?"-":"+") << " " << reinterpret_cast<char*>(str);
    os << " *2^" << std::hex << flt.m_exponent << std::dec << '\n';
    BitVector::Dispose(str);

    // 32-bit (single precision) format
    os << "32-bit: " << flt.get_sized(out, 4, 32, 0, false, 0) << ": ";
    os << std::hex << std::setfill('0') << std::setw(2);
    for (int i=0; i<4; i++)
        os << out[i] << " ";
    os << '\n';
    os.flags(origff);

    // 64-bit (double precision) format
    os << "64-bit: " << flt.get_sized(out, 8, 64, 0, false, 0) << ": ";
    os << std::hex << std::setfill('0') << std::setw(2);
    for (int i=0; i<8; i++)
        os << out[i] << " ";
    os << '\n';
    os.flags(origff);

    // 80-bit (extended precision) format
    os << "80-bit: " << flt.get_sized(out, 10, 80, 0, false, 0) << ": ";
    os << std::hex << std::setfill('0') << std::setw(2);
    for (int i=0; i<10; i++)
        os << out[i] << " ";
    os << '\n';
    os.flags(origff);

    return os;
}

} // namespace yasm

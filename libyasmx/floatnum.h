#ifndef YASM_FLOATNUM_H
#define YASM_FLOATNUM_H
///
/// @file
/// @brief Floating point (IEEE) interface.
///
/// @license
///  Copyright (C) 2001-2007  Peter Johnson
///
/// Based on public-domain x86 assembly code by Randall Hyde (8/28/91).
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
#include <algorithm>
#include <iosfwd>

#include "bitvect.h"
#include "export.h"
#include "operator.h"


namespace yasm
{

class FloatNumManager;
class FloatNumTest;

/// IEEE floating point number.
class YASM_LIB_EXPORT FloatNum
{
    friend YASM_LIB_EXPORT
    std::ostream& operator<< (std::ostream& os, const FloatNum& flt);
    friend class FloatNumManager;
    friend class FloatNumTest;

public:
    /// Create a new floatnum from a decimal string.  The input string
    /// must be in standard C representation ([+-]123.456e[-+]789).
    /// @param str  floating point decimal string
    explicit FloatNum(const char* str);

    /// Copy constructor.
    FloatNum(const FloatNum& rhs);
    /// Assignment operator.
    FloatNum& operator= (const FloatNum& rhs)
    {
        if (this != &rhs)
            FloatNum(rhs).swap(*this);
        return *this;
    }

    /// Exchanges this floatnum with another one.
    /// @param other    other floatnum
    void swap(FloatNum& oth);

    /// Create an explicit allocated copy.
    FloatNum* clone() const { return new FloatNum(*this); }

    /// Destructor.
    ~FloatNum() { BitVector::Destroy(m_mantissa); }

    /// Floating point calculation function: acc = acc op operand.
    /// @note Not all operations in Op::Op may be supported; unsupported
    ///       operations will result in an error.
    /// @param op       operation
    /// @param operand  floatnum operand
    void calc(Op::Op op, const FloatNum& operand) { calc(op, &operand); }
    void calc(Op::Op op, /*@null@*/ const FloatNum* operand = 0);

    /// Convert a floatnum to single-precision and return as 32-bit value.
    /// The 32-bit value is a "standard" C value (eg, of unknown endian).
    /// @param ret_val  pointer to storage for 32-bit output
    /// @return Nonzero if flt can't fit into single precision: -1 if
    ///         underflow occurred, 1 if overflow occurred.
    int get_int(/*@out@*/ unsigned long& ret_val) const;

    /// Output a floatnum to buffer in little-endian or big-endian.  Puts the
    /// value into the least significant bits of the destination, or may be
    /// shifted into more significant bits by the shift parameter.  The
    /// destination bits are cleared before being set.  [0] should be the
    /// first byte output to the file.
    /// @note Not all sizes are valid.  Currently, only 32 (single-precision),
    ///       64 (double-precision), and 80 (extended-precision) are valid
    ///       sizes.  Use yasm_floatnum_check_size() to check for supported
    ///       sizes.
    /// @param ptr          pointer to storage for size bytes of output
    /// @param destsize     destination size (in bytes)
    /// @param valsize      size (in bits)
    /// @param shift        left shift (in bits)
    /// @param bigendian    endianness (true=big, false=little)
    /// @param warn         enables standard overflow/underflow warnings
    /// @return Nonzero if flt can't fit into the specified precision: -1 if
    ///         underflow occurred, 1 if overflow occurred.
    int get_sized(unsigned char *ptr, size_t destsize, size_t valsize,
                  size_t shift, bool bigendian, int warn) const;

    /// Basic check to see if size is valid for flt conversion (using
    /// get_sized()).  Doesn't actually check for underflow/overflow
    /// but rather checks for size=32,64,80 (at present).
    /// @param size     number of bits of output space
    /// @return True if valid size, false if invalid size.
    bool is_valid_size(size_t size) const;

private:
    FloatNum(const unsigned char* mantissa, unsigned short exponent);

    void normalize();
    void mul(const FloatNum* op);

    /// Function used by conversion routines to actually perform the
    /// conversion.
    /// @param ptr          the array to return the little-endian value into
    /// @param byte_size    the size in bytes of the output format
    /// @param mant_bits    the size in bits of the output mantissa
    /// @param implicit1    does the output format have an implicit 1?
    /// @param exp_bits     the size in bits of the output exponent
    /// @return 0 on success, 1 if overflow, -1 if underflow.
    int get_common(/*@out@*/ unsigned char *ptr,
                   BitVector::N_int byte_size,
                   BitVector::N_int mant_bits,
                   bool implicit1,
                   BitVector::N_int exp_bits) const;


    /// 97-bit internal floating point format:
    /// 0000000s eeeeeeee eeeeeeee m.....................................m
    /// Sign          exponent     mantissa (80 bits)
    ///                            79                                    0
    ///
    /// Mantissa does NOT have an implied one bit (it's explicit).
    ///
    BitVector::wordptr m_mantissa; // Allocated to MANT_BITS bits

    /// Bias 32767 exponent.
    unsigned short m_exponent;

    /// Sign.  Only L.O. bit of is significant.  The rest is zero.
    unsigned char m_sign;

    /// Flags.
    unsigned char m_flags;
};

YASM_LIB_EXPORT
std::ostream& operator<< (std::ostream& os, const FloatNum& flt);

/// Specialized swap for algorithms.
inline void
swap(FloatNum& left, FloatNum& right)
{
    left.swap(right);
}

} // namespace yasm

namespace std
{

/// Specialized std::swap.
template <>
inline void
swap(yasm::FloatNum& left, yasm::FloatNum& right)
{
    left.swap(right);
}

} // namespace std

#endif

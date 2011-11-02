#ifndef YASM_INTNUM_H
#define YASM_INTNUM_H
///
/// @file
/// @brief Big integer number interface.
///
/// @license
///  Copyright (C) 2001-2007  Peter Johnson
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
#include <cstdlib>
#include <string>

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/StringRef.h"
#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Config/export.h"
#include "yasmx/Config/longlong.h"
#include "yasmx/DebugDumper.h"
#include "yasmx/Op.h"


namespace yasm
{
class Diagnostic;

/// Check to see if APInt will fit without overflow into size bits.
/// @param intn         APInt
/// @param size         number of bits of output space
/// @param rshift       right shift
/// @param rangetype    signed/unsigned range selection:
///                     0 => (0, unsigned max);
///                     1 => (signed min, signed max);
///                     2 => (signed min, unsigned max)
/// @return True if APInt will fit.
YASM_LIB_EXPORT
bool isOkSize(const llvm::APInt& intn,
              unsigned int size,
              unsigned int rshift,
              int rangetype);

class ExprTerm;

/// Raw storage for IntNum.
struct YASM_LIB_EXPORT IntNumData
{
#ifdef YASM_HAVE_LONG_LONG
    typedef long long SmallValue;
    typedef unsigned long long USmallValue;
#else
    typedef long SmallValue;
    typedef unsigned long USmallValue;
#endif
    union
    {
        SmallValue sv;          ///< integer value (for integers <=long bits)
        llvm::APInt* bv;        ///< big value (for integers >long bits)
    } m_val;
    enum { INTNUM_SV, INTNUM_BV } m_type;
};

/// Big integer number.
class YASM_LIB_EXPORT IntNum : private IntNumData
{
    friend YASM_LIB_EXPORT
    llvm::raw_ostream& operator<< (llvm::raw_ostream& os, const IntNum& intn);
    friend YASM_LIB_EXPORT
    int Compare(const IntNum& intn1, const IntNum& intn2);
    friend YASM_LIB_EXPORT
    bool operator==(const IntNum& lhs, const IntNum& rhs);
    friend YASM_LIB_EXPORT
    bool operator<(const IntNum& lhs, const IntNum& rhs);
    friend YASM_LIB_EXPORT
    bool operator>(const IntNum& lhs, const IntNum& rhs);
    friend class ExprTerm;

public:
    /// "Native" "word" size for intnum calculations.
    enum { BITVECT_NATIVE_SIZE = 256 };

    /// Default constructor.  Initializes value to 0.
    IntNum()
    {
        m_type = INTNUM_SV;
        m_val.sv = 0;
    }

#ifdef YASM_HAVE_LONG_LONG
    /// Create a new intnum from an unsigned integer value.
    /// @param i        unsigned integer value
    IntNum(unsigned long long i)
    {
        m_type = INTNUM_SV;
        set(i);
    }

    /// Create a new intnum from an signed integer value.
    /// @param i        signed integer value
    IntNum(long long i)
    {
        m_type = INTNUM_SV;
        m_val.sv = static_cast<SmallValue>(i);
    }
#endif
    /// Create a new intnum from an unsigned integer value.
    /// @param i        unsigned integer value
    IntNum(unsigned long i)
    {
        m_type = INTNUM_SV;
        set(static_cast<USmallValue>(i));
    }

    /// Create a new intnum from an signed integer value.
    /// @param i        signed integer value
    IntNum(long i)
    {
        m_type = INTNUM_SV;
        m_val.sv = static_cast<SmallValue>(i);
    }

    /// Create a new intnum from an unsigned integer value.
    /// @param i        unsigned integer value
    IntNum(unsigned int i)
    {
        m_type = INTNUM_SV;
        set(static_cast<USmallValue>(i));
    }

    /// Create a new intnum from an signed integer value.
    /// @param i        signed integer value
    IntNum(int i)
    {
        m_type = INTNUM_SV;
        m_val.sv = static_cast<SmallValue>(i);
    }

    /// Copy constructor.
    IntNum(const IntNum& rhs);

    /// Assignment operators.
    IntNum& operator= (const IntNum& rhs)
    {
        if (this != &rhs)
            IntNum(rhs).swap(*this);
        return *this;
    }
#ifdef YASM_HAVE_LONG_LONG
    IntNum& operator= (unsigned long long val)
    {
        set(val);
        return *this;
    }
    IntNum& operator= (long long val)
    {
        set(val);
        return *this;
    }
#endif
    IntNum& operator= (unsigned long val)
    {
        set(static_cast<USmallValue>(val));
        return *this;
    }
    IntNum& operator= (long val)
    {
        set(static_cast<SmallValue>(val));
        return *this;
    }
    IntNum& operator= (unsigned int val)
    {
        set(static_cast<USmallValue>(val));
        return *this;
    }
    IntNum& operator= (int val)
    {
        set(static_cast<SmallValue>(val));
        return *this;
    }

    /// Exchanges this integer with another one.
    /// @param oth      other integer number
    void swap(IntNum& oth)
    {
        std::swap(static_cast<IntNumData&>(*this),
                  static_cast<IntNumData&>(oth));
    }

    /// Get an allocated copy.
    IntNum* clone() const { return new IntNum(*this); }

    /// Destructor.
    ~IntNum()
    {
        if (m_type == INTNUM_BV)
            delete m_val.bv;
    }

    /// Integer calculation function: acc = acc op operand.
    /// @note Not all operations in Op::Op may be supported; unsupported
    ///       operations will result in an error.
    /// @param op       operation
    /// @param operand  intnum operand
    /// @param source   source location of operation
    /// @param diags    diagnostic reporting
    /// @return False if an error occurred.
    bool Calc(Op::Op op,
              const IntNum& operand,
              SourceLocation source,
              Diagnostic& diags)
    { return CalcImpl(op, &operand, source, &diags); }
    bool Calc(Op::Op op, SourceLocation source, Diagnostic& diags)
    { return CalcImpl(op, 0, source, &diags); }

    /// Integer calculation function: acc = acc op operand.
    /// @note Not all operations in Op::Op may be supported; unsupported
    ///       operations will result in an error.
    /// @param op       operation
    /// @param operand  intnum operand
    /// @note Asserts if an error occurs.
    void CalcAssert(Op::Op op, const IntNum& operand)
    { CalcImpl(op, &operand, SourceLocation(), 0); }
    void CalcAssert(Op::Op op)
    { CalcImpl(op, 0, SourceLocation(), 0); }

    /// Sign extension.  Takes the current value as a signed value of @p size
    /// bits and changes the value to its sign extended version.
    /// Example: 0xffffff7, size 32 results in -9.
    /// @param size     size to extend from, in bits
    void SignExtend(unsigned int size);

    /// Zero an intnum.
    void Zero() { set(static_cast<SmallValue>(0)); }

    /// Simple value check for 0.
    /// @return True if acc==0.
    bool isZero() const
    {
        return (m_type == INTNUM_SV && m_val.sv == 0);
    }

    /// Simple value check for 1.
    /// @param acc      intnum
    /// @return True if acc==1.
    bool isPos1() const
    {
        return (m_type == INTNUM_SV && m_val.sv == 1);
    }

    /// Simple value check for -1.
    /// @return True if acc==-1.
    bool isNeg1() const
    {
        return (m_type == INTNUM_SV && m_val.sv == -1);
    }

    /// Simple sign check.
    /// @return -1 if negative, 0 if zero, +1 if positive
    int getSign() const;

    /// Convert an intnum to an unsigned 32-bit value.
    /// The value is in "standard" C format (eg, of unknown endian).
    /// @note Parameter intnum is saturated to fit into 32 bits.  Use
    ///       intnum_check_size() to check for overflow.
    /// @return Unsigned 32-bit value of intn.
    unsigned long getUInt() const;

    /// Convert an intnum to a signed "long" value.
    /// The value is in "standard" C format (eg, of unknown endian).
    /// @note Parameter intnum is saturated to fit into a long.  Use
    ///       intnum_check_size() to check for overflow.
    /// @return Signed long value of intn.
    long getInt() const;

    /// Determine if intnum will fit in a signed "long" without saturating.
    bool isInt() const;

    /// Check to see if intnum will fit without overflow into size bits.
    /// @param intn         intnum
    /// @param size         number of bits of output space
    /// @param rshift       right shift
    /// @param rangetype    signed/unsigned range selection:
    ///                     0 => (0, unsigned max);
    ///                     1 => (signed min, signed max);
    ///                     2 => (signed min, unsigned max)
    /// @return True if intnum will fit.
    bool isOkSize(unsigned int size, unsigned int rshift, int rangetype) const;

    /// Check to see if intnum will fit into a particular numeric range.
    /// @param low      low end of range (inclusive)
    /// @param high     high end of range (inclusive)
    /// @return True if intnum is within range.
    bool isInRange(long low, long high) const;

    /// Set intnum value from a decimal/binary/octal/hexidecimal string.
    /// @param str      input string
    /// @param radix    numeric base (10=decimal, etc)
    /// @return False if overflow occurred.
    bool setStr(llvm::StringRef str, unsigned int radix=10);

    /// Get intnum value into a SmallString.  The returned string will
    /// contain a leading '-' if the intnum is negative.
    /// @param str          output SmallString
    /// @param base         numeric base (10=decimal, etc)
    /// @param lowercase    whether hex digits should be lowercase
    void getStr(llvm::SmallVectorImpl<char>& str,
                int base = 10,
                bool lowercase = true) const;
 
    /// Converts IntNum to a std::string.  This is an inefficient
    /// method, your should prefer passing in a SmallString instead.
    /// @param base         numeric base (10=decimal, etc)
    /// @param lowercase    whether hex digits should be lowercase
    /// @return String representing the value of the intnum.
    std::string getStr(int base = 10, bool lowercase = true) const;

    /// Extract width bits from IntNum, starting at bit lsb.
    /// Returns integer such that the bit lsb becomes the least significant bit
    /// of the return value.  All high bits above width in the return value are
    /// zero-filled.
    /// @param width    Number of bits to extract
    /// @param lsb      Bit number of least significant bit (0=lsb)
    /// @return Extracted bits.
    unsigned long Extract(unsigned int width, unsigned int lsb) const;

    /// Overloaded unary operators:

    /// Prefix increment.
    IntNum& operator++();

    /// Postfix increment.
    IntNum operator++(int)
    {
        IntNum old(*this);
        ++*this;
        return old;
    }

    /// Prefix decrement.
    IntNum& operator--();

    /// Postfix decrement.
    IntNum operator--(int)
    {
        IntNum old(*this);
        --*this;
        return old;
    }

    /// If intnum is a BV, returns its bitvector directly.
    /// If not, converts into passed bv and returns that instead.
    /// @param bv       bitvector to use if intnum is not bitvector.
    /// @return Passed bv or intnum internal bitvector.
    const llvm::APInt* getBV(llvm::APInt* bv) const;
    llvm::APInt* getBV(llvm::APInt* bv);

    /// Store a bitvector into intnum storage.
    /// If saved as a bitvector, clones the passed bitvector.
    /// Can modify the passed bitvector.
    /// @param bv       bitvector
    void setBV(const llvm::APInt& bv);

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    /// Print to stream.
    /// @param os           output stream
    /// @param base         numeric base (10=decimal, etc)
    /// @param lowercase    whether hex digits should be lowercase
    /// @param showbase     whether C-style non-decimal prefixes should be used:
    ///                     "0x" for hex, "0b" for bin, "0" for octal.
    /// @param bits         number of bits to pad output to (-1 for no padding);
    ///                     applys only to non-decimal output
    /// The negative sign (if required) is output before the non-decimal prefix.
    void Print(llvm::raw_ostream& os,
               int base = 10,
               bool lowercase = true,
               bool showbase = true,
               int bits = -1) const;

private:
    /// Integer calculation function: acc = acc op operand.
    /// @note Not all operations in Op::Op may be supported; unsupported
    ///       operations will result in an error.
    /// @param op       operation
    /// @param operand  intnum operand
    /// @param source   source location of operation
    /// @param diags    diagnostic reporting
    /// @return False if an error occurred.
    bool CalcImpl(Op::Op op,
                  /*@null@*/ const IntNum* operand,
                  SourceLocation source,
                  Diagnostic* diags);

    /// Set an intnum to an unsigned integer.
    /// @param val      integer value
    void set(USmallValue val);

    /// Set an intnum to an signed integer.
    /// @param val      integer value
    void set(SmallValue val)
    {
        if (m_type == INTNUM_BV)
            delete m_val.bv;
        m_type = INTNUM_SV;
        m_val.sv = val;
    }
};

/// Overloaded assignment binary operators.
/// @note These assert on failure.
inline IntNum& operator+=(IntNum& lhs, const IntNum& rhs)
{ lhs.CalcAssert(Op::ADD, rhs); return lhs; }
inline IntNum& operator-=(IntNum& lhs, const IntNum& rhs)
{ lhs.CalcAssert(Op::SUB, rhs); return lhs; }
inline IntNum& operator*=(IntNum& lhs, const IntNum& rhs)
{ lhs.CalcAssert(Op::MUL, rhs); return lhs; }
inline IntNum& operator/=(IntNum& lhs, const IntNum& rhs)
{ lhs.CalcAssert(Op::DIV, rhs); return lhs; }
inline IntNum& operator%=(IntNum& lhs, const IntNum& rhs)
{ lhs.CalcAssert(Op::MOD, rhs); return lhs; }
inline IntNum& operator^=(IntNum& lhs, const IntNum& rhs)
{ lhs.CalcAssert(Op::XOR, rhs); return lhs; }
inline IntNum& operator&=(IntNum& lhs, const IntNum& rhs)
{ lhs.CalcAssert(Op::AND, rhs); return lhs; }
inline IntNum& operator|=(IntNum& lhs, const IntNum& rhs)
{ lhs.CalcAssert(Op::OR, rhs); return lhs; }
inline IntNum& operator>>=(IntNum& lhs, const IntNum& rhs)
{ lhs.CalcAssert(Op::SHR, rhs); return lhs; }
inline IntNum& operator<<=(IntNum& lhs, const IntNum& rhs)
{ lhs.CalcAssert(Op::SHL, rhs); return lhs; }

/// Overloaded unary operators.
/// @note These assert on failure.
inline IntNum operator-(IntNum rhs)
{ rhs.CalcAssert(Op::NEG); return rhs; }
inline IntNum operator+(const IntNum& rhs)
{ return IntNum(rhs); }
inline IntNum operator~(IntNum rhs)
{ rhs.CalcAssert(Op::NOT); return rhs; }
inline bool operator!(const IntNum& rhs)
{ return rhs.isZero(); }

/// Overloaded binary operators.
/// @note These assert on failure.
inline const IntNum operator+(IntNum lhs, const IntNum& rhs)
{ return lhs += rhs; }
inline const IntNum operator-(IntNum lhs, const IntNum& rhs)
{ return lhs -= rhs; }
inline const IntNum operator*(IntNum lhs, const IntNum& rhs)
{ return lhs *= rhs; }
inline const IntNum operator/(IntNum lhs, const IntNum& rhs)
{ return lhs /= rhs; }
inline const IntNum operator%(IntNum lhs, const IntNum& rhs)
{ return lhs %= rhs; }
inline const IntNum operator^(IntNum lhs, const IntNum& rhs)
{ return lhs ^= rhs; }
inline const IntNum operator&(IntNum lhs, const IntNum& rhs)
{ return lhs &= rhs; }
inline const IntNum operator|(IntNum lhs, const IntNum& rhs)
{ return lhs |= rhs; }
inline const IntNum operator>>(IntNum lhs, const IntNum& rhs)
{ return lhs >>= rhs; }
inline const IntNum operator<<(IntNum lhs, const IntNum& rhs)
{ return lhs <<= rhs; }

/// Compare two intnums.
/// @param lhs      first intnum
/// @param rhs      second intnum
/// @return -1 if lhs < rhs, 0 if lhs == rhs, 1 if lhs > rhs.
YASM_LIB_EXPORT
int Compare(const IntNum& lhs, const IntNum& rhs);

/// Overloaded comparison operators.
YASM_LIB_EXPORT
bool operator==(const IntNum& lhs, const IntNum& rhs);
YASM_LIB_EXPORT
bool operator<(const IntNum& lhs, const IntNum& rhs);
YASM_LIB_EXPORT
bool operator>(const IntNum& lhs, const IntNum& rhs);

inline bool operator!=(const IntNum& lhs, const IntNum& rhs)
{ return !(lhs == rhs); }
inline bool operator<=(const IntNum& lhs, const IntNum& rhs)
{ return !(lhs > rhs); }
inline bool operator>=(const IntNum& lhs, const IntNum& rhs)
{ return !(lhs < rhs); }

/// Specialized swap for algorithms.
inline void
swap(IntNum& left, IntNum& right)
{
    left.swap(right);
}

inline llvm::raw_ostream&
operator<< (llvm::raw_ostream& os, const IntNum& intn)
{
    intn.Print(os);
    return os;
}

} // namespace yasm

namespace std
{

/// Specialized std::swap.
template <>
inline void
swap(yasm::IntNum& left, yasm::IntNum& right)
{
    left.swap(right);
}

} // namespace std

#endif

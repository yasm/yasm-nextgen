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

#include <cctype>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"
#include "yasmx/Basic/Diagnostic.h"


using namespace yasm;
using llvm::APInt;

/// Static bitvect used for conversions.
static APInt conv_bv(IntNum::BITVECT_NATIVE_SIZE, 0);

/// Static bitvects used for computation.
static APInt result(IntNum::BITVECT_NATIVE_SIZE, 0);
static APInt spare(IntNum::BITVECT_NATIVE_SIZE, 0);
static APInt op1static(IntNum::BITVECT_NATIVE_SIZE, 0);
static APInt op2static(IntNum::BITVECT_NATIVE_SIZE, 0);

/// Static bitvect used for sign extension.
static APInt signext_bv(IntNum::BITVECT_NATIVE_SIZE, 0);

enum
{
    SV_BITS = std::numeric_limits<IntNumData::SmallValue>::digits,
    LONG_BITS = std::numeric_limits<long>::digits,
    ULONG_BITS = std::numeric_limits<unsigned long>::digits
};

bool
yasm::isOkSize(const APInt& intn,
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
IntNum::setBV(const APInt& bv)
{
    if (bv.getMinSignedBits() <= SV_BITS)
    {
        if (m_type == INTNUM_BV)
            delete m_val.bv;
        m_type = INTNUM_SV;
        m_val.sv = static_cast<SmallValue>(bv.getSExtValue());
        return;
    }
    else if (m_type == INTNUM_BV)
    {
        *m_val.bv = bv;
    }
    else
    {
        m_type = INTNUM_BV;
        m_val.bv = new APInt(bv);
    }
    *m_val.bv = m_val.bv->sextOrTrunc(BITVECT_NATIVE_SIZE);
}

const APInt*
IntNum::getBV(APInt* bv) const
{
    if (m_type == INTNUM_BV)
        return m_val.bv;

    if (m_val.sv >= 0)
        *bv = static_cast<USmallValue>(m_val.sv);
    else
    {
        *bv = static_cast<USmallValue>(~m_val.sv);
        bv->flipAllBits();
    }
    return bv;
}

APInt*
IntNum::getBV(APInt* bv)
{
    if (m_type == INTNUM_BV)
        return m_val.bv;

    if (m_val.sv >= 0)
        *bv = static_cast<USmallValue>(m_val.sv);
    else
    {
        *bv = static_cast<USmallValue>(~m_val.sv);
        bv->flipAllBits();
    }
    return bv;
}

/// Return the value of the specified hex digit, or -1 if it's not valid.
static unsigned int
HexDigitValue(char ch)
{
    if (ch >= '0' && ch <= '9') return ch-'0';
    if (ch >= 'a' && ch <= 'f') return ch-'a'+10;
    if (ch >= 'A' && ch <= 'F') return ch-'A'+10;
    if (ch == '_') return 0;
    return ~0;
}

bool
IntNum::setStr(StringRef str, unsigned int radix)
{
    // Each computation below needs to know if its negative
    bool is_neg = (str[0] == '-');
    unsigned int minbits = is_neg ? 1 : 0;
    size_t len = str.size();
    StringRef::iterator begin = str.begin();

    if (is_neg)
    {
        ++begin;
        --len;
    }

    // For radixes of power-of-two values, the bits required is accurately and
    // easily computed.  For radix 10, we use a rough approximation.
    switch (radix)
    {
        case 2:     minbits += (len-minbits); break;
        case 8:     minbits += (len-minbits) * 3; break;
        case 16:    minbits += (len-minbits) * 4; break;
        case 10:    minbits += ((len-minbits) * 64) / 22; break;
        default:
        {
            unsigned int max_bits_per_digit = 1;
            while ((1U << max_bits_per_digit) < radix)
                max_bits_per_digit += 1;
            minbits += len * max_bits_per_digit;
        }
    }
    if (minbits > BITVECT_NATIVE_SIZE)
        return false;

    if (minbits < SV_BITS)
    {
        // shortcut "short" case
        SmallValue v = 0;
        for (StringRef::iterator i = begin, end = str.end(); i != end; ++i)
        {
            unsigned int c = HexDigitValue(*i);
            assert(c < radix && "invalid digit for given radix");
            v = v*radix + c;
        }
        set(is_neg ? -v : v);
        return true;
    }

    // long case
    conv_bv = 0;

    // Figure out if we can shift instead of multiply
    unsigned int shift =
        (radix == 16 ? 4 : radix == 8 ? 3 : radix == 2 ? 1 : 0);

    APInt& radixval = op1static;
    APInt& charval = op2static;
    APInt& oldval = spare;

    radixval = radix;
    oldval = 0;

    bool overflowed = false;
    for (StringRef::iterator i=begin, end=str.end(); i != end; ++i)
    {
        unsigned int c = HexDigitValue(*i);

        // If this letter is out of bound for this radix, reject it.
        assert(c < radix && "invalid digit for given radix");

        charval = c;

        // Add the digit to the value in the appropriate radix.  If adding in
        // digits made the value smaller, then this overflowed.
        oldval = conv_bv;

        // Shift or multiply by radix, did overflow occur on the multiply?
        if (shift)
            conv_bv <<= shift;
        else
            conv_bv *= radixval;
        overflowed |= conv_bv.udiv(radixval) != oldval;

        // Add value, did overflow occur on the value?
        //   (a + b) ult b  <=> overflow
        conv_bv += charval;
        overflowed |= conv_bv.ult(charval);
    }

    // If it's negative, put it in two's complement form
    if (is_neg)
    {
        --conv_bv;
        conv_bv.flipAllBits();
    }

    setBV(conv_bv);
    return overflowed;
}

IntNum::IntNum(const IntNum& rhs)
{
    m_type = rhs.m_type;
    if (rhs.m_type == INTNUM_BV)
        m_val.bv = new APInt(*rhs.m_val.bv);
    else
        m_val.sv = rhs.m_val.sv;
}

// Speedup function for non-bitvect calculations.
// Always makes conservative assumptions; we fall back to bitvect if this
// function does not set handled to true.
static bool
CalcSmallValue(bool* handled,
               Op::Op op,
               IntNumData::SmallValue* lhs,
               IntNumData::SmallValue rhs,
               SourceLocation source,
               DiagnosticsEngine* diags)
{
    static const IntNumData::SmallValue SV_MAX =
        std::numeric_limits<IntNumData::SmallValue>::max();
    static const IntNumData::SmallValue SV_MIN =
        std::numeric_limits<IntNumData::SmallValue>::min();

    *handled = false;
    switch (op)
    {
        case Op::ADD:
            if (*lhs >= SV_MAX/2 || *lhs <= SV_MIN/2 ||
                rhs >= SV_MAX/2 || rhs <= SV_MIN/2)
                return true;
            *lhs += rhs;
            break;
        case Op::SUB:
            if (*lhs >= SV_MAX/2 || *lhs <= SV_MIN/2 ||
                rhs >= SV_MAX/2 || rhs <= SV_MIN/2)
                return true;
            *lhs -= rhs;
            break;
        case Op::MUL:
        {
            // half range
            IntNumData::SmallValue minmax = 1;
            minmax <<= SV_BITS/2;
            if (*lhs > -minmax && *lhs < minmax)
            {
                if (rhs <= -minmax || rhs >= minmax)
                    return true;
                *lhs *= rhs;
                break;
            }
            // maybe someday?
            return true;
        }
        case Op::DIV:
            // TODO: make sure lhs and rhs are unsigned
        case Op::SIGNDIV:
            if (rhs == 0)
            {
                assert(diags && "divide by zero");
                diags->Report(source, diag::err_divide_by_zero);
                return false;
            }
            *lhs /= rhs;
            break;
        case Op::MOD:
            // TODO: make sure lhs and rhs are unsigned
        case Op::SIGNMOD:
            if (rhs == 0)
            {
                assert(diags && "divide by zero");
                diags->Report(source, diag::err_divide_by_zero);
                return false;
            }
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
            return true;
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
            return true;
    }
    *handled = true;
    return true;
}

/*@-nullderef -nullpass -branchstate@*/
bool
IntNum::CalcImpl(Op::Op op,
                 const IntNum* operand,
                 SourceLocation source,
                 DiagnosticsEngine* diags)
{
    assert((operand || op == Op::NEG || op == Op::NOT || op == Op::LNOT) &&
           "operation needs an operand");

    if (m_type == INTNUM_SV && (!operand || operand->m_type == INTNUM_SV))
    {
        bool handled = false;
        if (!CalcSmallValue(&handled, op, &m_val.sv,
                            operand ? operand->m_val.sv : 0, source, diags))
            return false;
        if (handled)
            return true;
    }

    // Always do computations with in full bit vector.
    // Bit vector results must be calculated through intermediate storage.
    const APInt* op1 = getBV(&op1static);
    const APInt* op2 = 0;
    if (operand)
        op2 = operand->getBV(&op2static);

    // A operation does a bitvector computation if result is allocated.
    switch (op)
    {
        case Op::ADD:
        {
            result = *op1;
            result += *op2;
            break;
        }
        case Op::SUB:
        {
            result = *op1;
            result -= *op2;
            break;
        }
        case Op::MUL:
            result = *op1;
            result *= *op2;
            break;
        case Op::DIV:
            // TODO: make sure op1 and op2 are unsigned
            if (!*op2)
            {
                assert(diags && "divide by zero");
                diags->Report(source, diag::err_divide_by_zero);
                return false;
            }
            result = op1->udiv(*op2);
            break;
        case Op::SIGNDIV:
            if (!*op2)
            {
                assert(diags && "divide by zero");
                diags->Report(source, diag::err_divide_by_zero);
                return false;
            }
            result = op1->sdiv(*op2);
            break;
        case Op::MOD:
            // TODO: make sure op1 and op2 are unsigned
            if (!*op2)
            {
                assert(diags && "divide by zero");
                diags->Report(source, diag::err_divide_by_zero);
                return false;
            }
            result = op1->urem(*op2);
            break;
        case Op::SIGNMOD:
            if (!*op2)
            {
                assert(diags && "divide by zero");
                diags->Report(source, diag::err_divide_by_zero);
                return false;
            }
            result = op1->srem(*op2);
            break;
        case Op::NEG:
            result = -*op1;
            break;
        case Op::NOT:
            result = *op1;
            result.flipAllBits();
            break;
        case Op::OR:
            result = *op1;
            result |= *op2;
            break;
        case Op::AND:
            result = *op1;
            result &= *op2;
            break;
        case Op::XOR:
            result = *op1;
            result ^= *op2;
            break;
        case Op::XNOR:
            result = *op1;
            result ^= *op2;
            result.flipAllBits();
            break;
        case Op::NOR:
            result = *op1;
            result |= *op2;
            result.flipAllBits();
            break;
        case Op::SHL:
            if (operand->m_type == INTNUM_SV)
            {
                if (operand->m_val.sv >= 0)
                    result = op1->shl(operand->m_val.sv);
                else
                    result = op1->ashr(-operand->m_val.sv);
            }
            else    // don't even bother, just zero result
                result.clearAllBits();
            break;
        case Op::SHR:
            if (operand->m_type == INTNUM_SV)
            {
                if (operand->m_val.sv >= 0)
                    result = op1->ashr(operand->m_val.sv);
                else
                    result = op1->shl(-operand->m_val.sv);
            }
            else    // don't even bother, just zero result
                result.clearAllBits();
            break;
        case Op::LOR:
            set(static_cast<SmallValue>((!!*op1) || (!!*op2)));
            return true;
        case Op::LAND:
            set(static_cast<SmallValue>((!!*op1) && (!!*op2)));
            return true;
        case Op::LNOT:
            set(static_cast<SmallValue>(!*op1));
            return true;
        case Op::LXOR:
            set(static_cast<SmallValue>((!!*op1) ^ (!!*op2)));
            return true;
        case Op::LXNOR:
            set(static_cast<SmallValue>(!((!!*op1) ^ (!!*op2))));
            return true;
        case Op::LNOR:
            set(static_cast<SmallValue>(!((!!*op1) || (!!*op2))));
            return true;
        case Op::EQ:
            set(static_cast<SmallValue>(op1->eq(*op2)));
            return true;
        case Op::LT:
            set(static_cast<SmallValue>(op1->slt(*op2)));
            return true;
        case Op::GT:
            set(static_cast<SmallValue>(op1->sgt(*op2)));
            return true;
        case Op::LE:
            set(static_cast<SmallValue>(op1->sle(*op2)));
            return true;
        case Op::GE:
            set(static_cast<SmallValue>(op1->sge(*op2)));
            return true;
        case Op::NE:
            set(static_cast<SmallValue>(op1->ne(*op2)));
            return true;
        case Op::SEG:
            assert(diags && "invalid use of operator 'SEG'");
            diags->Report(source, diag::err_invalid_op_use) << "SEG";
            return false;
        case Op::WRT:
            assert(diags && "invalid use of operator 'WRT'");
            diags->Report(source, diag::err_invalid_op_use) << "WRT";
            return false;
        case Op::SEGOFF:
            assert(diags && "invalid use of operator ':'");
            diags->Report(source, diag::err_invalid_op_use) << ":";
            return false;
        case Op::IDENT:
            result = *op1;
            break;
        default:
            assert(diags && "invalid integer operation");
            diags->Report(source, diag::err_int_invalid_op);
            return false;
    }

    // Try to fit the result into long if possible
    setBV(result);
    return true;
}
/*@=nullderef =nullpass =branchstate@*/

void
IntNum::SignExtend(unsigned int size)
{
    // For now, always implement with full bit vector.
    APInt* bv = getBV(&signext_bv);
    *bv = bv->trunc(size).sext(BITVECT_NATIVE_SIZE);
    setBV(*bv);
}

void
IntNum::set(IntNum::USmallValue val)
{
    if (val > static_cast<USmallValue>(std::numeric_limits<SmallValue>::max()))
    {
        if (m_type == INTNUM_BV)
            *m_val.bv = val;
        else
        {
            m_val.bv = new APInt(BITVECT_NATIVE_SIZE, val);
            m_type = INTNUM_BV;
        }
    }
    else
    {
        if (m_type == INTNUM_BV)
        {
            delete m_val.bv;
            m_type = INTNUM_SV;
        }
        m_val.sv = static_cast<SmallValue>(val);
    }
}

int
IntNum::getSign() const
{
    if (m_type == INTNUM_SV)
    {
        if (m_val.sv == 0)
            return 0;
        else if (m_val.sv < 0)
            return -1;
        else
            return 1;
    }
    else if (m_val.bv->isNegative())
        return -1;
    else
        return 1;
}

unsigned long
IntNum::getUInt() const
{
    if (m_type == INTNUM_SV)
    {
        if (m_val.sv < 0)
            return 0;
        return static_cast<unsigned long>(m_val.sv);
    }

    // Handle bigval
    if (m_val.bv->isNegative())
        return 0;
    if (m_val.bv->getActiveBits() > ULONG_BITS)
        return ULONG_MAX;
    return static_cast<unsigned long>(m_val.bv->getZExtValue());
}

long
IntNum::getInt() const
{
    if (m_type == INTNUM_SV)
    {
        if (m_val.sv < LONG_MIN)
            return LONG_MIN;
        if (m_val.sv > LONG_MAX)
            return LONG_MAX;
        return m_val.sv;
    }

    // since it's a BV, it must be >0x7FFFFFFF or <0x80000000
    if (m_val.bv->isNegative())
        return LONG_MIN;
    return LONG_MAX;
}

bool
IntNum::isInt() const
{
    if (m_type != INTNUM_SV)
        return false;

    return (m_val.sv >= LONG_MIN && m_val.sv <= LONG_MAX);
}

bool
IntNum::isOkSize(unsigned int size, unsigned int rshift, int rangetype) const
{
    static const IntNumData::SmallValue SV_MIN =
        std::numeric_limits<IntNumData::SmallValue>::min();

    // Non-bigval (for speed)
    if (m_type == INTNUM_SV)
    {
        SmallValue one = 1;
        SmallValue v = m_val.sv;
        v >>= rshift;
        switch (rangetype)
        {
            case 0:
                if (v < 0)
                    return false;
                if (size >= SV_BITS)
                    return true;
                return (v < (one<<size));
            case 1:
                if (v < 0)
                {
                    if (size >= SV_BITS+1)
                        return true;
                    if (v == SV_MIN)
                        return size >= SV_BITS;
                    v = 0-v;
                    return (v <= (one<<(size-1)));
                }
                if (size >= SV_BITS+1)
                    return true;
                return (v < (one<<(size-1)));
            case 2:
                if (v < 0)
                {
                    if (size >= SV_BITS+1)
                        return true;
                    if (v == SV_MIN)
                        return size >= SV_BITS;
                    v = 0-v;
                    return (v <= (one<<(size-1)));
                }
                if (size >= SV_BITS)
                    return true;
                return (v < (one<<size));
            default:
                assert(false && "invalid range type");
                return false;
        }
    }
    return yasm::isOkSize(*getBV(&conv_bv), size, rshift, rangetype);
}

bool
IntNum::isInRange(long low, long high) const
{
    if (m_type == INTNUM_SV)
        return (m_val.sv >= low && m_val.sv <= high);
    // bigval can't be in range
    return false;
}

IntNum&
IntNum::operator++()
{
    if (m_type == INTNUM_SV &&
        m_val.sv < std::numeric_limits<SmallValue>::max())
        ++m_val.sv;
    else
    {
        if (m_type == INTNUM_SV)
        {
            m_val.bv = getBV(new APInt(BITVECT_NATIVE_SIZE, 0));
            m_type = INTNUM_BV;
        }
        ++(*m_val.bv);
    }
    return *this;
}

IntNum&
IntNum::operator--()
{
    if (m_type == INTNUM_SV &&
        m_val.sv > std::numeric_limits<SmallValue>::min())
        --m_val.sv;
    else
    {
        if (m_type == INTNUM_SV)
        {
            m_val.bv = getBV(new APInt(BITVECT_NATIVE_SIZE, 0));
            m_type = INTNUM_BV;
        }
        --(*m_val.bv);
    }
    return *this;
}

int
yasm::Compare(const IntNum& lhs, const IntNum& rhs)
{
    if (lhs.m_type == IntNum::INTNUM_SV && rhs.m_type == IntNum::INTNUM_SV)
    {
        if (lhs.m_val.sv < rhs.m_val.sv)
            return -1;
        if (lhs.m_val.sv > rhs.m_val.sv)
            return 1;
        return 0;
    }

    const APInt* op1 = lhs.getBV(&op1static);
    const APInt* op2 = rhs.getBV(&op2static);
    if (op1->slt(*op2))
        return -1;
    if (op1->sgt(*op2))
        return 1;
    return 0;
}

bool
yasm::operator==(const IntNum& lhs, const IntNum& rhs)
{
    if (lhs.m_type == IntNum::INTNUM_SV && rhs.m_type == IntNum::INTNUM_SV)
        return lhs.m_val.sv == rhs.m_val.sv;

    const APInt* op1 = lhs.getBV(&op1static);
    const APInt* op2 = rhs.getBV(&op2static);
    return op1->eq(*op2);
}

bool
yasm::operator<(const IntNum& lhs, const IntNum& rhs)
{
    if (lhs.m_type == IntNum::INTNUM_SV && rhs.m_type == IntNum::INTNUM_SV)
        return lhs.m_val.sv < rhs.m_val.sv;

    const APInt* op1 = lhs.getBV(&op1static);
    const APInt* op2 = rhs.getBV(&op2static);
    return op1->slt(*op2);
}

bool
yasm::operator>(const IntNum& lhs, const IntNum& rhs)
{
    if (lhs.m_type == IntNum::INTNUM_SV && rhs.m_type == IntNum::INTNUM_SV)
        return lhs.m_val.sv > rhs.m_val.sv;

    const APInt* op1 = lhs.getBV(&op1static);
    const APInt* op2 = rhs.getBV(&op2static);
    return op1->sgt(*op2);
}

void
IntNum::getStr(SmallVectorImpl<char>& str, int base, bool lowercase) const
{
    if (m_type == INTNUM_BV)
    {
        m_val.bv->toString(str, static_cast<unsigned>(base), true, false,
                           lowercase);
        return;
    }

    long v = m_val.sv;
    if (v < 0)
    {
        v = -v;
        str.push_back('-');
    }

    const char* fmt = "%ld";
    switch (base)
    {
        case 8:     fmt = "%lo"; break;
        case 10:    fmt = "%ld"; break;
        case 16:
            if (lowercase)
                fmt = "%lx";
            else
                fmt = "%lX";
            break;
        default:
            // fall back to bigval
            getBV(&conv_bv)->toString(str, static_cast<unsigned>(base), true,
                                      false, lowercase);
            return;
    }

    char s[40];
    std::sprintf(s, fmt, m_val.sv);
    str.append(s, s+std::strlen(s));
}

std::string
IntNum::getStr(int base, bool lowercase) const
{
    SmallString<40> s;
    getStr(s, base, lowercase);
    return s.str();
}

unsigned long
IntNum::Extract(unsigned int width, unsigned int lsb) const
{
    assert(width <= ULONG_BITS);
    if (m_type == INTNUM_SV)
    {
        // cast after shift to preserve sign bits
        unsigned long rv = static_cast<unsigned long>(m_val.sv >> lsb);
        if (width < ULONG_BITS)
            rv &= ((1UL << width) - 1);
        return rv;
    }
    else
    {
        uint64_t v;
        APInt::tcExtract(&v, 1, m_val.bv->getRawData(), width, lsb);
        return static_cast<unsigned long>(v);
    }
}

#ifdef WITH_XML
pugi::xml_node
IntNum::Write(pugi::xml_node out) const
{
    SmallString<40> s;
    getStr(s);
    s += '\0';
    return append_data(out, s.str().data());
}
#endif // WITH_XML

void
IntNum::Print(raw_ostream& os,
              int base,
              bool lowercase,
              bool showbase,
              int bits) const
{
    const APInt* bv = getBV(&conv_bv);

    if (bv->isNegative())
    {
        // negate in place
        conv_bv = *bv;
        conv_bv.flipAllBits();
        ++conv_bv;
        bv = &conv_bv;
        os << '-';
    }

    SmallString<40> s;
    bv->toString(s, base, true, false, lowercase);

    // prefix and 0 padding, if required
    int padding = 0;
    if (base == 2)
    {
        if (showbase)
            os << '0' << (lowercase ? 'b' : 'B');
        if (bits > 0)
            padding = bits-s.size();
    }
    else if (base == 8)
    {
        if (showbase)
            os << '0';
        if (bits > 0)
            padding = (bits-s.size()*3+2)/3;
    }
    else if (base == 16)
    {
        if (showbase)
            os << '0' << (lowercase ? 'x' : 'X');
        if (bits > 0)
            padding = (bits-s.size()*4+3)/4;
    }
    for (int i=0; i<padding; ++i)
        os << '0';

    // value
    os << s.str();
}

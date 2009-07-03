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
#include <climits>
#include <cstdlib>
#include <cstring>

#include "llvm/ADT/SmallString.h"
#include "yasmx/Support/Compose.h"
#include "yasmx/Support/errwarn.h"


namespace yasm
{

YASM_LIB_EXPORT const int set_intnum_bits::m_idx = std::ios_base::xalloc();

/// Static bitvect used for conversions.
static llvm::APInt conv_bv(IntNum::BITVECT_NATIVE_SIZE, 0);

/// Static bitvects used for computation.
static llvm::APInt result(IntNum::BITVECT_NATIVE_SIZE, 0);
static llvm::APInt spare(IntNum::BITVECT_NATIVE_SIZE, 0);
static llvm::APInt op1static(IntNum::BITVECT_NATIVE_SIZE, 0);
static llvm::APInt op2static(IntNum::BITVECT_NATIVE_SIZE, 0);

bool
isOkSize(const llvm::APInt& intn,
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
IntNum::setBV(const llvm::APInt& bv)
{
    if (bv.getMinSignedBits() <= LONG_BITS)
    {
        if (m_type == INTNUM_BV)
            delete m_val.bv;
        m_type = INTNUM_L;
        m_val.l = static_cast<long>(bv.getSExtValue());
        return;
    }
    else if (m_type == INTNUM_BV)
    {
        *m_val.bv = bv;
    }
    else
    {
        m_type = INTNUM_BV;
        m_val.bv = new llvm::APInt(bv);
    }
    m_val.bv->sextOrTrunc(BITVECT_NATIVE_SIZE);
}

const llvm::APInt*
IntNum::getBV(llvm::APInt* bv) const
{
    if (m_type == INTNUM_BV)
        return m_val.bv;

    if (m_val.l >= 0)
        *bv = static_cast<unsigned long>(m_val.l);
    else
    {
        *bv = static_cast<unsigned long>(~m_val.l);
        bv->flip();
    }
    return bv;
}

llvm::APInt*
IntNum::getBV(llvm::APInt* bv)
{
    if (m_type == INTNUM_BV)
        return m_val.bv;

    if (m_val.l >= 0)
        *bv = static_cast<unsigned long>(m_val.l);
    else
    {
        *bv = static_cast<unsigned long>(~m_val.l);
        bv->flip();
    }
    return bv;
}

void
IntNum::setStr(const char* str, unsigned int len, int base)
{
    // Each computation below needs to know if its negative
    unsigned int minbits = (str[0] == '-') ? 1 : 0;

    // For radixes of power-of-two values, the bits required is accurately and
    // easily computed.  For radix 10, we use a rough approximation.
    switch (base)
    {
        case 2:     minbits += (len-minbits); break;
        case 8:     minbits += (len-minbits) * 3; break;
        case 16:    minbits += (len-minbits) * 4; break;
        case 10:    minbits += ((len-minbits) * 64) / 22; break;
        default:    assert(false && "Invalid radix");
    }
    if (minbits > BITVECT_NATIVE_SIZE)
    {
        throw OverflowError(
            N_("Numeric constant too large for internal format"));
    }

    if (minbits < LONG_BITS)
    {
        // shortcut "short" case
        long v;
        char cstr[LONG_BITS+1];
        std::strncpy(cstr, str, len);   // len must be < LONG_BITS per minbits
        cstr[len] = '\0';
        v = std::strtol(cstr, NULL, base);
        set(v);
    }
    else
    {
        conv_bv.fromString(str, len, base);
        setBV(conv_bv);
    }
}

IntNum::IntNum(const IntNum& rhs)
{
    m_type = rhs.m_type;
    if (rhs.m_type == INTNUM_BV)
        m_val.bv = new llvm::APInt(*rhs.m_val.bv);
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
CalcLong(Op::Op op, long* lhs, long rhs)
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
            if (*lhs > -(1L<<(IntNum::LONG_BITS/2))
                && *lhs < (1L<<(IntNum::LONG_BITS/2)))
            {
                if (rhs <= -(1L<<(IntNum::LONG_BITS/2))
                    || rhs >= (1L<<(IntNum::LONG_BITS/2)))
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
IntNum::Calc(Op::Op op, const IntNum* operand)
{
    if (!operand && op != Op::NEG && op != Op::NOT && op != Op::LNOT)
        throw ArithmeticError(N_("operation needs an operand"));

    if (m_type == INTNUM_L && (!operand || operand->m_type == INTNUM_L))
    {
        if (CalcLong(op, &m_val.l, operand ? operand->m_val.l : 0))
            return;
    }

    // Always do computations with in full bit vector.
    // Bit vector results must be calculated through intermediate storage.
    const llvm::APInt* op1 = getBV(&op1static);
    const llvm::APInt* op2 = 0;
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
                throw ZeroDivisionError(N_("divide by zero"));
            else
                result = op1->udiv(*op2);
            break;
        case Op::SIGNDIV:
            if (!*op2)
                throw ZeroDivisionError(N_("divide by zero"));
            else
                result = op1->sdiv(*op2);
            break;
        case Op::MOD:
            // TODO: make sure op1 and op2 are unsigned
            if (!*op2)
                throw ZeroDivisionError(N_("divide by zero"));
            else
                result = op1->urem(*op2);
            break;
        case Op::SIGNMOD:
            if (!*op2)
                throw ZeroDivisionError(N_("divide by zero"));
            else
                result = op1->srem(*op2);
            break;
        case Op::NEG:
            result = -*op1;
            break;
        case Op::NOT:
            result = *op1;
            result.flip();
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
            result.flip();
            break;
        case Op::NOR:
            result = *op1;
            result |= *op2;
            result.flip();
            break;
        case Op::SHL:
            if (operand->m_type == INTNUM_L)
            {
                if (operand->m_val.l >= 0)
                    result = op1->shl(operand->m_val.l);
                else
                    result = op1->ashr(-operand->m_val.l);
            }
            else    // don't even bother, just zero result
                result.clear();
            break;
        case Op::SHR:
            if (operand->m_type == INTNUM_L)
            {
                if (operand->m_val.l >= 0)
                    result = op1->ashr(operand->m_val.l);
                else
                    result = op1->shl(-operand->m_val.l);
            }
            else    // don't even bother, just zero result
                result.clear();
            break;
        case Op::LOR:
            set(static_cast<long>((!!*op1) || (!!*op2)));
            return;
        case Op::LAND:
            set(static_cast<long>((!!*op1) && (!!*op2)));
            return;
        case Op::LNOT:
            set(static_cast<long>(!*op1));
            return;
        case Op::LXOR:
            set(static_cast<long>((!!*op1) ^ (!!*op2)));
            return;
        case Op::LXNOR:
            set(static_cast<long>(!((!!*op1) ^ (!!*op2))));
            return;
        case Op::LNOR:
            set(static_cast<long>(!((!!*op1) || (!!*op2))));
            return;
        case Op::EQ:
            set(static_cast<long>(op1->eq(*op2)));
            return;
        case Op::LT:
            set(static_cast<long>(op1->slt(*op2)));
            return;
        case Op::GT:
            set(static_cast<long>(op1->sgt(*op2)));
            return;
        case Op::LE:
            set(static_cast<long>(op1->sle(*op2)));
            return;
        case Op::GE:
            set(static_cast<long>(op1->sge(*op2)));
            return;
        case Op::NE:
            set(static_cast<long>(op1->ne(*op2)));
            return;
        case Op::SEG:
            throw ArithmeticError(String::Compose(N_("invalid use of '%1'"),
                                                  "SEG"));
            break;
        case Op::WRT:
            throw ArithmeticError(String::Compose(N_("invalid use of '%1'"),
                                                  "WRT"));
            break;
        case Op::SEGOFF:
            throw ArithmeticError(String::Compose(N_("invalid use of '%1'"),
                                                  ":"));
            break;
        case Op::IDENT:
            result = *op1;
            break;
        default:
            throw ArithmeticError(
                N_("invalid operation in intnum calculation"));
    }

    // Try to fit the result into long if possible
    setBV(result);
}
/*@=nullderef =nullpass =branchstate@*/

void
IntNum::set(unsigned long val)
{
    if (val > LONG_MAX)
    {
        if (m_type == INTNUM_BV)
            *m_val.bv = val;
        else
        {
            m_val.bv = new llvm::APInt(BITVECT_NATIVE_SIZE, val);
            m_type = INTNUM_BV;
        }
    }
    else
    {
        if (m_type == INTNUM_BV)
        {
            delete m_val.bv;
            m_type = INTNUM_L;
        }
        m_val.l = static_cast<long>(val);
    }
}

int
IntNum::getSign() const
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
    else if (m_val.bv->isNegative())
        return -1;
    else
        return 1;
}

unsigned long
IntNum::getUInt() const
{
    if (m_type == INTNUM_L)
    {
        if (m_val.l < 0)
            return 0;
        return static_cast<unsigned long>(m_val.l);
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
    if (m_type == INTNUM_L)
        return m_val.l;

    // since it's a BV, it must be >0x7FFFFFFF or <0x80000000
    if (m_val.bv->isNegative())
        return LONG_MIN;
    return LONG_MAX;
}

bool
IntNum::isOkSize(unsigned int size, unsigned int rshift, int rangetype) const
{
    // Non-bigval (for speed)
    if (m_type == INTNUM_L)
    {
        long v = m_val.l;
        v >>= rshift;
        switch (rangetype)
        {
            case 0:
                if (v < 0)
                    return false;
                if (size >= IntNum::LONG_BITS)
                    return true;
                return (v < (1L<<size));
            case 1:
                if (v < 0)
                {
                    if (size >= IntNum::LONG_BITS+1)
                        return true;
                    v = 0-v;
                    return (v <= (1L<<(size-1)));
                }
                if (size >= IntNum::LONG_BITS+1)
                    return true;
                return (v < (1L<<(size-1)));
            case 2:
                if (v < 0)
                {
                    if (size >= IntNum::LONG_BITS+1)
                        return true;
                    v = 0-v;
                    return (v <= (1L<<(size-1)));
                }
                if (size >= IntNum::LONG_BITS)
                    return true;
                return (v < (1L<<size));
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
    if (m_type == INTNUM_L)
        return (m_val.l >= low && m_val.l <= high);
    // bigval can't be in range
    return false;
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
            m_val.bv = getBV(new llvm::APInt(BITVECT_NATIVE_SIZE, 0));
            m_type = INTNUM_BV;
        }
        ++(*m_val.bv);
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
            m_val.bv = getBV(new llvm::APInt(BITVECT_NATIVE_SIZE, 0));
            m_type = INTNUM_BV;
        }
        --(*m_val.bv);
    }
    return *this;
}

int
Compare(const IntNum& lhs, const IntNum& rhs)
{
    if (lhs.m_type == IntNum::INTNUM_L && rhs.m_type == IntNum::INTNUM_L)
    {
        if (lhs.m_val.l < rhs.m_val.l)
            return -1;
        if (lhs.m_val.l > rhs.m_val.l)
            return 1;
        return 0;
    }

    const llvm::APInt* op1 = lhs.getBV(&op1static);
    const llvm::APInt* op2 = rhs.getBV(&op2static);
    if (op1->slt(*op2))
        return -1;
    if (op1->sgt(*op2))
        return 1;
    return 0;
}

bool
operator==(const IntNum& lhs, const IntNum& rhs)
{
    if (lhs.m_type == IntNum::INTNUM_L && rhs.m_type == IntNum::INTNUM_L)
        return lhs.m_val.l == rhs.m_val.l;

    const llvm::APInt* op1 = lhs.getBV(&op1static);
    const llvm::APInt* op2 = rhs.getBV(&op2static);
    return op1->eq(*op2);
}

bool
operator<(const IntNum& lhs, const IntNum& rhs)
{
    if (lhs.m_type == IntNum::INTNUM_L && rhs.m_type == IntNum::INTNUM_L)
        return lhs.m_val.l < rhs.m_val.l;

    const llvm::APInt* op1 = lhs.getBV(&op1static);
    const llvm::APInt* op2 = rhs.getBV(&op2static);
    return op1->slt(*op2);
}

bool
operator>(const IntNum& lhs, const IntNum& rhs)
{
    if (lhs.m_type == IntNum::INTNUM_L && rhs.m_type == IntNum::INTNUM_L)
        return lhs.m_val.l > rhs.m_val.l;

    const llvm::APInt* op1 = lhs.getBV(&op1static);
    const llvm::APInt* op2 = rhs.getBV(&op2static);
    return op1->sgt(*op2);
}

void
IntNum::getStr(llvm::SmallVectorImpl<char>& str,
               int base,
               bool lowercase) const
{
    if (m_type == INTNUM_BV)
    {
        m_val.bv->toString(str, static_cast<unsigned>(base), true, lowercase);
        return;
    }

    long v = m_val.l;
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
            assert(false && "invalid base");
    }

    char s[40];
    sprintf(s, fmt, m_val.l);
    str.append(s, s+strlen(s));
}

std::string
IntNum::getStr(int base, bool lowercase) const
{
    llvm::SmallString<40> s;
    getStr(s, base, lowercase);
    return s.c_str();
}

unsigned long
IntNum::Extract(unsigned int width, unsigned int lsb) const
{
    assert(width <= ULONG_BITS);
    if (m_type == INTNUM_L)
    {
        // cast after shift to preserve sign bits
        return (static_cast<unsigned long>(m_val.l >> lsb)
                & ((1UL << width) - 1));
    }
    else
    {
        uint64_t v;
        llvm::APInt::tcExtract(&v, 1, m_val.bv->getRawData(), width, lsb);
        return static_cast<unsigned long>(v);
    }
}

std::ostream&
operator<< (std::ostream& os, const IntNum& intn)
{
    llvm::SmallString<40> s;
    int bits = os.iword(set_intnum_bits::m_idx);
    int padding = 0;
    conv_bv = *intn.getBV(&conv_bv);

    std::ios_base::fmtflags ff = os.flags();
    if ((ff & os.showpos) && (ff & os.dec) && !conv_bv.isNegative())
        os << '+';

    if (conv_bv.isNegative())
    {
        // negate in place
        conv_bv.flip();
        ++conv_bv;
        os << '-';
    }

    if (ff & os.oct)
    {
        if (ff & os.showbase)
            os << '0';
        conv_bv.toString(s, 8, true);
        padding = (bits-s.size()*3+2)/3;
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
        conv_bv.toString(s, 16, true, (ff & os.uppercase) == 0);
        padding = (bits-s.size()*4+3)/4;
    }
    else
        conv_bv.toString(s, 10, true);
    for (int i=0; i<padding; ++i)
        os << '0';
    os << s.c_str();
    return os;
}

} // namespace yasm

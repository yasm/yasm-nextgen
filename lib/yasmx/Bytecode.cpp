//
// YASM bytecode implementation
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
#define DEBUG_TYPE "Bytecode"

#include "yasmx/Bytecode.h"

#include "util.h"

#include "llvm/ADT/Statistic.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/marg_ostream.h"
#include "yasmx/BytecodeContainer.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytes.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/Location_util.h"
#include "yasmx/Object.h"
#include "yasmx/Op.h"
#include "yasmx/Symbol.h"
#include "yasmx/Value.h"


STATISTIC(num_fixed_value, "Total number of values appended to fixed portions");
STATISTIC(num_output, "Number of bytecodes output");
STATISTIC(fixed_output, "Total number of fixed bytes output");
STATISTIC(tail_output, "Total number of tail bytes output");

namespace yasm
{

Bytecode::Contents::Contents()
{
}

Bytecode::Contents::~Contents()
{
}

bool
Bytecode::Contents::Expand(Bytecode& bc,
                           unsigned long& len,
                           int span,
                           long old_val,
                           long new_val,
                           /*@out@*/ long* neg_thres,
                           /*@out@*/ long* pos_thres)
{
    assert(false && "bytecode does not have any dependent spans");
    return false;
}

Bytecode::Contents::SpecialType
Bytecode::Contents::getSpecial() const
{
    return SPECIAL_NONE;
}

Bytecode::Contents::Contents(const Contents& rhs)
{
}

void
Bytecode::Transform(std::auto_ptr<Contents> contents)
{
    m_contents.reset(contents.release());
}

Bytecode::Bytecode(std::auto_ptr<Contents> contents, unsigned long line)
    : m_contents(contents),
      m_container(0),
      m_len(0),
      m_line(line),
      m_offset(0),
      m_index(~0UL)
{
}

Bytecode::Bytecode()
    : m_contents(0),
      m_container(0),
      m_len(0),
      m_line(0),
      m_offset(0),
      m_index(~0UL)
{
}

Bytecode::Bytecode(const Bytecode& oth)
    : m_contents(oth.m_contents->clone()),
      m_container(oth.m_container),
      m_len(oth.m_len),
      m_line(oth.m_line),
      m_offset(oth.m_offset),
      m_index(oth.m_index),
      m_symbols(oth.m_symbols)
{}

Bytecode::~Bytecode()
{
}

void
Bytecode::swap(Bytecode& oth)
{
    m_fixed.swap(oth.m_fixed);
    m_fixed_fixups.swap(oth.m_fixed_fixups);
    m_contents.swap(oth.m_contents);
    std::swap(m_container, oth.m_container);
    std::swap(m_len, oth.m_len);
    std::swap(m_line, oth.m_line);
    std::swap(m_offset, oth.m_offset);
    std::swap(m_index, oth.m_index);
    m_symbols.swap(oth.m_symbols);
}

marg_ostream&
operator<< (marg_ostream &os, const Bytecode& bc)
{
    if (bc.m_fixed.size() > 0)
    {
        os << "Fixed: ";
        os << bc.m_fixed;
    }
    if (bc.m_contents.get() != 0)
        os << *bc.m_contents;
    else
        os << "EMPTY\n";
    os << "Length=" << bc.m_len << '\n';
    os << "Line Index=" << bc.m_line << '\n';
    os << "Offset=" << bc.m_offset << '\n';
    return os;
}

void
Bytecode::Finalize()
{
    for (std::vector<Fixup>::iterator i=m_fixed_fixups.begin(),
         end=m_fixed_fixups.end(); i != end; ++i)
    {
        try
        {
            if (!i->Finalize())
            {
                if (i->isJumpTarget())
                    throw TooComplexError(N_("jump target expression too complex"));
                else
                    throw TooComplexError(N_("expression too complex"));
            }

            if (i->isJumpTarget() && i->isComplexRelative())
                throw ValueError(N_("invalid jump target"));
        }
        catch (Error& err)
        {
            // associate the error with the value, not the bytecode, line
            err.m_line = i->getLine();
            throw;
        }

        // Do curpos subtraction for IP-relative flagged values.
        if (i->isIPRelative())
        {
            Location sub_loc = {this, i->getOffset()};
            i->SubRelative(m_container->getObject(), sub_loc);
        }

        WarnUpdateLine(i->getLine());
    }

    if (m_contents.get() != 0)
        m_contents->Finalize(*this);
}

void
Bytecode::CalcLen(const AddSpanFunc& add_span)
{
    if (m_contents.get() == 0)
    {
        m_len = 0;
        return;
    }
    m_len = m_contents->CalcLen(*this, add_span);
}

bool
Bytecode::Expand(int span,
                 long old_val,
                 long new_val,
                 /*@out@*/ long* neg_thres,
                 /*@out@*/ long* pos_thres)
{
    if (m_contents.get() == 0)
        return false;
    return m_contents->Expand(*this, m_len, span, old_val, new_val, neg_thres,
                              pos_thres);
}

void
Bytecode::Output(BytecodeOutput& bc_out)
{
    unsigned long start = bc_out.getNumOutput();

    // output fixups, outputting the fixed portions in between each one
    unsigned int last = 0;
    for (std::vector<Fixup>::iterator i=m_fixed_fixups.begin(),
         end=m_fixed_fixups.end(); i != end; ++i)
    {
        unsigned int off = i->getOffset();
        Location loc = {this, off};

        // Output fixed portion.
        Bytes& fixed = bc_out.getScratch();
        fixed.insert(fixed.end(), m_fixed.begin() + last,
                     m_fixed.begin() + off);
        bc_out.Output(fixed);

        // Output value.
        Bytes& vbytes = bc_out.getScratch();
        vbytes.insert(vbytes.end(), m_fixed.begin() + off,
                      m_fixed.begin() + off + i->getSize()/8);
        // Make a copy of the value to ensure things like
        // "TIMES x JMP label" work.
        Value vcopy = *i;
        try
        {
            bc_out.Output(vcopy, vbytes, loc, i->isSigned() ? -1 : 1);
        }
        catch (Error& err)
        {
            // associate the error with the value, not the bytecode, line
            err.m_line = vcopy.getLine();
            throw;
        }
        WarnUpdateLine(vcopy.getLine());

        last = off + i->getSize()/8;
    }
    // handle last part of fixed
    if (last < m_fixed.size())
    {
        Bytes& fixed = bc_out.getScratch();
        fixed.insert(fixed.end(), m_fixed.begin() + last, m_fixed.end());
        bc_out.Output(fixed);
    }

    start = start;  // avoid warning due to assert usage
    assert((bc_out.getNumOutput() - start) == getFixedLen() &&
           "failed to output correct number of fixed bytes");

    // handle tail contents
    if (m_contents.get() != 0)
        m_contents->Output(*this, bc_out);

    assert((bc_out.getNumOutput() - start) == getTotalLen() &&
           "failed to output correct number of bytes");

    ++num_output;
    fixed_output += getFixedLen();
    tail_output += getTailLen();
}

unsigned long
Bytecode::UpdateOffset(unsigned long offset)
{
    if (m_contents.get() != 0 &&
        m_contents->getSpecial() == Contents::SPECIAL_OFFSET)
    {
        // Recalculate/adjust len of offset-based bytecodes here
        long neg_thres = 0;
        long pos_thres = static_cast<long>(getNextOffset());
        Expand(1, 0, static_cast<long>(offset+getFixedLen()), &neg_thres,
               &pos_thres);
    }
    m_offset = offset;
    return getNextOffset();
}

void
Bytecode::AppendFixed(const Value& val)
{
    unsigned int valsize = val.getSize()/8;
    m_fixed_fixups.push_back(Fixup(m_fixed.size(), val));
    m_fixed.Write(valsize, 0);
    ++num_fixed_value;
}

void
Bytecode::AppendFixed(std::auto_ptr<Value> val)
{
    unsigned int valsize = val->getSize()/8;
    m_fixed_fixups.push_back(Fixup(m_fixed.size(), val));
    m_fixed.Write(valsize, 0);
    ++num_fixed_value;
}

void
Bytecode::AppendFixed(unsigned int size,
                      std::auto_ptr<Expr> e,
                      unsigned long line)
{
    m_fixed_fixups.push_back(Fixup(m_fixed.size(), size*8, e, line));
    m_fixed.Write(size, 0);
    ++num_fixed_value;
}

Bytecode::Fixup::Fixup(unsigned int off, const Value& val)
    : Value(val), m_off(off)
{
}

Bytecode::Fixup::Fixup(unsigned int off, std::auto_ptr<Value> val)
    : Value(0), m_off(off)
{
    Value::swap(*val);
}

Bytecode::Fixup::Fixup(unsigned int off,
                       unsigned int size,
                       std::auto_ptr<Expr> e,
                       unsigned long line)
    : Value(size, e), m_off(off)
{
    setLine(line);
}

void
Bytecode::Fixup::swap(Fixup& oth)
{
    Value::swap(oth);
    std::swap(m_off, oth.m_off);
}

} // namespace yasm

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
#include "yasmx/Bytecode.h"

#include "yasmx/Basic/Diagnostic.h"
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

Bytecode::Contents::Contents()
{
}

Bytecode::Contents::~Contents()
{
}

bool
Bytecode::Contents::Expand(Bytecode& bc,
                           unsigned long* len,
                           int span,
                           long old_val,
                           long new_val,
                           bool* keep,
                           /*@out@*/ long* neg_thres,
                           /*@out@*/ long* pos_thres,
                           Diagnostic& diags)
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

Bytecode::Bytecode(std::auto_ptr<Contents> contents, SourceLocation source)
    : m_contents(contents),
      m_container(0),
      m_len(0),
      m_source(source),
      m_offset(0),
      m_index(~0UL)
{
}

Bytecode::Bytecode()
    : m_contents(0),
      m_container(0),
      m_len(0),
      m_offset(0),
      m_index(~0UL)
{
}

Bytecode::Bytecode(const Bytecode& oth)
    : m_contents(oth.m_contents->clone()),
      m_container(oth.m_container),
      m_len(oth.m_len),
      m_source(oth.m_source),
      m_offset(oth.m_offset),
      m_index(oth.m_index)
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
    std::swap(m_source, oth.m_source);
    std::swap(m_offset, oth.m_offset);
    std::swap(m_index, oth.m_index);
}

bool
Bytecode::Finalize(Diagnostic& diags)
{
    for (std::vector<Fixup>::iterator i=m_fixed_fixups.begin(),
         end=m_fixed_fixups.end(); i != end; ++i)
    {
        if (!i->Finalize(diags, i->isJumpTarget() ? diag::err_too_complex_jump :
                         diag::err_too_complex_expression))
            return false;

        if (i->isJumpTarget() && i->isComplexRelative())
        {
            diags.Report(i->getSource().getBegin(),
                         diag::err_invalid_jump_target);
            return false;
        }

        // Do curpos subtraction for IP-relative flagged values.
        if (i->isIPRelative())
        {
            Location sub_loc = {this, i->getOffset()};
            if (!i->SubRelative(m_container->getObject(), sub_loc))
                diags.Report(i->getSource().getBegin(),
                             diag::err_too_complex_expression);
        }
    }

    if (m_contents.get() != 0)
        return m_contents->Finalize(*this, diags);
    return true;
}

bool
Bytecode::CalcLen(const AddSpanFunc& add_span, Diagnostic& diags)
{
    if (m_contents.get() == 0)
    {
        m_len = 0;
        return true;
    }
    unsigned long len;
    if (!m_contents->CalcLen(*this, &len, add_span, diags))
        return false;
    m_len = len;
    return true;
}

bool
Bytecode::Expand(int span,
                 long old_val,
                 long new_val,
                 bool* keep,
                 /*@out@*/ long* neg_thres,
                 /*@out@*/ long* pos_thres,
                 Diagnostic& diags)
{
    if (m_contents.get() == 0)
        return true;
    unsigned long len = m_len;
    if (!m_contents->Expand(*this, &len, span, old_val, new_val, keep,
                            neg_thres, pos_thres, diags))
        return false;
    m_len = len;
    return true;
}

bool
Bytecode::Output(BytecodeOutput& bc_out)
{
    unsigned long start = bc_out.getNumOutput();

    // make a copy of fixed portion
    Bytes& fixed = bc_out.m_bc_scratch;
    fixed.resize(0);
    fixed.insert(fixed.end(), m_fixed.begin(), m_fixed.end());

    // apply fixups
    for (std::vector<Fixup>::iterator i=m_fixed_fixups.begin(),
         end=m_fixed_fixups.end(); i != end; ++i)
    {
        unsigned int off = i->getOffset();
        unsigned int size = (i->getSize()+i->getShift()+7)/8;
        Location loc = {this, off};

        // Get bytes to be updated
        Bytes& bytes = bc_out.getScratch();
        assert((off+size) <= fixed.size());
        bytes.insert(bytes.end(), fixed.begin() + off,
                     fixed.begin() + off + size);

        // Make a copy of the value to ensure things like
        // "TIMES x JMP label" work.
        Value vcopy = *i;

        // Convert the value to bytes.
        NumericOutput num_out(bytes);
        i->ConfigureOutput(&num_out);
        if (!bc_out.ConvertValueToBytes(vcopy, loc, num_out))
            return false;
        num_out.EmitWarnings(bc_out.getDiagnostics());

        // Update bytes
        std::copy(bytes.begin(), bytes.end(), fixed.begin() + off);
    }

    // output fixed portion
    bc_out.OutputBytes(fixed, m_source);

    start = start;  // avoid warning due to assert usage
    assert((bc_out.getNumOutput() - start) == getFixedLen() &&
           "failed to output correct number of fixed bytes");

    // handle tail contents
    if (m_contents.get() != 0)
    {
        if (!m_contents->Output(*this, bc_out))
            return false;
    }

    assert((bc_out.getNumOutput() - start) == getTotalLen() &&
           "failed to output correct number of bytes");

    return true;
}

unsigned long
Bytecode::UpdateOffset(unsigned long offset, Diagnostic& diags)
{
    if (m_contents.get() != 0 &&
        m_contents->getSpecial() == Contents::SPECIAL_OFFSET)
    {
        // Recalculate/adjust len of offset-based bytecodes here
        bool keep = false;
        long neg_thres = 0;
        long pos_thres = static_cast<long>(getNextOffset());
        Expand(1, 0, static_cast<long>(offset+getFixedLen()), &keep,
               &neg_thres, &pos_thres, diags);
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
}

void
Bytecode::AppendFixed(std::auto_ptr<Value> val)
{
    unsigned int valsize = val->getSize()/8;
    m_fixed_fixups.push_back(Fixup(m_fixed.size(), val));
    m_fixed.Write(valsize, 0);
}

Value&
Bytecode::AppendFixed(unsigned int size,
                      std::auto_ptr<Expr> e,
                      SourceLocation source)
{
    m_fixed_fixups.push_back(Fixup(m_fixed.size(), size*8, e, source));
    m_fixed.Write(size, 0);
    return m_fixed_fixups.back();
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
                       SourceLocation source)
    : Value(size, e), m_off(off)
{
    setSource(source);
}

void
Bytecode::Fixup::swap(Fixup& oth)
{
    Value::swap(oth);
    std::swap(m_off, oth.m_off);
}

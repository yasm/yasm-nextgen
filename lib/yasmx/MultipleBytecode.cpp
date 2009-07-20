///
/// Multiple bytecode wrapper and container
///
///  Copyright (C) 2001-2007  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
/// 1. Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
/// 2. Redistributions in binary form must reproduce the above copyright
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
///
#include "yasmx/BytecodeContainer_util.h"

#include "util.h"

#include "YAML/emitter.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/BytecodeContainer.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/Location_util.h"


namespace
{

using namespace yasm;

class MultipleBytecode : public Bytecode::Contents
{
public:
    MultipleBytecode(std::auto_ptr<Expr> e);
    ~MultipleBytecode();

    /// Finalizes the bytecode after parsing.
    void Finalize(Bytecode& bc);

    /// Calculates the minimum size of a bytecode.
    unsigned long CalcLen(Bytecode& bc, const Bytecode::AddSpanFunc& add_span);

    /// Recalculates the bytecode's length based on an expanded span
    /// length.
    bool Expand(Bytecode& bc,
                unsigned long& len,
                int span,
                long old_val,
                long new_val,
                /*@out@*/ long* neg_thres,
                /*@out@*/ long* pos_thres);

    /// Convert a bytecode into its byte representation.
    void Output(Bytecode& bc, BytecodeOutput& bc_out);

    MultipleBytecode* clone() const;

    /// Write a YAML representation.  For debugging purposes.
    void Write(YAML::Emitter& out) const;

    BytecodeContainer& getContents() { return m_contents; }

private:
    /// Number of times contents is repeated.
    std::auto_ptr<Expr> m_multiple;

    /// Number of times contents is repeated, integer version.
    long m_mult_int;

    /// Contents to be repeated.
    BytecodeContainer m_contents;
};


MultipleBytecode::MultipleBytecode(std::auto_ptr<Expr> e)
    : m_multiple(e),
      m_mult_int(0)
{
}

MultipleBytecode::~MultipleBytecode()
{
}

void
MultipleBytecode::Finalize(Bytecode& bc)
{
    Value val(0, std::auto_ptr<Expr>(m_multiple->clone()));

    if (!val.Finalize())
        throw TooComplexError(N_("multiple expression too complex"));
    else if (val.isRelative())
        throw NotAbsoluteError(N_("multiple expression not absolute"));
    // Finalize creates NULL output if value=0, but bc->multiple is NULL
    // if value=1 (this difference is to make the common case small).
    // However, this means we need to set bc->multiple explicitly to 0
    // here if val.abs is NULL.
    if (const Expr* e = val.getAbs())
        m_multiple.reset(e->clone());
    else
        m_multiple.reset(new Expr(0));

    for (BytecodeContainer::bc_iterator i = m_contents.bytecodes_begin(),
         end = m_contents.bytecodes_end(); i != end; ++i)
    {
        if (i->getSpecial() == Bytecode::Contents::SPECIAL_OFFSET)
            throw ValueError(N_("cannot combine multiples and setting assembly position"));
        i->Finalize();
    }
}

unsigned long
MultipleBytecode::CalcLen(Bytecode& bc, const Bytecode::AddSpanFunc& add_span)
{
    // Calculate multiple value as an integer
    m_mult_int = 1;
    if (m_multiple->isIntNum())
    {
        IntNum num = m_multiple->getIntNum();
        if (num.getSign() < 0)
        {
            m_mult_int = 0;
            throw ValueError(N_("multiple is negative"));
        }
        else
            m_mult_int = num.getInt();
    }
    else
    {
        if (m_multiple->Contains(ExprTerm::FLOAT))
        {
            m_mult_int = 0;
            throw ValueError(
                N_("expression must not contain floating point value"));
        }
        else
        {
            Value value(0, Expr::Ptr(m_multiple->clone()));
            add_span(bc, 0, value, 0, 0);
            m_mult_int = 0;     // assume 0 to start
        }
    }

    unsigned long len = 0;
    for (BytecodeContainer::bc_iterator i = m_contents.bytecodes_begin(),
         end = m_contents.bytecodes_end(); i != end; ++i)
    {
        i->CalcLen(add_span);
        len += i->getTotalLen();
    }

    return len * m_mult_int;
}

bool
MultipleBytecode::Expand(Bytecode& bc,
                         unsigned long& len,
                         int span,
                         long old_val,
                         long new_val,
                         /*@out@*/ long* neg_thres,
                         /*@out@*/ long* pos_thres)
{
    if (span == 0)
    {
        m_mult_int = new_val;
        return true;
    }
    // XXX: support more than one bytecode here
    return m_contents.bytecodes_first().Expand(span, old_val, new_val,
                                               neg_thres, pos_thres);
}

void
MultipleBytecode::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    SimplifyCalcDist(*m_multiple);
    if (!m_multiple->isIntNum())
        throw ValueError(N_("could not determine multiple"));
    IntNum num = m_multiple->getIntNum();
    if (num.getSign() < 0)
        throw ValueError(N_("multiple is negative"));
    m_mult_int = num.getInt();
    if (m_mult_int == 0)
        return; // nothing to output

    unsigned long total_len = 0;
    unsigned long pos = 0;
    for (long mult=0; mult<m_mult_int; mult++, pos += total_len)
    {
        for (BytecodeContainer::bc_iterator i = m_contents.bytecodes_begin(),
             end = m_contents.bytecodes_end(); i != end; ++i)
        {
            i->Output(bc_out);
        }
    }
}

MultipleBytecode*
MultipleBytecode::clone() const
{
    // TODO: cloning
    assert(false);
    return 0;
}

void
MultipleBytecode::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << "Multiple";
    out << YAML::Key << "multiple" << YAML::Value << *m_multiple;
    out << YAML::Key << "multiple int" << YAML::Value << m_mult_int;
    out << YAML::Key << "contents" << YAML::Value << m_contents;
    out << YAML::EndMap;
}

} // anonymous namespace

namespace yasm
{

BytecodeContainer&
AppendMultiple(BytecodeContainer& container,
               std::auto_ptr<Expr> multiple,
               unsigned long line)
{
    Bytecode& bc = container.FreshBytecode();
    MultipleBytecode* multbc(new MultipleBytecode(multiple));
    BytecodeContainer& retval = multbc->getContents();
    bc.Transform(Bytecode::Contents::Ptr(multbc));
    bc.setLine(line);
    return retval;
}

} // namespace yasm

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
#include "yasmx/BytecodeContainer.h"

#include "util.h"

#include "YAML/emitter.h"
#include "yasmx/Bytecode.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/Location_util.h"


using namespace yasm;

namespace {
class MultipleBytecode : public Bytecode::Contents
{
public:
    MultipleBytecode(std::auto_ptr<Expr> e);
    ~MultipleBytecode();

    /// Finalizes the bytecode after parsing.
    bool Finalize(Bytecode& bc, Diagnostic& diags);

    /// Calculates the minimum size of a bytecode.
    bool CalcLen(Bytecode& bc,
                 /*@out@*/ unsigned long* len,
                 const Bytecode::AddSpanFunc& add_span,
                 Diagnostic& diags);

    /// Recalculates the bytecode's length based on an expanded span
    /// length.
    bool Expand(Bytecode& bc,
                unsigned long* len,
                int span,
                long old_val,
                long new_val,
                bool* keep,
                /*@out@*/ long* neg_thres,
                /*@out@*/ long* pos_thres,
                Diagnostic& diags);

    /// Convert a bytecode into its byte representation.
    bool Output(Bytecode& bc, BytecodeOutput& bc_out);

    llvm::StringRef getType() const;

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
} // anonymous namespace

MultipleBytecode::MultipleBytecode(std::auto_ptr<Expr> e)
    : m_multiple(e),
      m_mult_int(0)
{
}

MultipleBytecode::~MultipleBytecode()
{
}

bool
MultipleBytecode::Finalize(Bytecode& bc, Diagnostic& diags)
{
    Value val(0, std::auto_ptr<Expr>(m_multiple->clone()));

    if (!val.Finalize(diags, diag::err_multiple_too_complex))
        return false;
    if (val.isRelative())
    {
        diags.Report(bc.getSource(), diag::err_multiple_not_absolute);
        return false;
    }
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
        {
            diags.Report(bc.getSource(), diag::err_multiple_setpos);
            return false;
        }
        if (!i->Finalize(diags))
            return false;
    }
    return true;
}

bool
MultipleBytecode::CalcLen(Bytecode& bc,
                          /*@out@*/ unsigned long* len,
                          const Bytecode::AddSpanFunc& add_span,
                          Diagnostic& diags)
{
    // Calculate multiple value as an integer
    m_mult_int = 1;
    if (m_multiple->isIntNum())
    {
        IntNum num = m_multiple->getIntNum();
        if (num.getSign() < 0)
        {
            m_mult_int = 0;
            diags.Report(bc.getSource(), diag::err_multiple_negative);
            return false;
        }
        else
            m_mult_int = num.getInt();
    }
    else
    {
        if (m_multiple->Contains(ExprTerm::FLOAT))
        {
            m_mult_int = 0;
            diags.Report(bc.getSource(), diag::err_expr_contains_float);
            return false;
        }
        else
        {
            Value value(0, Expr::Ptr(m_multiple->clone()));
            add_span(bc, 0, value, 0, 0);
            m_mult_int = 0;     // assume 0 to start
        }
    }

    unsigned long ilen = 0;
    for (BytecodeContainer::bc_iterator i = m_contents.bytecodes_begin(),
         end = m_contents.bytecodes_end(); i != end; ++i)
    {
        if (!i->CalcLen(add_span, diags))
            return false;
        ilen += i->getTotalLen();
    }

    *len = ilen * m_mult_int;
    return true;
}

bool
MultipleBytecode::Expand(Bytecode& bc,
                         unsigned long* len,
                         int span,
                         long old_val,
                         long new_val,
                         bool* keep,
                         /*@out@*/ long* neg_thres,
                         /*@out@*/ long* pos_thres,
                         Diagnostic& diags)
{
    // XXX: support more than one bytecode here
    if (span == 0)
    {
        m_mult_int = new_val;
        *keep = true;
    }
    else
    {
        if (!m_contents.bytecodes_front().Expand(span, old_val, new_val, keep,
                                                 neg_thres, pos_thres, diags))
            return false;
    }
    *len = m_contents.bytecodes_front().getTotalLen() * m_mult_int;
    return true;
}

bool
MultipleBytecode::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    SimplifyCalcDist(*m_multiple, bc_out.getDiagnostics());
    if (!m_multiple->isIntNum())
    {
        bc_out.Diag(bc.getSource(), diag::err_multiple_unknown);
        return false;
    }
    IntNum num = m_multiple->getIntNum();
    if (num.getSign() < 0)
    {
        bc_out.Diag(bc.getSource(), diag::err_multiple_negative);
        return false;
    }
    assert(m_mult_int == num.getInt() && "multiple changed after optimize");
    m_mult_int = num.getInt();
    if (m_mult_int == 0)
        return true;    // nothing to output

    unsigned long total_len = 0;
    unsigned long pos = 0;
    for (long mult=0; mult<m_mult_int; mult++, pos += total_len)
    {
        for (BytecodeContainer::bc_iterator i = m_contents.bytecodes_begin(),
             end = m_contents.bytecodes_end(); i != end; ++i)
        {
            if (!i->Output(bc_out))
                return false;
        }
    }
    return true;
}

llvm::StringRef
MultipleBytecode::getType() const
{
    return "yasm::MultipleBytecode";
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

BytecodeContainer&
yasm::AppendMultiple(BytecodeContainer& container,
                     std::auto_ptr<Expr> multiple,
                     SourceLocation source)
{
    Bytecode& bc = container.FreshBytecode();
    MultipleBytecode* multbc(new MultipleBytecode(multiple));
    BytecodeContainer& retval = multbc->getContents();
    bc.Transform(Bytecode::Contents::Ptr(multbc));
    bc.setSource(source);
    return retval;
}

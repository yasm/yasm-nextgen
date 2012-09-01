///
/// LEB128 bytecode
///
///  Copyright (C) 2005-2009  Peter Johnson
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

#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_leb128.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/Location_util.h"


using namespace yasm;

namespace {
class LEB128Bytecode : public Bytecode::Contents
{
public:
    LEB128Bytecode(std::auto_ptr<Expr> expr, bool sign);
    ~LEB128Bytecode();

    /// Finalizes the bytecode after parsing.
    bool Finalize(Bytecode& bc, DiagnosticsEngine& diags);

    /// Calculates the minimum size of a bytecode.
    bool CalcLen(Bytecode& bc,
                 /*@out@*/ unsigned long* len,
                 const Bytecode::AddSpanFunc& add_span,
                 DiagnosticsEngine& diags);

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
                DiagnosticsEngine& diags);

    /// Convert a bytecode into its byte representation.
    bool Output(Bytecode& bc, BytecodeOutput& bc_out);

    StringRef getType() const;

    LEB128Bytecode* clone() const;

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

private:
    Value m_value;
};
} // anonymous namespace

LEB128Bytecode::LEB128Bytecode(std::auto_ptr<Expr> expr, bool sign)
    : m_value(0, expr)
{
    m_value.setSigned(sign);
}

LEB128Bytecode::~LEB128Bytecode()
{
}

bool
LEB128Bytecode::Finalize(Bytecode& bc, DiagnosticsEngine& diags)
{
    if (!m_value.Finalize(diags, diag::err_leb128_too_complex) ||
        m_value.isRelative())
        return false;
    return true;
}

bool
LEB128Bytecode::CalcLen(Bytecode& bc,
                        /*@out@*/ unsigned long* len,
                        const Bytecode::AddSpanFunc& add_span,
                        DiagnosticsEngine& diags)
{
    if (!m_value.hasAbs())
    {
        *len = 1;       // zero = 1 byte
        m_value.setSize(1);
        return true;
    }
    if (m_value.getAbs()->isIntNum())
    {
        *len = SizeLEB128(m_value.getAbs()->getIntNum(), m_value.isSigned());
        m_value.setSize(*len);
        return true;
    }
    *len = 1;   // start with 1 byte
    m_value.setSize(1);
    if (m_value.isSigned())
        add_span(bc, 2, m_value, -64, 63);
    else
        add_span(bc, 2, m_value, 0, 127);
    return true;
}

bool
LEB128Bytecode::Expand(Bytecode& bc,
                       unsigned long* len,
                       int span,
                       long old_val,
                       long new_val,
                       bool* keep,
                       /*@out@*/ long* neg_thres,
                       /*@out@*/ long* pos_thres,
                       DiagnosticsEngine& diags)
{
    unsigned long size = SizeLEB128(new_val, m_value.isSigned());

    // Don't allow length to shrink
    if (size > m_value.getSize())
    {
        *len += size-m_value.getSize();
        m_value.setSize(size);
    }

    if (m_value.isSigned())
    {
        *neg_thres = -(1<<(size*7-1));
        *pos_thres = (1<<(size*7-1))-1;
    }
    else
    {
        *neg_thres = 0;
        *pos_thres = (1<<(size*7))-1;
    }
    *keep = true;
    return true;
}

bool
LEB128Bytecode::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    Bytes& bytes = bc_out.getScratch();
    if (!m_value.hasAbs())
        Write8(bytes, 0);   // zero
    else
    {
        ExprTerm term;
        if (!Evaluate(*m_value.getAbs(), bc_out.getDiagnostics(), &term) ||
            !term.isType(ExprTerm::INT))
        {
            bc_out.getDiagnostics().Report(m_value.getSource().getBegin(),
                                           diag::err_leb128_too_complex);
            return false;
        }

        const IntNum* intn = term.getIntNum();
        if (intn->getSign() < 0 && !m_value.isSigned())
            bc_out.getDiagnostics().Report(m_value.getSource().getBegin(),
                                           diag::warn_negative_uleb128);

        // pad out in case final value is smaller than expanded size
        unsigned long size = SizeLEB128(*intn, m_value.isSigned());
        while (size < m_value.getSize())
        {
            if (m_value.isSigned())
                Write8(bytes, intn->getSign() < 0 ? 0xff : 0x80);
            else
                Write8(bytes, 0x80);
            ++size;
        }
        // write final value
        WriteLEB128(bytes, *intn, m_value.isSigned());
    }
    bc_out.OutputBytes(bytes, bc.getSource());
    return true;
}

StringRef
LEB128Bytecode::getType() const
{
    return "yasm::LEB128Bytecode";
}

LEB128Bytecode*
LEB128Bytecode::clone() const
{
    return new LEB128Bytecode(*this);
}

#ifdef WITH_XML
pugi::xml_node
LEB128Bytecode::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("LEB128");
    append_data(root, m_value);
    return root;
}
#endif // WITH_XML

void
yasm::AppendLEB128(BytecodeContainer& container,
                   const IntNum& intn,
                   bool sign,
                   SourceLocation source,
                   DiagnosticsEngine& diags)
{
    if (intn.getSign() < 0 && !sign)
        diags.Report(source, diag::warn_negative_uleb128);
    Bytecode& bc = container.FreshBytecode();
    WriteLEB128(bc.getFixed(), intn, sign);
}

void
yasm::AppendLEB128(BytecodeContainer& container,
                   std::auto_ptr<Expr> expr,
                   bool sign,
                   SourceLocation source,
                   DiagnosticsEngine& diags)
{
    // If expression is just an integer, output directly.
    expr->Simplify(diags);
    if (expr->isIntNum())
    {
        AppendLEB128(container, expr->getIntNum(), sign, source, diags);
        return;
    }

    // More complex; append LEB128 bytecode.
    Bytecode& bc = container.FreshBytecode();
    bc.Transform(Bytecode::Contents::Ptr(new LEB128Bytecode(expr, sign)));
    bc.setSource(source);
}

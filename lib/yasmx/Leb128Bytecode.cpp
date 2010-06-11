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

#include "util.h"

#include "YAML/emitter.h"
#include "yasmx/Support/Compose.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_leb128.h"
#include "yasmx/Diagnostic.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"


using namespace yasm;

namespace {
class LEB128Bytecode : public Bytecode::Contents
{
public:
    LEB128Bytecode(const Expr& expr, bool sign);
    LEB128Bytecode(std::auto_ptr<Expr> expr, bool sign);
    ~LEB128Bytecode();

    /// Finalizes the bytecode after parsing.
    bool Finalize(Bytecode& bc, Diagnostic& diags);

    /// Calculates the minimum size of a bytecode.
    bool CalcLen(Bytecode& bc,
                 /*@out@*/ unsigned long* len,
                 const Bytecode::AddSpanFunc& add_span,
                 Diagnostic& diags);

    /// Convert a bytecode into its byte representation.
    bool Output(Bytecode& bc, BytecodeOutput& bc_out);

    llvm::StringRef getType() const;

    LEB128Bytecode* clone() const;

    /// Write a YAML representation.  For debugging purposes.
    void Write(YAML::Emitter& out) const;

private:
    Expr m_expr;
    bool m_sign;
};
} // anonymous namespace

LEB128Bytecode::LEB128Bytecode(const Expr& expr, bool sign)
    : m_expr(expr),
      m_sign(sign)
{
}

LEB128Bytecode::LEB128Bytecode(std::auto_ptr<Expr> expr, bool sign)
    : m_sign(sign)
{
    std::swap(m_expr, *expr);
}

LEB128Bytecode::~LEB128Bytecode()
{
}

bool
LEB128Bytecode::Finalize(Bytecode& bc, Diagnostic& diags)
{
    m_expr.Simplify();
    if (!m_expr.isIntNum())
    {
        diags.Report(bc.getSource(), diag::err_value_not_constant);
        return false;
    }
    if (m_expr.getIntNum().getSign() < 0 && !m_sign)
        diags.Report(bc.getSource(), diag::warn_negative_uleb128);
    return true;
}

bool
LEB128Bytecode::CalcLen(Bytecode& bc,
                        /*@out@*/ unsigned long* len,
                        const Bytecode::AddSpanFunc& add_span,
                        Diagnostic& diags)
{
    *len = SizeLEB128(m_expr.getIntNum(), m_sign);
    return true;
}

bool
LEB128Bytecode::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    Bytes& bytes = bc_out.getScratch();
    WriteLEB128(bytes, m_expr.getIntNum(), m_sign);
    bc_out.OutputBytes(bytes, bc.getSource());
    return true;
}

llvm::StringRef
LEB128Bytecode::getType() const
{
    return "yasm::LEB128Bytecode";
}

LEB128Bytecode*
LEB128Bytecode::clone() const
{
    return new LEB128Bytecode(m_expr, m_sign);
}

void
LEB128Bytecode::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << "LEB128";
    out << YAML::Key << "expr" << YAML::Value << m_expr;
    out << YAML::Key << "sign" << YAML::Value << m_sign;
    out << YAML::EndMap;
}

void
yasm::AppendLEB128(BytecodeContainer& container,
                   const IntNum& intn,
                   bool sign,
                   clang::SourceLocation source,
                   Diagnostic& diags)
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
                   clang::SourceLocation source,
                   Diagnostic& diags)
{
    // If expression is just an integer, output directly.
    expr->Simplify();
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

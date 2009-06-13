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
#include "yasmx/BytecodeContainer_util.h"

#include "util.h"

#include "yasmx/Support/Compose.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/marg_ostream.h"
#include "yasmx/BytecodeContainer.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_leb128.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"


namespace
{

using namespace yasm;

class Leb128Bytecode : public Bytecode::Contents
{
public:
    Leb128Bytecode(const Expr& expr, bool sign);
    Leb128Bytecode(std::auto_ptr<Expr> expr, bool sign);
    ~Leb128Bytecode();

    /// Prints the implementation-specific data (for debugging purposes).
    void put(marg_ostream& os) const;

    /// Finalizes the bytecode after parsing.
    void finalize(Bytecode& bc);

    /// Calculates the minimum size of a bytecode.
    unsigned long calc_len(Bytecode& bc, const Bytecode::AddSpanFunc& add_span);

    /// Convert a bytecode into its byte representation.
    void output(Bytecode& bc, BytecodeOutput& bc_out);

    Leb128Bytecode* clone() const;

private:
    Expr m_expr;
    bool m_sign;
};


Leb128Bytecode::Leb128Bytecode(const Expr& expr, bool sign)
    : m_expr(expr),
      m_sign(sign)
{
}

Leb128Bytecode::Leb128Bytecode(std::auto_ptr<Expr> expr, bool sign)
    : m_sign(sign)
{
    std::swap(m_expr, *expr);
}

Leb128Bytecode::~Leb128Bytecode()
{
}

void
Leb128Bytecode::put(marg_ostream& os) const
{
    os << "_LEB128_\n";
    os << "Expr=" << m_expr << '\n';
    os << "Sign=" << m_sign << '\n';
}

void
Leb128Bytecode::finalize(Bytecode& bc)
{
    m_expr.simplify();
    if (!m_expr.is_intnum())
        throw NotConstantError(N_("LEB128 value must be a constant"));
}

unsigned long
Leb128Bytecode::calc_len(Bytecode& bc, const Bytecode::AddSpanFunc& add_span)
{
    return size_leb128(m_expr.get_intnum(), m_sign);
}

void
Leb128Bytecode::output(Bytecode& bc, BytecodeOutput& bc_out)
{
    Bytes& bytes = bc_out.get_scratch();
    write_leb128(bytes, m_expr.get_intnum(), m_sign);
    bc_out.output(bytes);
}

Leb128Bytecode*
Leb128Bytecode::clone() const
{
    return new Leb128Bytecode(m_expr, m_sign);
}

} // anonymous namespace

namespace yasm
{

void append_leb128(BytecodeContainer& container,
                   const IntNum& intn,
                   bool sign,
                   unsigned long line)
{
    if (intn.sign() < 0 && !sign)
        warn_set(WARN_GENERAL, N_("negative value in unsigned LEB128"));
    Bytecode& bc = container.fresh_bytecode();
    write_leb128(bc.get_fixed(), intn, sign);
}

void append_leb128(BytecodeContainer& container,
                   std::auto_ptr<Expr> expr,
                   bool sign,
                   unsigned long line)
{
    // If expression is just an integer, output directly.
    expr->simplify();
    if (expr->is_intnum())
    {
        append_leb128(container, expr->get_intnum(), sign, line);
        return;
    }

    // More complex; append Leb128 bytecode.
    Bytecode& bc = container.fresh_bytecode();
    bc.transform(Bytecode::Contents::Ptr(new Leb128Bytecode(expr, sign)));
    bc.set_line(line);
}

} // namespace yasm

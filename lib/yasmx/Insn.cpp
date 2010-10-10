//
// Mnemonic instruction
//
//  Copyright (C) 2005-2007  Peter Johnson
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
#include "yasmx/Insn.h"

#include <algorithm>

#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Config/functional.h"
#include "yasmx/Arch.h"
#include "yasmx/EffAddr.h"
#include "yasmx/Expr.h"
#include "yasmx/Expr_util.h"


using namespace yasm;

TargetModifier::~TargetModifier()
{
}

Operand::Operand(const Register* reg)
    : m_reg(reg),
      m_seg(0),
      m_targetmod(0),
      m_size(0),
      m_deref(0),
      m_strict(0),
      m_type(REG)
{
}

Operand::Operand(const SegmentRegister* segreg)
    : m_segreg(segreg),
      m_seg(0),
      m_targetmod(0),
      m_size(0),
      m_deref(0),
      m_strict(0),
      m_type(SEGREG)
{
}

Operand::Operand(std::auto_ptr<EffAddr> ea)
    : m_ea(ea.release()),
      m_seg(0),
      m_targetmod(0),
      m_size(0),
      m_deref(0),
      m_strict(0),
      m_type(MEMORY)
{
}

Operand::Operand(std::auto_ptr<Expr> val)
    : m_seg(0),
      m_targetmod(0),
      m_size(0),
      m_deref(0),
      m_strict(0),
      m_type(IMM)
{
    if (val->isRegister())
    {
        m_type = REG;
        m_reg = val->getRegister();
    }
    else
        m_val = val.release();
}

void
Operand::Destroy()
{
    switch (m_type)
    {
        case MEMORY:
            delete m_ea;
            m_ea = 0;
            break;
        case IMM:
            delete m_val;
            m_val = 0;
            break;
        default:
            break;
    }
    delete m_seg;
    m_seg = 0;
}

Operand
Operand::clone() const
{
    Operand op(*this);

    // Deep copy things that need to be deep copied.
    switch (m_type)
    {
        case MEMORY:
            op.m_ea = op.m_ea->clone();
            break;
        case IMM:
            op.m_val = op.m_val->clone();
            break;
        default:
            break;
    }

    if (op.m_seg)
        op.m_seg = op.m_seg->clone();

    return op;
}

bool
Operand::Finalize(Diagnostic& diags)
{
    switch (m_type)
    {
        case MEMORY:
            if (!m_ea)
                break;
            if (Expr* abs = m_ea->m_disp.getAbs())
            {
                if (!ExpandEqu(*abs))
                {
                    diags.Report(m_source, diag::err_equ_circular_reference_mem);
                    return false;
                }

                // Don't get over-ambitious here; some archs' memory expr
                // parser are sensitive to the presence of *1, etc, so don't
                // simplify reg*1 identities.
                abs->Simplify(diags, false);
            }
            break;
        case IMM:
            if (!ExpandEqu(*m_val))
            {
                diags.Report(m_source, diag::err_equ_circular_reference_imm);
                return false;
            }
            m_val->Simplify(diags);
            break;
        default:
            break;
    }
    return true;
}

std::auto_ptr<EffAddr>
Operand::ReleaseMemory()
{
    if (m_type != MEMORY)
        return std::auto_ptr<EffAddr>(0);
    std::auto_ptr<EffAddr> ea(m_ea);
    Release();
    return ea;
}

std::auto_ptr<Expr>
Operand::ReleaseImm()
{
    if (m_type != IMM)
        return std::auto_ptr<Expr>(0);
    std::auto_ptr<Expr> imm(m_val);
    Release();
    return imm;
}

std::auto_ptr<Expr>
Operand::ReleaseSeg()
{
    std::auto_ptr<Expr> seg(m_seg);
    m_seg = 0;
    return seg;
}

void
Operand::setSeg(std::auto_ptr<Expr> seg)
{
    if (m_seg)
        delete m_seg;
    m_seg = seg.release();
}

Prefix::~Prefix()
{
}

Insn::Insn()
    : m_segreg(0)
{
}

Insn::Insn(const Insn& rhs)
    : m_operands(),
      m_prefixes(rhs.m_prefixes),
      m_segreg(rhs.m_segreg),
      m_segreg_source(rhs.m_segreg_source)
{
    m_operands.reserve(rhs.m_operands.size());
    std::transform(rhs.m_operands.begin(), rhs.m_operands.end(),
                   m_operands.begin(), TR1::mem_fn(&Operand::clone));
}

Insn::~Insn()
{
    std::for_each(m_operands.begin(), m_operands.end(),
                  TR1::mem_fn(&Operand::Destroy));
}

bool
Insn::Append(BytecodeContainer& container,
             SourceLocation source,
             Diagnostic& diags)
{
    // Simplify the operands' expressions.
    bool ok = true;
    for (Operands::iterator i = m_operands.begin(), end = m_operands.end();
         i != end; ++i)
    {
         if (!i->Finalize(diags))
             ok = false;
    }
    if (!ok)
        return false;
    return DoAppend(container, source, diags);
}

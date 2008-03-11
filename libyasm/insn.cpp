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
#include "insn.h"

#include "util.h"

#include <algorithm>
#include <ostream>

#include "arch.h"
#include "effaddr.h"
#include "errwarn.h"
#include "expr.h"
#include "expr_util.h"
#include "marg_ostream.h"


namespace yasm
{

Insn::Operand::TargetModifier::TargetModifier()
{
}

Insn::Operand::TargetModifier::~TargetModifier()
{
}

Insn::Operand::Operand(const Register* reg)
    : m_type(REG),
      m_reg(reg),
      m_targetmod(0),
      m_size(0),
      m_deref(0),
      m_strict(0)
{
}

Insn::Operand::Operand(const SegmentRegister* segreg)
    : m_type(SEGREG),
      m_segreg(segreg),
      m_targetmod(0),
      m_size(0),
      m_deref(0),
      m_strict(0)
{
}

Insn::Operand::Operand(std::auto_ptr<EffAddr> ea)
    : m_type(MEMORY),
      m_ea(ea.release()),
      m_targetmod(0),
      m_size(0),
      m_deref(0),
      m_strict(0)
{
}

Insn::Operand::Operand(std::auto_ptr<Expr> val)
    : m_type(IMM),
      m_targetmod(0),
      m_size(0),
      m_deref(0),
      m_strict(0)
{
    const Register* reg;

    reg = val->get_reg();
    if (reg)
    {
        m_type = REG;
        m_reg = reg;
    }
    else
        m_val = val.release();
}

void
Insn::Operand::destroy()
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
}

Insn::Operand
Insn::Operand::clone() const
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

    return op;
}

void
Insn::Operand::put(marg_ostream& os) const
{
    switch (m_type)
    {
        case NONE:
            os << "None\n";
            break;
        case REG:
            os << "Reg=" << *m_reg << '\n';
            break;
        case SEGREG:
            os << "SegReg=" << *m_segreg << '\n';
            break;
        case MEMORY:
            os << "Memory=\n";
            ++os;
            os << *m_ea;
            --os;
            break;
        case IMM:
            os << "Imm=" << *m_val << '\n';
            break;
    }
    ++os;
    os << "TargetMod=" << *m_targetmod << '\n';
    os << "Size=" << m_size << '\n';
    os << "Deref=" << m_deref << ", Strict=" << m_strict << '\n';
    --os;
}

void
Insn::put(marg_ostream& os) const
{
    std::for_each(m_operands.begin(), m_operands.end(),
                  BIND::bind(&Insn::Operand::put, _1, REF::ref(os)));
}

void
Insn::Operand::finalize()
{
    switch (m_type)
    {
        case MEMORY:
            // Don't get over-ambitious here; some archs' memory expr
            // parser are sensitive to the presence of *1, etc, so don't
            // simplify reg*1 identities.
            try
            {
                if (m_ea)
                {
                    if (Expr* abs = m_ea->m_disp.get_abs())
                    {
                        expand_equ(abs);
                        abs->level_tree(true, true, false);
                    }
                }
            }
            catch (Error& err)
            {
                // Add a pointer to where it was used
                err.m_message += " in memory expression";
                throw;
            }
            break;
        case IMM:
            try
            {
                expand_equ(m_val);
                m_val->level_tree(true, true, true);
            }
            catch (Error& err)
            {
                // Add a pointer to where it was used
                err.m_message += " in immediate expression";
                throw;
            }
            break;
        default:
            break;
    }
}

std::auto_ptr<EffAddr>
Insn::Operand::release_memory()
{
    if (m_type != MEMORY)
        return std::auto_ptr<EffAddr>(0);
    std::auto_ptr<EffAddr> ea(m_ea);
    release();
    return ea;
}

std::auto_ptr<Expr>
Insn::Operand::release_imm()
{
    if (m_type != IMM)
        return std::auto_ptr<Expr>(0);
    std::auto_ptr<Expr> imm(m_val);
    release();
    return imm;
}

Insn::Insn()
{
}

Insn::Insn(const Insn& rhs)
    : m_operands(),
      m_prefixes(rhs.m_prefixes),
      m_segregs(rhs.m_segregs)
{
    m_operands.reserve(rhs.m_operands.size());
    std::transform(rhs.m_operands.begin(), rhs.m_operands.end(),
                   m_operands.begin(), MEMFN::mem_fn(&Operand::clone));
}

Insn::~Insn()
{
    std::for_each(m_operands.begin(), m_operands.end(),
                  MEMFN::mem_fn(&Operand::destroy));
}

void
Insn::append(BytecodeContainer& container)
{
    // Simplify the operands' expressions.
    std::for_each(m_operands.begin(), m_operands.end(),
                  MEMFN::mem_fn(&Operand::finalize));
    do_append(container);
}

} // namespace yasm

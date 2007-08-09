//
// Mnemonic instruction bytecode
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
#include "util.h"

#include <algorithm>
#include <iomanip>

#include <boost/bind.hpp>
#include <boost/ref.hpp>

#include "errwarn.h"
#include "expr.h"
#include "insn.h"


namespace yasm {

#if 0
void
yasm_ea_set_segreg(yasm_effaddr *ea, uintptr_t segreg)
{
    if (!ea)
        return;

    if (segreg != 0 && ea->segreg != 0)
        yasm_warn_set(YASM_WARN_GENERAL,
                      N_("multiple segment overrides, using leftmost"));

    ea->segreg = segreg;
}
#endif
Insn::Operand::Operand(const Register* reg)
    : m_type(REG),
      m_targetmod(0),
      m_size(0),
      m_deref(0),
      m_strict(0)
{
    m_data.reg = reg;
}

Insn::Operand::Operand(const SegmentRegister* segreg)
    : m_type(SEGREG),
      m_targetmod(0),
      m_size(0),
      m_deref(0),
      m_strict(0)
{
    m_data.segreg = segreg;
}

Insn::Operand::Operand(std::auto_ptr<EffAddr> ea)
    : m_type(MEMORY),
      m_targetmod(0),
      m_size(0),
      m_deref(0),
      m_strict(0)
{
    m_data.ea = ea.release();
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
    if (reg) {
        m_type = REG;
        m_data.reg = reg;
    } else
        m_data.val = val.release();
}

Insn::Operand::~Operand()
{
    switch (m_type) {
        //case MEMORY:    delete m_data.ea; break;
        case IMM:       delete m_data.val; break;
        default:        break;
    }
}

void
Insn::Operand::put(std::ostream& os, int indent_level) const
{
    switch (m_type) {
        case REG:
            //os << std::setw(indent_level) << "";
            //os << "Reg=" << *m_data.reg << std::endl;
            break;
        case SEGREG:
            //os << std::setw(indent_level) << "";
            //os << "SegReg=" << *m_data.segreg << std::endl;
            break;
        case MEMORY:
            //os << std::setw(indent_level) << "";
            //os << "Memory=" << *m_data.ea << std::endl;
            break;
        case IMM:
            os << std::setw(indent_level) << "";
            os << "Imm=" << *m_data.val << std::endl;
            break;
    }
    //os << std::setw(indent_level+1) << "";
    //os << "TargetMod=" << *m_targetmod << std::endl;
    os << std::setw(indent_level+1) << "" << "Size=" << m_size << std::endl;
    os << std::setw(indent_level+1) << "";
    os << "Deref=" << m_deref << ", Strict=" << m_strict << std::endl;
}

void
Insn::put(std::ostream& os, int indent_level) const
{
    std::for_each(m_operands.begin(), m_operands.end(),
                  boost::bind(&Insn::Operand::put, _1,
                              boost::ref(os),
                              indent_level));
}

void
Insn::Operand::finalize()
{
    switch (m_type) {
        case MEMORY:
            // Don't get over-ambitious here; some archs' memory expr
            // parser are sensitive to the presence of *1, etc, so don't
            // simplify reg*1 identities.
#if 0
            try {
                if (m_data.ea && m_data.ea->disp.get_abs())
                    m_data.ea->disp.get_abs()->level_tree(true, true, false);
            } catch (Error& err) {
                // Add a pointer to where it was used
                err.m_message += " in memory expression";
                throw;
            }
#endif
            break;
        case IMM:
            try {
                m_data.val->level_tree(true, true, true);
            } catch (Error& err) {
                // Add a pointer to where it was used
                err.m_message += " in immediate expression";
                throw;
            }
            break;
        default:
            break;
    }
}

void
Insn::finalize(Bytecode& bc, Bytecode& prev_bc)
{
    // Simplify the operands' expressions.
    std::for_each(m_operands.begin(), m_operands.end(),
                  boost::mem_fn(&Operand::finalize));
    do_finalize(bc, prev_bc);
}

unsigned long
Insn::calc_len(Bytecode& bc, Bytecode::AddSpanFunc add_span)
{
    // simply pass down to the base class
    return Bytecode::Contents::calc_len(bc, add_span);
}

bool
Insn::expand(Bytecode& bc, unsigned long& len, int span,
             long old_val, long new_val,
             /*@out@*/ long& neg_thres, /*@out@*/ long& pos_thres)
{
    // simply pass down to the base class
    return Bytecode::Contents::expand(bc, len, span, old_val, new_val,
                                      neg_thres, pos_thres);
}

void
Insn::to_bytes(Bytecode& bc, unsigned char* &buf, OutputValueFunc output_value,
               OutputRelocFunc output_reloc)
{
    // simply pass down to the base class
    Bytecode::Contents::to_bytes(bc, buf, output_value, output_reloc);
}

Insn::Contents::SpecialType
Insn::get_special() const
{
    return Contents::SPECIAL_INSN;
}

} // namespace yasm

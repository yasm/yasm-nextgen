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
#include "BytecodeContainer_util.h"

#include "util.h"

#include "Support/marg_ostream.h"

#include "BytecodeContainer.h"
#include "Bytecode.h"
#include "errwarn.h"
#include "Expr.h"
#include "IntNum.h"
#include "Location_util.h"


namespace
{

using namespace yasm;

class MultipleBytecode : public Bytecode::Contents
{
public:
    MultipleBytecode(std::auto_ptr<Expr> e);
    ~MultipleBytecode();

    /// Prints the implementation-specific data (for debugging purposes).
    void put(marg_ostream& os) const;

    /// Finalizes the bytecode after parsing.
    void finalize(Bytecode& bc);

    /// Calculates the minimum size of a bytecode.
    unsigned long calc_len(Bytecode& bc, const Bytecode::AddSpanFunc& add_span);

    /// Recalculates the bytecode's length based on an expanded span
    /// length.
    bool expand(Bytecode& bc,
                unsigned long& len,
                int span,
                long old_val,
                long new_val,
                /*@out@*/ long* neg_thres,
                /*@out@*/ long* pos_thres);

    /// Convert a bytecode into its byte representation.
    void output(Bytecode& bc, BytecodeOutput& bc_out);

    MultipleBytecode* clone() const;

    BytecodeContainer& get_contents() { return m_contents; }

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
MultipleBytecode::put(marg_ostream& os) const
{
    os << "_Multiple_\n";
    os << "Multiple=" << *m_multiple << '\n';
    os << "Multiple (int)=" << m_mult_int << '\n';
    os << "Contents:\n";
    ++os;
    os << m_contents;
    --os;
}

void
MultipleBytecode::finalize(Bytecode& bc)
{
    Value val(0, std::auto_ptr<Expr>(m_multiple->clone()));

    if (!val.finalize())
        throw TooComplexError(N_("multiple expression too complex"));
    else if (val.is_relative())
        throw NotAbsoluteError(N_("multiple expression not absolute"));
    // Finalize creates NULL output if value=0, but bc->multiple is NULL
    // if value=1 (this difference is to make the common case small).
    // However, this means we need to set bc->multiple explicitly to 0
    // here if val.abs is NULL.
    if (const Expr* e = val.get_abs())
        m_multiple.reset(e->clone());
    else
        m_multiple.reset(new Expr(0));

    for (BytecodeContainer::bc_iterator i = m_contents.bcs_begin(),
         end = m_contents.bcs_end(); i != end; ++i)
    {
        if (i->get_special() == Bytecode::Contents::SPECIAL_OFFSET)
            throw ValueError(N_("cannot combine multiples and setting assembly position"));
        i->finalize();
    }
}

unsigned long
MultipleBytecode::calc_len(Bytecode& bc, const Bytecode::AddSpanFunc& add_span)
{
    // Calculate multiple value as an integer
    m_mult_int = 1;
    if (const IntNum* num = m_multiple->get_intnum())
    {
        if (num->sign() < 0)
        {
            m_mult_int = 0;
            throw ValueError(N_("multiple is negative"));
        }
        else
            m_mult_int = num->get_int();
    }
    else
    {
        if (m_multiple->contains(ExprTerm::FLOAT))
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
    for (BytecodeContainer::bc_iterator i = m_contents.bcs_begin(),
         end = m_contents.bcs_end(); i != end; ++i)
    {
        i->calc_len(add_span);
        len += i->get_total_len();
    }

    return len * m_mult_int;
}

bool
MultipleBytecode::expand(Bytecode& bc,
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
    return m_contents.bcs_first().expand(span, old_val, new_val, neg_thres,
                                         pos_thres);
}

void
MultipleBytecode::output(Bytecode& bc, BytecodeOutput& bc_out)
{
    simplify_calc_dist(*m_multiple);
    const IntNum* num = m_multiple->get_intnum();
    if (!num)
        throw ValueError(N_("could not determine multiple"));
    if (num->sign() < 0)
        throw ValueError(N_("multiple is negative"));
    m_mult_int = num->get_int();
    if (m_mult_int == 0)
        return; // nothing to output

    unsigned long total_len = 0;
    unsigned long pos = 0;
    for (long mult=0; mult<m_mult_int; mult++, pos += total_len)
    {
        for (BytecodeContainer::bc_iterator i = m_contents.bcs_begin(),
             end = m_contents.bcs_end(); i != end; ++i)
        {
            i->output(bc_out);
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

} // anonymous namespace

namespace yasm
{

BytecodeContainer&
append_multiple(BytecodeContainer& container,
                std::auto_ptr<Expr> multiple,
                unsigned long line)
{
    Bytecode& bc = container.fresh_bytecode();
    MultipleBytecode* multbc(new MultipleBytecode(multiple));
    BytecodeContainer& retval = multbc->get_contents();
    bc.transform(Bytecode::Contents::Ptr(multbc));
    bc.set_line(line);
    return retval;
}

} // namespace yasm

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
#include "bytecode.h"

#include "util.h"

#include "bytes.h"
#include "errwarn.h"
#include "expr.h"
#include "intnum.h"
#include "location_util.h"
#include "marg_ostream.h"
#include "operator.h"
#include "symbol.h"
#include "value.h"


namespace yasm
{

BytecodeOutput::BytecodeOutput()
{
}

BytecodeOutput::~BytecodeOutput()
{
}

void
BytecodeOutput::output(Symbol* sym,
                       Bytes& bytes,
                       Bytecode& bc,
                       unsigned int valsize,
                       int warn)
{
    output(bytes);
}

Bytecode::Contents::Contents()
{
}

Bytecode::Contents::~Contents()
{
}

bool
Bytecode::Contents::expand(Bytecode& bc,
                           unsigned long& len,
                           int span,
                           long old_val,
                           long new_val,
                           /*@out@*/ long& neg_thres,
                           /*@out@*/ long& pos_thres)
{
    throw InternalError(N_("bytecode does not have any dependent spans"));
}

Bytecode::Contents::SpecialType
Bytecode::Contents::get_special() const
{
    return SPECIAL_NONE;
}

Bytecode::Contents::Contents(const Contents& rhs)
{
}

void
Bytecode::transform(std::auto_ptr<Contents> contents)
{
    m_contents.reset(contents.release());
}

Bytecode::Bytecode(std::auto_ptr<Contents> contents, unsigned long line)
    : m_contents(contents),
      m_container(0),
      m_len(0),
      m_line(line),
      m_offset(~0UL),   // obviously incorrect / uninitialized value
      m_index(~0UL)
{
}

Bytecode::Bytecode()
    : m_contents(0),
      m_container(0),
      m_len(0),
      m_line(0),
      m_offset(~0UL),   // obviously incorrect / uninitialized value
      m_index(~0UL)
{
}

Bytecode::Bytecode(const Bytecode& oth)
    : m_contents(oth.m_contents->clone()),
      m_container(oth.m_container),
      m_len(oth.m_len),
      m_line(oth.m_line),
      m_offset(oth.m_offset),
      m_index(oth.m_index),
      m_symbols(oth.m_symbols)
{}

Bytecode::~Bytecode()
{
}

Bytecode&
Bytecode::operator= (const Bytecode& oth)
{
    if (this != &oth)
    {
        if (oth.m_contents.get() != 0)
            m_contents.reset(oth.m_contents->clone());
        else
            m_contents.reset(0);
        m_container = oth.m_container;
        m_len = oth.m_len;
        m_line = oth.m_line;
        m_offset = oth.m_offset;
        m_index = oth.m_index;
        m_symbols = oth.m_symbols;
    }
    return *this;
}

marg_ostream&
operator<< (marg_ostream &os, const Bytecode& bc)
{
    if (bc.m_fixed.size() > 0)
    {
        os << "Fixed: ";
        os << bc.m_fixed;
    }
    if (bc.m_contents.get() != 0)
        os << *bc.m_contents;
    else
        os << "EMPTY\n";
    os << "Length=" << bc.m_len << '\n';
    os << "Line Index=" << bc.m_line << '\n';
    os << "Offset=" << bc.m_offset << '\n';
    return os;
}

void
Bytecode::finalize()
{
    for (std::vector<Fixup>::iterator i=m_fixed_fixups.begin(),
         end=m_fixed_fixups.end(); i != end; ++i)
    {
        Location loc = {this, i->get_off()};
        if (i->finalize(loc))
        {
            if (i->m_jump_target)
                throw TooComplexError(i->get_line(),
                                      N_("jump target expression too complex"));
            else
                throw TooComplexError(i->get_line(),
                                      N_("expression too complex"));
        }
        if (i->m_jump_target)
        {
            if (i->m_seg_of || i->m_rshift || i->m_curpos_rel)
                throw ValueError(i->get_line(), N_("invalid jump target"));
            i->set_curpos_rel(*this, false);
        }
        warn_update_line(i->get_line());
    }

    if (m_contents.get() != 0)
        m_contents->finalize(*this);
}

void
Bytecode::calc_len(AddSpanFunc add_span)
{
    if (m_contents.get() == 0)
    {
        m_len = 0;
        return;
    }
    m_len = m_contents->calc_len(*this, add_span);
}

bool
Bytecode::expand(int span,
                 long old_val,
                 long new_val,
                 /*@out@*/ long& neg_thres,
                 /*@out@*/ long& pos_thres)
{
    if (m_contents.get() == 0)
        return false;
    return m_contents->expand(*this, m_len, span, old_val, new_val, neg_thres,
                              pos_thres);
}

void
Bytecode::output(BytecodeOutput& bc_out)
{
    // output fixups, outputting the fixed portions in between each one
    unsigned int last = 0;
    for (std::vector<Fixup>::iterator i=m_fixed_fixups.begin(),
         end=m_fixed_fixups.end(); i != end; ++i)
    {
        unsigned int off = i->get_off();
        Location loc = {this, off};

        // Output fixed portion.
        Bytes& fixed = bc_out.get_scratch();
        fixed.insert(fixed.end(), m_fixed.begin() + last,
                     m_fixed.begin() + off);
        bc_out.output(fixed);

        // Output value.
        Bytes& vbytes = bc_out.get_scratch();
        vbytes.insert(vbytes.end(), m_fixed.begin() + off,
                      m_fixed.begin() + off + i->m_size/8);
        // Make a copy of the value to ensure things like
        // "TIMES x JMP label" work.
        Value vcopy = *i;
        bc_out.output(vcopy, vbytes, loc, i->m_sign ? -1 : 1);
        warn_update_line(i->get_line());

        last = off + i->m_size/8;
    }
    // handle last part of fixed
    if (last < m_fixed.size())
    {
        Bytes& fixed = bc_out.get_scratch();
        fixed.insert(fixed.end(), m_fixed.begin() + last, m_fixed.end());
        bc_out.output(fixed);
    }

    // handle tail contents
    if (m_contents.get() != 0)
        m_contents->output(*this, bc_out);
}

unsigned long
Bytecode::update_offset(unsigned long offset)
{
    if (m_contents.get() != 0 &&
        m_contents->get_special() == Contents::SPECIAL_OFFSET)
    {
        // Recalculate/adjust len of offset-based bytecodes here
        long neg_thres = 0;
        long pos_thres = (long)next_offset();
        expand(1, 0, (long)offset, neg_thres, pos_thres);
    }
    m_offset = offset;
    return next_offset();
}

void
Bytecode::append_fixed(const Value& val)
{
    m_fixed_fixups.push_back(Fixup(m_fixed.size(), val, m_line));
    m_fixed.write(val.m_size/8, 0);
}

void
Bytecode::append_fixed(unsigned int size, std::auto_ptr<Expr> e)
{
    m_fixed_fixups.push_back(Fixup(m_fixed.size(), Value(size*8, e), m_line));
    m_fixed.write(size, 0);
}

Bytecode::Fixup::Fixup(unsigned int off, const Value& val, unsigned long line)
    : Value(val), m_line(line), m_off(off)
{
}

} // namespace yasm

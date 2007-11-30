//
// Section implementation.
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
#include "section.h"

#include "util.h"

#include <iomanip>
#include <ostream>

#include "bytecode.h"
#include "expr.h"
#include "intnum.h"


namespace yasm {

Reloc::Reloc(std::auto_ptr<IntNum> addr, Symbol* sym)
    : m_addr(addr.release()),
      m_sym(sym)
{
}

Reloc::~Reloc()
{
}

Section::Section(const std::string& name,
                 std::auto_ptr<Expr> start,
                 unsigned long align,
                 bool code,
                 bool res_only,
                 unsigned long line)
    : m_object(0),
      m_name(name),
      m_start(0),
      m_align(align),
      m_code(code),
      m_res_only(res_only),
      m_def(false),
      m_bcs_owner(m_bcs),
      m_relocs_owner(m_relocs)
{
    if (start.get() != 0)
        m_start.reset(start.release());
    else
        m_start.reset(new Expr(new IntNum(0), line));
}

Section::~Section()
{
}

void
Section::append_bytecode(std::auto_ptr<Bytecode> bc)
{
    if (bc.get() != 0) {
        bc->m_section = this;   // record parent section
        m_bcs.push_back(bc.release());
    }
}

void
Section::set_start(std::auto_ptr<Expr> start)
{
    m_start.reset(start.release());
}

void
Section::put(std::ostream& os, int indent_level, bool with_bcs) const
{
    os << std::setw(indent_level) << "" << "name=" << m_name << '\n';
    os << std::setw(indent_level) << ""
       << "start=" << *(m_start.get()) << '\n';
    os << std::setw(indent_level) << "" << "align=" << m_align << '\n';
    os << std::setw(indent_level) << "" << "code=" << m_code << '\n';
    os << std::setw(indent_level) << "" << "res_only=" << m_res_only << '\n';
    os << std::setw(indent_level) << "" << "default=" << m_def << '\n';
    os << std::setw(indent_level) << "" << "Associated data:\n";
    put_assoc_data(os, indent_level+1);

    if (!with_bcs)
        return;

    os << std::setw(indent_level) << "" << "Bytecodes:\n";

    for (const_bc_iterator bc=m_bcs.begin(), end=m_bcs.end();
         bc != end; ++bc) {
        os << std::setw(indent_level+1) << "" << "Next Bytecode:\n";
        bc->put(os, indent_level+2);
    }

    // TODO: relocs
}

void
Section::finalize(Errwarns& errwarns)
{
    for (bc_iterator bc=m_bcs.begin(), end=m_bcs.end(); bc != end; ++bc)
        bc->finalize(errwarns);
}

void
Section::update_bc_offsets(Errwarns& errwarns)
{
    unsigned long offset = 0;
    m_bcs.front().set_offset(0);
    for (bc_iterator bc=m_bcs.begin(), end=m_bcs.end(); bc != end; ++bc)
        offset = bc->update_offset(offset, errwarns);
}

} // namespace yasm

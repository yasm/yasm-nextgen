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


namespace {

using namespace yasm;

class EmptyBytecode : public Bytecode::Contents {
public:
    EmptyBytecode() {}
    ~EmptyBytecode() {}

    void put(std::ostream& os, int indent_level) const;

    void finalize(Bytecode& bc, Bytecode& prev_bc);

    unsigned long calc_len(Bytecode& bc, Bytecode::AddSpanFunc add_span);

    bool expand(Bytecode& bc, unsigned long& len, int span,
                long old_val, long new_val,
                /*@out@*/ long& neg_thres, /*@out@*/ long& pos_thres);

    void to_bytes(Bytecode& bc, Bytes& bytes, OutputValueFunc output_value,
                  OutputRelocFunc output_reloc = 0);

    EmptyBytecode* clone() const;

    static Bytecode* create(unsigned long line)
    { return new Bytecode(Ptr(new EmptyBytecode()), line); }
};

void
EmptyBytecode::put(std::ostream& os, int indent_level) const
{
    os << std::setw(indent_level) << "" << "(Empty)\n";
}

void
EmptyBytecode::finalize(Bytecode& bc, Bytecode& prev_bc)
{
    // do nothing
}

unsigned long
EmptyBytecode::calc_len(Bytecode& bc, Bytecode::AddSpanFunc add_span)
{
    return 0;
}

bool
EmptyBytecode::expand(Bytecode& bc, unsigned long& len, int span,
                      long old_val, long new_val,
                      /*@out@*/ long& neg_thres, /*@out@*/ long& pos_thres)
{
    // should never expand
    return Contents::expand(bc, len, span, old_val, new_val, neg_thres,
                            pos_thres);
}

void
EmptyBytecode::to_bytes(Bytecode& bc, Bytes& bytes,
                        OutputValueFunc output_value,
                        OutputRelocFunc output_reloc)
{
    // should never get converted to bytes
    Contents::to_bytes(bc, bytes, output_value, output_reloc);
}

EmptyBytecode*
EmptyBytecode::clone() const
{
    return new EmptyBytecode();
}

} // anonymous namespace

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

    // Initialize bytecodes with one empty bytecode (acts as "prior" for
    // first real bytecode in section to avoid NULL checks.
    m_bcs.push_back(EmptyBytecode::create(line));
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
    for (bc_iterator bc=m_bcs.begin()+1, end=m_bcs.end(), prevbc=m_bcs.begin();
         bc != end; ++bc, ++prevbc)
        bc->finalize(*prevbc, errwarns);
}

void
Section::update_bc_offsets(Errwarns& errwarns)
{
    unsigned long offset = 0;
    m_bcs.front().set_offset(0);
    for (bc_iterator bc=m_bcs.begin()+1, end=m_bcs.end(), prevbc=m_bcs.begin();
         bc != end; ++bc, ++prevbc)
        offset = bc->update_offset(offset, *prevbc, errwarns);
}

} // namespace yasm

//
// Bytecode container implementation.
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
#include "bc_container.h"

#include "util.h"

#include <iomanip>
#include <ostream>

#include "bytecode.h"


namespace yasm {

BytecodeContainer::BytecodeContainer()
    : m_object(0),
      m_bcs_owner(m_bcs)
{
    // A container always has at least one bytecode.
    start_bytecode();
}

BytecodeContainer::~BytecodeContainer()
{
}

Section*
BytecodeContainer::as_section()
{
    return 0;
}

const Section*
BytecodeContainer::as_section() const
{
    return 0;
}

void
BytecodeContainer::put(std::ostream& os, int indent_level) const
{
    for (const_bc_iterator bc=m_bcs.begin(), end=m_bcs.end();
         bc != end; ++bc) {
        os << std::setw(indent_level) << "" << "Next Bytecode:\n";
        bc->put(os, indent_level+1);
    }
}

void
BytecodeContainer::append_bytecode(std::auto_ptr<Bytecode> bc)
{
    if (bc.get() != 0) {
        bc->m_container = this; // record parent
        m_bcs.push_back(bc.release());
    }
}

Bytecode&
BytecodeContainer::start_bytecode()
{
    Bytecode* bc = new Bytecode;
    bc->m_container = this; // record parent
    m_bcs.push_back(bc);
    return *bc;
}

Bytecode&
BytecodeContainer::fresh_bytecode()
{
    Bytecode& bc = bcs_last();
    if (bc.has_contents())
        return start_bytecode();
    return bc;
}

void
BytecodeContainer::finalize(Errwarns& errwarns)
{
    for (bc_iterator bc=m_bcs.begin(), end=m_bcs.end(); bc != end; ++bc)
        bc->finalize(errwarns);
}

void
BytecodeContainer::update_offsets(Errwarns& errwarns)
{
    unsigned long offset = 0;
    m_bcs.front().set_offset(0);
    for (bc_iterator bc=m_bcs.begin(), end=m_bcs.end(); bc != end; ++bc)
        offset = bc->update_offset(offset, errwarns);
}

} // namespace yasm

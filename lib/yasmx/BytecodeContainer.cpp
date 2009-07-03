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
#include "yasmx/BytecodeContainer.h"

#include "util.h"

#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/marg_ostream.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytecode_util.h"
#include "yasmx/Expr.h"


namespace
{

using namespace yasm;

class GapBytecode : public Bytecode::Contents
{
public:
    GapBytecode(unsigned long size);
    ~GapBytecode();

    /// Prints the implementation-specific data (for debugging purposes).
    void Put(marg_ostream& os) const;

    /// Finalizes the bytecode after parsing.
    void Finalize(Bytecode& bc);

    /// Calculates the minimum size of a bytecode.
    unsigned long CalcLen(Bytecode& bc, const Bytecode::AddSpanFunc& add_span);

    /// Output a bytecode.
    void Output(Bytecode& bc, BytecodeOutput& bc_out);

    /// Increase the gap size.
    /// @param size     size in bytes
    void Extend(unsigned long size);

    GapBytecode* clone() const;

private:
    unsigned long m_size;       ///< size of gap (in bytes)
};


GapBytecode::GapBytecode(unsigned long size)
    : m_size(size)
{
}

GapBytecode::~GapBytecode()
{
}

void
GapBytecode::Put(marg_ostream& os) const
{
    os << "_Gap_\n";
    os << "Size=" << m_size << '\n';
}

void
GapBytecode::Finalize(Bytecode& bc)
{
}

unsigned long
GapBytecode::CalcLen(Bytecode& bc, const Bytecode::AddSpanFunc& add_span)
{
    return m_size;
}

void
GapBytecode::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    bc_out.OutputGap(m_size);
}

GapBytecode*
GapBytecode::clone() const
{
    return new GapBytecode(m_size);
}

void
GapBytecode::Extend(unsigned long size)
{
    m_size += size;
}

} // anonymous namespace

namespace yasm
{

BytecodeContainer::BytecodeContainer()
    : m_object(0),
      m_bcs_owner(m_bcs),
      m_last_gap(false)
{
    // A container always has at least one bytecode.
    StartBytecode();
}

BytecodeContainer::~BytecodeContainer()
{
}

Section*
BytecodeContainer::AsSection()
{
    return 0;
}

const Section*
BytecodeContainer::AsSection() const
{
    return 0;
}

void
BytecodeContainer::AppendBytecode(std::auto_ptr<Bytecode> bc)
{
    if (bc.get() != 0)
    {
        bc->m_container = this; // record parent
        m_bcs.push_back(bc.release());
    }
    m_last_gap = false;
}

Bytecode&
BytecodeContainer::AppendGap(unsigned long size, unsigned long line)
{
    if (m_last_gap)
    {
        static_cast<GapBytecode&>(*m_bcs.back().m_contents).Extend(size);
        return m_bcs.back();
    }
    Bytecode& bc = FreshBytecode();
    bc.Transform(Bytecode::Contents::Ptr(new GapBytecode(size)));
    bc.setLine(line);
    m_last_gap = true;
    return bc;
}

Bytecode&
BytecodeContainer::StartBytecode()
{
    Bytecode* bc = new Bytecode;
    bc->m_container = this; // record parent
    m_bcs.push_back(bc);
    m_last_gap = false;
    return *bc;
}

Bytecode&
BytecodeContainer::FreshBytecode()
{
    Bytecode& bc = bytecodes_last();
    if (bc.hasContents())
        return StartBytecode();
    return bc;
}

void
BytecodeContainer::Finalize(Errwarns& errwarns)
{
    for (bc_iterator bc=m_bcs.begin(), end=m_bcs.end(); bc != end; ++bc)
        ::Finalize(*bc, errwarns);
}

void
BytecodeContainer::UpdateOffsets(Errwarns& errwarns)
{
    unsigned long offset = 0;
    m_bcs.front().setOffset(0);
    for (bc_iterator bc=m_bcs.begin(), end=m_bcs.end(); bc != end; ++bc)
        offset = UpdateOffset(*bc, offset, errwarns);
}

marg_ostream&
operator<< (marg_ostream& os, const BytecodeContainer& container)
{
    os << "Bytecodes:\n";
    ++os;
    for (BytecodeContainer::const_bc_iterator bc=container.bytecodes_begin(),
         end=container.bytecodes_end(); bc != end; ++bc)
    {
        os << "Next Bytecode:\n";
        ++os;
        os << *bc;
        --os;
    }
    --os;
    return os;
}

} // namespace yasm

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

#include "llvm/ADT/Twine.h"
#include "llvm/Support/raw_ostream.h"
#include "YAML/emitter.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Expr.h"


using namespace yasm;

namespace {
class GapBytecode : public Bytecode::Contents
{
public:
    GapBytecode(unsigned long size);
    ~GapBytecode();

    /// Finalizes the bytecode after parsing.
    bool Finalize(Bytecode& bc, Diagnostic& diags);

    /// Calculates the minimum size of a bytecode.
    bool CalcLen(Bytecode& bc,
                 /*@out@*/ unsigned long* len,
                 const Bytecode::AddSpanFunc& add_span,
                 Diagnostic& diags);

    /// Output a bytecode.
    bool Output(Bytecode& bc, BytecodeOutput& bc_out);

    /// Increase the gap size.
    /// @param size     size in bytes
    void Extend(unsigned long size);

    llvm::StringRef getType() const;

    GapBytecode* clone() const;

    /// Write a YAML representation.  For debugging purposes.
    void Write(YAML::Emitter& out) const;

private:
    unsigned long m_size;       ///< size of gap (in bytes)
};
} // anonymous namespace

GapBytecode::GapBytecode(unsigned long size)
    : m_size(size)
{
}

GapBytecode::~GapBytecode()
{
}

bool
GapBytecode::Finalize(Bytecode& bc, Diagnostic& diags)
{
    return true;
}

bool
GapBytecode::CalcLen(Bytecode& bc,
                     /*@out@*/ unsigned long* len,
                     const Bytecode::AddSpanFunc& add_span,
                     Diagnostic& diags)
{
    *len = m_size;
    return true;
}

bool
GapBytecode::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    bc_out.OutputGap(m_size, bc.getSource());
    return true;
}

llvm::StringRef
GapBytecode::getType() const
{
    return "yasm::GapBytecode";
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

void
GapBytecode::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << "Gap";
    out << YAML::Key << "size" << YAML::Value << m_size;
    out << YAML::EndMap;
}

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
BytecodeContainer::AppendGap(unsigned long size, SourceLocation source)
{
    if (m_last_gap)
    {
        static_cast<GapBytecode&>(*m_bcs.back().m_contents).Extend(size);
        return m_bcs.back();
    }
    Bytecode& bc = FreshBytecode();
    bc.Transform(Bytecode::Contents::Ptr(new GapBytecode(size)));
    bc.setSource(source);
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
    Bytecode& bc = bytecodes_back();
    if (bc.hasContents())
        return StartBytecode();
    return bc;
}

Location
BytecodeContainer::getEndLoc()
{
    Bytecode& last = m_bcs.back();
    Location loc = { &last, last.getTotalLen() };
    return loc;
}

void
BytecodeContainer::Finalize(Diagnostic& diags)
{
    for (bc_iterator bc=m_bcs.begin(), end=m_bcs.end(); bc != end; ++bc)
        bc->Finalize(diags);
}

void
BytecodeContainer::UpdateOffsets(Diagnostic& diags)
{
    unsigned long offset = 0;
    m_bcs.front().setOffset(0);
    for (bc_iterator bc=m_bcs.begin(), end=m_bcs.end(); bc != end; ++bc)
        offset = bc->UpdateOffset(offset, diags);
}

void
BytecodeContainer::Write(YAML::Emitter& out) const
{
    out << YAML::BeginSeq;
    for (BytecodeContainer::const_bc_iterator bc=m_bcs.begin(), end=m_bcs.end();
         bc != end; ++bc)
    {
        out << YAML::Anchor("BC@" + llvm::Twine::utohexstr((uint64_t)&(*bc)))
            << *bc;
    }
    out << YAML::EndSeq;
}

void
BytecodeContainer::Dump() const
{
    YAML::Emitter out;
    Write(out);
    llvm::errs() << out.c_str() << '\n';
}

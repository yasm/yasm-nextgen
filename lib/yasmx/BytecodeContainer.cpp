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

#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Expr.h"
#include "yasmx/Optimizer.h"


using namespace yasm;

namespace {
class GapBytecode : public Bytecode::Contents
{
public:
    GapBytecode(unsigned long size);
    ~GapBytecode();

    /// Finalizes the bytecode after parsing.
    bool Finalize(Bytecode& bc, DiagnosticsEngine& diags);

    /// Calculates the minimum size of a bytecode.
    bool CalcLen(Bytecode& bc,
                 /*@out@*/ unsigned long* len,
                 const Bytecode::AddSpanFunc& add_span,
                 DiagnosticsEngine& diags);

    /// Output a bytecode.
    bool Output(Bytecode& bc, BytecodeOutput& bc_out);

    /// Increase the gap size.
    /// @param size     size in bytes
    void Extend(unsigned long size);

    StringRef getType() const;

    GapBytecode* clone() const;

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

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
GapBytecode::Finalize(Bytecode& bc, DiagnosticsEngine& diags)
{
    return true;
}

bool
GapBytecode::CalcLen(Bytecode& bc,
                     /*@out@*/ unsigned long* len,
                     const Bytecode::AddSpanFunc& add_span,
                     DiagnosticsEngine& diags)
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

StringRef
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

#ifdef WITH_XML
pugi::xml_node
GapBytecode::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("Gap");
    append_child(root, "Size", m_size);
    return root;
}
#endif // WITH_XML

BytecodeContainer::BytecodeContainer(Section* sect)
    : m_sect(sect),
      m_bcs_owner(m_bcs),
      m_last_gap(false)
{
    // A container always has at least one bytecode.
    StartBytecode();
}

BytecodeContainer::~BytecodeContainer()
{
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
    Bytecode& last = FreshBytecode();
    Location loc = { &last, last.getFixedLen() };
    return loc;
}

void
BytecodeContainer::Finalize(DiagnosticsEngine& diags)
{
    for (bc_iterator bc=m_bcs.begin(), end=m_bcs.end(); bc != end; ++bc)
        bc->Finalize(diags);
}

void
BytecodeContainer::UpdateOffsets(DiagnosticsEngine& diags)
{
    unsigned long offset = 0;
    m_bcs.front().setOffset(0);
    for (bc_iterator bc=m_bcs.begin(), end=m_bcs.end(); bc != end; ++bc)
        offset = bc->UpdateOffset(offset, diags);
}

void
BytecodeContainer::Optimize(DiagnosticsEngine& diags)
{
    Optimizer opt(diags);

    // Step 1a
    unsigned long bc_index = 0;
    unsigned long offset = 0;

    // Set the offset of the first (empty) bytecode.
    m_bcs.front().setIndex(bc_index++);
    m_bcs.front().setOffset(0);

    // Iterate through the remainder, if any.
    for (bc_iterator bc=m_bcs.begin(), bcend=m_bcs.end(); bc != bcend; ++bc)
    {
        bc->setIndex(bc_index++);
        bc->setOffset(offset);

        if (bc->CalcLen(TR1::bind(&Optimizer::AddSpan, &opt,
                                  _1, _2, _3, _4, _5),
                        diags))
        {
            if (bc->getSpecial() == Bytecode::Contents::SPECIAL_OFFSET)
                opt.AddOffsetSetter(*bc);

            offset = bc->getNextOffset();
        }
    }

    if (diags.hasErrorOccurred())
        return;

    // Step 1b
    opt.Step1b();
    if (diags.hasErrorOccurred())
        return;

    // Step 1c
    UpdateOffsets(diags);
    if (diags.hasErrorOccurred())
        return;

    // Step 1d
    if (opt.Step1d())
        return;

    // Step 1e
    opt.Step1e();
    if (diags.hasErrorOccurred())
        return;

    // Step 2
    opt.Step2();
    if (diags.hasErrorOccurred())
        return;

    // Step 3
    UpdateOffsets(diags);
}

#ifdef WITH_XML
pugi::xml_node
BytecodeContainer::Write(pugi::xml_node out) const
{
    for (BytecodeContainer::const_bc_iterator bc=m_bcs.begin(), end=m_bcs.end();
         bc != end; ++bc)
        append_data(out, *bc);
    return out;
}
#endif // WITH_XML

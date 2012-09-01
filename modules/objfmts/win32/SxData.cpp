//
// Bytecode for Win32 .sxdata sections
//
//  Copyright (C) 2002-2008  Peter Johnson
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
#include "SxData.h"

#include "yasmx/BytecodeContainer.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes_util.h"

#include "modules/objfmts/coff/CoffSymbol.h"


using namespace yasm;
using namespace yasm::objfmt;

namespace {
class SxData : public Bytecode::Contents
{
public:
    SxData(SymbolRef sym);
    ~SxData();

    /// Finalizes the bytecode after parsing.
    bool Finalize(Bytecode& bc, DiagnosticsEngine& diags);

    /// Calculates the minimum size of a bytecode.
    bool CalcLen(Bytecode& bc,
                 /*@out@*/ unsigned long* len,
                 const Bytecode::AddSpanFunc& add_span,
                 DiagnosticsEngine& diags);

    /// Convert a bytecode into its byte representation.
    bool Output(Bytecode& bc, BytecodeOutput& bc_out);

    StringRef getType() const;

    SxData* clone() const;

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

private:
    SymbolRef m_sym;            ///< symbol
};
} // anonymous namespace

SxData::SxData(SymbolRef sym)
    : m_sym(sym)
{
}

SxData::~SxData()
{
}

bool
SxData::Finalize(Bytecode& bc, DiagnosticsEngine& diags)
{
    return true;
}

bool
SxData::CalcLen(Bytecode& bc,
                /*@out@*/ unsigned long* len,
                const Bytecode::AddSpanFunc& add_span,
                DiagnosticsEngine& diags)
{
    *len = 4;
    return true;
}

bool
SxData::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    CoffSymbol* coffsym = m_sym->getAssocData<CoffSymbol>();
    assert(coffsym && "no symbol data for SAFESEH symbol");

    Bytes& bytes = bc_out.getScratch();
    bytes.setLittleEndian();
    Write32(bytes, coffsym->m_index);
    bc_out.OutputBytes(bytes, bc.getSource());
    return true;
}

StringRef
SxData::getType() const
{
    return "yasm::objfmt::SxData";
}

SxData*
SxData::clone() const
{
    return new SxData(m_sym);
}

#ifdef WITH_XML
pugi::xml_node
SxData::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("SxData");
    append_child(root, "Sym", m_sym);
    return root;
}
#endif // WITH_XML

void
objfmt::AppendSxData(BytecodeContainer& container,
                     SymbolRef sym,
                     SourceLocation source)
{
    Bytecode& bc = container.FreshBytecode();
    bc.Transform(Bytecode::Contents::Ptr(new SxData(sym)));
    bc.setSource(source);
}

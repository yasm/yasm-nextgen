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

#include <YAML/emitter.h>
#include <yasmx/BytecodeContainer.h>
#include <yasmx/BytecodeOutput.h>
#include <yasmx/Bytecode.h>
#include <yasmx/Bytes_util.h>

#include "modules/objfmts/coff/CoffSymbol.h"


namespace
{

using namespace yasm;

class SxData : public Bytecode::Contents
{
public:
    SxData(SymbolRef sym);
    ~SxData();

    /// Finalizes the bytecode after parsing.
    void Finalize(Bytecode& bc);

    /// Calculates the minimum size of a bytecode.
    unsigned long CalcLen(Bytecode& bc, const Bytecode::AddSpanFunc& add_span);

    /// Convert a bytecode into its byte representation.
    void Output(Bytecode& bc, BytecodeOutput& bc_out);

    SxData* clone() const;

    /// Write a YAML representation.  For debugging purposes.
    void Write(YAML::Emitter& out) const;

private:
    SymbolRef m_sym;            ///< symbol
};

SxData::SxData(SymbolRef sym)
    : m_sym(sym)
{
}

SxData::~SxData()
{
}

void
SxData::Finalize(Bytecode& bc)
{
}

unsigned long
SxData::CalcLen(Bytecode& bc, const Bytecode::AddSpanFunc& add_span)
{
    return 4;
}

void
SxData::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    yasm::objfmt::coff::CoffSymbol* coffsym =
        m_sym->getAssocData<yasm::objfmt::coff::CoffSymbol>();
    assert(coffsym && "no symbol data for SAFESEH symbol");

    Bytes& bytes = bc_out.getScratch();
    bytes.setLittleEndian();
    Write32(bytes, coffsym->m_index);
    bc_out.Output(bytes);
}

SxData*
SxData::clone() const
{
    return new SxData(m_sym);
}

void
SxData::Write(YAML::Emitter& out) const
{
    out << YAML::Flow << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << "SxData";
    out << YAML::Key << "sym" << YAML::Value << m_sym;
    out << YAML::EndMap;
}

} // anonymous namespace

namespace yasm
{
namespace objfmt
{
namespace win32
{

void
AppendSxData(BytecodeContainer& container, SymbolRef sym, unsigned long line)
{
    Bytecode& bc = container.FreshBytecode();
    bc.Transform(Bytecode::Contents::Ptr(new SxData(sym)));
    bc.setLine(line);
}

}}} // namespace yasm::objfmt::win32

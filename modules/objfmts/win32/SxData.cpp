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

#include <libyasmx/BytecodeContainer.h>
#include <libyasmx/BytecodeOutput.h>
#include <libyasmx/Bytecode.h>
#include <libyasmx/Bytes_util.h>

#include "modules/objfmts/coff/CoffSymbol.h"


namespace
{

using namespace yasm;

class SxData : public Bytecode::Contents
{
public:
    SxData(SymbolRef sym);
    ~SxData();

    /// Prints the implementation-specific data (for debugging purposes).
    void put(marg_ostream& os) const;

    /// Finalizes the bytecode after parsing.
    void finalize(Bytecode& bc);

    /// Calculates the minimum size of a bytecode.
    unsigned long calc_len(Bytecode& bc, const Bytecode::AddSpanFunc& add_span);

    /// Convert a bytecode into its byte representation.
    void output(Bytecode& bc, BytecodeOutput& bc_out);

    SxData* clone() const;

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
SxData::put(marg_ostream& os) const
{
    // TODO
}

void
SxData::finalize(Bytecode& bc)
{
}

unsigned long
SxData::calc_len(Bytecode& bc, const Bytecode::AddSpanFunc& add_span)
{
    return 4;
}

void
SxData::output(Bytecode& bc, BytecodeOutput& bc_out)
{
    yasm::objfmt::coff::CoffSymbol* coffsym =
        yasm::objfmt::coff::get_coff(*m_sym);
    assert(coffsym && "no symbol data for SAFESEH symbol");

    Bytes& bytes = bc_out.get_scratch();
    bytes << little_endian;
    write_32(bytes, coffsym->m_index);
    bc_out.output(bytes);
}

SxData*
SxData::clone() const
{
    return new SxData(m_sym);
}

} // anonymous namespace

namespace yasm
{
namespace objfmt
{
namespace win32
{

void
append_sxdata(BytecodeContainer& container, SymbolRef sym, unsigned long line)
{
    Bytecode& bc = container.fresh_bytecode();
    bc.transform(Bytecode::Contents::Ptr(new SxData(sym)));
    bc.set_line(line);
}

}}} // namespace yasm::objfmt::win32

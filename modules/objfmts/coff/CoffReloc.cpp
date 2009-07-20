//
// COFF object format relocation implementation
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
#include "CoffReloc.h"

#include <util.h>

#include <YAML/emitter.h>
#include <yasmx/Bytes.h>
#include <yasmx/Bytes_util.h>
#include <yasmx/Errwarns.h>

#include "CoffSymbol.h"


namespace yasm
{
namespace objfmt
{
namespace coff
{

CoffReloc::CoffReloc(const IntNum& addr, SymbolRef sym, Type type)
    : Reloc(addr, sym)
    , m_type(type)
{
}

CoffReloc::~CoffReloc()
{
}

Expr
CoffReloc::getValue() const
{
    return Expr(m_sym);
}

void
CoffReloc::Write(Bytes& bytes) const
{
    const CoffSymbol* csym = getCoff(*m_sym);
    assert(csym != 0);      // need symbol data for relocated symbol

    bytes << little_endian;

    Write32(bytes, m_addr);             // address of relocation
    Write32(bytes, csym->m_index);      // relocated symbol
    Write16(bytes, m_type);             // type of relocation
}

Coff32Reloc::~Coff32Reloc()
{
}

std::string
Coff32Reloc::getTypeName() const
{
    const char* name;
    switch (m_type)
    {
        case ABSOLUTE:          name = "ABSOLUTE"; break;
        case I386_ADDR16:       name = "I386_ADDR16"; break;
        case I386_REL16:        name = "I386_REL16"; break;
        case I386_ADDR32:       name = "I386_ADDR32"; break;
        case I386_ADDR32NB:     name = "I386_ADDR32NB"; break;
        case I386_SEG12:        name = "I386_SEG12"; break;
        case I386_SECTION:      name = "I386_SECTION"; break;
        case I386_SECREL:       name = "I386_SECREL"; break;
        case I386_TOKEN:        name = "I386_TOKEN"; break;
        case I386_SECREL7:      name = "I386_SECREL7"; break;
        case I386_REL32:        name = "I386_REL32"; break;
        default:                name = "***UNKNOWN***"; break;
    }

    return name;
}

Coff64Reloc::~Coff64Reloc()
{
}

std::string
Coff64Reloc::getTypeName() const
{
    const char* name;
    switch (m_type)
    {
        case ABSOLUTE:          name = "ABSOLUTE"; break;
        case AMD64_ADDR64:      name = "AMD64_ADDR64"; break;
        case AMD64_ADDR32:      name = "AMD64_ADDR32"; break;
        case AMD64_ADDR32NB:    name = "AMD64_ADDR32NB"; break;
        case AMD64_REL32:       name = "AMD64_REL32"; break;
        case AMD64_REL32_1:     name = "AMD64_REL32_1"; break;
        case AMD64_REL32_2:     name = "AMD64_REL32_2"; break;
        case AMD64_REL32_3:     name = "AMD64_REL32_3"; break;
        case AMD64_REL32_4:     name = "AMD64_REL32_4"; break;
        case AMD64_REL32_5:     name = "AMD64_REL32_5"; break;
        case AMD64_SECTION:     name = "AMD64_SECTION"; break;
        case AMD64_SECREL:      name = "AMD64_SECREL"; break;
        case AMD64_SECREL7:     name = "AMD64_SECREL7"; break;
        case AMD64_TOKEN:       name = "AMD64_TOKEN"; break;
        default:                name = "***UNKNOWN***"; break;
    }

    return name;
}

}}} // namespace yasm::objfmt::coff

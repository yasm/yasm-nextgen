//
// Mach-O relocation
//
//  Copyright (C) 2004-2012  Peter Johnson
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
#include "MachReloc.h"

#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"

#include "MachSection.h"
#include "MachSymbol.h"


using namespace yasm;
using namespace yasm::objfmt;

MachReloc::MachReloc(const IntNum& addr,
                     SymbolRef sym,
                     Type type,
                     bool pcrel,
                     unsigned int length,
                     bool ext)
    : Reloc(addr, sym)
    , m_pcrel(pcrel)
    , m_length(length)
    , m_ext(ext)
    , m_type(type)
{
}

MachReloc::~MachReloc()
{
}

void
MachReloc::Write(Bytes& bytes) const
{
    bytes.setLittleEndian();

    Write32(bytes, m_addr);         // address of relocation

    unsigned long symnum;
    if (m_ext)
    {
        const MachSymbol* msym = m_sym->getAssocData<MachSymbol>();
        assert(msym != 0);      // need symbol data for relocated symbol
        symnum = msym->m_index;
    }
    else
    {
        symnum = 0; // default to absolute

        // find section where the symbol relates to
        Location loc;
        Section* sect;
        MachSection* msect;
        if (m_sym->getLabel(&loc) &&
            (sect = loc.bc->getContainer()->getSection()) &&
            (msect = sect->getAssocData<MachSection>()))
            symnum = msect->scnum+1;
    }
    Write32(bytes,
            (symnum & 0x00ffffffUL) |
            (m_pcrel ? 1UL<<24 : 0UL) |
            ((m_length & 3UL) << 25) |
            (m_ext ? 1UL<<27 : 0UL) |
            ((m_type & 0xfUL) << 28));
}

#ifdef WITH_XML
pugi::xml_node
MachReloc::DoWrite(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("MachReloc");
    append_child(root, "Type", getTypeName());
    append_child(root, "PcRel", m_pcrel);
    append_child(root, "Length", m_length);
    append_child(root, "Ext", m_ext);
    return root;
}
#endif // WITH_XML

Mach32Reloc::~Mach32Reloc()
{
}

std::string
Mach32Reloc::getTypeName() const
{
    const char* name;
    switch (m_type)
    {
        case GENERIC_RELOC_VANILLA:     name = "GENERIC_RELOC_VANILLA"; break;
        case GENERIC_RELOC_PAIR:        name = "GENERIC_RELOC_PAIR"; break;
        case GENERIC_RELOC_SECTDIFF:    name = "GENERIC_RELOC_SECTDIFF"; break;
        case GENERIC_RELOC_PB_LA_PTR:   name = "GENERIC_RELOC_PB_LA_PTR"; break;
        case GENERIC_RELOC_LOCAL_SECTDIFF:
            name = "GENERIC_RELOC_LOCAL_SECTDIFF"; break;
        default:                        name = "***UNKNOWN***"; break;
    }

    return name;
}

Mach64Reloc::~Mach64Reloc()
{
}

std::string
Mach64Reloc::getTypeName() const
{
    const char* name;
    switch (m_type)
    {
        case X86_64_RELOC_UNSIGNED:     name = "X86_64_RELOC_UNSIGNED"; break;
        case X86_64_RELOC_SIGNED:       name = "X86_64_RELOC_SIGNED"; break;
        case X86_64_RELOC_BRANCH:       name = "X86_64_RELOC_BRANCH"; break;
        case X86_64_RELOC_GOT_LOAD:     name = "X86_64_RELOC_GOT_LOAD"; break;
        case X86_64_RELOC_GOT:          name = "X86_64_RELOC_GOT"; break;
        case X86_64_RELOC_SUBTRACTOR:   name = "X86_64_RELOC_SUBTRACTOR"; break;
        case X86_64_RELOC_SIGNED_1:     name = "X86_64_RELOC_SIGNED_1"; break;
        case X86_64_RELOC_SIGNED_2:     name = "X86_64_RELOC_SIGNED_2"; break;
        case X86_64_RELOC_SIGNED_4:     name = "X86_64_RELOC_SIGNED_4"; break;
        default:                        name = "***UNKNOWN***"; break;
    }

    return name;
}

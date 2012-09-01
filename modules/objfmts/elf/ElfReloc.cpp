//
// ELF object format relocation
//
//  Copyright (C) 2003-2007  Michael Urman
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
#include "ElfReloc.h"

#include "yasmx/Bytes_util.h"
#include "yasmx/InputBuffer.h"

#include "ElfConfig.h"
#include "ElfMachine.h"
#include "ElfSymbol.h"


using namespace yasm;
using namespace yasm::objfmt;

ElfReloc::ElfReloc(SymbolRef sym, const IntNum& addr)
    : Reloc(addr, sym)
    , m_type(0xff)      // default to invalid type
    , m_addend(0)
{
    assert(sym != 0 && "sym is null");
}

ElfReloc::ElfReloc(const ElfConfig& config,
                   const ElfSymtab& symtab,
                   const MemoryBuffer& in,
                   unsigned long* pos,
                   bool rela)
    : Reloc(0, SymbolRef(0))
    , m_type(0xff)
    , m_addend(0)
{
    InputBuffer inbuf(in, *pos);
    config.setEndian(inbuf);

    unsigned long size = 0;
    if (config.cls == ELFCLASS32)
    {
        size = rela ? RELOC32A_SIZE : RELOC32_SIZE;
        if (size > inbuf.getReadableSize())
            return;

        m_addr = ReadU32(inbuf);

        unsigned long info = ReadU32(inbuf);
        m_sym = symtab.at(ELF32_R_SYM(info));
        m_type = ELF32_R_TYPE(info);

        if (rela)
            m_addend = ReadS32(inbuf);
    }
    else if (config.cls == ELFCLASS64)
    {
        size = rela ? RELOC64A_SIZE : RELOC64_SIZE;
        if (size > inbuf.getReadableSize())
            return;

        m_addr = ReadU64(inbuf);

        IntNum info = ReadU64(inbuf);
        m_sym = symtab.at(ELF64_R_SYM(info));
        m_type = ELF64_R_TYPE(info);

        if (rela)
            m_addend = ReadS64(inbuf);
    }
    else
        assert(false && "unknown elf class");

    *pos += size;
}

ElfReloc::~ElfReloc()
{
}

bool
ElfReloc::setWrt(SymbolRef wrt, size_t valsize)
{
    assert(wrt != 0 && "wrt is null");

    const ElfSpecialSymbol* ssym = wrt->getAssocData<ElfSpecialSymbol>();
    if (!ssym || valsize != ssym->size)
        return false;

    // Force TLS type; this is required by the linker.
    if (ssym->thread_local)
    {
        if (ElfSymbol* esym = m_sym->getAssocData<ElfSymbol>())
            esym->setType(STT_TLS);
    }
    m_wrt = wrt;
    m_type = ssym->reloc;
    return true;
}

Expr
ElfReloc::getValue() const
{
    assert(isValid() && "invalid relocation");
    Expr e(m_sym);
    if (!m_addend.isZero())
        e += m_addend;
    return e;
}

void
ElfReloc::HandleAddend(IntNum* intn,
                       const ElfConfig& config,
                       unsigned int insn_start)
{
    assert(isValid() && "invalid relocation");
    // rela sections put the addend into the relocation, and write 0 in
    // data area.
    if (config.rela)
    {
        m_addend = *intn;
        *intn = 0;
    }
}

void
ElfReloc::Write(Bytes& bytes, const ElfConfig& config)
{
    assert(isValid() && "invalid relocation");
    unsigned long r_sym = STN_UNDEF;

    if (ElfSymbol* esym = getSymbol()->getAssocData<ElfSymbol>())
        r_sym = esym->getSymbolIndex();

    bytes.resize(0);
    config.setEndian(bytes);

    if (config.cls == ELFCLASS32)
    {
        Write32(bytes, m_addr);
        Write32(bytes,
                ELF32_R_INFO(r_sym, static_cast<unsigned char>(m_type)));

        if (config.rela)
            Write32(bytes, m_addend);
    }
    else if (config.cls == ELFCLASS64)
    {
        Write64(bytes, m_addr);
        Write64(bytes,
                ELF64_R_INFO(r_sym, static_cast<unsigned char>(m_type)));
        if (config.rela)
            Write64(bytes, m_addend);
    }
}

#ifdef WITH_XML
pugi::xml_node
ElfReloc::DoWrite(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("ElfReloc");
    append_child(root, "Addend", m_addend);
    return root;
}
#endif // WITH_XML

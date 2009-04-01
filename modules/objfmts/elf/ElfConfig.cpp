//
// ELF object format config
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
#include "ElfConfig.h"

#include <util.h>

#include "yasmx/Support/errwarn.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/Object.h"

#include "ElfSymbol.h"


namespace yasm
{
namespace objfmt
{
namespace elf
{

ElfConfig::ElfConfig()
    : cls(ELFCLASSNONE)
    , encoding(ELFDATANONE)
    , version(EV_CURRENT)
    , osabi(ELFOSABI_SYSV)
    , abi_version(0)
    , file_type(ET_REL)
    , start(0)
    , rela(false)
    , proghead_pos(0)
    , proghead_count(0)
    , proghead_size(0)
    , secthead_pos(0)
    , secthead_count(0)
    , secthead_size(0)
    , machine_flags(0)
    , shstrtab_index(0)
{
}

ElfSymbolIndex
ElfConfig::symtab_setindexes(Object& object, ElfSymbolIndex* nlocal) const
{
    // start at 1 due to undefined symbol (0)
    ElfSymbolIndex num = 1;
    *nlocal = 1;
    for (Object::symbol_iterator i=object.symbols_begin(),
         end=object.symbols_end(); i != end; ++i)
    {
        ElfSymbol* elfsym = get_elf(*i);
        if (!elfsym)
            continue;

        elfsym->set_symindex(num);

        if (elfsym->is_local())
            *nlocal = num;

        ++num;
    }
    return num;
}

unsigned long
ElfConfig::symtab_write(std::ostream& os,
                        Object& object,
                        Errwarns& errwarns,
                        Bytes& scratch) const
{
    unsigned long size = 0;

    // write undef symbol
    ElfSymbol undef;
    scratch.resize(0);
    undef.write(scratch, *this);
    os << scratch;
    size += scratch.size();

    // write other symbols
    for (Object::symbol_iterator sym=object.symbols_begin(),
         end=object.symbols_end(); sym != end; ++sym)
    {
        ElfSymbol* elfsym = get_elf(*sym);
        if (!elfsym)
            continue;

        elfsym->finalize(*sym, errwarns);

        scratch.resize(0);
        elfsym->write(scratch, *this);
        os << scratch;
        size += scratch.size();
    }
    return size;
}

bool
ElfConfig::symtab_read(std::istream&      is,
                       ElfSymtab&         symtab,
                       Object&            object,
                       unsigned long      size,
                       ElfSize            symsize,
                       const StringTable& strtab,
                       Section*           sections[]) const
{
    is.seekg(symsize, std::ios_base::cur);  // skip first symbol (undef)
    symtab.push_back(SymbolRef(0));

    Bytes bytes;
    ElfSymbolIndex index = 1;
    for (unsigned long pos=symsize; pos<size; pos += symsize, ++index)
    {
        bytes.resize(0);
        bytes.write(is, symsize);
        if (!is)
            throw Error(N_("could not read symbol entry"));

        std::auto_ptr<ElfSymbol> elfsym(
            new ElfSymbol(*this, bytes, index, sections));

        SymbolRef sym = elfsym->create_symbol(object, strtab);
        symtab.push_back(sym);

        if (sym)
        {
            // Associate symbol data with symbol
            sym->add_assoc_data(ElfSymbol::key,
                std::auto_ptr<AssocData>(elfsym.release()));
        }
    }

    return true;
}

unsigned long
ElfConfig::proghead_get_size() const
{
    if (cls == ELFCLASS32)
        return EHDR32_SIZE;
    else if (cls == ELFCLASS64)
        return EHDR64_SIZE;
    else
        return 0;
}

bool
ElfConfig::proghead_read(std::istream& is)
{
    Bytes bytes;

    // read magic number and elf class
    is.seekg(0);
    bytes.write(is, 5);
    if (!is)
        return false;

    if (read_u8(bytes) != ELFMAG0)
        return false;
    if (read_u8(bytes) != ELFMAG1)
        return false;
    if (read_u8(bytes) != ELFMAG2)
        return false;
    if (read_u8(bytes) != ELFMAG3)
        return false;

    cls = static_cast<ElfClass>(read_u8(bytes));

    // determine header size
    unsigned long hdrsize = proghead_get_size();
    if (hdrsize < 5)
        return false;

    // read remainder of header
    bytes.write(is, hdrsize-5);
    if (!is)
        return false;

    encoding = static_cast<ElfDataEncoding>(read_u8(bytes));
    if (!setup_endian(bytes))
        return false;

    version = static_cast<ElfVersion>(read_u8(bytes));
    if (version != EV_CURRENT)
        return false;

    osabi = static_cast<ElfOsabiIndex>(read_u8(bytes));
    abi_version = read_u8(bytes);
    bytes.set_readpos(EI_NIDENT);
    file_type = static_cast<ElfFileType>(read_u16(bytes));
    machine_type = static_cast<ElfMachineType>(read_u16(bytes));
    version = static_cast<ElfVersion>(read_u32(bytes));
    if (version != EV_CURRENT)
        return false;

    if (cls == ELFCLASS32)
    {
        start = read_u32(bytes);
        proghead_pos = read_u32(bytes);
        secthead_pos = read_u32(bytes);
    }
    else if (cls == ELFCLASS64)
    {
        start = read_u64(bytes);
        proghead_pos = read_u64(bytes).get_uint();
        secthead_pos = read_u64(bytes).get_uint();
    }

    machine_flags = read_u32(bytes);
    read_u16(bytes);                // e_ehsize (don't care)
    proghead_size = read_u16(bytes);
    proghead_count = read_u16(bytes);
    secthead_size = read_u16(bytes);
    secthead_count = read_u16(bytes);
    shstrtab_index = read_u16(bytes);

    return true;
}

void
ElfConfig::proghead_write(std::ostream& os, Bytes& scratch)
{
    scratch.resize(0);
    setup_endian(scratch);

    // ELF magic number
    write_8(scratch, ELFMAG0);
    write_8(scratch, ELFMAG1);
    write_8(scratch, ELFMAG2);
    write_8(scratch, ELFMAG3);

    write_8(scratch, cls);              // elf class
    write_8(scratch, encoding);         // data encoding :: MSB?
    write_8(scratch, version);          // elf version
    write_8(scratch, osabi);            // os/abi
    write_8(scratch, abi_version);      // ABI version
    while (scratch.size() < EI_NIDENT)
        write_8(scratch, 0);            // e_ident padding

    write_16(scratch, file_type);       // e_type
    write_16(scratch, machine_type);    // e_machine - or others
    write_32(scratch, version);         // elf version

    unsigned int ehdr_size = 0;
    if (cls == ELFCLASS32)
    {
        write_32(scratch, start);           // e_entry execution startaddr
        write_32(scratch, proghead_pos);    // e_phoff program header off
        write_32(scratch, secthead_pos);    // e_shoff section header off
        ehdr_size = EHDR32_SIZE;
        secthead_size = SHDR32_SIZE;
    }
    else if (cls == ELFCLASS64)
    {
        write_64(scratch, start);           // e_entry execution startaddr
        write_64(scratch, proghead_pos);    // e_phoff program header off
        write_64(scratch, secthead_pos);    // e_shoff section header off
        ehdr_size = EHDR64_SIZE;
        secthead_size = SHDR64_SIZE;
    }

    write_32(scratch, machine_flags);   // e_flags
    write_16(scratch, ehdr_size);       // e_ehsize
    write_16(scratch, proghead_size);   // e_phentsize
    write_16(scratch, proghead_count);  // e_phnum
    write_16(scratch, secthead_size);   // e_shentsize
    write_16(scratch, secthead_count);  // e_shnum
    write_16(scratch, shstrtab_index);  // e_shstrndx

    assert(scratch.size() == proghead_get_size());

    os << scratch;
}

std::string
ElfConfig::name_reloc_section(const std::string& basesect) const
{
    if (rela)
        return ".rela"+basesect;
    else
        return ".rel"+basesect;
}

bool
ElfConfig::setup_endian(Bytes& bytes) const
{
    if (encoding == ELFDATA2LSB)
        bytes << little_endian;
    else if (encoding == ELFDATA2MSB)
        bytes << big_endian;
    else
        return false;
    return true;
}

}}} // namespace yasm::objfmt::elf

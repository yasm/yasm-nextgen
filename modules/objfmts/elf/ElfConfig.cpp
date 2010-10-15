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

#include "llvm/Support/raw_ostream.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/InputBuffer.h"
#include "yasmx/Object.h"

#include "ElfSection.h"
#include "ElfSymbol.h"


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
ElfConfig::AssignSymbolIndices(Object& object, ElfSymbolIndex* nlocal) const
{
    ElfSymbolIndex num = *nlocal;

    for (Object::symbol_iterator i=object.symbols_begin(),
         end=object.symbols_end(); i != end; ++i)
    {
        ElfSymbol* elfsym = i->getAssocData<ElfSymbol>();
        if (!elfsym || !elfsym->isInTable())
            continue;

        if (elfsym->getSymbolIndex() != 0)
            continue;

        elfsym->setSymbolIndex(num);

        ++num;
        if (elfsym->isLocal())
            *nlocal = num;
    }
    return num;
}

unsigned long
ElfConfig::WriteSymbolTable(llvm::raw_ostream& os,
                            Object& object,
                            Diagnostic& diags,
                            Bytes& scratch) const
{
    unsigned long size = 0;

    // write undef symbol
    ElfSymbol undef;
    scratch.resize(0);
    undef.Write(scratch, *this, diags);
    os << scratch;
    size += scratch.size();

    // write other symbols
    for (Object::symbol_iterator sym=object.symbols_begin(),
         end=object.symbols_end(); sym != end; ++sym)
    {
        ElfSymbol* elfsym = sym->getAssocData<ElfSymbol>();
        if (!elfsym || !elfsym->isInTable())
            continue;

        scratch.resize(0);
        elfsym->Write(scratch, *this, diags);
        os << scratch;
        size += scratch.size();
    }
    return size;
}

bool
ElfConfig::ReadSymbolTable(const llvm::MemoryBuffer&    in,
                           const ElfSection&            symtab_sect,
                           ElfSymtab&                   symtab,
                           Object&                      object,
                           const StringTable&           strtab,
                           Section*                     sections[],
                           Diagnostic&                  diags) const
{
    ElfSize symsize = symtab_sect.getEntSize();
    if (symsize == 0)
    {
        diags.Report(SourceLocation(), diag::err_symbol_entity_size_zero);
        return false;
    }

    unsigned long size = symtab_sect.getSize().getUInt();

    // Symbol table always starts with null entry
    symtab.push_back(SymbolRef(0));

    ElfSymbolIndex index = 1;
    for (unsigned long pos=symsize; pos<size; pos += symsize, ++index)
    {
        std::auto_ptr<ElfSymbol> elfsym(
            new ElfSymbol(*this, in, symtab_sect, index, sections, diags));
        if (diags.hasErrorOccurred())
            return false;

        SymbolRef sym = elfsym->CreateSymbol(object, strtab);
        symtab.push_back(sym);

        if (sym)
            sym->AddAssocData(elfsym);  // Associate symbol data with symbol
    }
    return true;
}

unsigned long
ElfConfig::getProgramHeaderSize() const
{
    if (cls == ELFCLASS32)
        return EHDR32_SIZE;
    else if (cls == ELFCLASS64)
        return EHDR64_SIZE;
    else
        return 0;
}

bool
ElfConfig::ReadProgramHeader(const llvm::MemoryBuffer& in)
{
    InputBuffer inbuf(in);

    // check magic number and read elf class
    if (inbuf.getReadableSize() < 5)
        return false;

    if (ReadU8(inbuf) != ELFMAG0)
        return false;
    if (ReadU8(inbuf) != ELFMAG1)
        return false;
    if (ReadU8(inbuf) != ELFMAG2)
        return false;
    if (ReadU8(inbuf) != ELFMAG3)
        return false;

    cls = static_cast<ElfClass>(ReadU8(inbuf));

    // determine header size
    unsigned long hdrsize = getProgramHeaderSize();
    if (hdrsize < 5)
        return false;

    // read remainder of header
    if (inbuf.getReadableSize() < hdrsize-5)
        return false;

    encoding = static_cast<ElfDataEncoding>(ReadU8(inbuf));
    if (!setEndian(inbuf))
        return false;

    version = static_cast<ElfVersion>(ReadU8(inbuf));
    if (version != EV_CURRENT)
        return false;

    osabi = static_cast<ElfOsabiIndex>(ReadU8(inbuf));
    abi_version = ReadU8(inbuf);
    inbuf.setPosition(EI_NIDENT);
    file_type = static_cast<ElfFileType>(ReadU16(inbuf));
    machine_type = static_cast<ElfMachineType>(ReadU16(inbuf));
    version = static_cast<ElfVersion>(ReadU32(inbuf));
    if (version != EV_CURRENT)
        return false;

    if (cls == ELFCLASS32)
    {
        start = ReadU32(inbuf);
        proghead_pos = ReadU32(inbuf);
        secthead_pos = ReadU32(inbuf);
    }
    else if (cls == ELFCLASS64)
    {
        start = ReadU64(inbuf);
        proghead_pos = ReadU64(inbuf).getUInt();
        secthead_pos = ReadU64(inbuf).getUInt();
    }

    machine_flags = ReadU32(inbuf);
    ReadU16(inbuf);                 // e_ehsize (don't care)
    proghead_size = ReadU16(inbuf);
    proghead_count = ReadU16(inbuf);
    secthead_size = ReadU16(inbuf);
    secthead_count = ReadU16(inbuf);
    shstrtab_index = ReadU16(inbuf);

    return true;
}

void
ElfConfig::WriteProgramHeader(llvm::raw_ostream& os, Bytes& scratch)
{
    scratch.resize(0);
    setEndian(scratch);

    // ELF magic number
    Write8(scratch, ELFMAG0);
    Write8(scratch, ELFMAG1);
    Write8(scratch, ELFMAG2);
    Write8(scratch, ELFMAG3);

    Write8(scratch, cls);               // elf class
    Write8(scratch, encoding);          // data encoding :: MSB?
    Write8(scratch, version);           // elf version
    Write8(scratch, osabi);             // os/abi
    Write8(scratch, abi_version);       // ABI version
    while (scratch.size() < EI_NIDENT)
        Write8(scratch, 0);             // e_ident padding

    Write16(scratch, file_type);        // e_type
    Write16(scratch, machine_type);     // e_machine - or others
    Write32(scratch, version);          // elf version

    unsigned int ehdr_size = 0;
    if (cls == ELFCLASS32)
    {
        Write32(scratch, start);            // e_entry execution startaddr
        Write32(scratch, proghead_pos);     // e_phoff program header off
        Write32(scratch, secthead_pos);     // e_shoff section header off
        ehdr_size = EHDR32_SIZE;
        secthead_size = SHDR32_SIZE;
    }
    else if (cls == ELFCLASS64)
    {
        Write64(scratch, start);            // e_entry execution startaddr
        Write64(scratch, proghead_pos);     // e_phoff program header off
        Write64(scratch, secthead_pos);     // e_shoff section header off
        ehdr_size = EHDR64_SIZE;
        secthead_size = SHDR64_SIZE;
    }

    Write32(scratch, machine_flags);    // e_flags
    Write16(scratch, ehdr_size);        // e_ehsize
    Write16(scratch, proghead_size);    // e_phentsize
    Write16(scratch, proghead_count);   // e_phnum
    Write16(scratch, secthead_size);    // e_shentsize
    Write16(scratch, secthead_count);   // e_shnum
    Write16(scratch, shstrtab_index);   // e_shstrndx

    assert(scratch.size() == getProgramHeaderSize());

    os << scratch;
}

std::string
ElfConfig::getRelocSectionName(const std::string& basesect) const
{
    if (rela)
        return ".rela"+basesect;
    else
        return ".rel"+basesect;
}

bool
ElfConfig::setEndian(EndianState& state) const
{
    if (encoding == ELFDATA2LSB)
        state.setLittleEndian();
    else if (encoding == ELFDATA2MSB)
        state.setBigEndian();
    else
        return false;
    return true;
}

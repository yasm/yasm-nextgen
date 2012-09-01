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


using namespace yasm;
using namespace yasm::objfmt;

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
ElfConfig::WriteSymbolTable(raw_ostream& os,
                            Object& object,
                            DiagnosticsEngine& diags,
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
ElfConfig::ReadSymbolTable(const MemoryBuffer&  in,
                           const ElfSection&    symtab_sect,
                           ElfSymtab&           symtab,
                           Object&              object,
                           const StringTable&   strtab,
                           Section*             sections[],
                           DiagnosticsEngine&   diags) const
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
ElfConfig::ReadProgramHeader(const MemoryBuffer& in)
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
ElfConfig::WriteProgramHeader(raw_ostream& os, Bytes& scratch)
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

#ifdef WITH_XML
pugi::xml_node
ElfConfig::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("ElfConfig");
    switch (cls)
    {
        case ELFCLASS32:    append_child(root, "Cls", "ELFCLASS32"); break;
        case ELFCLASS64:    append_child(root, "Cls", "ELFCLASS64"); break;
        default:    append_child(root, "Cls", static_cast<int>(cls)); break;
    }

    switch (encoding)
    {
        case ELFDATA2LSB:   append_child(root, "Encoding", "2LSB"); break;
        case ELFDATA2MSB:   append_child(root, "Encoding", "2MSB"); break;
        default:
            append_child(root, "Encoding", static_cast<int>(encoding));
            break;
    }

    if (version == EV_CURRENT)
        append_child(root, "Version", "EV_CURRENT");
    else
        append_child(root, "Version", static_cast<int>(version));

    const char* osabi_str = NULL;
    switch (osabi)
    {
        case ELFOSABI_SYSV:         osabi_str = "SYSV"; break;
        case ELFOSABI_HPUX:         osabi_str = "HPUX"; break;
        case ELFOSABI_STANDALONE:   osabi_str = "STANDALONE"; break;
    }
    if (osabi_str)
        append_child(root, "OsAbi", osabi_str);
    else
        append_child(root, "OsAbi", static_cast<int>(osabi));

    append_child(root, "AbiVersion", static_cast<unsigned int>(abi_version));

    const char* ft_str = NULL;
    switch (file_type)
    {
        case ET_NONE:   ft_str = "NONE"; break;
        case ET_REL:    ft_str = "REL"; break;
        case ET_EXEC:   ft_str = "EXEC"; break;
        case ET_DYN:    ft_str = "DYN"; break;
        case ET_CORE:   ft_str = "CORE"; break;
        default:        break;
    }
    if (ft_str)
        append_child(root, "FileType", ft_str);
    else
        append_child(root, "FileType", static_cast<int>(file_type));

    const char* mt_str = NULL;
    switch (machine_type)
    {
        case EM_NONE:           mt_str = "NONE"; break;
        case EM_M32:            mt_str = "M32"; break;
        case EM_SPARC:          mt_str = "SPARC"; break;
        case EM_386:            mt_str = "386"; break;
        case EM_68K:            mt_str = "68K"; break;
        case EM_88K:            mt_str = "88K"; break;
        case EM_860:            mt_str = "860"; break;
        case EM_MIPS:           mt_str = "MIPS"; break;
        case EM_S370:           mt_str = "S370"; break;
        case EM_MIPS_RS4_BE:    mt_str = "MIPS_RS4_BE"; break;
        case EM_PARISC:         mt_str = "PARISC"; break;
        case EM_SPARC32PLUS:    mt_str = "SPARC32PLUS"; break;
        case EM_PPC:            mt_str = "PPC"; break;
        case EM_PPC64:          mt_str = "PPC64"; break;
        case EM_ARM:            mt_str = "ARM"; break;
        case EM_SPARCV9:        mt_str = "SPARCV9"; break;
        case EM_IA_64:          mt_str = "IA_64"; break;
        case EM_X86_64:         mt_str = "X86_64"; break;
        case EM_ALPHA:          mt_str = "ALPHA"; break;
    }
    if (mt_str)
        append_child(root, "MachineType", mt_str);
    else
        append_child(root, "MachineType", static_cast<int>(machine_type));

    append_child(root, "Start", start);
    append_child(root, "Rela", rela);
    pugi::xml_node ph_node = root.append_child("ProgHead");
    ph_node.append_attribute("pos") = proghead_pos;
    ph_node.append_attribute("count") = proghead_count;
    ph_node.append_attribute("size") = proghead_size;
    pugi::xml_node sh_node = root.append_child("SectHead");
    sh_node.append_attribute("pos") = secthead_pos;
    sh_node.append_attribute("count") = secthead_count;
    sh_node.append_attribute("size") = secthead_size;
    append_child(root, "MachineFlags", machine_flags);
    append_child(root, "ShstrtabIndex", shstrtab_index);
    return root;
}
#endif // WITH_XML

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

#include "util.h"

#include "llvm/Support/raw_ostream.h"
#include "YAML/emitter.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/InputBuffer.h"
#include "yasmx/Object.h"

#include "ElfSection.h"
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
ElfConfig::AssignSymbolIndices(Object& object, ElfSymbolIndex* nlocal) const
{
    // start at 1 due to undefined symbol (0)
    ElfSymbolIndex num = 1;
    *nlocal = 1;
    for (Object::symbol_iterator i=object.symbols_begin(),
         end=object.symbols_end(); i != end; ++i)
    {
        ElfSymbol* elfsym = i->getAssocData<ElfSymbol>();
        if (!elfsym)
            continue;

        elfsym->setSymbolIndex(num);

        if (elfsym->isLocal())
            *nlocal = num;

        ++num;
    }
    return num;
}

unsigned long
ElfConfig::WriteSymbolTable(llvm::raw_ostream& os,
                            Object& object,
                            Errwarns& errwarns,
                            Bytes& scratch) const
{
    unsigned long size = 0;

    // write undef symbol
    ElfSymbol undef;
    scratch.resize(0);
    undef.Write(scratch, *this);
    os << scratch;
    size += scratch.size();

    // write other symbols
    for (Object::symbol_iterator sym=object.symbols_begin(),
         end=object.symbols_end(); sym != end; ++sym)
    {
        ElfSymbol* elfsym = sym->getAssocData<ElfSymbol>();
        if (!elfsym)
            continue;

        elfsym->Finalize(*sym, errwarns);

        scratch.resize(0);
        elfsym->Write(scratch, *this);
        os << scratch;
        size += scratch.size();
    }
    return size;
}

void
ElfConfig::ReadSymbolTable(const llvm::MemoryBuffer&    in,
                           const ElfSection&            symtab_sect,
                           ElfSymtab&                   symtab,
                           Object&                      object,
                           const StringTable&           strtab,
                           Section*                     sections[]) const
{
    ElfSize symsize = symtab_sect.getEntSize();
    if (symsize == 0)
        throw Error(N_("symbol table entity size is zero"));

    unsigned long size = symtab_sect.getSize().getUInt();

    // Symbol table always starts with null entry
    symtab.push_back(SymbolRef(0));

    ElfSymbolIndex index = 1;
    for (unsigned long pos=symsize; pos<size; pos += symsize, ++index)
    {
        std::auto_ptr<ElfSymbol> elfsym(
            new ElfSymbol(*this, in, symtab_sect, index, sections));

        SymbolRef sym = elfsym->CreateSymbol(object, strtab);
        symtab.push_back(sym);

        if (sym)
            sym->AddAssocData(elfsym);  // Associate symbol data with symbol
    }
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

void
ElfConfig::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "cls" << YAML::Value;
    switch (cls)
    {
        case ELFCLASS32:    out << "ELFCLASS32"; break;
        case ELFCLASS64:    out << "ELFCLASS64"; break;
        default:            out << static_cast<int>(cls); break;
    }

    out << YAML::Key << "encoding" << YAML::Value;
    switch (encoding)
    {
        case ELFDATA2LSB:   out << "2LSB"; break;
        case ELFDATA2MSB:   out << "2MSB"; break;
        default:            out << static_cast<int>(encoding); break;
    }

    out << YAML::Key << "version" << YAML::Value;
    if (version == EV_CURRENT)
        out << "EV_CURRENT";
    else
        out << static_cast<int>(version);

    out << YAML::Key << "osabi" << YAML::Value;
    switch (osabi)
    {
        case ELFOSABI_SYSV:         out << "SYSV"; break;
        case ELFOSABI_HPUX:         out << "HPUX"; break;
        case ELFOSABI_STANDALONE:   out << "STANDALONE"; break;
        default:                    out << static_cast<int>(osabi); break;
    }

    out << YAML::Key << "abi version" << YAML::Value << abi_version;

    out << YAML::Key << "file type" << YAML::Value;
    switch (file_type)
    {
        case ET_NONE:   out << "NONE"; break;
        case ET_REL:    out << "REL"; break;
        case ET_EXEC:   out << "EXEC"; break;
        case ET_DYN:    out << "DYN"; break;
        case ET_CORE:   out << "CORE"; break;
        default:        out << YAML::Hex << static_cast<int>(file_type); break;
    }

    out << YAML::Key << "machine type" << YAML::Value;
    switch (machine_type)
    {
        case EM_NONE:           out << "NONE"; break;
        case EM_M32:            out << "M32"; break;
        case EM_SPARC:          out << "SPARC"; break;
        case EM_386:            out << "386"; break;
        case EM_68K:            out << "68K"; break;
        case EM_88K:            out << "88K"; break;
        case EM_860:            out << "860"; break;
        case EM_MIPS:           out << "MIPS"; break;
        case EM_S370:           out << "S370"; break;
        case EM_MIPS_RS4_BE:    out << "MIPS_RS4_BE"; break;
        case EM_PARISC:         out << "PARISC"; break;
        case EM_SPARC32PLUS:    out << "SPARC32PLUS"; break;
        case EM_PPC:            out << "PPC"; break;
        case EM_PPC64:          out << "PPC64"; break;
        case EM_ARM:            out << "ARM"; break;
        case EM_SPARCV9:        out << "SPARCV9"; break;
        case EM_IA_64:          out << "IA_64"; break;
        case EM_X86_64:         out << "X86_64"; break;
        case EM_ALPHA:          out << "ALPHA"; break;
        default:    out << YAML::Hex << static_cast<int>(machine_type); break;
    }

    out << YAML::Key << "start" << YAML::Value << start;
    out << YAML::Key << "rela" << YAML::Value << rela;
    out << YAML::Key << "proghead pos" << YAML::Value << proghead_pos;
    out << YAML::Key << "proghead count" << YAML::Value << proghead_count;
    out << YAML::Key << "proghead size" << YAML::Value << proghead_size;
    out << YAML::Key << "secthead pos" << YAML::Value << secthead_pos;
    out << YAML::Key << "secthead count" << YAML::Value << secthead_count;
    out << YAML::Key << "secthead size" << YAML::Value << secthead_size;
    out << YAML::Key << "machine flags";
    out << YAML::Value << YAML::Hex << machine_flags;
    out << YAML::Key << "shstrtab index" << YAML::Value << shstrtab_index;
    out << YAML::EndMap;
}

void
ElfConfig::Dump() const
{
    YAML::Emitter out;
    Write(out);
    llvm::errs() << out.c_str() << '\n';
}

}}} // namespace yasm::objfmt::elf

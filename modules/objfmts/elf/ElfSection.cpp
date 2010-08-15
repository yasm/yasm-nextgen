//
// ELF object format section
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
#include "ElfSection.h"

#include "util.h"

#include "llvm/Support/raw_ostream.h"
#include "YAML/emitter.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/Diagnostic.h"
#include "yasmx/InputBuffer.h"
#include "yasmx/StringTable.h"
#include "yasmx/Symbol.h"

#include "ElfConfig.h"
#include "ElfMachine.h"
#include "ElfReloc.h"


using namespace yasm;
using namespace yasm::objfmt;

const char* ElfSection::key = "objfmt::elf::ElfSection";

ElfSection::ElfSection(const ElfConfig&             config,
                       const llvm::MemoryBuffer&    in,
                       ElfSectionIndex              index,
                       Diagnostic&                  diags)
    : m_config(config)
    , m_index(index)
    , m_rel_name_index(0)
    , m_rel_index(0)
    , m_rel_offset(0)
{
    InputBuffer inbuf(in);

    // Go to section header
    inbuf.setPosition(config.secthead_pos + index * config.secthead_size);
    if (inbuf.getReadableSize() < 8)
    {
        diags.Report(clang::SourceLocation(),
                     diag::err_section_header_too_small);
        return;
    }

    m_config.setEndian(inbuf);

    m_name_index = ReadU32(inbuf);
    m_type = static_cast<ElfSectionType>(ReadU32(inbuf));

    if (m_config.cls == ELFCLASS32)
    {
        if (inbuf.getReadableSize() < SHDR32_SIZE-8)
        {
            diags.Report(clang::SourceLocation(),
                         diag::err_section_header_too_small);
            return;
        }

        m_flags = static_cast<ElfSectionFlags>(ReadU32(inbuf));
        m_addr = ReadU32(inbuf);

        m_offset = static_cast<ElfAddress>(ReadU32(inbuf));
        m_size = ReadU32(inbuf);
        m_link = static_cast<ElfSectionIndex>(ReadU32(inbuf));
        m_info = static_cast<ElfSectionInfo>(ReadU32(inbuf));

        m_align = ReadU32(inbuf);
        m_entsize = static_cast<ElfSize>(ReadU32(inbuf));
    }
    else if (m_config.cls == ELFCLASS64)
    {
        if (inbuf.getReadableSize() < SHDR64_SIZE-8)
        {
            diags.Report(clang::SourceLocation(),
                         diag::err_section_header_too_small);
            return;
        }

        m_flags = static_cast<ElfSectionFlags>(ReadU64(inbuf).getUInt());
        m_addr = ReadU64(inbuf);

        m_offset = static_cast<ElfAddress>(ReadU64(inbuf).getUInt());
        m_size = ReadU64(inbuf);
        m_link = static_cast<ElfSectionIndex>(ReadU32(inbuf));
        m_info = static_cast<ElfSectionInfo>(ReadU32(inbuf));

        m_align = ReadU64(inbuf).getUInt();
        m_entsize = static_cast<ElfSize>(ReadU64(inbuf).getUInt());
    }
}

ElfSection::ElfSection(const ElfConfig&     config,
                       ElfSectionType       type,
                       ElfSectionFlags      flags,
                       bool                 symtab)
    : m_config(config)
    , m_type(type)
    , m_flags(flags)
    , m_addr(0)
    , m_offset(0)
    , m_size(0)
    , m_link(0)
    , m_info(0)
    , m_align(0)
    , m_entsize(0)
    , m_name_index(0)
    , m_index(0)
    , m_rel_name_index(0)
    , m_rel_index(0)
    , m_rel_offset(0)
{
    if (symtab)
    {
        if (m_config.cls == ELFCLASS32)
        {
            m_entsize = SYMTAB32_SIZE;
            m_align = SYMTAB32_ALIGN;
        }
        else if (m_config.cls == ELFCLASS64)
        {
            m_entsize = SYMTAB64_SIZE;
            m_align = SYMTAB64_ALIGN;
        }
    }
}

ElfSection::~ElfSection()
{
}

void
ElfSection::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << key;
    out << YAML::Key << "config" << YAML::Value << m_config;

    out << YAML::Key << "secttype" << YAML::Value;
    switch (m_type)
    {
        case SHT_NULL:              out << "NULL"; break;
        case SHT_PROGBITS:          out << "PROGBITS"; break;
        case SHT_SYMTAB:            out << "SYMTAB"; break;
        case SHT_STRTAB:            out << "STRTAB"; break;
        case SHT_RELA:              out << "RELA"; break;
        case SHT_HASH:              out << "HASH"; break;
        case SHT_DYNAMIC:           out << "DYNAMIC"; break;
        case SHT_NOTE:              out << "NOTE"; break;
        case SHT_NOBITS:            out << "NOBITS"; break;
        case SHT_REL:               out << "REL"; break;
        case SHT_SHLIB:             out << "SHLIB"; break;
        case SHT_DYNSYM:            out << "DYNSYM"; break;
        case SHT_INIT_ARRAY:        out << "INIT_ARRAY"; break;
        case SHT_FINI_ARRAY:        out << "FINI_ARRAY"; break;
        case SHT_PREINIT_ARRAY:     out << "PREINIT_ARRAY"; break;
        case SHT_GROUP:             out << "GROUP"; break;
        case SHT_SYMTAB_SHNDX:      out << "SYMTAB_SHNDX"; break;
        default: out << YAML::Hex << static_cast<unsigned int>(m_type); break;
    }

    out << YAML::Key << "flags" << YAML::Value << m_flags;
    out << YAML::Key << "addr" << YAML::Value << m_addr;
    out << YAML::Key << "offset" << YAML::Value << m_offset;
    out << YAML::Key << "size" << YAML::Value << m_size;
    out << YAML::Key << "link" << YAML::Value << m_link;
    out << YAML::Key << "info" << YAML::Value << m_info;
    out << YAML::Key << "align" << YAML::Value << m_align;
    out << YAML::Key << "entsize" << YAML::Value << m_entsize;
    out << YAML::Key << "name index" << YAML::Value << m_name_index;
    out << YAML::Key << "sect index" << YAML::Value << m_index;
    out << YAML::Key << "rel name index" << YAML::Value << m_rel_name_index;
    out << YAML::Key << "rel sect index" << YAML::Value << m_rel_index;
    out << YAML::Key << "rel offset" << YAML::Value << m_rel_offset;
    out << YAML::EndMap;
}

unsigned long
ElfSection::Write(llvm::raw_ostream& os, Bytes& scratch) const
{
    scratch.resize(0);
    m_config.setEndian(scratch);

    Write32(scratch, m_name_index);
    Write32(scratch, m_type);

    if (m_config.cls == ELFCLASS32)
    {
        Write32(scratch, m_flags);
        Write32(scratch, m_addr);

        Write32(scratch, m_offset);
        Write32(scratch, m_size);
        Write32(scratch, m_link);
        Write32(scratch, m_info);

        Write32(scratch, m_align);
        Write32(scratch, m_entsize);

        assert(scratch.size() == SHDR32_SIZE);
    }
    else if (m_config.cls == ELFCLASS64)
    {
        Write64(scratch, m_flags);
        Write64(scratch, m_addr);

        Write64(scratch, m_offset);
        Write64(scratch, m_size);
        Write32(scratch, m_link);
        Write32(scratch, m_info);

        Write64(scratch, m_align);
        Write64(scratch, m_entsize);

        assert(scratch.size() == SHDR64_SIZE);
    }

    os << scratch;
    return scratch.size();
}

std::auto_ptr<Section>
ElfSection::CreateSection(const StringTable& shstrtab) const
{
    bool bss = (m_type == SHT_NOBITS || m_offset == 0);

    std::auto_ptr<Section> section(
        new Section(shstrtab.getString(m_name_index), m_flags & SHF_EXECINSTR,
                    bss, clang::SourceLocation()));

    section->setFilePos(m_offset);
    section->setVMA(m_addr);
    section->setLMA(m_addr);
    section->setAlign(m_align);

    if (bss)
    {
        Bytecode& gap =
            section->AppendGap(m_size.getUInt(), clang::SourceLocation());
        Diagnostic nodiags(0);
        gap.CalcLen(0, nodiags);    // force length calculation of gap
    }

    return section;
}

bool
ElfSection::LoadSectionData(Section& sect,
                            const llvm::MemoryBuffer& in,
                            Diagnostic& diags) const
{
    if (sect.isBSS())
        return true;

    // Read section data
    InputBuffer inbuf(in, m_offset);

    unsigned long size = m_size.getUInt();
    if (inbuf.getReadableSize() < size)
    {
        diags.Report(clang::SourceLocation(), diag::err_section_data_unreadable)
            << sect.getName();
        return false;
    }

    sect.bytecodes_front().getFixed().Write(inbuf.Read(size), size);
    return true;
}

unsigned long
ElfSection::WriteRel(llvm::raw_ostream& os,
                     ElfSectionIndex symtab_idx,
                     Section& sect,
                     Bytes& scratch)
{
    if (sect.getRelocs().size() == 0)
        return 0;       // no relocations, no .rel.* section header

    scratch.resize(0);
    m_config.setEndian(scratch);

    Write32(scratch, m_rel_name_index);
    Write32(scratch, m_config.rela ? SHT_RELA : SHT_REL);

    unsigned int size = 0;
    if (m_config.cls == ELFCLASS32)
    {
        size = m_config.rela ? RELOC32A_SIZE : RELOC32_SIZE;
        Write32(scratch, 0);                    // flags=0
        Write32(scratch, 0);                    // vmem address=0
        Write32(scratch, m_rel_offset);
        Write32(scratch, size * sect.getRelocs().size());// size
        Write32(scratch, symtab_idx);           // link: symtab index
        Write32(scratch, m_index);              // info: relocated's index
        Write32(scratch, RELOC32_ALIGN);        // align
        Write32(scratch, size);                 // entity size

        assert(scratch.size() == SHDR32_SIZE);
    }
    else if (m_config.cls == ELFCLASS64)
    {
        size = m_config.rela ? RELOC64A_SIZE : RELOC64_SIZE;
        Write64(scratch, 0);
        Write64(scratch, 0);
        Write64(scratch, m_rel_offset);
        Write64(scratch, size * sect.getRelocs().size());// size
        Write32(scratch, symtab_idx);           // link: symtab index
        Write32(scratch, m_index);              // info: relocated's index
        Write64(scratch, RELOC64_ALIGN);        // align
        Write64(scratch, size);                 // entity size

        assert(scratch.size() == SHDR64_SIZE);
    }

    os << scratch;
    return scratch.size();
}

unsigned long
ElfSection::WriteRelocs(llvm::raw_ostream& os,
                        Section& sect,
                        Bytes& scratch,
                        const ElfMachine& machine,
                        Diagnostic& diags)
{
    if (sect.getRelocs().size() == 0)
        return 0;

    // first align section to multiple of 4
    uint64_t pos = os.tell();
    if (os.has_error())
    {
        diags.Report(clang::SourceLocation(), diag::err_file_output_position);
        pos = 0;
    }
    uint64_t apos = (pos + 3) & ~3;
    for (; pos < apos; ++pos)
        os << '\0';
    m_rel_offset = static_cast<unsigned long>(pos);

    unsigned long size = 0;
    for (Section::reloc_iterator i=sect.relocs_begin(), end=sect.relocs_end();
         i != end; ++i)
    {
        ElfReloc& reloc = static_cast<ElfReloc&>(*i);
        scratch.resize(0);
        reloc.Write(scratch, m_config);
        os << scratch;
        size += scratch.size();
    }
    return size;
}

void
ElfSection::ReadRelocs(const llvm::MemoryBuffer&    in,
                       const ElfSection&            reloc_sect,
                       Section&                     sect,
                       const ElfMachine&            machine,
                       const ElfSymtab&             symtab,
                       bool                         rela) const
{
    unsigned long start = reloc_sect.getFileOffset();
    unsigned long end = start + reloc_sect.getSize().getUInt();
    for (unsigned long pos = start; pos < end; )
    {
        sect.AddReloc(std::auto_ptr<Reloc>(
            machine.ReadReloc(m_config, symtab, in, &pos, rela).release()));
    }
}

unsigned long
ElfSection::setFileOffset(unsigned long pos)
{
    const unsigned long align = m_align;

    if (align == 0 || align == 1)
    {
        m_offset = pos;
        return pos;
    }
    else
        assert((align & (align - 1)) == 0 && "alignment is not a power of 2");

    m_offset = (pos + align - 1) & ~(align - 1);
    return m_offset;
}

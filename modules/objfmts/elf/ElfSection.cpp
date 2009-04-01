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

#include <util.h>

#include "yasmx/Support/Compose.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/marg_ostream.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/StringTable.h"
#include "yasmx/Symbol.h"

#include "ElfConfig.h"
#include "ElfMachine.h"
#include "ElfReloc.h"


namespace yasm
{
namespace objfmt
{
namespace elf
{

const char* ElfSection::key = "objfmt::elf::ElfSection";

ElfSection::ElfSection(const ElfConfig&     config,
                       std::istream&        is,
                       ElfSectionIndex      index)
    : m_config(config)
    , m_sym(0)
    , m_index(index)
    , m_rel_name_index(0)
    , m_rel_index(0)
    , m_rel_offset(0)
{
    Bytes bytes;

    bytes.write(is, m_config.secthead_size);
    if (!is)
        throw Error(N_("could not read section header"));

    m_config.setup_endian(bytes);

    m_name_index = read_u32(bytes);
    m_type = static_cast<ElfSectionType>(read_u32(bytes));

    if (m_config.cls == ELFCLASS32)
    {
        if (bytes.size() < SHDR32_SIZE)
            throw Error(N_("section header too small"));

        m_flags = static_cast<ElfSectionFlags>(read_u32(bytes));
        m_addr = read_u32(bytes);

        m_offset = static_cast<ElfAddress>(read_u32(bytes));
        m_size = read_u32(bytes);
        m_link = static_cast<ElfSectionIndex>(read_u32(bytes));
        m_info = static_cast<ElfSectionInfo>(read_u32(bytes));

        m_align = read_u32(bytes);
        m_entsize = static_cast<ElfSize>(read_u32(bytes));
    }
    else if (m_config.cls == ELFCLASS64)
    {
        if (bytes.size() < SHDR64_SIZE)
            throw Error(N_("section header too small"));

        m_flags = static_cast<ElfSectionFlags>(read_u64(bytes).get_uint());
        m_addr = read_u64(bytes);

        m_offset = static_cast<ElfAddress>(read_u64(bytes).get_uint());
        m_size = read_u64(bytes);
        m_link = static_cast<ElfSectionIndex>(read_u32(bytes));
        m_info = static_cast<ElfSectionInfo>(read_u32(bytes));

        m_align = read_u64(bytes).get_uint();
        m_entsize = static_cast<ElfSize>(read_u64(bytes).get_uint());
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
    , m_sym(0)
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
ElfSection::put(marg_ostream& os) const
{
    os << "\nsym=\n";
    ++os;
    os << *m_sym;
    --os;
    os << std::hex << std::showbase;
    os << "index=" << m_index << '\n';
    os << "flags=";
    if (m_flags & SHF_WRITE)
        os << "WRITE ";
    if (m_flags & SHF_ALLOC)
        os << "ALLOC ";
    if (m_flags & SHF_EXECINSTR)
        os << "EXEC ";
    /*if (m_flags & SHF_MASKPROC)
        os << "PROC-SPECIFIC "; */
    os << "\noffset=" << m_offset << '\n';
    os << "size=" << m_size << '\n';
    os << "link=" << m_link << '\n';
    os << std::dec << std::noshowbase;
    os << "align=" << m_align << '\n';
}

unsigned long
ElfSection::write(std::ostream& os, Bytes& scratch) const
{
    scratch.resize(0);
    m_config.setup_endian(scratch);

    write_32(scratch, m_name_index);
    write_32(scratch, m_type);

    if (m_config.cls == ELFCLASS32)
    {
        write_32(scratch, m_flags);
        write_32(scratch, m_addr);

        write_32(scratch, m_offset);
        write_32(scratch, m_size);
        write_32(scratch, m_link);
        write_32(scratch, m_info);

        write_32(scratch, m_align);
        write_32(scratch, m_entsize);

        assert(scratch.size() == SHDR32_SIZE);
    }
    else if (m_config.cls == ELFCLASS64)
    {
        write_64(scratch, m_flags);
        write_64(scratch, m_addr);

        write_64(scratch, m_offset);
        write_64(scratch, m_size);
        write_32(scratch, m_link);
        write_32(scratch, m_info);

        write_64(scratch, m_align);
        write_64(scratch, m_entsize);

        assert(scratch.size() == SHDR64_SIZE);
    }

    os << scratch;
    if (!os)
        throw IOError(N_("Failed to write an elf section header"));

    return scratch.size();
}

std::auto_ptr<Section>
ElfSection::create_section(const StringTable& shstrtab) const
{
    bool bss = (m_type == SHT_NOBITS || m_offset == 0);

    std::auto_ptr<Section> section(
        new Section(shstrtab.get_str(m_name_index), m_flags & SHF_EXECINSTR,
                    bss, 0));

    section->set_filepos(m_offset);
    section->set_vma(m_addr);
    section->set_lma(m_addr);
    section->set_align(m_align);

    if (bss)
    {
        Bytecode& gap = section->append_gap(m_size.get_uint(), 0);
        gap.calc_len(0);    // force length calculation of gap
    }

    return section;
}

void
ElfSection::load_section_data(Section& sect, std::istream& is) const
{
    if (sect.is_bss())
        return;

    std::streampos oldpos = is.tellg();

    // Read section data
    is.seekg(m_offset);
    if (!is)
        throw Error(String::compose(
            N_("could not seek to section `%1'"), get_name()));

    sect.bcs_first().get_fixed().write(is, m_size.get_uint());
    if (!is)
        throw Error(String::compose(
            N_("could not read section `%1' data"), get_name()));

    is.seekg(oldpos);
}

unsigned long
ElfSection::write_rel(std::ostream& os,
                      ElfSectionIndex symtab_idx,
                      Section& sect,
                      Bytes& scratch)
{
    if (sect.get_relocs().size() == 0)
        return 0;       // no relocations, no .rel.* section header

    scratch.resize(0);
    m_config.setup_endian(scratch);

    write_32(scratch, m_rel_name_index);
    write_32(scratch, m_config.rela ? SHT_RELA : SHT_REL);

    unsigned int size = 0;
    if (m_config.cls == ELFCLASS32)
    {
        size = m_config.rela ? RELOC32A_SIZE : RELOC32_SIZE;
        write_32(scratch, 0);                   // flags=0
        write_32(scratch, 0);                   // vmem address=0
        write_32(scratch, m_rel_offset);
        write_32(scratch, size * sect.get_relocs().size());// size
        write_32(scratch, symtab_idx);          // link: symtab index
        write_32(scratch, m_index);             // info: relocated's index
        write_32(scratch, RELOC32_ALIGN);       // align
        write_32(scratch, size);                // entity size

        assert(scratch.size() == SHDR32_SIZE);
    }
    else if (m_config.cls == ELFCLASS64)
    {
        size = m_config.rela ? RELOC64A_SIZE : RELOC64_SIZE;
        write_64(scratch, 0);
        write_64(scratch, 0);
        write_64(scratch, m_rel_offset);
        write_64(scratch, size * sect.get_relocs().size());// size
        write_32(scratch, symtab_idx);          // link: symtab index
        write_32(scratch, m_index);             // info: relocated's index
        write_64(scratch, RELOC64_ALIGN);       // align
        write_64(scratch, size);                // entity size

        assert(scratch.size() == SHDR64_SIZE);
    }

    os << scratch;
    if (!os)
        throw IOError(N_("Failed to write an elf section header"));

    return scratch.size();
}

unsigned long
ElfSection::write_relocs(std::ostream& os,
                         Section& sect,
                         Errwarns& errwarns,
                         Bytes& scratch,
                         const ElfMachine& machine)
{
    if (sect.get_relocs().size() == 0)
        return 0;

    // first align section to multiple of 4
    long pos = os.tellp();
    if (pos < 0)
        throw IOError(N_("couldn't read position on output stream"));
    pos = (pos + 3) & ~3;
    os.seekp(pos);
    if (!os)
        throw IOError(N_("couldn't seek on output stream"));
    m_rel_offset = static_cast<unsigned long>(pos);

    unsigned long size = 0;
    for (Section::reloc_iterator i=sect.relocs_begin(), end=sect.relocs_end();
         i != end; ++i)
    {
        ElfReloc& reloc = static_cast<ElfReloc&>(*i);
        scratch.resize(0);
        reloc.write(scratch, m_config);
        os << scratch;
        size += scratch.size();
    }
    return size;
}

bool
ElfSection::read_relocs(std::istream&       is,
                        Section&            sect,
                        unsigned long       size,
                        const ElfMachine&   machine,
                        const ElfSymtab&    symtab,
                        bool                rela) const
{
    for (unsigned long pos=is.tellg();
         static_cast<unsigned long>(is.tellg()) < (pos+size);)
    {
        sect.add_reloc(std::auto_ptr<Reloc>(
            machine.read_reloc(m_config, symtab, is, rela).release()));
        if (!is)
            throw Error(N_("could not read relocation entry"));
    }

    return true;
}

unsigned long
ElfSection::set_file_offset(unsigned long pos)
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

}}} // namespace yasm::objfmt::elf

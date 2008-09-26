//
// ELF object format helpers
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
#include "elf.h"

#include <util.h>

#include <iomanip>
#include <ostream>

#include <libyasmx/bytecode.h>
#include <libyasmx/bytes.h>
#include <libyasmx/bytes_util.h>
#include <libyasmx/compose.h>
#include <libyasmx/errwarn.h>
#include <libyasmx/errwarns.h>
#include <libyasmx/expr.h>
#include <libyasmx/location_util.h>
#include <libyasmx/marg_ostream.h>
#include <libyasmx/section.h>
#include <libyasmx/object.h>

#include "elf-machine.h"


namespace yasm
{
namespace objfmt
{
namespace elf
{

const char* ElfSymbol::key = "objfmt::elf::ElfSymbol";
const char* ElfSection::key = "objfmt::elf::ElfSection";

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

//
// reloc functions
//
ElfReloc::ElfReloc(SymbolRef sym,
                   SymbolRef wrt,
                   const IntNum& addr,
                   bool rel,
                   size_t valsize,
                   const ElfMachine& machine)
    : Reloc(addr, sym)
    , m_rtype_rel(rel)
    , m_valsize(valsize)
    , m_addend(0)
    , m_wrt(wrt)
{
    if (wrt)
    {
        const ElfSpecialSymbol* ssym = get_elf_ssym(*wrt);
        if (!ssym || valsize != ssym->size)
            throw TypeError(N_("elf: invalid WRT"));
    }
    else if (!machine.accepts_reloc(valsize))
        throw TypeError(N_("elf: invalid relocation size"));

    if (sym == 0)
        throw InternalError("sym is null");
}

ElfReloc::~ElfReloc()
{
}

std::auto_ptr<Expr>
ElfReloc::get_value() const
{
    return Expr::Ptr(new Expr(m_sym, Op::ADD, m_addend, 0));
}

std::string
ElfReloc::get_type_name() const
{
    return "";  // TODO
}

void
ElfReloc::handle_addend(IntNum* intn, const ElfConfig& config)
{
    // rela sections put the addend into the relocation, and write 0 in
    // data area.
    if (config.rela)
    {
        m_addend = *intn;
        *intn = 0;
    }
}

void
ElfReloc::write(Bytes& bytes, const ElfConfig& config, unsigned int r_type)
{
    unsigned long r_sym = STN_UNDEF;

    if (ElfSymbol* esym = get_elf(*get_sym()))
        r_sym = esym->get_symindex();

    bytes.resize(0);
    config.setup_endian(bytes);

    if (config.cls == ELFCLASS32)
    {
        write_32(bytes, m_addr);
        write_32(bytes,
                 ELF32_R_INFO(r_sym, static_cast<unsigned char>(r_type)));

        if (config.rela)
            write_32(bytes, m_addend);
    }
    else if (config.cls == ELFCLASS64)
    {
        write_64(bytes, m_addr);
        write_64(bytes, ELF64_R_INFO(IntNum(r_sym),
                                     static_cast<unsigned char>(r_type)));
        if (config.rela)
            write_64(bytes, m_addend);
    }
}

void
ElfStrtab::set_str(Entry* entry, const std::string& str)
{
    stdx::ptr_vector<Entry>::iterator i=m_strs.begin(), end=m_strs.end();

    // Find entry in question
    for (; i != end; ++i)
    {
        if (&(*i) == entry)
            break;
    }

    int lendiff = str.length() - entry->m_str.length();
    entry->m_str = str;

    if (lendiff == 0)
        return;

    // Update indexes on all following entries
    for (; i != end; ++i)
        i->m_index += lendiff;
}

ElfStrtab::ElfStrtab()
    : m_strs_owner(m_strs)
{
    m_strs.push_back(new Entry(0, ""));
}

ElfStrtab::Entry*
ElfStrtab::append_str(const std::string& str)
{
    Entry& back = m_strs.back();
    unsigned long newindex = back.m_index + back.m_str.length() + 1;
    m_strs.push_back(new Entry(newindex, str));
    return &m_strs.back();
}

ElfStrtab::~ElfStrtab()
{
}

unsigned long
ElfStrtab::write(std::ostream& os)
{
    unsigned long size = 0;
    // consider optimizing tables here
    for (stdx::ptr_vector<Entry>::iterator i=m_strs.begin(), end=m_strs.end();
         i != end; ++i)
    {
        os << i->m_str;
        os << '\0';
        size += i->m_str.length() + 1;
    }
    assert(size == (m_strs.back().m_index + m_strs.back().m_str.length() + 1));
    return size;
}



ElfSymbol::ElfSymbol(const ElfConfig&   config,
                     Bytes&             bytes,
                     ElfSymbolIndex     index,
                     ElfStrtab&         strtab,
                     const char*        strtab_str,
                     unsigned long      strtab_size,
                     Section*           sections[])
    : m_sect(0)
    , m_value(0)
    , m_xsize(0)
    , m_size(0)
    , m_symindex(index)
{
    bytes.set_readpos(0);
    config.setup_endian(bytes);

    unsigned long name_idx = read_u32(bytes);
    if (name_idx < strtab_size)
        m_name = strtab.append_str(&strtab_str[name_idx]);
    else
        m_name = 0;

    if (config.cls == ELFCLASS32)
    {
        m_value = read_u32(bytes);
        m_size = read_u32(bytes);
    }

    unsigned char info = read_u8(bytes);
    m_bind = ELF_ST_BIND(info);
    m_type = ELF_ST_TYPE(info);
    m_vis = ELF_ST_VISIBILITY(read_u8(bytes));

    m_index = static_cast<ElfSectionIndex>(read_u16(bytes));
    if (m_index != SHN_UNDEF && m_index < config.secthead_count)
        m_sect = sections[m_index];

    if (config.cls == ELFCLASS64)
    {
        m_value = read_u64(bytes);
        m_size = read_u64(bytes);
    }
}

ElfSymbol::ElfSymbol(ElfStrtab::Entry* name)
    : m_sect(0)
    , m_name(name)
    , m_value(0)
    , m_xsize(0)
    , m_size(0)
    , m_index(SHN_UNDEF)
    , m_bind(STB_LOCAL)
    , m_type(STT_NOTYPE)
    , m_vis(STV_DEFAULT)
    , m_symindex(STN_UNDEF)
{
}

ElfSymbol::~ElfSymbol()
{
}

SymbolRef
ElfSymbol::create_symbol(Object& object) const
{
    SymbolRef sym(0);
    std::string name = m_name ? m_name->get_str() : "";

    if (m_bind == STB_GLOBAL || m_bind == STB_WEAK)
    {
        sym = object.get_sym(name);
        if (m_index == SHN_UNDEF)
            sym->declare(Symbol::EXTERN, 0);
        else
            sym->declare(Symbol::GLOBAL, 0);
    }
    else
    {
        // don't index by name, just append
        sym = object.append_symbol(std::auto_ptr<Symbol>(new Symbol(name)));
    }

    if (m_index == SHN_ABS)
    {
        sym->define_equ(Expr::Ptr(new Expr(m_size, 0)), 0);
    }
    else if (m_index == SHN_COMMON)
    {
        sym->declare(Symbol::COMMON, 0);
    }
    else if (m_sect != 0)
    {
        Location loc = {&m_sect->bcs_first(), m_value.get_uint()};
        sym->define_label(loc, 0);
    }

    return sym;
}

void
ElfSymbol::put(marg_ostream& os) const
{
    os << "bind=";
    switch (m_bind)
    {
        case STB_LOCAL:         os << "local\n";    break;
        case STB_GLOBAL:        os << "global\n";   break;
        case STB_WEAK:          os << "weak\n";     break;
        default:                os << "undef\n";    break;
    }
    os << "type=";
    switch (m_type)
    {
        case STT_NOTYPE:        os << "notype\n";   break;
        case STT_OBJECT:        os << "object\n";   break;
        case STT_FUNC:          os << "func\n";     break;
        case STT_SECTION:       os << "section\n";  break;
        case STT_FILE:          os << "file\n";     break;
        default:                os << "undef\n";    break;
    }
    os << "size=";
    if (m_xsize)
        os << (*m_xsize);
    else
        os << m_size;
    os << '\n';
}
#if 0
void
insert_local_sym(Object& object,
                 std::auto_ptr<Symbol> sym,
                 std::auto_ptr<ElfSymbol> entry)
{
    elf_symtab_entry *after = STAILQ_FIRST(symtab);
    elf_symtab_entry *before = NULL;

    while (after && (after->bind == STB_LOCAL)) {
        before = after;
        if (before->type == STT_FILE) break;
        after = STAILQ_NEXT(after, qlink);
    }
    STAILQ_INSERT_AFTER(symtab, before, entry, qlink);
}
#endif
ElfSymbolIndex
assign_sym_indices(Object& object)
{
    ElfSymbolIndex symindex=0;
    ElfSymbolIndex last_local=0;

    for (Object::symbol_iterator sym=object.symbols_begin(),
         end=object.symbols_end(); sym != end; ++sym)
    {
        ElfSymbol* entry = get_elf(*sym);
        if (!entry)
            continue;       // XXX: or create?
        entry->set_symindex(symindex);
        if (entry->is_local())
            last_local = symindex;
        ++symindex;
    }
    return last_local + 1;
}

void
ElfSymbol::finalize(Symbol& sym, Errwarns& errwarns)
{
    // If symbol is in a TLS section, force its type to TLS.
    Location loc;
    Section* sect;
    ElfSection* elfsect;
    if (sym.get_label(&loc) &&
        (sect = loc.bc->get_container()->as_section()) &&
        (elfsect = get_elf(*sect)) &&
        (elfsect->get_flags() & SHF_TLS))
    {
        m_type = STT_TLS;
    }

    // get size (if specified); expr overrides stored integer
    if (m_xsize)
    {
        m_xsize->simplify(&xform_calc_dist);
        IntNum* xsize = m_xsize->get_intnum();
        if (xsize)
            m_size = *xsize;
        else
            errwarns.propagate(m_xsize->get_line(), ValueError(
                N_("size specifier not an integer expression")));
    }

    // get EQU value for constants
    const Expr* equ_expr_c = sym.get_equ();

    if (equ_expr_c != 0)
    {
        std::auto_ptr<Expr> equ_expr(equ_expr_c->clone());
        equ_expr->simplify(&xform_calc_dist);
        IntNum* equ_intn = equ_expr->get_intnum();

        if (equ_intn)
            m_value = *equ_intn;
        else
            errwarns.propagate(equ_expr->get_line(), ValueError(
                N_("EQU value not an integer expression")));

        m_index = SHN_ABS;
    }
}

void
ElfSymbol::write(Bytes& bytes, const ElfConfig& config)
{
    bytes.resize(0);
    config.setup_endian(bytes);

    write_32(bytes, m_name ? m_name->get_index() : 0);

    if (config.cls == ELFCLASS32)
    {
        write_32(bytes, m_value);
        write_32(bytes, m_size);
    }

    write_8(bytes, ELF_ST_INFO(m_bind, m_type));
    write_8(bytes, ELF_ST_OTHER(m_vis));

    if (m_sect)
    {
        ElfSection* elfsect = get_elf(*m_sect);
        assert(elfsect != 0);
        write_16(bytes, elfsect->get_index());
    }
    else
    {
        write_16(bytes, m_index);
    }

    if (config.cls == ELFCLASS64)
    {
        write_64(bytes, m_value);
        write_64(bytes, m_size);
    }

    if (config.cls == ELFCLASS32)
        assert(bytes.size() == SYMTAB32_SIZE);
    else if (config.cls == ELFCLASS64)
        assert(bytes.size() == SYMTAB64_SIZE);
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
    ElfSymbol undef(0);
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
ElfConfig::symtab_read(std::istream&    is,
                       Object&          object,
                       unsigned long    size,
                       ElfSize          symsize,
                       ElfStrtab&       strtab,
                       const char*      strtab_str,
                       unsigned long    strtab_size,
                       Section*         sections[]) const
{
    is.seekg(symsize, std::ios_base::cur);  // skip first symbol (undef)

    Bytes bytes;
    ElfSymbolIndex index = 1;
    for (unsigned long pos=symsize; pos<size; pos += symsize, ++index)
    {
        bytes.resize(0);
        bytes.write(is, symsize);
        if (!is)
            throw Error(N_("could not read symbol entry"));

        std::auto_ptr<ElfSymbol> elfsym(
            new ElfSymbol(*this, bytes, index, strtab, strtab_str,
                          strtab_size, sections));

        SymbolRef sym = elfsym->create_symbol(object);

        if (sym)
        {
            // Associate symbol data with symbol
            sym->add_assoc_data(ElfSymbol::key,
                std::auto_ptr<AssocData>(elfsym.release()));
        }
    }

    return true;
}

void
ElfSymbol::set_size(std::auto_ptr<Expr> size)
{
    m_xsize.reset(size.release());
}                            

ElfSection::ElfSection(const ElfConfig&     config,
                       std::istream&        is,
                       ElfSectionIndex      index,
                       ElfStrtab&           shstrtab,
                       const char*          shstrtab_str,
                       unsigned long        shstrtab_size)
    : m_config(config)
    , m_sym(0)
    , m_index(index)
    , m_rel_name(0)
    , m_rel_index(0)
    , m_rel_offset(0)
{
    Bytes bytes;

    bytes.write(is, m_config.secthead_size);
    if (!is)
        throw Error(N_("could not read section header"));

    m_config.setup_endian(bytes);

    unsigned long name_idx = read_u32(bytes);
    if (shstrtab_str && name_idx < shstrtab_size)
        m_name = shstrtab.append_str(&shstrtab_str[name_idx]);
    else
        m_name = 0;
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
                       ElfStrtab::Entry*    name,
                       ElfSectionType       type,
                       ElfSectionFlags      flags)
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
    , m_name(name)
    , m_index(0)
    , m_rel_name(0)
    , m_rel_index(0)
    , m_rel_offset(0)
{
    if (name && name->get_str() == ".symtab")
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
    os << "name=";
    if (m_name)
        os << m_name->get_str();
    else
        os << "<undef>";
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

    write_32(scratch, m_name ? m_name->get_index() : 0);
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
ElfSection::create_section() const
{
    bool bss = (m_type == SHT_NOBITS || m_offset == 0);

    std::auto_ptr<Section> section(
        new Section(get_name(), m_flags & SHF_EXECINSTR, bss, 0));

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

    write_32(scratch, m_rel_name ? m_rel_name->get_index() : 0);
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

        unsigned int r_type;
        if (reloc.m_wrt)
        {
            ElfSpecialSymbol* ssym = get_elf_ssym(*reloc.m_wrt);
            if (!ssym || reloc.m_valsize != ssym->size)
                throw InternalError(N_("Unsupported WRT"));

            // Force TLS type; this is required by the linker.
            if (ssym->thread_local)
            {
                if (ElfSymbol* esym = get_elf(*reloc.get_sym()))
                    esym->set_type(STT_TLS);
            }
            r_type = ssym->reloc;
        }
        else
            r_type = machine.map_reloc_info_to_type(reloc);

        scratch.resize(0);
        reloc.write(scratch, m_config, r_type);
        os << scratch;
        size += scratch.size();
    }
    return size;
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
    else if (align & (align - 1))
        throw InternalError(String::compose(
            N_("alignment %1 for section `%2' is not a power of 2"),
            align, m_name->get_str()));

    m_offset = (pos + align - 1) & ~(align - 1);
    return m_offset;
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
    if (!os.good())
        throw InternalError(N_("Failed to write ELF program header"));
}

std::string
ElfConfig::name_reloc_section(const std::string& basesect) const
{
    if (rela)
        return ".rela"+basesect;
    else
        return ".rel"+basesect;
}

}}} // namespace yasm::objfmt::elf

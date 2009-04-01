//
// ELF object format symbol
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
#include "ElfSymbol.h"

#include <util.h>

#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/marg_ostream.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/Errwarns.h"
#include "yasmx/Expr.h"
#include "yasmx/Location_util.h"
#include "yasmx/Object.h"
#include "yasmx/StringTable.h"

#include "ElfConfig.h"
#include "ElfSection.h"


namespace yasm
{
namespace objfmt
{
namespace elf
{

const char* ElfSymbol::key = "objfmt::elf::ElfSymbol";

ElfSymbol::ElfSymbol(const ElfConfig&   config,
                     Bytes&             bytes,
                     ElfSymbolIndex     index,
                     Section*           sections[])
    : m_sect(0)
    , m_value(0)
    , m_xsize(0)
    , m_size(0)
    , m_symindex(index)
{
    bytes.set_readpos(0);
    config.setup_endian(bytes);

    m_name_index = read_u32(bytes);

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

ElfSymbol::ElfSymbol()
    : m_sect(0)
    , m_name_index(0)
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
ElfSymbol::create_symbol(Object& object, const StringTable& strtab) const
{
    SymbolRef sym(0);
    std::string name = strtab.get_str(m_name_index);

    if (m_bind == STB_GLOBAL || m_bind == STB_WEAK)
    {
        sym = object.get_symbol(name);
        if (m_index == SHN_UNDEF)
            sym->declare(Symbol::EXTERN, 0);
        else
            sym->declare(Symbol::GLOBAL, 0);
    }
    else
    {
        // don't index by name, just append
        sym = object.append_symbol(name);
    }

    if (m_index == SHN_ABS)
    {
        sym->define_equ(Expr::Ptr(new Expr(m_size)), 0);
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
        simplify_calc_dist(*m_xsize);
        IntNum* xsize = m_xsize->get_intnum();
        if (xsize)
            m_size = *xsize;
        else
            errwarns.propagate(m_size_line, ValueError(
                N_("size specifier not an integer expression")));
    }

    // get EQU value for constants
    const Expr* equ_expr_c = sym.get_equ();

    if (equ_expr_c != 0)
    {
        Expr equ_expr = *equ_expr_c;
        simplify_calc_dist(equ_expr);
        IntNum* equ_intn = equ_expr.get_intnum();

        if (equ_intn)
            m_value = *equ_intn;
        else
            errwarns.propagate(sym.get_def_line(), ValueError(
                N_("EQU value not an integer expression")));

        m_index = SHN_ABS;
    }
}

void
ElfSymbol::write(Bytes& bytes, const ElfConfig& config)
{
    bytes.resize(0);
    config.setup_endian(bytes);

    write_32(bytes, m_name_index);

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

void
ElfSymbol::set_size(std::auto_ptr<Expr> size, unsigned long line)
{
    m_xsize.reset(size.release());
    m_size_line = line;
}

}}} // namespace yasm::objfmt::elf

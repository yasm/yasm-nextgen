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
    , m_size(0)
    , m_symindex(index)
{
    bytes.setReadPosition(0);
    config.setEndian(bytes);

    m_name_index = ReadU32(bytes);

    if (config.cls == ELFCLASS32)
    {
        m_value = ReadU32(bytes);
        m_size = Expr(ReadU32(bytes));
    }

    unsigned char info = ReadU8(bytes);
    m_bind = ELF_ST_BIND(info);
    m_type = ELF_ST_TYPE(info);
    m_vis = ELF_ST_VISIBILITY(ReadU8(bytes));

    m_index = static_cast<ElfSectionIndex>(ReadU16(bytes));
    if (m_index != SHN_UNDEF && m_index < config.secthead_count)
        m_sect = sections[m_index];

    if (config.cls == ELFCLASS64)
    {
        m_value = ReadU64(bytes);
        m_size = Expr(ReadU64(bytes));
    }
}

ElfSymbol::ElfSymbol()
    : m_sect(0)
    , m_name_index(0)
    , m_value(0)
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
ElfSymbol::CreateSymbol(Object& object, const StringTable& strtab) const
{
    SymbolRef sym(0);
    std::string name = strtab.getString(m_name_index);

    if (m_bind == STB_GLOBAL || m_bind == STB_WEAK)
    {
        sym = object.getSymbol(name);
        if (m_index == SHN_UNDEF)
            sym->Declare(Symbol::EXTERN, 0);
        else
            sym->Declare(Symbol::GLOBAL, 0);
    }
    else
    {
        // don't index by name, just append
        sym = object.AppendSymbol(name);
    }

    if (m_index == SHN_ABS)
    {
        sym->DefineEqu(m_size, 0);
    }
    else if (m_index == SHN_COMMON)
    {
        sym->Declare(Symbol::COMMON, 0);
    }
    else if (m_sect != 0)
    {
        Location loc = {&m_sect->bytecodes_first(), m_value.getUInt()};
        sym->DefineLabel(loc, 0);
    }

    return sym;
}

void
ElfSymbol::Put(marg_ostream& os) const
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
    os << "size=" << m_size << '\n';
}

void
ElfSymbol::Finalize(Symbol& sym, Errwarns& errwarns)
{
    // If symbol is in a TLS section, force its type to TLS.
    Location loc;
    Section* sect;
    ElfSection* elfsect;
    if (sym.getLabel(&loc) &&
        (sect = loc.bc->getContainer()->AsSection()) &&
        (elfsect = getElf(*sect)) &&
        (elfsect->getFlags() & SHF_TLS))
    {
        m_type = STT_TLS;
    }

    // get size (if specified); expr overrides stored integer
    if (!m_size.isEmpty())
    {
        SimplifyCalcDist(m_size);
        if (!m_size.isIntNum())
            errwarns.Propagate(m_size_line, ValueError(
                N_("size specifier not an integer expression")));
    }

    // get EQU value for constants
    const Expr* equ_expr_c = sym.getEqu();

    if (equ_expr_c != 0)
    {
        Expr equ_expr = *equ_expr_c;
        SimplifyCalcDist(equ_expr);

        if (equ_expr.isIntNum())
            m_value = equ_expr.getIntNum();
        else
            errwarns.Propagate(sym.getDefLine(), ValueError(
                N_("EQU value not an integer expression")));

        m_index = SHN_ABS;
    }
}

void
ElfSymbol::Write(Bytes& bytes, const ElfConfig& config)
{
    bytes.resize(0);
    config.setEndian(bytes);

    Write32(bytes, m_name_index);

    if (config.cls == ELFCLASS32)
    {
        Write32(bytes, m_value);
        Write32(bytes, m_size.getIntNum());
    }

    Write8(bytes, ELF_ST_INFO(m_bind, m_type));
    Write8(bytes, ELF_ST_OTHER(m_vis));

    if (m_sect)
    {
        ElfSection* elfsect = getElf(*m_sect);
        assert(elfsect != 0);
        Write16(bytes, elfsect->getIndex());
    }
    else
    {
        Write16(bytes, m_index);
    }

    if (config.cls == ELFCLASS64)
    {
        Write64(bytes, m_value);
        Write64(bytes, m_size.getIntNum());
    }

    if (config.cls == ELFCLASS32)
        assert(bytes.size() == SYMTAB32_SIZE);
    else if (config.cls == ELFCLASS64)
        assert(bytes.size() == SYMTAB64_SIZE);
}

}}} // namespace yasm::objfmt::elf

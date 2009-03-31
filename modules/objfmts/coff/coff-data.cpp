//
// COFF object format data implementations
//
//  Copyright (C) 2002-2008  Peter Johnson
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
#include "coff-data.h"

#include <util.h>

#include <cstring>

#include <libyasmx/Bytecode.h>
#include <libyasmx/Bytes.h>
#include <libyasmx/Bytes_util.h>
#include <libyasmx/Compose.h>
#include <libyasmx/errwarn.h>
#include <libyasmx/Errwarns.h>
#include <libyasmx/Location_util.h>
#include <libyasmx/marg_ostream.h>
#include <libyasmx/scoped_ptr.h>
#include <libyasmx/Section.h>
#include <libyasmx/StringTable.h>
#include <libyasmx/Symbol.h>
#include <libyasmx/Symbol_util.h>


namespace yasm
{
namespace objfmt
{
namespace coff
{

const char* CoffSection::key = "objfmts::coff::CoffSection";
const char* CoffSymbol::key = "objfmt::coff::CoffSymbol";

const unsigned long CoffSection::TEXT        = 0x00000020UL;
const unsigned long CoffSection::DATA        = 0x00000040UL;
const unsigned long CoffSection::BSS         = 0x00000080UL;
const unsigned long CoffSection::INFO        = 0x00000200UL;
const unsigned long CoffSection::STD_MASK    = 0x000003FFUL;
const unsigned long CoffSection::ALIGN_MASK  = 0x00F00000UL;
const unsigned int  CoffSection::ALIGN_SHIFT = 20;

// Win32-specific flags
const unsigned long CoffSection::NRELOC_OVFL = 0x01000000UL;
const unsigned long CoffSection::DISCARD     = 0x02000000UL;
const unsigned long CoffSection::NOCACHE     = 0x04000000UL;
const unsigned long CoffSection::NOPAGE      = 0x08000000UL;
const unsigned long CoffSection::SHARED      = 0x10000000UL;
const unsigned long CoffSection::EXECUTE     = 0x20000000UL;
const unsigned long CoffSection::READ        = 0x40000000UL;
const unsigned long CoffSection::WRITE       = 0x80000000UL;
const unsigned long CoffSection::WIN32_MASK  = 0xFF000000UL;


CoffReloc::CoffReloc(const IntNum& addr, SymbolRef sym, Type type)
    : Reloc(addr, sym)
    , m_type(type)
{
}

CoffReloc::~CoffReloc()
{
}

Expr
CoffReloc::get_value() const
{
    return Expr(m_sym);
}

void
CoffReloc::write(Bytes& bytes) const
{
    const CoffSymbol* csym = get_coff(*m_sym);
    assert(csym != 0);      // need symbol data for relocated symbol

    bytes << little_endian;

    write_32(bytes, m_addr);            // address of relocation
    write_32(bytes, csym->m_index);     // relocated symbol
    write_16(bytes, m_type);            // type of relocation
}

Coff32Reloc::~Coff32Reloc()
{
}

std::string
Coff32Reloc::get_type_name() const
{
    const char* name;
    switch (m_type)
    {
        case ABSOLUTE:          name = "ABSOLUTE"; break;
        case I386_ADDR16:       name = "I386_ADDR16"; break;
        case I386_REL16:        name = "I386_REL16"; break;
        case I386_ADDR32:       name = "I386_ADDR32"; break;
        case I386_ADDR32NB:     name = "I386_ADDR32NB"; break;
        case I386_SEG12:        name = "I386_SEG12"; break;
        case I386_SECTION:      name = "I386_SECTION"; break;
        case I386_SECREL:       name = "I386_SECREL"; break;
        case I386_TOKEN:        name = "I386_TOKEN"; break;
        case I386_SECREL7:      name = "I386_SECREL7"; break;
        case I386_REL32:        name = "I386_REL32"; break;
        default:                name = "***UNKNOWN***"; break;
    }

    return name;
}

Coff64Reloc::~Coff64Reloc()
{
}

std::string
Coff64Reloc::get_type_name() const
{
    const char* name;
    switch (m_type)
    {
        case ABSOLUTE:          name = "ABSOLUTE"; break;
        case AMD64_ADDR64:      name = "AMD64_ADDR64"; break;
        case AMD64_ADDR32:      name = "AMD64_ADDR32"; break;
        case AMD64_ADDR32NB:    name = "AMD64_ADDR32NB"; break;
        case AMD64_REL32:       name = "AMD64_REL32"; break;
        case AMD64_REL32_1:     name = "AMD64_REL32_1"; break;
        case AMD64_REL32_2:     name = "AMD64_REL32_2"; break;
        case AMD64_REL32_3:     name = "AMD64_REL32_3"; break;
        case AMD64_REL32_4:     name = "AMD64_REL32_4"; break;
        case AMD64_REL32_5:     name = "AMD64_REL32_5"; break;
        case AMD64_SECTION:     name = "AMD64_SECTION"; break;
        case AMD64_SECREL:      name = "AMD64_SECREL"; break;
        case AMD64_SECREL7:     name = "AMD64_SECREL7"; break;
        case AMD64_TOKEN:       name = "AMD64_TOKEN"; break;
        default:                name = "***UNKNOWN***"; break;
    }

    return name;
}

CoffSection::CoffSection(SymbolRef sym)
    : m_sym(sym)
    , m_scnum(0)
    , m_flags(0)
    , m_size(0)
    , m_relptr(0)
    , m_strtab_name(0)
    , m_nobase(false)
    , m_isdebug(false)
    , m_setalign(false)
{
}

CoffSection::~CoffSection()
{
}

void
CoffSection::put(marg_ostream& os) const
{
    os << "sym=\n";
    ++os;
    os << *m_sym;
    --os;
    os << "scnum=" << m_scnum << '\n';
    os << "flags=";
    switch (m_flags & STD_MASK)
    {
        case TEXT:
            os << "TEXT";
            break;
        case DATA:
            os << "DATA";
            break;
        case BSS:
            os << "BSS";
            break;
        default:
            os << "UNKNOWN";
            break;
    }
    os << std::showbase;
    os << '(' << std::hex << m_flags << std::dec << ")\n";
    os << "size=" << m_size << '\n';
    os << "relptr=" << std::hex << m_relptr << std::dec << '\n';
}

void
CoffSection::write(Bytes& bytes, const Section& sect) const
{
    bytes << little_endian;

    // Check to see if alignment is supported size
    unsigned long align = sect.get_align();
    if (align > 8192)
        align = 8192;

    // Convert alignment into flags setting
    unsigned long flags = m_flags;
    flags &= ~ALIGN_MASK;
    while (align != 0)
    {
        flags += 1<<ALIGN_SHIFT;
        align >>= 1;
    }

    // section name
    char name[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    if (sect.get_name().length() > 8)
    {
        std::string namenum = "/";
        namenum += String::format(m_strtab_name);
        std::strncpy(name, namenum.c_str(), 8);
    }
    else
        std::strncpy(name, sect.get_name().c_str(), 8);
    bytes.write(reinterpret_cast<unsigned char*>(name), 8);
    if (m_isdebug)
    {
        write_32(bytes, 0);         // physical address
        write_32(bytes, 0);         // virtual address
    }
    else
    {
        write_32(bytes, sect.get_lma());    // physical address
        write_32(bytes, sect.get_vma());    // virtual address
    }
    write_32(bytes, m_size);                // section size
    write_32(bytes, sect.get_filepos());    // file ptr to data
    write_32(bytes, m_relptr);              // file ptr to relocs
    write_32(bytes, 0);                     // file ptr to line nums
    if (sect.get_relocs().size() >= 64*1024)
        write_16(bytes, 0xFFFF);            // max out
    else
        write_16(bytes, sect.get_relocs().size()); // num of relocation entries
    write_16(bytes, 0);                     // num of line number entries
    write_32(bytes, flags);                 // flags
}

CoffSymbol::CoffSymbol(StorageClass sclass, AuxType auxtype)
    : m_index(0)
    , m_sclass(sclass)
    , m_auxtype(auxtype)
{
    if (auxtype != AUX_NONE)
        m_aux.resize(1);
}

CoffSymbol::~CoffSymbol()
{
}

void
CoffSymbol::put(marg_ostream& os) const
{
    os << "symtab index=" << m_index << '\n';
    os << "sclass=" << m_sclass << '\n';
}

void
CoffSymbol::write(Bytes& bytes,
                  const Symbol& sym,
                  Errwarns& errwarns,
                  StringTable& strtab) const
{
    int vis = sym.get_visibility();

    IntNum value = 0;
    unsigned int scnum = 0xfffe;    // -2 = debugging symbol
    unsigned long scnlen = 0;   // for sect auxent
    unsigned long nreloc = 0;   // for sect auxent

    // Look at symrec for value/scnum/etc.
    Location loc;
    if (sym.get_label(&loc))
    {
        Section* sect = 0;
        if (loc.bc)
            sect = loc.bc->get_container()->as_section();
        // it's a label: get value and offset.
        // If there is not a section, leave as debugging symbol.
        if (sect)
        {
            CoffSection* coffsect = get_coff(*sect);
            assert(coffsect != 0);

            scnum = coffsect->m_scnum;
            scnlen = coffsect->m_size;
            nreloc = sect->get_relocs().size();
            value = sect->get_vma();
            if (loc.bc)
                value += loc.get_offset();
        }
    }
    else if (const Expr* equ_val_c = sym.get_equ())
    {
        Expr equ_val(*equ_val_c);
        simplify_calc_dist(equ_val);
        if (const IntNum* intn = equ_val.get_intnum())
            value = *intn;
        else if (vis & Symbol::GLOBAL)
        {
            errwarns.propagate(sym.get_def_line(), NotConstantError(
                N_("global EQU value not an integer expression")));
        }

        scnum = 0xffff;     // -1 = absolute symbol
    }
    else
    {
        if (vis & Symbol::COMMON)
        {
            assert(get_common_size(sym) != 0);
            Expr csize(*get_common_size(sym));
            simplify_calc_dist(csize);
            if (const IntNum* intn = csize.get_intnum())
                value = *intn;
            else
            {
                errwarns.propagate(sym.get_def_line(), NotConstantError(
                    N_("COMMON data size not an integer expression")));
            }
            scnum = 0;
        }
        if (vis & Symbol::EXTERN)
            scnum = 0;
    }

    bytes << little_endian;

    std::string name;
    size_t len;

    if (sym.is_abs())
        name = ".absolut";
    else
        name = sym.get_name();
    len = name.length();

    if (len > 8)
    {
        write_32(bytes, 0);                         // "zeros" field
        write_32(bytes, strtab.get_index(name));    // strtab offset
    }
    else
    {
        // <8 chars, so no string table entry needed
        bytes.write(reinterpret_cast<const unsigned char*>(name.data()), len);
        bytes.write(8-len, 0);
    }
    write_32(bytes, value);         // value
    write_16(bytes, scnum);         // section number
    write_16(bytes, 0);             // type is always zero (for now)
    write_8(bytes, m_sclass);       // storage class
    write_8(bytes, m_aux.size());   // number of aux entries

    assert(bytes.size() == 18);

    for (std::vector<AuxEntry>::const_iterator i=m_aux.begin(), end=m_aux.end();
         i != end; ++i)
    {
        switch (m_auxtype)
        {
            case AUX_NONE:
                bytes.write(18, 0);
                break;
            case AUX_SECT:
                write_32(bytes, scnlen);    // section length
                write_16(bytes, nreloc);    // number relocs
                bytes.write(12, 0);         // number line nums, 0 fill
                break;
            case AUX_FILE:
                len = i->fname.length();
                if (len > 18)
                {
                    write_32(bytes, 0);
                    write_32(bytes, strtab.get_index(i->fname));
                    bytes.write(18-8, 0);
                }
                else
                {
                    bytes.write(reinterpret_cast<const unsigned char*>
                                (i->fname.data()), len);
                    bytes.write(18-len, 0);
                }
                break;
            default:
                assert(false);  // unrecognized aux symtab type
        }
    }

    assert(bytes.size() == 18+18*m_aux.size());
}

}}} // namespace yasm::objfmt::coff

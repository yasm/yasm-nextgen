//
// COFF object format symbol data implementation
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
#include "CoffSymbol.h"

#include <util.h>

#include <libyasmx/Bytecode.h>
#include <libyasmx/Bytes_util.h>
#include <libyasmx/errwarn.h>
#include <libyasmx/Errwarns.h>
#include <libyasmx/IntNum.h>
#include <libyasmx/Location_util.h>
#include <libyasmx/marg_ostream.h>
#include <libyasmx/Section.h>
#include <libyasmx/Symbol_util.h>
#include <libyasmx/StringTable.h>

#include "CoffSection.h"


namespace yasm
{
namespace objfmt
{
namespace coff
{

const char* CoffSymbol::key = "objfmt::coff::CoffSymbol";

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

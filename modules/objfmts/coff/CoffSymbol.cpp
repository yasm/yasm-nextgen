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

#include <YAML/emitter.h>
#include <yasmx/Support/errwarn.h>
#include <yasmx/Bytecode.h>
#include <yasmx/Bytes_util.h>
#include <yasmx/Errwarns.h>
#include <yasmx/IntNum.h>
#include <yasmx/Location_util.h>
#include <yasmx/Section.h>
#include <yasmx/Symbol_util.h>
#include <yasmx/StringTable.h>

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
CoffSymbol::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << key;
    out << YAML::Key << "symtab index" << YAML::Value << m_index;
    out << YAML::Key << "sclass" << YAML::Value << static_cast<int>(m_sclass);

    out << YAML::Key << "aux type" << YAML::Value;
    switch (m_auxtype)
    {
        case AUX_SECT:  out << "SECT"; break;
        case AUX_FILE:  out << "FILE"; break;
        case AUX_NONE:
        default:        out << YAML::Null; break;
    }

    out << YAML::Key << "aux" << YAML::Value;
    if (m_aux.empty())
        out << YAML::Flow;
    out << YAML::BeginSeq;
    for (std::vector<AuxEntry>::const_iterator i=m_aux.begin(), end=m_aux.end();
         i != end; ++i)
        out << i->fname;
    out << YAML::EndSeq;
    out << YAML::EndMap;
}

void
CoffSymbol::Write(Bytes& bytes,
                  const Symbol& sym,
                  Errwarns& errwarns,
                  StringTable& strtab) const
{
    int vis = sym.getVisibility();

    IntNum value = 0;
    unsigned int scnum = 0xfffe;    // -2 = debugging symbol
    unsigned long scnlen = 0;   // for sect auxent
    unsigned long nreloc = 0;   // for sect auxent

    // Look at symrec for value/scnum/etc.
    Location loc;
    if (sym.getLabel(&loc))
    {
        Section* sect = 0;
        if (loc.bc)
            sect = loc.bc->getContainer()->AsSection();
        // it's a label: get value and offset.
        // If there is not a section, leave as debugging symbol.
        if (sect)
        {
            CoffSection* coffsect = sect->getAssocData<CoffSection>();
            assert(coffsect != 0);

            scnum = coffsect->m_scnum;
            scnlen = coffsect->m_size;
            nreloc = sect->getRelocs().size();
            value = sect->getVMA();
            if (loc.bc)
                value += loc.getOffset();
        }
    }
    else if (const Expr* equ_val_c = sym.getEqu())
    {
        Expr equ_val(*equ_val_c);
        SimplifyCalcDist(equ_val);
        if (equ_val.isIntNum())
            value = equ_val.getIntNum();
        else if (vis & Symbol::GLOBAL)
        {
            errwarns.Propagate(sym.getDefLine(), NotConstantError(
                N_("global EQU value not an integer expression")));
        }

        scnum = 0xffff;     // -1 = absolute symbol
    }
    else
    {
        if (vis & Symbol::COMMON)
        {
            assert(getCommonSize(sym) != 0);
            Expr csize(*getCommonSize(sym));
            SimplifyCalcDist(csize);
            if (csize.isIntNum())
                value = csize.getIntNum();
            else
            {
                errwarns.Propagate(sym.getDefLine(), NotConstantError(
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

    if (sym.isAbsoluteSymbol())
        name = ".absolut";
    else
        name = sym.getName();
    len = name.length();

    if (len > 8)
    {
        Write32(bytes, 0);                      // "zeros" field
        Write32(bytes, strtab.getIndex(name));  // strtab offset
    }
    else
    {
        // <8 chars, so no string table entry needed
        bytes.Write(reinterpret_cast<const unsigned char*>(name.data()), len);
        bytes.Write(8-len, 0);
    }
    Write32(bytes, value);          // value
    Write16(bytes, scnum);          // section number
    Write16(bytes, 0);              // type is always zero (for now)
    Write8(bytes, m_sclass);        // storage class
    Write8(bytes, m_aux.size());    // number of aux entries

    assert(bytes.size() == 18);

    for (std::vector<AuxEntry>::const_iterator i=m_aux.begin(), end=m_aux.end();
         i != end; ++i)
    {
        switch (m_auxtype)
        {
            case AUX_NONE:
                bytes.Write(18, 0);
                break;
            case AUX_SECT:
                Write32(bytes, scnlen);     // section length
                Write16(bytes, nreloc);     // number relocs
                bytes.Write(12, 0);         // number line nums, 0 fill
                break;
            case AUX_FILE:
                len = i->fname.length();
                if (len > 18)
                {
                    Write32(bytes, 0);
                    Write32(bytes, strtab.getIndex(i->fname));
                    bytes.Write(18-8, 0);
                }
                else
                {
                    bytes.Write(reinterpret_cast<const unsigned char*>
                                (i->fname.data()), len);
                    bytes.Write(18-len, 0);
                }
                break;
            default:
                assert(false);  // unrecognized aux symtab type
        }
    }

    assert(bytes.size() == 18+18*m_aux.size());
}

}}} // namespace yasm::objfmt::coff

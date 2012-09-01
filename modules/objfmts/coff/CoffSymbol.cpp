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

#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/Expr_util.h"
#include "yasmx/IntNum.h"
#include "yasmx/Location_util.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol_util.h"
#include "yasmx/StringTable.h"

#include "CoffSection.h"


using namespace yasm;
using namespace yasm::objfmt;

const char* CoffSymbol::key = "objfmt::coff::CoffSymbol";

CoffSymbol::CoffSymbol(StorageClass sclass, AuxType auxtype)
    : m_forcevis(false)
    , m_index(0)
    , m_sclass(sclass)
    , m_type(0)
    , m_auxtype(auxtype)
{
    if (auxtype != AUX_NONE)
        m_aux.resize(1);
}

CoffSymbol::~CoffSymbol()
{
}

#ifdef WITH_XML
pugi::xml_node
CoffSymbol::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("CoffSymbol");
    root.append_attribute("key") = key;
    append_child(root, "ForceVis", m_forcevis);
    append_child(root, "SymIndex", m_index);
    append_child(root, "SClass", static_cast<int>(m_sclass));
    append_child(root, "SymbolType", m_type);

    switch (m_auxtype)
    {
        case AUX_SECT:  append_child(root, "AuxType", "SECT"); break;
        case AUX_FILE:  append_child(root, "AuxType", "FILE"); break;
        case AUX_NONE:  break;
    }

    pugi::xml_node aux = root.append_child("Aux");
    for (std::vector<AuxEntry>::const_iterator i=m_aux.begin(), end=m_aux.end();
         i != end; ++i)
        append_child(aux, "FName", i->fname);
    return root;
}
#endif // WITH_XML

void
CoffSymbol::Write(Bytes& bytes,
                  const Symbol& sym,
                  DiagnosticsEngine& diags,
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
            sect = loc.bc->getContainer()->getSection();
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
    else if (const Expr* equ_expr_c = sym.getEqu())
    {
        Expr equ_expr = *equ_expr_c;
        if (!ExpandEqu(equ_expr))
        {
            diags.Report(sym.getDefSource(), diag::err_equ_circular_reference);
            return;
        }
        SimplifyCalcDist(equ_expr, diags);

        // trivial case: simple integer
        if (equ_expr.isIntNum())
        {
            scnum = 0xffff;     // -1 = absolute symbol
            value = equ_expr.getIntNum();
        }
        else
        {
            // otherwise might contain relocatable value (e.g. symbol alias)
            std::auto_ptr<Expr> equ_expr_p(new Expr);
            equ_expr_p->swap(equ_expr);
            Value val(64, equ_expr_p);
            val.setSource(sym.getDefSource());
            if (!val.Finalize(diags, diag::err_equ_too_complex))
                return;
            if (val.isComplexRelative())
            {
                diags.Report(sym.getDefSource(), diag::err_equ_too_complex);
                return;
            }

            // set section appropriately based on if value is relative
            if (val.isRelative())
            {
                SymbolRef rel = val.getRelative();
                Location loc;
                if (!rel->getLabel(&loc) || !loc.bc)
                {
                    // Referencing an undefined label?
                    // GNU as silently allows this... but doesn't gen the symbol?
                    // We make it an error instead.
                    diags.Report(sym.getDefSource(), diag::err_equ_too_complex);
                    return;
                }

                Section* sect = loc.bc->getContainer()->getSection();
                CoffSection* coffsect = sect->getAssocData<CoffSection>();
                assert(coffsect != 0);
                scnum = coffsect->m_scnum;
                value = sect->getVMA() + loc.getOffset();
            }
            else
            {
                scnum = 0xffff;     // -1 = absolute symbol
                value = 0;
            }

            // add in any remaining absolute portion
            if (Expr* abs = val.getAbs())
            {
                SimplifyCalcDist(*abs, diags);
                if (abs->isIntNum())
                    value += abs->getIntNum();
                else
                    diags.Report(sym.getDefSource(), diag::err_equ_not_integer);
            }
        }
    }
    else
    {
        if (vis & Symbol::COMMON)
        {
            assert(getCommonSize(sym) != 0);
            Expr csize(*getCommonSize(sym));
            SimplifyCalcDist(csize, diags);
            if (csize.isIntNum())
                value = csize.getIntNum();
            else
            {
                diags.Report(sym.getDefSource(),
                             diag::err_common_size_not_integer);
            }
            scnum = 0;
        }
        if (vis & Symbol::EXTERN)
            scnum = 0;
    }

    bytes.setLittleEndian();

    StringRef name;
    if (sym.isAbsoluteSymbol())
        name = ".absolut";
    else
        name = sym.getName();

    size_t len = name.size();
    if (len > 8)
    {
        Write32(bytes, 0);                      // "zeros" field
        Write32(bytes, strtab.getIndex(name));  // strtab offset
    }
    else
    {
        // <8 chars, so no string table entry needed
        bytes.WriteString(name);
        bytes.Write(8-len, 0);
    }
    Write32(bytes, value);          // value
    Write16(bytes, scnum);          // section number
    Write16(bytes, m_type);         // type
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
                    bytes.WriteString(i->fname);
                    bytes.Write(18-len, 0);
                }
                break;
            default:
                assert(false);  // unrecognized aux symtab type
        }
    }

    assert(bytes.size() == 18+18*m_aux.size());
}

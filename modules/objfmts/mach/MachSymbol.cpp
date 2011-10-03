//
// Mach-O symbol
//
//  Copyright (C) 2004-2007  Peter Johnson
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
#include "MachSymbol.h"

#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/Expr.h"
#include "yasmx/Expr_util.h"
#include "yasmx/Location.h"
#include "yasmx/Location_util.h"
#include "yasmx/Section.h"
#include "yasmx/StringTable.h"
#include "yasmx/Symbol.h"
#include "yasmx/Symbol_util.h"

#include "MachSection.h"


using namespace yasm;
using namespace yasm::objfmt;

const char* MachSymbol::key = "objfmt::MachSymbol";

MachSymbol&
MachSymbol::Build(Symbol& sym)
{
    MachSymbol* msym = sym.getAssocData<MachSymbol>();
    if (!msym)
    {
        msym = new MachSymbol;
        sym.AddAssocData(std::auto_ptr<MachSymbol>(msym));
    }
    return *msym;
}

MachSymbol::MachSymbol()
    : m_private_extern(false)
    , m_no_dead_strip(false)
    , m_weak_ref(false)
    , m_weak_def(false)
    , m_ref_flag(REFERENCE_TYPE)
    , m_required(false)
    , m_index(0)
    , m_desc_override(false)
{
}

MachSymbol::~MachSymbol()
{
}

#ifdef WITH_XML
pugi::xml_node
MachSymbol::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("MachSymbol");
    root.append_attribute("key") = key;
    append_child(root, "Index", m_index);
    return root;
}
#endif // WITH_XML

void
MachSymbol::Finalize(const Symbol& sym, DiagnosticsEngine& diags)
{
    int vis = sym.getVisibility();

    IntNum value = 0;
    long scnum = -3;        // -3 = debugging symbol
    unsigned int n_type = 0;

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
            if (&(*sect->getSymbol()) == &sym)
                return;     // don't store section names

            MachSection* msect = sect->getAssocData<MachSection>();
            assert(msect != 0);
            scnum = msect->scnum;
            n_type = N_SECT;

            // all values are subject to correction: base offset is first
            // raw section, therefore add section offset
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

        if (equ_expr.isIntNum())
            value = equ_expr.getIntNum();
        else if (vis & Symbol::GLOBAL)
            diags.Report(sym.getDefSource(), diag::err_equ_too_complex);
        n_type = N_ABS;
        scnum = -2;         // -2 = absolute symbol
    }

    // map standard declared visibility
    if (vis & Symbol::EXTERN)
    {
        n_type = N_EXT;
        scnum = -1;
    }
    else if (vis & Symbol::COMMON)
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
        n_type = N_UNDF | N_EXT;
    }
    else if (vis & Symbol::GLOBAL)
    {
        n_type |= N_EXT;
    }

    // map special declarations
    if (m_private_extern)
        n_type |= N_PEXT;

    unsigned int n_desc = 0;
    if (m_desc_override)
    {
        n_desc = m_desc_value;
        if ((n_type & N_TYPE) == N_UNDF)
            n_type |= N_EXT;
    }
    else
    {
        if (m_weak_ref)
        {
            n_desc |= N_WEAK_REF;
            if ((n_type & N_TYPE) == N_UNDF)
                n_type |= N_EXT;
        }

        if (m_no_dead_strip)
            n_desc |= N_NO_DEAD_STRIP;
        if (m_weak_def)
            n_desc |= N_WEAK_DEF;
        if (m_ref_flag != REFERENCE_TYPE && (n_type & N_TYPE) == N_UNDF)
        {
            n_desc |= m_ref_flag & REFERENCE_TYPE;
            n_type |= N_EXT;
        }
    }

    m_n_type = n_type;
    m_n_sect = (scnum >= 0) ? scnum + 1 : (unsigned int)NO_SECT;
    m_n_desc = n_desc;
    m_value = value;
}

void
MachSymbol::Write(Bytes& bytes,
                  const Symbol& sym,
                  StringTable& strtab,
                  int long_int_size) const
{
    bytes.setLittleEndian();
    Write32(bytes, strtab.getIndex(sym.getName())); // offset in string table
    Write8(bytes, m_n_type);          // type of symbol entry
    Write8(bytes, m_n_sect);          // referring section where symbol is found
    Write16(bytes, m_n_desc);               // extra description
    WriteN(bytes, m_value, long_int_size);  // value/argument
}

//
// Flat-format binary object format associated data
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
#include "bin-data.h"

#include <libyasmx/bc_container.h>
#include <libyasmx/bytecode.h>
#include <libyasmx/expr.h>
#include <libyasmx/marg_ostream.h>
#include <libyasmx/section.h>
#include <libyasmx/symbol.h>


namespace yasm
{
namespace objfmt
{
namespace bin
{

BinSectionData::BinSectionData()
    : has_align(false),
      has_valign(false),
      has_istart(false),
      has_ivstart(false),
      has_length(false)
{
}

BinSectionData::~BinSectionData()
{
}

void
BinSectionData::put(marg_ostream& os) const
{
}

BinSymbolData::BinSymbolData(const BinSectionData& bsd, SpecialSym which)
    : m_bsd(bsd), m_which(which)
{
}

BinSymbolData::~BinSymbolData()
{
}

void
BinSymbolData::put(marg_ostream& os) const
{
    os << "which=";
    switch (m_which)
    {
        case START: os << "START"; break;
        case VSTART: os << "VSTART"; break;
        case LENGTH: os << "LENGTH"; break;
    }
    os << '\n';
}

const IntNum*
BinSymbolData::get_value() const
{
    switch (m_which)
    {
        case START:
            if (m_bsd.has_istart)
                return 0;
            return &m_bsd.istart;
        case VSTART:
            if (m_bsd.has_ivstart)
                return 0;
            return &m_bsd.ivstart;
        case LENGTH:
            if (m_bsd.has_length)
                return 0;
            return &m_bsd.length;
        default:
            assert(false);
            return 0;
    }
}

void
expr_xform(Expr* e, const void* assoc_key)
{
    for (Expr::Terms::iterator i=e->get_terms().begin(),
         end=e->get_terms().end(); i != end; ++i)
    {
        Symbol* sym;

        // Transform our special symrecs into the appropriate value
        const IntNum* ssymval;
        if ((sym = i->get_sym()) && (ssymval = get_ssym_value(sym, assoc_key)))
            *i = *ssymval;

        // Transform symrecs or precbcs that reference sections into
        // vstart + intnum(dist).
        Location loc;
        if ((sym = i->get_sym()) && sym->get_label(&loc))
            ;
        else if (Location* locp = i->get_loc())
            loc = *locp;
        else
            continue;

        BytecodeContainer* container = loc.bc->get_container();
        Location first = {&container->bcs_first(), 0};
        IntNum dist;
        if (calc_dist(first, loc, &dist))
        {
            const Section* sect = container->as_section();
            const BinSectionData* bsd = static_cast<const BinSectionData*>
                (sect->get_assoc_data(assoc_key));
            assert(bsd);
            assert(bsd->has_ivstart);
            dist += bsd->ivstart;
            //i->destroy(); // don't need to, as it's a sym or loc
            *i = dist;
        }
    }
}

}}} // namespace yasm::objfmt::bin

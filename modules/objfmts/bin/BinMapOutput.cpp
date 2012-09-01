//
// Flat-format binary object format map file output
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
#include "BinMapOutput.h"

#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/Location.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"
#include "yasmx/Object.h"

#include "BinSection.h"
#include "BinSymbol.h"


using namespace yasm;
using namespace yasm::objfmt;
using llvm::format;

static void
MapPrescanBytes(const Section& sect, const BinSection& bsd, int* bytes)
{
    while (!bsd.length.isOkSize(*bytes * 8, 0, 0))
        *bytes *= 2;
    while (!sect.getLMA().isOkSize(*bytes * 8, 0, 0))
        *bytes *= 2;
    while (!sect.getVMA().isOkSize(*bytes * 8, 0, 0))
        *bytes *= 2;
}

BinMapOutput::BinMapOutput(raw_ostream& os,
                           const Object& object,
                           const IntNum& origin,
                           const BinGroups& groups,
                           DiagnosticsEngine& diags)
    : m_os(os)
    , m_object(object)
    , m_origin(origin)
    , m_groups(groups)
    , m_diags(diags)
{
    // Prescan all values to figure out what width we should make the output
    // fields.  Start with a minimum of 4.
    m_bytes = 4;
    while (!origin.isOkSize(m_bytes * 8, 0, 0))
        m_bytes *= 2;
    for (Object::const_section_iterator sect = object.sections_begin(),
         end = object.sections_end(); sect != end; ++sect)
    {
        const BinSection* bsd = sect->getAssocData<BinSection>();
        assert(bsd != 0);
        MapPrescanBytes(*sect, *bsd, &m_bytes);
    }
}

BinMapOutput::~BinMapOutput()
{
}

inline void
BinMapOutput::OutputIntNum(const IntNum& intn)
{
    intn.Print(m_os, 16, true, false, m_bytes*8);
}

void
BinMapOutput::OutputHeader()
{
    m_os << "\n- YASM Map file ";
    for (int i=0; i<63; ++i)
        m_os << '-';
    m_os << "\n\nSource file:  " << m_object.getSourceFilename() << '\n';
    m_os << "Output file:  " << m_object.getObjectFilename() << "\n\n";
}

void
BinMapOutput::OutputOrigin()
{
    m_os << "-- Program origin ";
    for (int i=0; i<61; ++i)
        m_os << '-';
    m_os << "\n\n";
    OutputIntNum(m_origin);
    m_os << "\n\n";
}

void
BinMapOutput::InnerSectionsSummary(const BinGroups& groups)
{
    for (BinGroups::const_iterator group = groups.begin(), end=groups.end();
         group != end; ++group)
    {
        const Section& sect = group->m_section;
        const BinSection& bsd = group->m_bsd;

        OutputIntNum(sect.getVMA());
        m_os << "  ";

        OutputIntNum(sect.getVMA() + bsd.length);
        m_os << "  ";

        OutputIntNum(sect.getLMA());
        m_os << "  ";

        OutputIntNum(sect.getLMA() + bsd.length);
        m_os << "  ";

        OutputIntNum(bsd.length);
        m_os << "  ";

        m_os << (group->m_section.isBSS() ? "nobits    " : "progbits  ");
        m_os << group->m_section.getName() << '\n';

        // Recurse to loop through follow groups
        InnerSectionsSummary(group->m_follow_groups);
    }
}

void
BinMapOutput::OutputSectionsSummary()
{
    m_os << "-- Sections (summary) ";
    for (int i=0; i<57; ++i)
        m_os << '-';

    m_os << "\n\n";
    m_os << format("%-*s", m_bytes*2+2, (const char*)"Vstart");
    m_os << format("%-*s", m_bytes*2+2, (const char*)"Vstop");
    m_os << format("%-*s", m_bytes*2+2, (const char*)"Start");
    m_os << format("%-*s", m_bytes*2+2, (const char*)"Stop");
    m_os << format("%-*s", m_bytes*2+2, (const char*)"Length");
    m_os << format("%-10s", (const char*)"Class");
    m_os << "Name\n";
    InnerSectionsSummary(m_groups);
    m_os << '\n';
}

void
BinMapOutput::InnerSectionsDetail(const BinGroups& groups)
{
    for (BinGroups::const_iterator group = groups.begin(), end=groups.end();
         group != end; ++group)
    {
        const Section& sect = group->m_section;
        StringRef name = sect.getName();
        m_os << "---- Section " << name << " ";
        for (size_t i=0; i<65-name.size(); ++i)
            m_os << '-';

        const BinSection& bsd = group->m_bsd;

        m_os << "\n\nclass:     ";
        m_os << (group->m_section.isBSS() ? "nobits" : "progbits");
        m_os << "\nlength:    ";
        OutputIntNum(bsd.length);
        m_os << "\nstart:     ";
        OutputIntNum(sect.getLMA());
        m_os << "\nalign:     ";
        OutputIntNum(bsd.align);
        m_os << "\nfollows:   ";
        m_os << (!bsd.follows.empty() ? bsd.follows : "not defined");
        m_os << "\nvstart:    ";
        OutputIntNum(sect.getVMA());
        m_os << "\nvalign:    ";
        OutputIntNum(bsd.valign);
        m_os << "\nvfollows:  ";
        m_os << (!bsd.vfollows.empty() ? bsd.vfollows : "not defined");
        m_os << "\n\n";

        // Recurse to loop through follow groups
        InnerSectionsDetail(group->m_follow_groups);
    }
}

void
BinMapOutput::OutputSectionsDetail()
{
    m_os << "-- Sections (detailed) ";
    for (int i=0; i<56; ++i)
        m_os << '-';
    m_os << "\n\n";
    InnerSectionsDetail(m_groups);
}

static unsigned long
CountSymbols(const Object& object, const Section* sect)
{
    unsigned long count = 0;

    for (Object::const_symbol_iterator sym = object.symbols_begin(),
         end = object.symbols_end(); sym != end; ++sym)
    {
        Location loc;
        // TODO: autodetect wider size
        if ((sect == 0 && sym->getEqu()) ||
            (sym->getLabel(&loc) && loc.bc->getContainer() == sect))
            count++;
    }

    return count;
}

void
BinMapOutput::OutputSymbols(const Section* sect)
{
    for (Object::const_symbol_iterator sym = m_object.symbols_begin(),
         end = m_object.symbols_end(); sym != end; ++sym)
    {
        const std::string& name = sym->getName();
        const Expr* equ;
        Location loc;

        if (sect == 0 && (equ = sym->getEqu()))
        {
            std::auto_ptr<Expr> realequ(equ->clone());
            realequ->Simplify(m_diags);
            BinSimplify(*realequ);
            realequ->Simplify(m_diags);
            OutputIntNum(realequ->getIntNum());
            m_os << "  " << name << '\n';
        }
        else if (sym->getLabel(&loc) && loc.bc->getContainer() == sect)
        {
            // Real address
            OutputIntNum(sect->getLMA() + loc.getOffset());
            m_os << "  ";

            // Virtual address
            OutputIntNum(sect->getVMA() + loc.getOffset());

            // Name
            m_os << "  " << name << '\n';
        }
    }
}

void
BinMapOutput::InnerSectionsSymbols(const BinGroups& groups)
{
    for (BinGroups::const_iterator group = groups.begin(), end=groups.end();
         group != end; ++group)
    {
        if (CountSymbols(m_object, &group->m_section) > 0)
        {
            StringRef name = group->m_section.getName();
            m_os << "---- Section " << name << ' ';
            for (size_t i=0; i<65-name.size(); ++i)
                m_os << '-';

            m_os << "\n\n";
            m_os << format("%-*s", m_bytes*2+2, (const char*)"Real");
            m_os << format("%-*s", m_bytes*2+2, (const char*)"Virtual");
            m_os << "Name\n";
            OutputSymbols(&group->m_section);
            m_os << "\n\n";
        }

        // Recurse to loop through follow groups
        InnerSectionsSymbols(group->m_follow_groups);
    }
}

void
BinMapOutput::OutputSectionsSymbols()
{
    m_os << "-- Symbols ";
    for (int i=0; i<68; ++i)
        m_os << '-';
    m_os << "\n\n";

    // We do two passes for EQU and each section; the first pass
    // determines the byte width to use for the value and whether any
    // symbols are present, the second pass actually outputs the text.

    // EQUs
    if (CountSymbols(m_object, 0) > 0)
    {
        m_os << "---- No Section ";
        for (int i=0; i<63; ++i)
            m_os << '-';
        m_os << "\n\n";
        m_os << format("%-*s", m_bytes*2+2, (const char*)"Value");
        m_os << "Name\n";
        OutputSymbols(0);
        m_os << "\n\n";
    }

    // Other sections
    InnerSectionsSymbols(m_groups);
}

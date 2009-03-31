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
#include "bin-map.h"

#include <iomanip>
#include <ostream>

#include <libyasmx/Bytecode.h>
#include <libyasmx/Expr.h>
#include <libyasmx/IntNum.h>
#include <libyasmx/Location.h>
#include <libyasmx/Section.h>
#include <libyasmx/Symbol.h>
#include <libyasmx/Object.h>

#include "bin-data.h"
#include "bin-link.h"


namespace yasm
{
namespace objfmt
{
namespace bin
{

static void
map_prescan_bytes(const Section& sect, const BinSectionData& bsd, int* bytes)
{
    while (!bsd.length.ok_size(*bytes * 8, 0, 0))
        *bytes *= 2;
    while (!sect.get_lma().ok_size(*bytes * 8, 0, 0))
        *bytes *= 2;
    while (!sect.get_vma().ok_size(*bytes * 8, 0, 0))
        *bytes *= 2;
}

MapOutput::MapOutput(std::ostream& os,
                     const Object& object,
                     const IntNum& origin,
                     const BinGroups& groups)
    : m_os(os),
      m_object(object),
      m_origin(origin),
      m_groups(groups)
{
    // Prescan all values to figure out what width we should make the output
    // fields.  Start with a minimum of 4.
    m_bytes = 4;
    while (!origin.ok_size(m_bytes * 8, 0, 0))
        m_bytes *= 2;
    for (Object::const_section_iterator sect = object.sections_begin(),
         end = object.sections_end(); sect != end; ++sect)
    {
        const BinSectionData* bsd = get_bin_sect(*sect);
        assert(bsd != 0);
        map_prescan_bytes(*sect, *bsd, &m_bytes);
    }

    m_buf = new unsigned char[m_bytes];
}

MapOutput::~MapOutput()
{
    delete[] m_buf;
}

void
MapOutput::output_intnum(const IntNum& intn)
{
    intn.get_sized(m_buf, m_bytes, m_bytes*8, 0, false, 0);
    m_os.fill('0');
    m_os.setf(std::ios::right, std::ios::adjustfield);
    m_os << std::hex;
    for (int i=m_bytes; i != 0; i--)
        m_os << std::setw(2) << static_cast<unsigned int>(m_buf[i-1]);
    m_os.fill(' ');
    m_os << std::dec;
}

void
MapOutput::output_header()
{
    m_os << "\n- YASM Map file ";
    m_os.fill('-');
    m_os << std::setw(63) << '-';
    m_os.fill(' ');
    m_os << "\n\nSource file:  " << m_object.get_source_fn() << '\n';
    m_os << "Output file:  " << m_object.get_object_fn() << "\n\n";
}

void
MapOutput::output_origin()
{
    m_os << "-- Program origin ";
    m_os.fill('-');
    m_os << std::setw(61) << '-';

    m_os.fill(' ');

    m_os << "\n\n";
    output_intnum(m_origin);
    m_os << "\n\n";
}

void
MapOutput::inner_sections_summary(const BinGroups& groups)
{
    for (BinGroups::const_iterator group = groups.begin(), end=groups.end();
         group != end; ++group)
    {
        const Section& sect = group->m_section;
        const BinSectionData& bsd = group->m_bsd;

        output_intnum(sect.get_vma());
        m_os << "  ";

        output_intnum(sect.get_vma() + bsd.length);
        m_os << "  ";

        output_intnum(sect.get_lma());
        m_os << "  ";

        output_intnum(sect.get_lma() + bsd.length);
        m_os << "  ";

        output_intnum(bsd.length);
        m_os << "  ";

        m_os.setf(std::ios::left, std::ios::adjustfield);
        m_os << std::setw(10)
             << (group->m_section.is_bss() ? "nobits" : "progbits");
        m_os << group->m_section.get_name() << '\n';

        // Recurse to loop through follow groups
        inner_sections_summary(group->m_follow_groups);
    }
}

void
MapOutput::output_sections_summary()
{
    m_os << "-- Sections (summary) ";
    m_os.fill('-');
    m_os << std::setw(57) << '-';

    m_os.fill(' ');
    m_os.setf(std::ios::left, std::ios::adjustfield);

    m_os << "\n\n";
    m_os << std::setw(m_bytes*2+2) << "Vstart";
    m_os << std::setw(m_bytes*2+2) << "Vstop";
    m_os << std::setw(m_bytes*2+2) << "Start";
    m_os << std::setw(m_bytes*2+2) << "Stop";
    m_os << std::setw(m_bytes*2+2) << "Length";
    m_os << std::setw(10) << "Class";
    m_os << "Name\n";
    inner_sections_summary(m_groups);
    m_os << '\n';
}

void
MapOutput::inner_sections_detail(const BinGroups& groups)
{
    for (BinGroups::const_iterator group = groups.begin(), end=groups.end();
         group != end; ++group)
    {
        const Section& sect = group->m_section;
        const std::string& name = sect.get_name();
        m_os << "---- Section " << name << " ";
        m_os.fill('-');
        m_os << std::setw(65-name.length()) << '-';
        m_os.fill(' ');

        const BinSectionData& bsd = group->m_bsd;

        m_os << "\n\nclass:     ";
        m_os << (group->m_section.is_bss() ? "nobits" : "progbits");
        m_os << "\nlength:    ";
        output_intnum(bsd.length);
        m_os << "\nstart:     ";
        output_intnum(sect.get_lma());
        m_os << "\nalign:     ";
        output_intnum(bsd.align);
        m_os << "\nfollows:   ";
        m_os << (!bsd.follows.empty() ? bsd.follows : "not defined");
        m_os << "\nvstart:    ";
        output_intnum(sect.get_vma());
        m_os << "\nvalign:    ";
        output_intnum(bsd.valign);
        m_os << "\nvfollows:  ";
        m_os << (!bsd.vfollows.empty() ? bsd.vfollows : "not defined");
        m_os << "\n\n";

        // Recurse to loop through follow groups
        inner_sections_detail(group->m_follow_groups);
    }
}

void
MapOutput::output_sections_detail()
{
    m_os << "-- Sections (detailed) ";
    m_os.fill('-');
    m_os << std::setw(56) << '-';

    m_os.fill(' ');

    m_os << "\n\n";
    inner_sections_detail(m_groups);
}

static unsigned long
count_symbols(const Object& object, const Section* sect)
{
    unsigned long count = 0;

    for (Object::const_symbol_iterator sym = object.symbols_begin(),
         end = object.symbols_end(); sym != end; ++sym)
    {
        Location loc;
        // TODO: autodetect wider size
        if ((sect == 0 && sym->get_equ()) ||
            (sym->get_label(&loc) && loc.bc->get_container() == sect))
            count++;
    }

    return count;
}

void
MapOutput::output_symbols(const Section* sect)
{
    for (Object::const_symbol_iterator sym = m_object.symbols_begin(),
         end = m_object.symbols_end(); sym != end; ++sym)
    {
        const std::string& name = sym->get_name();
        const Expr* equ;
        Location loc;

        if (sect == 0 && (equ = sym->get_equ()))
        {
            std::auto_ptr<Expr> realequ(equ->clone());
            realequ->simplify();
            bin_simplify(*realequ);
            realequ->simplify();
            IntNum* intn = realequ->get_intnum();
            assert(intn != 0);
            output_intnum(*intn);
            m_os << "  " << name << '\n';
        }
        else if (sym->get_label(&loc) && loc.bc->get_container() == sect)
        {
            // Real address
            output_intnum(sect->get_lma() + loc.get_offset());
            m_os << "  ";

            // Virtual address
            output_intnum(sect->get_vma() + loc.get_offset());

            // Name
            m_os << "  " << name << '\n';
        }
    }
}

void
MapOutput::inner_sections_symbols(const BinGroups& groups)
{
    for (BinGroups::const_iterator group = groups.begin(), end=groups.end();
         group != end; ++group)
    {
        if (count_symbols(m_object, &group->m_section) > 0)
        {
            const std::string& name = group->m_section.get_name();
            m_os << "---- Section " << name << ' ';
            m_os.fill('-');
            m_os << std::setw(65-name.length()) << '-';

            m_os.fill(' ');
            m_os.setf(std::ios::left, std::ios::adjustfield);

            m_os << "\n\n";
            m_os << std::setw(m_bytes*2+2) << "Real";
            m_os << std::setw(m_bytes*2+2) << "Virtual";
            m_os << "Name\n";
            output_symbols(&group->m_section);
            m_os << "\n\n";
        }

        // Recurse to loop through follow groups
        inner_sections_symbols(group->m_follow_groups);
    }
}

void
MapOutput::output_sections_symbols()
{
    m_os << "-- Symbols ";
    m_os.fill('-');
    m_os << std::setw(68) << '-';

    m_os.fill(' ');

    m_os << "\n\n";

    // We do two passes for EQU and each section; the first pass
    // determines the byte width to use for the value and whether any
    // symbols are present, the second pass actually outputs the text.

    // EQUs
    if (count_symbols(m_object, 0) > 0)
    {
        m_os << "---- No Section ";
        m_os.fill('-');
        m_os << std::setw(63) << '-';
        m_os.fill(' ');
        m_os << "\n\n";
        m_os << std::setw(m_bytes*2+2) << "Value";
        m_os << "Name\n";
        output_symbols(0);
        m_os << "\n\n";
    }

    // Other sections
    inner_sections_symbols(m_groups);
}

}}} // namespace yasm::objfmt::bin

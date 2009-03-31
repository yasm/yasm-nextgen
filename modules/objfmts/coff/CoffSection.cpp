//
// COFF object format section data implementation
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
#include "CoffSection.h"

#include <util.h>

#include <yasmx/Support/marg_ostream.h>
#include <yasmx/Bytes.h>
#include <yasmx/Bytes_util.h>
#include <yasmx/Compose.h>
#include <yasmx/Symbol.h>


namespace yasm
{
namespace objfmt
{
namespace coff
{

const char* CoffSection::key = "objfmts::coff::CoffSection";

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

}}} // namespace yasm::objfmt::coff

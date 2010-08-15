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

#include <cstring>

#include "util.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"
#include "YAML/emitter.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/Symbol.h"


using namespace yasm;
using namespace yasm::objfmt;

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
CoffSection::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << key;
    out << YAML::Key << "sym" << YAML::Value << m_sym;
    out << YAML::Key << "scnum" << YAML::Value << m_scnum;
    out << YAML::Key << "flags" << YAML::Value;
    switch (m_flags & STD_MASK)
    {
        case TEXT:
            out << "TEXT";
            break;
        case DATA:
            out << "DATA";
            break;
        case BSS:
            out << "BSS";
            break;
        default:
            out << "UNKNOWN";
            break;
    }
    out << YAML::Key << "flags value" << YAML::Value << YAML::Hex << m_flags;
    out << YAML::Key << "size" << YAML::Value << m_size;
    out << YAML::Key << "relptr" << YAML::Value << m_relptr;
    out << YAML::Key << "strtab name offset" << YAML::Value << m_strtab_name;
    out << YAML::Key << "no base" << YAML::Value << m_nobase;
    out << YAML::Key << "debug" << YAML::Value << m_isdebug;
    out << YAML::Key << "set align" << YAML::Value << m_setalign;
    out << YAML::EndMap;
}

void
CoffSection::Write(Bytes& bytes, const Section& sect) const
{
    bytes.setLittleEndian();

    // Check to see if alignment is supported size
    unsigned long align = sect.getAlign();
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
    llvm::StringRef fullname = sect.getName();
    char name[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    if (fullname.size() > 8)
    {
        llvm::SmallString<20> namenum;
        llvm::raw_svector_ostream os(namenum);
        os << '/' << m_strtab_name;
        std::strncpy(name, os.str().data(), 8);
    }
    else
        std::memcpy(name, fullname.data(), fullname.size());
    bytes.Write(reinterpret_cast<unsigned char*>(name), 8);
    if (m_isdebug)
    {
        Write32(bytes, 0);          // physical address
        Write32(bytes, 0);          // virtual address
    }
    else
    {
        Write32(bytes, sect.getLMA());      // physical address
        Write32(bytes, sect.getVMA());      // virtual address
    }
    Write32(bytes, m_size);                 // section size
    Write32(bytes, sect.getFilePos());      // file ptr to data
    Write32(bytes, m_relptr);               // file ptr to relocs
    Write32(bytes, 0);                      // file ptr to line nums
    if (sect.getRelocs().size() >= 64*1024)
        Write16(bytes, 0xFFFF);             // max out
    else
        Write16(bytes, sect.getRelocs().size()); // num of relocation entries
    Write16(bytes, 0);                      // num of line number entries
    Write32(bytes, flags);                  // flags
}

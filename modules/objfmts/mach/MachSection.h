#ifndef YASM_MACHSECTION_H
#define YASM_MACHSECTION_H
//
// Mach-O section
//
//  Copyright (C) 2007 Henryk Richter, built upon xdf objfmt (C) Peter Johnson
//  Copyright (C) 2010 Peter Johnson
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
#include "yasmx/Config/export.h"
#include "yasmx/AssocData.h"
#include "yasmx/Section.h"
#include "yasmx/SymbolRef.h"


namespace llvm { class MemoryBuffer; }

namespace yasm
{

class Bytes;
class IntNum;

namespace objfmt
{

struct YASM_STD_EXPORT MachSection : public AssocData
{
    static const char* key;

    MachSection(llvm::StringRef segname_, llvm::StringRef sectname_);
    ~MachSection();
#ifdef WITH_XML
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    void Write(Bytes& bytes, const Section& sect, int long_int_size) const;

    long scnum;                 // section number (0=first section)
    std::string segname;        // segment name in file
    std::string sectname;       // section name in file
    unsigned long flags;        // S_* flags
    unsigned long size;         // size of raw data (section data) in bytes
    bool extreloc;              // external relocations present
    unsigned long reloff;       // offset of relocations in file

    enum {
        SECTION_TYPE = 0x000000ffU,         // section type mask
        SECTION_ATTRIBUTES = 0xffffff00U,   // section attributes mask

        S_REGULAR                   = 0x0U, // standard section
        S_ZEROFILL                  = 0x1U, // zerofill, in-memory only
        S_CSTRING_LITERALS          = 0x2U, // literal C strings
        S_4BYTE_LITERALS            = 0x3U, // only 4-byte literals
        S_8BYTE_LITERALS            = 0x4U, // only 8-byte literals
        S_LITERAL_POINTERS          = 0x5U, // only pointers to literals
        S_NON_LAZY_SYMBOL_POINTERS  = 0x6U, // only non-lazy symbol pointers
        S_LAZY_SYMBOL_POINTERS      = 0x7U, // only lazy symbol pointers
        // only symbol stubs; byte size of stub in the reserved2 field
        S_SYMBOL_STUBS              = 0x8U,
        S_MOD_INIT_FUNC_POINTERS    = 0x9U, // only function pointers for init
        S_MOD_TERM_FUNC_POINTERS    = 0xaU, // only function pointers for term
        S_COALESCED                 = 0xbU, // symbols that are to be coalesced
        S_GB_ZEROFILL               = 0xcU, // >4GB zero fill on demand section
        // only pairs of function pointers for interposing
        S_INTERPOSING               = 0xdU,
        S_16BYTE_LITERALS           = 0xeU, // only 16 byte literals
        S_DTRACE_DOF                = 0xfU, // contains DTrace object
        S_LAZY_DYLIB_SYMBOL_POINTERS = 0x10U, // lazy symbol pointers to dylib

        //
        // system setable attributes
        //
        SECTION_ATTRIBUTES_SYS      = 0xffffU<<8,
        // section contains some machine instructions
        S_ATTR_SOME_INSTRUCTIONS    = 0x1U<<10,
        // section has external relocation entries
        S_ATTR_EXT_RELOC            = 0x1U<<9,
        // section has local relocation entries
        S_ATTR_LOC_RELOC            = 0x1U<<8,

        //
        // User setable attributes
        //
        SECTION_ATTRIBUTES_USR      = 0xffU<<24,
        // only true machine insns
        S_ATTR_PURE_INSTRUCTIONS    = 0x1U<<31,
        // coalesced symbols that are not to be in a ranlib table of contents
        S_ATTR_NO_TOC               = 0x1U<<30,
        // ok to strip static symbols in this section in files with the
        // MH_DYLDLINK flag
        S_ATTR_STRIP_STATIC_SYMS    = 0x1U<<29,
        // no dead stripping
        S_ATTR_NO_DEAD_STRIP        = 0x1U<<28,
        // blocks are live if they reference live blocks
        S_ATTR_LIVE_SUPPORT         = 0x1U<<27,
        // Used with i386 code stubs written on by dyld
        S_ATTR_SELF_MODIFYING_CODE  = 0x1U<<26,
        // a debug section
        S_ATTR_DEBUG                = 0x1U<<25
    };
};

}} // namespace yasm::objfmt

#endif

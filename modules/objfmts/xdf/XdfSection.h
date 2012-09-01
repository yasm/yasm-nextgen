#ifndef YASM_XDFSECTION_H
#define YASM_XDFSECTION_H
//
// Extended Dynamic Object format section
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
#include "yasmx/Config/export.h"
#include "yasmx/AssocData.h"
#include "yasmx/Section.h"
#include "yasmx/SymbolRef.h"


namespace yasm
{

class Bytes;
class IntNum;

namespace objfmt
{

struct YASM_STD_EXPORT XdfSection : public AssocData
{
    static const char* key;

    XdfSection(SymbolRef sym_);
    ~XdfSection();
#ifdef WITH_XML
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    void Write(Bytes& bytes, const Section& sect) const;
    void Read(const MemoryBuffer& in,
              unsigned long headpos,
              /*@out@*/ unsigned long* name_sym_index,
              /*@out@*/ IntNum* lma,
              /*@out@*/ IntNum* vma,
              /*@out@*/ unsigned long* align,
              /*@out@*/ bool* bss,
              /*@out@*/ unsigned long* filepos,
              /*@out@*/ unsigned long* nrelocs);

    // Section flags.
    // Not used directly in this class, but used in file representation.
    enum Flags
    {
        XDF_ABSOLUTE = 0x01,
        XDF_FLAT = 0x02,
        XDF_BSS = 0x04,
        XDF_EQU = 0x08,             // unused
        XDF_USE_16 = 0x10,
        XDF_USE_32 = 0x20,
        XDF_USE_64 = 0x40
    };

    SymbolRef sym;          //< symbol created for this section

    bool has_addr;          //< absolute address set by user?
    bool has_vaddr;         //< virtual address set by user?

    long scnum;             //< section number (0=first section)

    bool flat;              //< specified by user as "flat" section
    unsigned long bits;     //< "bits" (aka use16/use32/use64) of section

    unsigned long size;     //< size of raw data (section data) in bytes
    unsigned long relptr;   //< file ptr to relocation
};

}} // namespace yasm::objfmt

#endif

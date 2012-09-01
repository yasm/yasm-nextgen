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
#include "XdfSection.h"

#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/InputBuffer.h"

#include "XdfSymbol.h"


using namespace yasm;
using namespace yasm::objfmt;

const char* XdfSection::key = "objfmt::xdf::XdfSection";

XdfSection::XdfSection(SymbolRef sym_)
    : sym(sym_)
    , has_addr(false)
    , has_vaddr(false)
    , scnum(0)
    , flat(false)
    , bits(0)
    , size(0)
    , relptr(0)
{
}

XdfSection::~XdfSection()
{
}

#ifdef WITH_XML
pugi::xml_node
XdfSection::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("XdfSection");
    root.append_attribute("key") = key;
    append_child(root, "Sym", sym);
    if (has_addr)
        root.append_attribute("has_addr") = true;
    if (has_vaddr)
        root.append_attribute("has_vaddr") = true;
    append_child(root, "ScNum", scnum);
    append_child(root, "Flat", flat);
    append_child(root, "Bits", bits);
    append_child(root, "Size", size);
    append_child(root, "RelPtr", relptr);
    return root;
}
#endif // WITH_XML

void
XdfSection::Write(Bytes& bytes, const Section& sect) const
{
    const XdfSymbol* xsym = sym->getAssocData<XdfSymbol>();
    assert(xsym != 0);

    bytes.setLittleEndian();

    Write32(bytes, xsym->index);        // section name symbol
    Write64(bytes, sect.getLMA());      // physical address

    if (has_vaddr)
        Write64(bytes, sect.getVMA());  // virtual address
    else
        Write64(bytes, sect.getLMA());  // virt=phys if unspecified

    Write16(bytes, sect.getAlign());    // alignment

    // flags
    unsigned short flags = 0;
    if (has_addr)
        flags |= XDF_ABSOLUTE;
    if (flat)
        flags |= XDF_FLAT;
    if (sect.isBSS())
        flags |= XDF_BSS;
    switch (bits)
    {
        case 16: flags |= XDF_USE_16; break;
        case 32: flags |= XDF_USE_32; break;
        case 64: flags |= XDF_USE_64; break;
    }
    Write16(bytes, flags);

    Write32(bytes, sect.getFilePos());  // file ptr to data
    Write32(bytes, size);               // section size
    Write32(bytes, relptr);             // file ptr to relocs
    Write32(bytes, sect.getRelocs().size());// num of relocation entries
}

void
XdfSection::Read(const MemoryBuffer& in,
                 unsigned long headpos,
                 /*@out@*/ unsigned long* name_sym_index,
                 /*@out@*/ IntNum* lma,
                 /*@out@*/ IntNum* vma,
                 /*@out@*/ unsigned long* align,
                 /*@out@*/ bool* bss,
                 /*@out@*/ unsigned long* filepos,
                 /*@out@*/ unsigned long* nrelocs)
{
    InputBuffer inbuf(in, headpos);
    inbuf.setLittleEndian();

    *name_sym_index = ReadU32(inbuf);   // section name symbol index
    *lma = ReadU64(inbuf);              // physical address
    *vma = ReadU64(inbuf);              // virtual address
    has_vaddr = true;                   // virtual specified by object file
    *align = ReadU16(inbuf);            // alignment

    // flags
    unsigned short flags = ReadU16(inbuf);
    has_addr = (flags & XDF_ABSOLUTE) != 0;
    flat = (flags & XDF_FLAT) != 0;
    *bss = (flags & XDF_BSS) != 0;
    if ((flags & XDF_USE_16) != 0)
        bits = 16;
    else if ((flags & XDF_USE_32) != 0)
        bits = 32;
    else if ((flags & XDF_USE_64) != 0)
        bits = 64;

    *filepos = ReadU32(inbuf);          // file ptr to data
    size = ReadU32(inbuf);              // section size
    relptr = ReadU32(inbuf);            // file ptr to relocs
    *nrelocs = ReadU32(inbuf);          // num of relocation entries
}

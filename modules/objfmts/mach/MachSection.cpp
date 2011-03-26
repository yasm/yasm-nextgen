//
// Mach-O section
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
#include "MachSection.h"

#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/InputBuffer.h"


using namespace yasm;
using namespace yasm::objfmt;

const char* MachSection::key = "objfmt::MachSection";

MachSection::MachSection(llvm::StringRef segname_, llvm::StringRef sectname_)
    : scnum(0)
    , segname(segname_)
    , sectname(sectname_)
    , flags(0)
    , size(0)
    , extreloc(false)
    , reloff(0)
{
}

MachSection::~MachSection()
{
}

#ifdef WITH_XML
pugi::xml_node
MachSection::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("MachSection");
    root.append_attribute("key") = key;
    append_child(root, "ScNum", scnum);
    append_child(root, "SegName", segname);
    append_child(root, "SectName", sectname);
    append_child(root, "Flags", flags);
    append_child(root, "Size", size);
    append_child(root, "ExtReloc", extreloc);
    return root;
}
#endif // WITH_XML

static int
exp2_to_bits(unsigned long val)
{
    int ret = 0;

    while (val)
    {
        val >>= 1;
        ret++;
    }
    ret = (ret > 0) ? ret - 1 : 0;

    return ret;
}

void
MachSection::Write(Bytes& bytes, const Section& sect, int long_int_size) const
{
    // section and segname names, padded/truncated to 16 bytes
    char namebuf[16];
    std::strncpy(namebuf, sectname.c_str(), 16);
    bytes.Write((unsigned char*)namebuf, 16);
    std::strncpy(namebuf, segname.c_str(), 16);
    bytes.Write((unsigned char*)namebuf, 16);

    // section address, size depend on 32/64 bit mode
    WriteN(bytes, sect.getVMA(), long_int_size);    // address in memory
    WriteN(bytes, size, long_int_size);             // size in memory

    // offset,align,reloff,nreloc,flags,reserved1,reserved2 are 32 bit
    unsigned long write_flags = flags;
    if ((flags & SECTION_TYPE) != S_ZEROFILL)
    {
        Write32(bytes, sect.getFilePos());
        Write32(bytes, exp2_to_bits(sect.getAlign()));
        size_t nreloc = sect.getRelocs().size();
        if (nreloc != 0)
        {
            write_flags |= S_ATTR_LOC_RELOC;
            if (extreloc)
                write_flags |= S_ATTR_EXT_RELOC;
            Write32(bytes, reloff);     // offset in file
            Write32(bytes, nreloc);     // nreloc
        }
        else
        {
            Write32(bytes, 0);
            Write32(bytes, 0);
        }
    }
    else
    {
        Write32(bytes, 0);      // these are zero in BSS
        Write32(bytes, 0);
        Write32(bytes, 0);
        Write32(bytes, 0);
    }

    Write32(bytes, write_flags);        // flags
    Write32(bytes, 0);                  // reserved 1
    WriteN(bytes, 0, long_int_size);    // reserved 2/3
}

#ifndef YASM_MACHRELOC_H
#define YASM_MACHRELOC_H
//
// Mach-O relocation
//
//  Copyright (C) 2007 Henryk Richter, built upon xdf objfmt (C) Peter Johnson
//  Copyright (C) 2012 Peter Johnson
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
#include <string>

#include "yasmx/Config/export.h"
#include "yasmx/Reloc.h"
#include "yasmx/SymbolRef.h"


namespace yasm
{

class Bytes;
class IntNum;
class Value;

namespace objfmt
{

class YASM_STD_EXPORT MachReloc : public Reloc
{
public:
    enum Type
    {
        // x86 relocations
        GENERIC_RELOC_VANILLA = 0,      // generic relocation
        GENERIC_RELOC_PAIR = 1,         // Only follows a GENERIC_RELOC_SECTDIFF
        GENERIC_RELOC_SECTDIFF = 2,
        GENERIC_RELOC_PB_LA_PTR = 3,    // prebound lazy pointer
        GENERIC_RELOC_LOCAL_SECTDIFF = 4,

        // x86-64 relocations
        X86_64_RELOC_UNSIGNED = 0,      // for absolute addresses
        X86_64_RELOC_SIGNED = 1,        // for signed 32-bit displacement
        X86_64_RELOC_BRANCH = 2,        // a CALL/JMP insn with 32-bit disp
        X86_64_RELOC_GOT_LOAD = 3,      // a MOVQ load of a GOT entry
        X86_64_RELOC_GOT = 4,           // other GOT references
        X86_64_RELOC_SUBTRACTOR = 5,    // must be followed by a X86_64_RELOC_UNSIGNED
        X86_64_RELOC_SIGNED_1 = 6,      // signed 32-bit disp, -1 addend
        X86_64_RELOC_SIGNED_2 = 7,      // signed 32-bit disp, -2 addend
        X86_64_RELOC_SIGNED_4 = 8       // signed 32-bit disp, -4 addend
    };

    MachReloc(const IntNum& addr,
              SymbolRef sym,
              Type type,
              bool pcrel,
              unsigned int length,
              bool ext);
    ~MachReloc();

    virtual std::string getTypeName() const = 0;

    Type getType() const { return m_type; }

    void Write(Bytes& bytes) const;

protected:
#ifdef WITH_XML
    pugi::xml_node DoWrite(pugi::xml_node out) const;
#endif // WITH_XML

    bool m_pcrel;
    unsigned int m_length;
    bool m_ext;
    Type m_type;
};

class Mach32Reloc : public MachReloc
{
public:
    Mach32Reloc(const IntNum& addr,
                SymbolRef sym,
                Type type,
                bool pcrel,
                unsigned int length,
                bool ext)
        : MachReloc(addr, sym, type, pcrel, length, ext)
    {}
    virtual ~Mach32Reloc();

    virtual std::string getTypeName() const;
};

class Mach64Reloc : public MachReloc
{
public:
    Mach64Reloc(const IntNum& addr,
                SymbolRef sym,
                Type type,
                bool pcrel,
                unsigned int length,
                bool ext)
        : MachReloc(addr, sym, type, pcrel, length, ext)
    {}
    virtual ~Mach64Reloc();

    virtual std::string getTypeName() const;
};

}} // namespace yasm::objfmt

#endif

#ifndef YASM_COFFRELOC_H
#define YASM_COFFRELOC_H
//
// COFF object format relocation
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
#include <string>

#include <libyasmx/Expr.h>
#include <libyasmx/Reloc.h>
#include <libyasmx/SymbolRef.h>

namespace yasm
{

class Bytes;
class IntNum;

namespace objfmt
{
namespace coff
{

class CoffReloc : public Reloc
{
public:
    enum Type
    {
        ABSOLUTE = 0,           ///< absolute, no reloc needed

        // I386 relocations
        I386_ADDR16 = 0x1,      ///< 16-bit absolute reference
        I386_REL16 = 0x2,       ///< 16-bit PC-relative reference
        I386_ADDR32 = 0x6,      ///< 32-bit absolute reference
        I386_ADDR32NB = 0x7,    ///< 32-bit absolute ref w/o base
        I386_SEG12 = 0x9,       ///< 16-bit absolute segment ref
        I386_SECTION = 0xA,     ///< section index
        I386_SECREL = 0xB,      ///< offset from start of segment
        I386_TOKEN = 0xC,       ///< CLR metadata token
        I386_SECREL7 = 0xD,     ///< 7-bit offset from base of sect
        I386_REL32 = 0x14,      ///< 32-bit PC-relative reference

        // AMD64 relocations
        AMD64_ADDR64 = 0x1,     ///< 64-bit address (VA)
        AMD64_ADDR32 = 0x2,     ///< 32-bit address (VA)
        AMD64_ADDR32NB = 0x3,   ///< 32-bit address w/o base (RVA)
        AMD64_REL32 = 0x4,      ///< 32-bit relative (0 byte dist)
        AMD64_REL32_1 = 0x5,    ///< 32-bit relative (1 byte dist)
        AMD64_REL32_2 = 0x6,    ///< 32-bit relative (2 byte dist)
        AMD64_REL32_3 = 0x7,    ///< 32-bit relative (3 byte dist)
        AMD64_REL32_4 = 0x8,    ///< 32-bit relative (4 byte dist)
        AMD64_REL32_5 = 0x9,    ///< 32-bit relative (5 byte dist)
        AMD64_SECTION = 0xA,    ///< section index
        AMD64_SECREL = 0xB,     ///< 32-bit offset from base of sect
        AMD64_SECREL7 = 0xC,    ///< 7-bit offset from base of sect
        AMD64_TOKEN = 0xD       ///< CLR metadata token
    };

    CoffReloc(const IntNum& addr, SymbolRef sym, Type type);
    virtual ~CoffReloc();

    virtual Expr get_value() const;
    virtual std::string get_type_name() const = 0;

    void write(Bytes& bytes) const;

protected:
    Type m_type;    ///< type of relocation
};

class Coff32Reloc : public CoffReloc
{
public:
    Coff32Reloc(const IntNum& addr, SymbolRef sym, Type type)
        : CoffReloc(addr, sym, type)
    {}
    virtual ~Coff32Reloc();

    virtual std::string get_type_name() const;
};

class Coff64Reloc : public CoffReloc
{
public:
    Coff64Reloc(const IntNum& addr, SymbolRef sym, Type type)
        : CoffReloc(addr, sym, type)
    {}
    virtual ~Coff64Reloc();

    virtual std::string get_type_name() const;
};

}}} // namespace yasm::objfmt::coff

#endif

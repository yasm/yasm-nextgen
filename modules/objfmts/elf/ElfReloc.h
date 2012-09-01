#ifndef YASM_ELFRELOC_H
#define YASM_ELFRELOC_H
//
// ELF object format relocation
//
//  Copyright (C) 2003-2007  Michael Urman
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
#include "yasmx/IntNum.h"
#include "yasmx/Reloc.h"
#include "yasmx/SymbolRef.h"

#include "ElfTypes.h"


namespace yasm
{

class Bytes;
class Expr;

namespace objfmt
{

class YASM_STD_EXPORT ElfReloc : public Reloc
{
public:
    // Constructor that reads from memory buffer.
    ElfReloc(const ElfConfig& config,
             const ElfSymtab& symtab,
             const MemoryBuffer& in,
             unsigned long* pos,
             bool rela);
    ElfReloc(SymbolRef sym, const IntNum& addr);
    virtual ~ElfReloc();

    /// Set relocation type for relative symbols (typical case).
    /// @param rel      PC-relative?
    /// @param GOT_sym  _GLOBAL_OFFSET_TABLE_ symbol
    /// @param valsize  relocation size (in bits)
    /// @param sign     signed?
    /// @return False if unable to determine relocation type.
    virtual bool setRel(bool rel,
                        SymbolRef GOT_sym,
                        size_t valsize,
                        bool sign) = 0;

    /// Set relocation type via WRT (special symbol) mechanism.
    /// @param valsize  relocation size (in bits)
    /// @return False if invalid wrt.
    bool setWrt(SymbolRef wrt, size_t valsize);

    bool isValid() const { return m_type != 0xff; }

    Expr getValue() const;
    virtual std::string getTypeName() const = 0;

    virtual void HandleAddend(IntNum* intn,
                              const ElfConfig& config,
                              unsigned int insn_start);
    void Write(Bytes& bytes, const ElfConfig& config);

protected:
    SymbolRef           m_wrt;
    ElfRelocationType   m_type;
    IntNum              m_addend;

#ifdef WITH_XML
    virtual pugi::xml_node DoWrite(pugi::xml_node out) const;
#endif // WITH_XML
};

}} // namespace yasm::objfmt

#endif

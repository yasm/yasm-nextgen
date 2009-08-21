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

#include "yasmx/IntNum.h"
#include "yasmx/Reloc.h"
#include "yasmx/SymbolRef.h"

#include "ElfTypes.h"


namespace llvm { class MemoryBuffer; }

namespace yasm
{

class Bytes;
class Expr;

namespace objfmt
{
namespace elf
{

class ElfReloc : public Reloc
{
public:
    // Constructor that reads from memory buffer.
    ElfReloc(const ElfConfig& config,
             const ElfSymtab& symtab,
             const llvm::MemoryBuffer& in,
             unsigned long* pos,
             bool rela);
    ElfReloc(SymbolRef sym, SymbolRef wrt, const IntNum& addr, size_t valsize);
    virtual ~ElfReloc();

    Expr getValue() const;
    virtual std::string getTypeName() const = 0;

    virtual void HandleAddend(IntNum* intn,
                              const ElfConfig& config,
                              unsigned int insn_start);
    void Write(Bytes& bytes, const ElfConfig& config);

protected:
    ElfRelocationType   m_type;
    IntNum              m_addend;

    virtual void DoWrite(YAML::Emitter& out) const;
};

}}} // namespace yasm::objfmt::elf

#endif

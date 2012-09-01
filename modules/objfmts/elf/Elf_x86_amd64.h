#ifndef YASM_ELF_X86_AMD64_H
#define YASM_ELF_X86_AMD64_H
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
#include "ElfReloc.h"


namespace yasm
{
namespace objfmt
{

class YASM_STD_EXPORT ElfReloc_x86_amd64 : public ElfReloc
{
public:
    ElfReloc_x86_amd64(const ElfConfig& config,
                       const ElfSymtab& symtab,
                       const MemoryBuffer& in,
                       unsigned long* pos,
                       bool rela)
        : ElfReloc(config, symtab, in, pos, rela)
    {}
    ElfReloc_x86_amd64(SymbolRef sym, const IntNum& addr)
        : ElfReloc(sym, addr)
    {}

    virtual ~ElfReloc_x86_amd64() {}

    virtual bool setRel(bool rel, SymbolRef GOT_sym, size_t valsize, bool sign);
    virtual std::string getTypeName() const;
};

}} // namespace yasm::objfmt

#endif

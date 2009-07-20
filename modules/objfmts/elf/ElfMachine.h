#ifndef ELF_MACHINE_H_INCLUDED
#define ELF_MACHINE_H_INCLUDED
//
// ELF object machine specific format helpers
//
//  Copyright (C) 2004-2007  Michael Urman
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
#include <memory>
#include <string>

#include <yasmx/AssocData.h>
#include <yasmx/Symbol.h>

#include "ElfTypes.h"


namespace yasm
{

class Arch;
class IntNum;
class Object;

namespace objfmt
{
namespace elf
{

struct SpecialSymbolData
{
    const char* name;   // name without parser prefix (e.g. name for ..name)

    unsigned int reloc;     // relocation type
    unsigned int size;      // legal data size

    // flags
    bool sym_relative:1;    // symbol relative (instead of section relative)
    bool thread_local:1;    // thread local
    bool curpos_adjust:1;   //
};

struct ElfSpecialSymbol : public AssocData, public SpecialSymbolData
{
    static const char* key;

    explicit ElfSpecialSymbol(const SpecialSymbolData& rhs)
        : SpecialSymbolData(rhs)
    {}
    ~ElfSpecialSymbol();

    void Write(YAML::Emitter& out) const;
};

inline ElfSpecialSymbol*
getElfSSym(Symbol& sym)
{
    return static_cast<ElfSpecialSymbol*>
        (sym.getAssocData(ElfSpecialSymbol::key));
}

inline const ElfSpecialSymbol*
getElfSSym(const Symbol& sym)
{
    return static_cast<const ElfSpecialSymbol*>
        (sym.getAssocData(ElfSpecialSymbol::key));
}

inline bool
isWRTSymRelative(const Symbol& wrt)
{
    const ElfSpecialSymbol* ssym = getElfSSym(wrt);
    if (!ssym)
        return false;
    return ssym->sym_relative;
}

inline bool
isWRTPosAdjusted(const Symbol& wrt)
{
    const ElfSpecialSymbol* ssym = getElfSSym(wrt);
    if (!ssym)
        return false;
    return ssym->curpos_adjust;
}

class ElfMachine
{
public:
    virtual ~ElfMachine();

    virtual void Configure(/*@out@*/ ElfConfig* config) const = 0;
    virtual void AddSpecialSymbols(Object& object,
                                   const std::string& parser) const = 0;

    virtual std::auto_ptr<ElfReloc>
        ReadReloc(const ElfConfig& config,
                  const ElfSymtab& symtab,
                  std::istream& is,
                  bool rela) const = 0;

    virtual std::auto_ptr<ElfReloc>
        MakeReloc(SymbolRef sym,
                  SymbolRef wrt,
                  const IntNum& addr,
                  bool rel,
                  size_t valsize) const = 0;
};

bool isOkElfMachine(const Arch& arch, ElfClass cls);
std::auto_ptr<ElfMachine> CreateElfMachine(const Arch& arch, ElfClass cls);

void AddElfSSym(Object& object, const SpecialSymbolData& ssym);

}}} // namespace yasm::objfmt::elf

#endif // ELF_MACHINE_H_INCLUDED

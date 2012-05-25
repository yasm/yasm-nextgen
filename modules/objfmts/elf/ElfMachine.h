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

#include "yasmx/Config/export.h"
#include "yasmx/AssocData.h"
#include "yasmx/Symbol.h"

#include "ElfTypes.h"


namespace llvm { class MemoryBuffer; }

namespace yasm
{

class Arch;
class IntNum;
class Object;

namespace objfmt
{

struct ElfSpecialSymbolData
{
    const char* name;   // name without parser prefix (e.g. name for ..name)

    unsigned int reloc;     // relocation type
    unsigned int size;      // legal data size

    // flags
    bool sym_relative:1;    // symbol relative (instead of section relative)
    bool thread_local:1;    // thread local
    bool curpos_adjust:1;   //
    bool needs_got:1;       // needs GOT symbol in symbol table
};

struct YASM_STD_EXPORT ElfSpecialSymbol
    : public AssocData, public ElfSpecialSymbolData
{
    static const char* key;

    explicit ElfSpecialSymbol(const ElfSpecialSymbolData& rhs)
        : ElfSpecialSymbolData(rhs)
    {}
    ~ElfSpecialSymbol();

#ifdef WITH_XML
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML
};

inline bool
isWRTElfSymRelative(const Symbol& wrt)
{
    const ElfSpecialSymbol* ssym = wrt.getAssocData<ElfSpecialSymbol>();
    if (!ssym)
        return false;
    return ssym->sym_relative;
}

inline bool
isWRTElfPosAdjusted(const Symbol& wrt)
{
    const ElfSpecialSymbol* ssym = wrt.getAssocData<ElfSpecialSymbol>();
    if (!ssym)
        return false;
    return ssym->curpos_adjust;
}

inline bool
isWRTElfNeedsGOT(const Symbol& wrt)
{
    const ElfSpecialSymbol* ssym = wrt.getAssocData<ElfSpecialSymbol>();
    if (!ssym)
        return false;
    return ssym->needs_got;
}

class YASM_STD_EXPORT ElfMachine
{
public:
    virtual ~ElfMachine();

    virtual void Configure(/*@out@*/ ElfConfig* config) const = 0;
    virtual void AddSpecialSymbols(Object& object,
                                   llvm::StringRef parser) const = 0;

    virtual std::auto_ptr<ElfReloc>
        ReadReloc(const ElfConfig& config,
                  const ElfSymtab& symtab,
                  const llvm::MemoryBuffer& in,
                  unsigned long* pos,
                  bool rela) const = 0;

    virtual std::auto_ptr<ElfReloc>
        MakeReloc(SymbolRef sym, const IntNum& addr) const = 0;
};

YASM_STD_EXPORT
bool isOkElfMachine(const Arch& arch, ElfClass cls);

YASM_STD_EXPORT
std::auto_ptr<ElfMachine> CreateElfMachine(const Arch& arch, ElfClass cls);

YASM_STD_EXPORT
void AddElfSSym(Object& object, const ElfSpecialSymbolData& ssym);

namespace impl {
bool ElfMatch_x86_x86(llvm::StringRef arch_keyword,
                      llvm::StringRef arch_machine,
                      ElfClass cls);
bool ElfMatch_x86_amd64(llvm::StringRef arch_keyword,
                        llvm::StringRef arch_machine,
                        ElfClass cls);

std::auto_ptr<ElfMachine> ElfCreate_x86_x86();
std::auto_ptr<ElfMachine> ElfCreate_x86_amd64();
} // namespace impl

}} // namespace yasm::objfmt

#endif // ELF_MACHINE_H_INCLUDED

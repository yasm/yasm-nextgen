//
// ELF machine frontend
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
#include "elf-machine.h"

#include <util.h>

#include <string>

#include <libyasmx/arch.h>
#include <libyasmx/errwarn.h>
#include <libyasmx/marg_ostream.h>
#include <libyasmx/object.h>


namespace yasm
{
namespace objfmt
{
namespace elf
{

extern bool elf_x86_x86_match(const std::string& arch_keyword,
                              const std::string& arch_machine,
                              ElfClass cls);
extern bool elf_x86_amd64_match(const std::string& arch_keyword,
                                const std::string& arch_machine,
                                ElfClass cls);

extern std::auto_ptr<ElfMachine> elf_x86_x86_create();
extern std::auto_ptr<ElfMachine> elf_x86_amd64_create();

struct MachineCheckCreate
{
    bool (*match) (const std::string& arch_keyword,
                   const std::string& arch_machine,
                   ElfClass cls);
    std::auto_ptr<ElfMachine> (*create) ();
};

static const MachineCheckCreate machines[] =
{
    {&elf_x86_x86_match, &elf_x86_x86_create},
    {&elf_x86_amd64_match, &elf_x86_amd64_create},
};

const char* ElfSpecialSymbol::key = "objfmt::elf::ElfSpecialSymbol";

ElfSpecialSymbol::~ElfSpecialSymbol()
{
}

void
ElfSpecialSymbol::put(marg_ostream& os) const
{
    // TODO
}

ElfMachine::~ElfMachine()
{
}

bool
ok_elf_machine(const Arch& arch, ElfClass cls)
{
    std::string keyword = arch.get_keyword();
    std::string machine = arch.get_machine();
    for (unsigned int i=0; i<NELEMS(machines); ++i)
    {
        if (machines[i].match(keyword, machine, cls))
            return true;
    }
    return false;
}

std::auto_ptr<ElfMachine>
create_elf_machine(const Arch& arch, ElfClass cls)
{
    std::string keyword = arch.get_keyword();
    std::string machine = arch.get_machine();
    for (unsigned int i=0; i<NELEMS(machines); ++i)
    {
        if (machines[i].match(keyword, machine, cls))
            return machines[i].create();
    }
    assert(false && "could not find elf machine");
}

void
add_ssym(Object& object, const SpecialSymbolData& ssym)
{
    SymbolRef sym = object.add_special_symbol(ssym.name);
    sym->define_special(Symbol::EXTERN);
    sym->add_assoc_data(ElfSpecialSymbol::key,
        std::auto_ptr<AssocData>(new ElfSpecialSymbol(ssym)));
}

}}} // namespace yasm::objfmt::elf

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
#include "ElfMachine.h"

#include <string>

#include "yasmx/Arch.h"
#include "yasmx/Object.h"


using namespace yasm;
using namespace yasm::objfmt;

namespace {
struct MachineCheckCreate
{
    bool (*Match) (StringRef arch_keyword,
                   StringRef arch_machine,
                   ElfClass cls);
    std::auto_ptr<ElfMachine> (*Create) ();
};
} // anonymous namespace

static const MachineCheckCreate machines[] =
{
    {&impl::ElfMatch_x86_x86, &impl::ElfCreate_x86_x86},
    {&impl::ElfMatch_x86_amd64, &impl::ElfCreate_x86_amd64},
    {&impl::ElfMatch_x86_x32, &impl::ElfCreate_x86_x32},
};
static const size_t nmachines = sizeof(machines)/sizeof(machines[0]);

const char* ElfSpecialSymbol::key = "objfmt::elf::ElfSpecialSymbol";

ElfSpecialSymbol::~ElfSpecialSymbol()
{
}

#ifdef WITH_XML
pugi::xml_node
ElfSpecialSymbol::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("ElfSpecialSymbol");
    root.append_attribute("type") = key;
    append_child(root, "Name", name);
    append_child(root, "RelocType", reloc);
    append_child(root, "DataSize", size);
    append_child(root, "SymRelative", sym_relative);
    append_child(root, "ThreadLocal", thread_local);
    append_child(root, "CurposAdjust", curpos_adjust);
    return root;
}
#endif // WITH_XML

ElfMachine::~ElfMachine()
{
}

bool
objfmt::isOkElfMachine(const Arch& arch, ElfClass cls)
{
    std::string keyword = arch.getModule().getKeyword();
    std::string machine = arch.getMachine();
    for (size_t i=0; i<nmachines; ++i)
    {
        if (machines[i].Match(keyword, machine, cls))
            return true;
    }
    return false;
}

std::auto_ptr<ElfMachine>
objfmt::CreateElfMachine(const Arch& arch, ElfClass cls)
{
    std::string keyword = arch.getModule().getKeyword();
    std::string machine = arch.getMachine();
    for (size_t i=0; i<nmachines; ++i)
    {
        if (machines[i].Match(keyword, machine, cls))
            return machines[i].Create();
    }
    assert(false && "could not find elf machine");
    return machines[0].Create();
}

void
objfmt::AddElfSSym(Object& object, const ElfSpecialSymbolData& ssym)
{
    SymbolRef sym = object.AddSpecialSymbol(ssym.name);
    sym->DefineSpecial(Symbol::EXTERN);
    sym->AddAssocData(
        std::auto_ptr<ElfSpecialSymbol>(new ElfSpecialSymbol(ssym)));
}

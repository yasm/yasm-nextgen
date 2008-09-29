//
// ELF object format helpers - x86:x86
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
#include <util.h>

#include <libyasmx/errwarn.h>
#include <libyasmx/nocase.h>

#include "elf.h"
#include "elf-machine.h"


namespace yasm
{
namespace objfmt
{
namespace elf
{

class Elf_x86_x86 : public ElfMachine
{
public:
    ~Elf_x86_x86() {}

    void configure(ElfConfig* config) const;
    void add_special_syms(Object& object, const std::string& parser) const;

    bool map_reloc_type(ElfRelocationType* type,
                        bool rel,
                        size_t valsize) const;
};

bool
elf_x86_x86_match(const std::string& arch_keyword,
                  const std::string& arch_machine,
                  ElfClass cls)
{
    return (String::nocase_equal(arch_keyword, "x86") &&
            String::nocase_equal(arch_machine, "x86") &&
            (cls == ELFCLASSNONE || cls == ELFCLASS32));
}

std::auto_ptr<ElfMachine>
elf_x86_x86_create()
{
    return std::auto_ptr<ElfMachine>(new Elf_x86_x86);
}

void
Elf_x86_x86::configure(ElfConfig* config) const
{
    config->cls = ELFCLASS32;
    config->encoding = ELFDATA2LSB;
    config->osabi = ELFOSABI_SYSV;
    config->abi_version = 0;
    config->machine_type = EM_386;
    config->rela = false;
}

void
Elf_x86_x86::add_special_syms(Object& object, const std::string& parser) const
{
    static const SpecialSymbolData ssyms[] =
    {
        //name,         type,             size,symrel,thread,curpos
        {"plt",         R_386_PLT32,        32,  true, false, false},
        {"gotoff",      R_386_GOTOFF,       32, false, false, false},
        // special one for NASM
        {"gotpc",       R_386_GOTPC,        32, false, false,  true},
        {"tlsgd",       R_386_TLS_GD,       32,  true,  true, false},
        {"tlsldm",      R_386_TLS_LDM,      32,  true,  true, false},
        {"gottpoff",    R_386_TLS_IE_32,    32,  true,  true, false},
        {"tpoff",       R_386_TLS_LE_32,    32,  true,  true, false},
        {"ntpoff",      R_386_TLS_LE,       32,  true,  true, false},
        {"dtpoff",      R_386_TLS_LDO_32,   32,  true,  true, false},
        {"gotntpoff",   R_386_TLS_GOTIE,    32,  true,  true, false},
        {"indntpoff",   R_386_TLS_IE,       32,  true,  true, false},
        {"got",         R_386_GOT32,        32,  true, false, false}
    };

    for (unsigned int i=0; i<NELEMS(ssyms); ++i)
        add_ssym(object, ssyms[i]);
}

bool
Elf_x86_x86::map_reloc_type(ElfRelocationType* type,
                            bool rel,
                            size_t valsize) const
{
    if (rel)
    {
        switch (valsize)
        {
            case 8: *type = R_386_PC8;
            case 16: *type = R_386_PC16;
            case 32: *type = R_386_PC32;
            default: return false;
        }
    }
    else
    {
        switch (valsize)
        {
            case 8: *type = R_386_8;
            case 16: *type = R_386_16;
            case 32: *type = R_386_32;
            default: return false;
        }
    }
    return true;
}

}}} // namespace yasm::objfmt::elf

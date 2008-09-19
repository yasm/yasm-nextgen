//
// ELF object format helpers - x86:amd64
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

class Elf_x86_amd64 : public ElfMachine
{
public:
    ~Elf_x86_amd64() {}

    void configure(ElfConfig* config) const;
    void add_special_syms(Object& object, const std::string& parser) const;

    bool accepts_reloc(size_t val) const;
    void handle_reloc_addend(IntNum* intn, ElfReloc* reloc) const;
    unsigned int map_reloc_info_to_type(const ElfReloc& reloc) const;
};

bool
elf_x86_amd64_match(const std::string& arch_keyword,
                    const std::string& arch_machine,
                    ElfClass cls)
{
    return (String::nocase_equal(arch_keyword, "x86") &&
            String::nocase_equal(arch_machine, "amd64") &&
            (cls == ELFCLASSNONE || cls == ELFCLASS64));
}

std::auto_ptr<ElfMachine>
elf_x86_amd64_create()
{
    return std::auto_ptr<ElfMachine>(new Elf_x86_amd64);
}

void
Elf_x86_amd64::configure(ElfConfig* config) const
{
    config->cls = ELFCLASS64;
    config->encoding = ELFDATA2LSB;
    config->osabi = ELFOSABI_SYSV;
    config->abi_version = 0;
    config->machine_type = EM_X86_64;
    config->rela = true;
}

void
Elf_x86_amd64::add_special_syms(Object& object,
                                const std::string& parser) const
{
    static const SpecialSymbolData ssyms[] =
    {
        //name,         type,             size,symrel,thread,curpos
        {"plt",         R_X86_64_PLT32,     32,  true, false, false},
        {"gotpcrel",    R_X86_64_GOTPCREL,  32,  true, false, false},
        {"tlsgd",       R_X86_64_TLSGD,     32,  true,  true, false},
        {"tlsld",       R_X86_64_TLSLD,     32,  true,  true, false},
        {"gottpoff",    R_X86_64_GOTTPOFF,  32,  true,  true, false},
        {"tpoff",       R_X86_64_TPOFF32,   32,  true,  true, false},
        {"dtpoff",      R_X86_64_DTPOFF32,  32,  true,  true, false},
        {"got",         R_X86_64_GOT32,     32,  true, false, false}
    };

    for (unsigned int i=0; i<NELEMS(ssyms); ++i)
        add_ssym(object, ssyms[i]);
}

bool
Elf_x86_amd64::accepts_reloc(size_t val) const
{
    return (val&(val-1)) ? 0 : ((val & (8|16|32|64)) != 0);
}

void
Elf_x86_amd64::handle_reloc_addend(IntNum* intn, ElfReloc* reloc) const
{
    // .rela: copy value out as addend, replace original with 0
    reloc->m_addend = 0;
    std::swap(*intn, reloc->m_addend);
}

unsigned int
Elf_x86_amd64::map_reloc_info_to_type(const ElfReloc& reloc) const
{
    if (reloc.m_rtype_rel)
    {
        switch (reloc.m_valsize)
        {
            case 8: return R_X86_64_PC8;
            case 16: return R_X86_64_PC16;
            case 32: return R_X86_64_PC32;
            default: throw InternalError(N_("Unsupported relocation size"));
        }
    }
    else
    {
        switch (reloc.m_valsize)
        {
            case 8: return R_X86_64_8;
            case 16: return R_X86_64_16;
            case 32: return R_X86_64_32;
            case 64: return R_X86_64_64;
            default: throw InternalError(N_("Unsupported relocation size"));
        }
    }
    return 0;
}

}}} // namespace yasm::objfmt::elf

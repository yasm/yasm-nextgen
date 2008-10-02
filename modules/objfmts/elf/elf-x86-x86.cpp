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

class ElfReloc_x86_x86 : public ElfReloc
{
public:
    ElfReloc_x86_x86(const ElfConfig& config,
                     const ElfSymtab& symtab,
                     std::istream& is,
                     bool rela)
        : ElfReloc(config, symtab, is, rela)
    {}
    ElfReloc_x86_x86(SymbolRef sym,
                     SymbolRef wrt,
                     const IntNum& addr,
                     bool rel,
                     size_t valsize);
    ~ElfReloc_x86_x86() {}

    std::string get_type_name() const;
};

class Elf_x86_x86 : public ElfMachine
{
public:
    ~Elf_x86_x86() {}

    void configure(ElfConfig* config) const;
    void add_special_syms(Object& object, const std::string& parser) const;

    std::auto_ptr<ElfReloc>
    read_reloc(const ElfConfig& config,
               const ElfSymtab& symtab,
               std::istream& is,
               bool rela) const
    {
        return std::auto_ptr<ElfReloc>
            (new ElfReloc_x86_x86(config, symtab, is, rela));
    }

    std::auto_ptr<ElfReloc>
    make_reloc(SymbolRef sym,
               SymbolRef wrt,
               const IntNum& addr,
               bool rel,
               size_t valsize) const
    {
        return std::auto_ptr<ElfReloc>
            (new ElfReloc_x86_x86(sym, wrt, addr, rel, valsize));
    }
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

ElfReloc_x86_x86::ElfReloc_x86_x86(SymbolRef sym,
                                   SymbolRef wrt,
                                   const IntNum& addr,
                                   bool rel,
                                   size_t valsize)
    : ElfReloc(sym, wrt, addr, valsize)
{
    if (m_type != 0)
        ;
    else if (rel)
    {
        switch (valsize)
        {
            case 8: m_type = R_386_PC8;
            case 16: m_type = R_386_PC16;
            case 32: m_type = R_386_PC32;
            default: throw TypeError(N_("elf: invalid relocation size"));
        }
    }
    else
    {
        switch (valsize)
        {
            case 8: m_type = R_386_8;
            case 16: m_type = R_386_16;
            case 32: m_type = R_386_32;
            default: throw TypeError(N_("elf: invalid relocation size"));
        }
    }
}

std::string
ElfReloc_x86_x86::get_type_name() const
{
    const char* name = "***UNKNOWN***";
    switch (static_cast<ElfRelocationType_386>(m_type))
    {
        case R_386_NONE: name = "R_386_NONE"; break;
        case R_386_32: name = "R_386_32"; break;
        case R_386_PC32: name = "R_386_PC32"; break;
        case R_386_GOT32: name = "R_386_GOT32"; break;
        case R_386_PLT32: name = "R_386_PLT32"; break;
        case R_386_COPY: name = "R_386_COPY"; break;
        case R_386_GLOB_DAT: name = "R_386_GLOB_DAT"; break;
        case R_386_JMP_SLOT: name = "R_386_JMP_SLOT"; break;
        case R_386_RELATIVE: name = "R_386_RELATIVE"; break;
        case R_386_GOTOFF: name = "R_386_GOTOFF"; break;
        case R_386_GOTPC: name = "R_386_GOTPC"; break;
        case R_386_TLS_TPOFF: name = "R_386_TLS_TPOFF"; break;
        case R_386_TLS_IE: name = "R_386_TLS_IE"; break;
        case R_386_TLS_GOTIE: name = "R_386_TLS_GOTIE"; break;
        case R_386_TLS_LE: name = "R_386_TLS_LE"; break;
        case R_386_TLS_GD: name = "R_386_TLS_GD"; break;
        case R_386_TLS_LDM: name = "R_386_TLS_LDM"; break;
        case R_386_16: name = "R_386_16"; break;
        case R_386_PC16: name = "R_386_PC16"; break;
        case R_386_8: name = "R_386_8"; break;
        case R_386_PC8: name = "R_386_PC8"; break;
        case R_386_TLS_GD_32: name = "R_386_TLS_GD_32"; break;
        case R_386_TLS_GD_PUSH: name = "R_386_TLS_GD_PUSH"; break;
        case R_386_TLS_GD_CALL: name = "R_386_TLS_GD_CALL"; break;
        case R_386_TLS_GD_POP: name = "R_386_TLS_GD_POP"; break;
        case R_386_TLS_LDM_32: name = "R_386_TLS_LDM_32"; break;
        case R_386_TLS_LDM_PUSH: name = "R_386_TLS_LDM_PUSH"; break;
        case R_386_TLS_LDM_CALL: name = "R_386_TLS_LDM_CALL"; break;
        case R_386_TLS_LDM_POP: name = "R_386_TLS_LDM_POP"; break;
        case R_386_TLS_LDO_32: name = "R_386_TLS_LDO_32"; break;
        case R_386_TLS_IE_32: name = "R_386_TLS_IE_32"; break;
        case R_386_TLS_LE_32: name = "R_386_TLS_LE_32"; break;
        case R_386_TLS_DTPMOD32: name = "R_386_TLS_DTPMOD32"; break;
        case R_386_TLS_DTPOFF32: name = "R_386_TLS_DTPOFF32"; break;
        case R_386_TLS_TPOFF32: name = "R_386_TLS_TPOFF32"; break;
    }

    return name;
}

}}} // namespace yasm::objfmt::elf

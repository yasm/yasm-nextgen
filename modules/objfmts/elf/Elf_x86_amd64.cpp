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

#include <yasmx/Support/errwarn.h>
#include <yasmx/Support/nocase.h>

#include "elf.h"
#include "ElfMachine.h"


namespace yasm
{
namespace objfmt
{
namespace elf
{

class ElfReloc_x86_amd64 : public ElfReloc
{
public:
    ElfReloc_x86_amd64(const ElfConfig& config,
                       const ElfSymtab& symtab,
                       std::istream& is,
                       bool rela)
        : ElfReloc(config, symtab, is, rela)
    {}
    ElfReloc_x86_amd64(SymbolRef sym,
                       SymbolRef wrt,
                       const IntNum& addr,
                       bool rel,
                       size_t valsize);
    ~ElfReloc_x86_amd64() {}

    std::string get_type_name() const;
};

class Elf_x86_amd64 : public ElfMachine
{
public:
    ~Elf_x86_amd64() {}

    void configure(ElfConfig* config) const;
    void add_special_syms(Object& object, const std::string& parser) const;

    std::auto_ptr<ElfReloc>
    read_reloc(const ElfConfig& config,
               const ElfSymtab& symtab,
               std::istream& is,
               bool rela) const
    {
        return std::auto_ptr<ElfReloc>
            (new ElfReloc_x86_amd64(config, symtab, is, rela));
    }

    std::auto_ptr<ElfReloc>
    make_reloc(SymbolRef sym,
               SymbolRef wrt,
               const IntNum& addr,
               bool rel,
               size_t valsize) const
    {
        return std::auto_ptr<ElfReloc>
            (new ElfReloc_x86_amd64(sym, wrt, addr, rel, valsize));
    }
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

ElfReloc_x86_amd64::ElfReloc_x86_amd64(SymbolRef sym,
                                       SymbolRef wrt,
                                       const IntNum& addr,
                                       bool rel,
                                       size_t valsize)
    : ElfReloc(sym, wrt, addr, valsize)
{
    // Map PC-relative GOT to appropriate relocation
    if (rel && m_type == R_X86_64_GOT32)
        m_type = R_X86_64_GOTPCREL;
    else if (m_type != 0)
        ;
    else if (rel)
    {
        switch (valsize)
        {
            case 8: m_type = R_X86_64_PC8; break;
            case 16: m_type = R_X86_64_PC16; break;
            case 32: m_type = R_X86_64_PC32; break;
            default: throw TypeError(N_("elf: invalid relocation size"));
        }
    }
    else
    {
        switch (valsize)
        {
            case 8: m_type = R_X86_64_8; break;
            case 16: m_type = R_X86_64_16; break;
            case 32: m_type = R_X86_64_32; break;
            case 64: m_type = R_X86_64_64; break;
            default: throw TypeError(N_("elf: invalid relocation size"));
        }
    }
}

std::string
ElfReloc_x86_amd64::get_type_name() const
{
    const char* name = "***UNKNOWN***";
    switch (static_cast<ElfRelocationType_x86_64>(m_type))
    {
        case R_X86_64_NONE: name = "R_X86_64_NONE"; break;
        case R_X86_64_64: name = "R_X86_64_64"; break;
        case R_X86_64_PC32: name = "R_X86_64_PC32"; break;
        case R_X86_64_GOT32: name = "R_X86_64_GOT32"; break;
        case R_X86_64_PLT32: name = "R_X86_64_PLT32"; break;
        case R_X86_64_COPY: name = "R_X86_64_COPY"; break;
        case R_X86_64_GLOB_DAT: name = "R_X86_64_GLOB_DAT"; break;
        case R_X86_64_JMP_SLOT: name = "R_X86_64_JMP_SLOT"; break;
        case R_X86_64_RELATIVE: name = "R_X86_64_RELATIVE"; break;
        case R_X86_64_GOTPCREL: name = "R_X86_64_GOTPCREL"; break;
        case R_X86_64_32: name = "R_X86_64_32"; break;
        case R_X86_64_32S: name = "R_X86_64_32S"; break;
        case R_X86_64_16: name = "R_X86_64_16"; break;
        case R_X86_64_PC16: name = "R_X86_64_PC16"; break;
        case R_X86_64_8: name = "R_X86_64_8"; break;
        case R_X86_64_PC8: name = "R_X86_64_PC8"; break;
        case R_X86_64_DPTMOD64: name = "R_X86_64_DPTMOD64"; break;
        case R_X86_64_DTPOFF64: name = "R_X86_64_DTPOFF64"; break;
        case R_X86_64_TPOFF64: name = "R_X86_64_TPOFF64"; break;
        case R_X86_64_TLSGD: name = "R_X86_64_TLSGD"; break;
        case R_X86_64_TLSLD: name = "R_X86_64_TLSLD"; break;
        case R_X86_64_DTPOFF32: name = "R_X86_64_DTPOFF32"; break;
        case R_X86_64_GOTTPOFF: name = "R_X86_64_GOTTPOFF"; break;
        case R_X86_64_TPOFF32: name = "R_X86_64_TPOFF32"; break;
    }

    return name;
}

}}} // namespace yasm::objfmt::elf

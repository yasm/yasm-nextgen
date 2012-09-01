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
#include "Elf_x86_amd64.h"
#include "ElfConfig.h"
#include "ElfMachine.h"
#include "ElfTypes.h"


using namespace yasm;
using namespace yasm::objfmt;

namespace {
class Elf_x86_amd64 : public ElfMachine
{
public:
    ~Elf_x86_amd64() {}

    void Configure(ElfConfig* config) const;
    void AddSpecialSymbols(Object& object, StringRef parser) const;

    std::auto_ptr<ElfReloc>
    ReadReloc(const ElfConfig& config,
              const ElfSymtab& symtab,
              const MemoryBuffer& in,
              unsigned long* pos,
              bool rela) const
    {
        return std::auto_ptr<ElfReloc>
            (new ElfReloc_x86_amd64(config, symtab, in, pos, rela));
    }

    std::auto_ptr<ElfReloc>
    MakeReloc(SymbolRef sym, const IntNum& addr) const
    {
        return std::auto_ptr<ElfReloc>(new ElfReloc_x86_amd64(sym, addr));
    }
};
} // anonymous namespace

bool
impl::ElfMatch_x86_amd64(StringRef arch_keyword,
                         StringRef arch_machine,
                         ElfClass cls)
{
    return (arch_keyword.equals_lower("x86") &&
            arch_machine.equals_lower("amd64") &&
            (cls == ELFCLASSNONE || cls == ELFCLASS64));
}

std::auto_ptr<ElfMachine>
impl::ElfCreate_x86_amd64()
{
    return std::auto_ptr<ElfMachine>(new Elf_x86_amd64);
}

void
Elf_x86_amd64::Configure(ElfConfig* config) const
{
    config->cls = ELFCLASS64;
    config->encoding = ELFDATA2LSB;
    config->osabi = ELFOSABI_SYSV;
    config->abi_version = 0;
    config->machine_type = EM_X86_64;
    config->rela = true;
}

void
Elf_x86_amd64::AddSpecialSymbols(Object& object, StringRef parser) const
{
    static const ElfSpecialSymbolData ssyms[] =
    {
        //name,         type,             size,symrel,thread,curpos,needsgot
        {"pltoff",      R_X86_64_PLTOFF64,  64,  true, false, false, false},
        {"plt",         R_X86_64_PLT32,     32,  true, false, false, true},
        {"gotplt",      R_X86_64_GOTPLT64,  64,  true, false, false, false},
        {"gotoff",      R_X86_64_GOTOFF64,  64,  true, false, false, true},
        {"gotpcrel",    R_X86_64_GOTPCREL,  32,  true, false, false, true},
        {"tlsgd",       R_X86_64_TLSGD,     32,  true,  true, false, true},
        {"tlsld",       R_X86_64_TLSLD,     32,  true,  true, false, true},
        {"gottpoff",    R_X86_64_GOTTPOFF,  32,  true,  true, false, true},
        {"tpoff",       R_X86_64_TPOFF32,   32,  true,  true, false, true},
        {"dtpoff",      R_X86_64_DTPOFF32,  32,  true,  true, false, true},
        {"got",         R_X86_64_GOT32,     32,  true, false, false, true},
        {"tlsdesc", R_X86_64_GOTPC32_TLSDESC, 32,true,  true, false, false},
        {"tlscall", R_X86_64_TLSDESC_CALL,  32,  true,  true, false, false}
    };

    for (size_t i=0; i<sizeof(ssyms)/sizeof(ssyms[0]); ++i)
        AddElfSSym(object, ssyms[i]);
}

bool
ElfReloc_x86_amd64::setRel(bool rel,
                           SymbolRef GOT_sym,
                           size_t valsize,
                           bool sign)
{
    // Map PC-relative GOT to appropriate relocation
    if (rel && m_type == R_X86_64_GOT32)
        m_type = R_X86_64_GOTPCREL;
    else if (m_sym == GOT_sym && valsize == 32)
        m_type = R_X86_64_GOTPC32;
    else if (m_sym == GOT_sym && valsize == 64)
        m_type = R_X86_64_GOTPC64;
    else if (rel)
    {
        switch (valsize)
        {
            case 8: m_type = R_X86_64_PC8; break;
            case 16: m_type = R_X86_64_PC16; break;
            case 32: m_type = R_X86_64_PC32; break;
            case 64: m_type = R_X86_64_PC64; break;
            default: return false;
        }
    }
    else
    {
        switch (valsize)
        {
            case 8: m_type = R_X86_64_8; break;
            case 16: m_type = R_X86_64_16; break;
            case 32:
                if (sign)
                    m_type = R_X86_64_32S;
                else
                    m_type = R_X86_64_32;
                break;
            case 64: m_type = R_X86_64_64; break;
            default: return false;
        }
    }
    return true;
}

std::string
ElfReloc_x86_amd64::getTypeName() const
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
        case R_X86_64_PC64: name = "R_X86_64_PC64"; break;
        case R_X86_64_GOTOFF64: name = "R_X86_64_GOTOFF64"; break;
        case R_X86_64_GOTPC32: name = "R_X86_64_GOTPC32"; break;
        case R_X86_64_GOT64: name = "R_X86_64_GOT64"; break;
        case R_X86_64_GOTPCREL64: name = "R_X86_64_GOTPCREL64"; break;
        case R_X86_64_GOTPC64: name = "R_X86_64_GOTPC64"; break;
        case R_X86_64_GOTPLT64: name = "R_X86_64_GOTPLT64"; break;
        case R_X86_64_PLTOFF64: name = "R_X86_64_PLTOFF64"; break;
        case R_X86_64_GOTPC32_TLSDESC: name = "R_X86_64_GOTPC32_TLSDESC"; break;
        case R_X86_64_TLSDESC_CALL: name = "R_X86_64_TLSDESC_CALL"; break;
        case R_X86_64_TLSDESC: name = "R_X86_64_TLSDESC"; break;
        case R_X86_64_IRELATIVE: name = "R_X86_64_IRELATIVE"; break;
        case R_X86_64_RELATIVE64: name = "R_X86_64_RELATIVE64"; break;
    }

    return name;
}

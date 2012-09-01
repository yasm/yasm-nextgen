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
#include "ElfConfig.h"
#include "ElfMachine.h"
#include "ElfReloc.h"
#include "ElfTypes.h"


using namespace yasm;
using namespace yasm::objfmt;

namespace {
class ElfReloc_x86_x86 : public ElfReloc
{
public:
    ElfReloc_x86_x86(const ElfConfig& config,
                     const ElfSymtab& symtab,
                     const MemoryBuffer& in,
                     unsigned long* pos,
                     bool rela)
        : ElfReloc(config, symtab, in, pos, rela)
    {}
    ElfReloc_x86_x86(SymbolRef sym, const IntNum& addr)
        : ElfReloc(sym, addr)
    {}

    ~ElfReloc_x86_x86() {}

    bool setRel(bool rel, SymbolRef GOT_sym, size_t valsize, bool sign);
    std::string getTypeName() const;
    void HandleAddend(IntNum* intn,
                      const ElfConfig& config,
                      unsigned int insn_start);
};

class Elf_x86_x86 : public ElfMachine
{
public:
    ~Elf_x86_x86() {}

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
            (new ElfReloc_x86_x86(config, symtab, in, pos, rela));
    }

    std::auto_ptr<ElfReloc>
    MakeReloc(SymbolRef sym, const IntNum& addr) const
    {
        return std::auto_ptr<ElfReloc>(new ElfReloc_x86_x86(sym, addr));
    }
};
} // anonymous namespace

bool
impl::ElfMatch_x86_x86(StringRef arch_keyword,
                       StringRef arch_machine,
                       ElfClass cls)
{
    return (arch_keyword.equals_lower("x86") &&
            arch_machine.equals_lower("x86") &&
            (cls == ELFCLASSNONE || cls == ELFCLASS32));
}

std::auto_ptr<ElfMachine>
impl::ElfCreate_x86_x86()
{
    return std::auto_ptr<ElfMachine>(new Elf_x86_x86);
}

void
Elf_x86_x86::Configure(ElfConfig* config) const
{
    config->cls = ELFCLASS32;
    config->encoding = ELFDATA2LSB;
    config->osabi = ELFOSABI_SYSV;
    config->abi_version = 0;
    config->machine_type = EM_386;
    config->rela = false;
}

void
Elf_x86_x86::AddSpecialSymbols(Object& object, StringRef parser) const
{
    static const ElfSpecialSymbolData ssyms[] =
    {
        //name,         type,             size,symrel,thread,curpos,needsgot
        {"plt",         R_386_PLT32,        32,  true, false, false, true},
        {"gotoff",      R_386_GOTOFF,       32, false, false, false, true},
        // special one for NASM
        {"gotpc",       R_386_GOTPC,        32, false, false,  true, false},
        {"tlsgd",       R_386_TLS_GD,       32,  true,  true, false, true},
        {"tlsldm",      R_386_TLS_LDM,      32,  true,  true, false, true},
        {"gottpoff",    R_386_TLS_IE_32,    32,  true,  true, false, true},
        {"tpoff",       R_386_TLS_LE_32,    32,  true,  true, false, true},
        {"ntpoff",      R_386_TLS_LE,       32,  true,  true, false, true},
        {"dtpoff",      R_386_TLS_LDO_32,   32,  true,  true, false, true},
        {"gotntpoff",   R_386_TLS_GOTIE,    32,  true,  true, false, true},
        {"indntpoff",   R_386_TLS_IE,       32,  true,  true, false, true},
        {"got",         R_386_GOT32,        32,  true, false, false, true},
        {"tlsdesc",     R_386_TLS_GOTDESC,  32,  true,  true, false, false},
        {"tlscall",     R_386_TLS_DESC_CALL,32,  true,  true, false, false}
    };

    for (size_t i=0; i<sizeof(ssyms)/sizeof(ssyms[0]); ++i)
        AddElfSSym(object, ssyms[i]);
}

bool
ElfReloc_x86_x86::setRel(bool rel,
                         SymbolRef GOT_sym,
                         size_t valsize,
                         bool sign)
{
    if (m_sym == GOT_sym && valsize == 32)
        m_type = R_386_GOTPC;
    else if (rel)
    {
        switch (valsize)
        {
            case 8: m_type = R_386_PC8; break;
            case 16: m_type = R_386_PC16; break;
            case 32: m_type = R_386_PC32; break;
            default: return false;
        }
    }
    else
    {
        switch (valsize)
        {
            case 8: m_type = R_386_8; break;
            case 16: m_type = R_386_16; break;
            case 32: m_type = R_386_32; break;
            default: return false;
        }
    }
    return true;
}

std::string
ElfReloc_x86_x86::getTypeName() const
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
        case R_386_TLS_GOTDESC: name = "R_386_TLS_GOTDESC"; break;
        case R_386_TLS_DESC_CALL: name = "R_386_TLS_DESC_CALL"; break;
        case R_386_TLS_DESC: name = "R_386_TLS_DESC"; break;
    }

    return name;
}

void
ElfReloc_x86_x86::HandleAddend(IntNum* intn,
                               const ElfConfig& config,
                               unsigned int insn_start)
{
    if (!m_wrt && m_type == R_386_GOTPC)
    {
        // Need fixup to the value position within the instruction.
        *intn += insn_start;
    }
    ElfReloc::HandleAddend(intn, config, insn_start);
}

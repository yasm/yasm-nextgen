#ifndef YASM_ELFCONFIG_H
#define YASM_ELFCONFIG_H
//
// ELF object format configuration
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
#include "yasmx/Config/export.h"
#include "yasmx/DebugDumper.h"
#include "yasmx/IntNum.h"

#include "ElfTypes.h"


namespace llvm { class MemoryBuffer; }

namespace yasm
{

class Bytes;
class Diagnostic;
class EndianState;
class Object;
class Section;
class StringTable;

namespace objfmt
{

struct YASM_STD_EXPORT ElfConfig
{
    ElfClass        cls;            // ELF class (32/64)
    ElfDataEncoding encoding;       // ELF encoding (MSB/LSB)
    ElfVersion      version;        // ELF version
    ElfOsabiIndex   osabi;          // OS/ABI
    unsigned char   abi_version;    // ABI version

    ElfFileType     file_type;      // ELF file type (reloc/exe/so)
    ElfMachineType  machine_type;   // machine type (386/68K/...)

    IntNum          start;          // execution start address
    bool            rela;           // relocations have explicit addends?

    // other program header fields; may not always be valid
    unsigned long   proghead_pos;   // file offset of program header (0=none)
    unsigned int    proghead_count; // number of program header entries (0=none)
    unsigned int    proghead_size;  // program header entry size (0=none)

    unsigned long   secthead_pos;   // file offset of section header (0=none)
    unsigned int    secthead_count; // number of section header entries (0=none)
    unsigned int    secthead_size;  // section header entry size (0=none)

    unsigned long   machine_flags;  // machine-specific flags
    ElfSectionIndex shstrtab_index; // section index of section string table

    ElfConfig();
    ~ElfConfig() {}

    unsigned long getProgramHeaderSize() const;
    bool ReadProgramHeader(const llvm::MemoryBuffer& in);
    void WriteProgramHeader(llvm::raw_ostream& os, Bytes& scratch);

    ElfSymbolIndex AssignSymbolIndices(Object& object, ElfSymbolIndex* nlocal)
        const;

    unsigned long WriteSymbolTable(llvm::raw_ostream& os,
                                   Object& object,
                                   Diagnostic& diags,
                                   Bytes& scratch) const;
    bool ReadSymbolTable(const llvm::MemoryBuffer&  in,
                         const ElfSection&          symtab_sect,
                         ElfSymtab&                 symtab,
                         Object&                    object,
                         const StringTable&         strtab,
                         Section*                   sections[],
                         Diagnostic&                diags) const;

    std::string getRelocSectionName(const std::string& basesect) const;

    bool setEndian(EndianState& state) const;

#ifdef WITH_XML
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML
};

}} // namespace yasm::objfmt

#endif

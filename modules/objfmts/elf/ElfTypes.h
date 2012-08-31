#ifndef YASM_ELFTYPES_H
#define YASM_ELFTYPES_H
//
// ELF object format types
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

#include <vector>

#include "yasmx/SymbolRef.h"


namespace yasm
{
namespace objfmt
{

struct ElfConfig;
class ElfMachine;
class ElfReloc;
class ElfSection;
class ElfSymbol;

typedef unsigned long ElfAddress;
typedef unsigned long ElfOffset;
typedef unsigned long ElfSize;
typedef unsigned long ElfSectionInfo;
typedef unsigned long ElfStringIndex;
typedef unsigned char ElfRelocationType;

enum ElfFileType
{
    ET_NONE = 0,
    ET_REL = 1,                                 // Relocatable
    ET_EXEC = 2,                                // Executable
    ET_DYN = 3,                                 // Shared object
    ET_CORE = 4,                                // Core
    ET_LOOS = 0xfe00,                           // Environment specific
    ET_HIOS = 0xfeff,
    ET_LOPROC = 0xff00,                         // Processor specific
    ET_HIPROC = 0xffff
};

enum ElfMachineType
{
    EM_NONE = 0,
    EM_M32 = 1,                                 // AT&T WE 32100
    EM_SPARC = 2,                               // SPARC
    EM_386 = 3,                                 // Intel 80386
    EM_68K = 4,                                 // Motorola 68000
    EM_88K = 5,                                 // Motorola 88000
    EM_860 = 7,                                 // Intel 80860
    EM_MIPS = 8,                                // MIPS RS3000

    EM_S370 = 9,                                // IBM System/370
    EM_MIPS_RS4_BE = 10,                        // MIPS R4000 Big-Endian (dep)
    EM_PARISC = 15,                             // HPPA
    EM_SPARC32PLUS = 18,                        // SPARC v8plus
    EM_PPC = 20,                                // PowerPC 32-bit
    EM_PPC64 = 21,                              // PowerPC 64-bit
    EM_ARM = 40,                                // ARM
    EM_SPARCV9 = 43,                            // SPARC v9 64-bit
    EM_IA_64 = 50,                              // Intel IA-64
    EM_X86_64 = 62,                             // AMD x86-64
    EM_ALPHA = 0x9026                           // Alpha (no ABI)
};

enum ElfMagic
{
    ELFMAG0 = 0x7f,
    ELFMAG1 = 0x45,
    ELFMAG2 = 0x4c,
    ELFMAG3 = 0x46
};

enum ElfVersion
{
    EV_NONE = 0,                                // invalid
    EV_CURRENT = 1                              // current
};

enum ElfIdentificationIndex
{
    EI_MAG0 = 0,                                // File id
    EI_MAG1 = 1,
    EI_MAG2 = 2,
    EI_MAG3 = 3,
    EI_CLASS = 4,
    EI_DATA = 5,                                // Data encoding
    EI_VERSION = 6,                             // File version
    EI_OSABI = 7,                               // OS and ABI
    EI_ABIVERSION = 8,                          // version of ABI

    EI_PAD = 9,                                 // Pad to end; start here
    EI_NIDENT = 16                              // Sizeof e_ident[]
};

enum ElfOsabiIndex
{
    ELFOSABI_SYSV = 0,                          // System V ABI
    ELFOSABI_HPUX = 1,                          // HP-UX os
    ELFOSABI_STANDALONE = 255                   // Standalone / embedded app
};

enum ElfClass
{
    ELFCLASSNONE = 0,                           // invalid
    ELFCLASS32 = 1,                             // 32-bit
    ELFCLASS64 = 2                              // 64-bit
};

enum ElfDataEncoding
{
    ELFDATANONE = 0,
    ELFDATA2LSB = 1,
    ELFDATA2MSB = 2
};

// elf section types - index of semantics
enum ElfSectionType
{
    SHT_NULL = 0,               // inactive section - no associated data
    SHT_PROGBITS = 1,           // defined by program for its own meaning
    SHT_SYMTAB = 2,             // symbol table (primarily) for linking
    SHT_STRTAB = 3,             // string table - symbols need names
    SHT_RELA = 4,               // relocation entries w/ explicit addends
    SHT_HASH = 5,               // symbol hash table - for dynamic linking
    SHT_DYNAMIC = 6,            // information for dynamic linking
    SHT_NOTE = 7,               // extra data marking the file somehow
    SHT_NOBITS = 8,             // no stored data, but occupies runtime space
    SHT_REL = 9,                // relocations entries w/o explicit addends
    SHT_SHLIB = 10,             // reserved; unspecified semantics
    SHT_DYNSYM = 11,            // like symtab, but more for dynamic linking
    SHT_INIT_ARRAY = 14,        // Array of constructors
    SHT_FINI_ARRAY = 15,        // Array of destructors
    SHT_PREINIT_ARRAY = 16,     // Array of pre-constructors
    SHT_GROUP = 17,             // Section group
    SHT_SYMTAB_SHNDX = 18,      // Extended section indices
    SHT_NUM = 19,               // Number of defined types
    SHT_LOOS = 0x60000000,      // reserved for environment specific use
    SHT_HIOS = 0x6fffffff,
    SHT_LOPROC = 0x70000000,    // reserved for processor specific semantics
    SHT_UNWIND = 0x70000001,    // @unwind section type
    SHT_HIPROC = 0x7fffffff/*,
    SHT_LOUSER = 0x80000000,*/  // reserved for applications; safe
    /*SHT_HIUSER = 0xffffffff*/
};

// elf section flags - bitfield of attributes
enum ElfSectionFlagValues
{
    SHF_WRITE = 0x1,            // data should be writable at runtime
    SHF_ALLOC = 0x2,            // occupies memory at runtime
    SHF_EXECINSTR = 0x4,        // contains machine instructions
    SHF_MERGE = 0x10,           // data can be merged
    SHF_STRINGS = 0x20,         // contains 0-terminated strings
    SHF_GROUP = 0x200,          // member of a section group
    SHF_TLS = 0x400,            // thread local storage
    SHF_MASKOS = 0x0f000000/*,  // environment specific use
    SHF_MASKPROC = 0xf0000000*/ // bits reserved for processor specific needs
};
typedef unsigned long ElfSectionFlags;

// elf section index - just the special ones
enum ElfSectionIndexValues
{
    SHN_UNDEF = 0,              // undefined symbol; requires other global
    SHN_LORESERVE = 0xff00,     // reserved for various semantics
    SHN_LOPROC = 0xff00,        // reserved for processor specific semantics
    SHN_HIPROC = 0xff1f,
    SHN_LOOS = 0xff20,          // reserved for environment specific use
    SHN_HIOS = 0xff3f,
    SHN_ABS = 0xfff1,           // associated symbols don't change on reloc
    SHN_COMMON = 0xfff2,        // associated symbols refer to unallocated
    SHN_HIRESERVE = 0xffff
};
typedef unsigned int ElfSectionIndex;

// elf symbol binding - index of visibility/behavior
enum ElfSymbolBinding
{
    STB_LOCAL = 0,              // invisible outside defining file
    STB_GLOBAL = 1,             // visible to all combined object files
    STB_WEAK = 2,               // global but lower precedence

    STB_LOOS = 10,              // Environment specific use
    STB_HIOS = 12,
    STB_LOPROC = 13,            // reserved for processor specific semantics
    STB_HIPROC = 15
};

// elf symbol type - index of classifications
enum ElfSymbolType
{
    STT_NOTYPE = 0,             // type not specified
    STT_OBJECT = 1,             // data object such as a variable, array, etc
    STT_FUNC = 2,               // a function or executable code
    STT_SECTION = 3,            // a section: often for relocation, STB_LOCAL
    STT_FILE = 4,               // often source filename: STB_LOCAL, SHN_ABS
    STT_COMMON = 5,             // Uninitialized common block.
    STT_TLS = 6,                // TLS object.
    STT_NUM = 7,

    STT_LOOS = 10,              // Environment specific use
    STT_HIOS = 12,
    STT_LOPROC = 13,            // reserved for processor specific semantics
    STT_HIPROC = 15
};

enum ElfSymbolIndexValues
{
    STN_UNDEF = 0
};
typedef unsigned int ElfSymbolIndex;

// elf symbol visibility - lower two bits of OTHER field
enum ElfSymbolVis
{
    STV_DEFAULT = 0,            // Default symbol visibility rules
    STV_INTERNAL = 1,           // Processor specific hidden class
    STV_HIDDEN = 2,             // Sym unavailable in other modules
    STV_PROTECTED = 3           // Not preemptable, not exported
};

#define ELF_VISIBILITY_MASK             0x03
#define ELF_ST_VISIBILITY(v) \
    static_cast<ElfSymbolVis>((static_cast<unsigned int>(v) \
                               & ELF_VISIBILITY_MASK))

#define ELF_ST_BIND(val) \
    static_cast<ElfSymbolBinding>(((unsigned char)(val)) >> 4)
#define ELF_ST_TYPE(val)                static_cast<ElfSymbolType>((val) & 0xf)
#define ELF_ST_INFO(bind, type)         (((bind) << 4) + ((type) & 0xf))
#define ELF_ST_OTHER(vis)               ELF_ST_VISIBILITY(vis)

#define ELF32_R_SYM(i)              static_cast<ElfSymbolIndex>((i) >> 8)
#define ELF32_R_TYPE(i)             static_cast<ElfRelocationType>((i) & 0xff)
#define ELF32_R_INFO(s, t)          (((s)<<8)+(unsigned char)(t & 0xff))

#define ELF64_R_SYM(i) \
    static_cast<ElfSymbolIndex>(((i) >> 32).getUInt())
#define ELF64_R_TYPE(i)             ((i.getUInt()) & 0xffffffff)
#define ELF64_R_INFO(s, t)          ((IntNum(s)<<32) + ((t) & 0xffffffffUL))

#define EHDR32_SIZE 52
#define EHDR64_SIZE 64
#define EHDR_MAXSIZE 64

#define SHDR32_SIZE 40
#define SHDR64_SIZE 64
#define SHDR_MAXSIZE 64

#define SYMTAB32_SIZE 16
#define SYMTAB64_SIZE 24
#define SYMTAB_MAXSIZE 24

#define SYMTAB32_ALIGN 4
#define SYMTAB64_ALIGN 8

#define RELOC32_SIZE 8
#define RELOC32A_SIZE 12
#define RELOC64_SIZE 16
#define RELOC64A_SIZE 24
#define RELOC_MAXSIZE 24

#define RELOC32_ALIGN 4
#define RELOC64_ALIGN 8


// elf relocation type - index of semantics
//
// A = Addend (r_addend for RELA, value at location for REL)
// B = Base address
// G = Offset into global offset table (GOT)
// GOT = Address of the global offset table (GOT)
// L = Location of procedure linkage table (PLT)
// P = Location of location being relocated (r_offset)
// S = Value of symbol
enum ElfRelocationType_386
{
    R_386_NONE = 0,             // none
    R_386_32 = 1,               // word32, S + A
    R_386_PC32 = 2,             // word32, S + A - P
    R_386_GOT32 = 3,            // word32, G + A - P
    R_386_PLT32 = 4,            // word32, L + A - P
    R_386_COPY = 5,             // none
    R_386_GLOB_DAT = 6,         // word32, S
    R_386_JMP_SLOT = 7,         // word32, S
    R_386_RELATIVE = 8,         // word32, B + A
    R_386_GOTOFF = 9,           // word32, S + A - GOT
    R_386_GOTPC = 10,           // word32, GOT + A - P
    R_386_TLS_TPOFF = 14,       // Negative offset in static TLS block (GNU
                                //  version)
    R_386_TLS_IE = 15,          // Absolute address of GOT entry for negative
                                //  static TLS block offset
    R_386_TLS_GOTIE = 16,       // GOT entry for negative static TLS block
                                //  offset
    R_386_TLS_LE = 17,          // Negative offset relative to static TLS
                                //  (GNU version)
    R_386_TLS_GD = 18,          // Direct 32 bit for GNU version of GD TLS
    R_386_TLS_LDM = 19,         // Direct 32 bit for GNU version of LD TLS
                                //  in LE code
    R_386_16 = 20,              // word16, S + A (GNU extension)
    R_386_PC16 = 21,            // word16, S + A - P (GNU extension)
    R_386_8 = 22,               // word8, S + A (GNU extension)
    R_386_PC8 = 23,             // word8, S + A - P (GNU extension)
    R_386_TLS_GD_32 = 24,       // Direct 32 bit for GD TLS
    R_386_TLS_GD_PUSH = 25,     // Tag for pushl in GD TLS code
    R_386_TLS_GD_CALL = 26,     // Relocation for call to
    R_386_TLS_GD_POP = 27,      // Tag for popl in GD TLS code
    R_386_TLS_LDM_32 = 28,      // Direct 32 bit for local dynamic code
    R_386_TLS_LDM_PUSH = 29,    // Tag for pushl in LDM TLS code
    R_386_TLS_LDM_CALL = 30,    // Relocation for call to
    R_386_TLS_LDM_POP = 31,     // Tag for popl in LDM TLS code
    R_386_TLS_LDO_32 = 32,      // Offset relative to TLS block
    R_386_TLS_IE_32 = 33,       // GOT entry for static TLS block
    R_386_TLS_LE_32 = 34,       // Offset relative to static TLS block
    R_386_TLS_DTPMOD32 = 35,    // ID of module containing symbol
    R_386_TLS_DTPOFF32 = 36,    // Offset in TLS block
    R_386_TLS_TPOFF32 = 37,     // Offset in static TLS block
    R_386_TLS_GOTDESC = 39,
    R_386_TLS_DESC_CALL = 40,
    R_386_TLS_DESC = 41
};

enum ElfRelocationType_x86_64
{
    R_X86_64_NONE = 0,          // none
    R_X86_64_64 = 1,            // word64, S + A
    R_X86_64_PC32 = 2,          // word32, S + A - P
    R_X86_64_GOT32 = 3,         // word32, G + A
    R_X86_64_PLT32 = 4,         // word32, L + A - P
    R_X86_64_COPY = 5,          // none
    R_X86_64_GLOB_DAT = 6,      // wordclass, S, set GOT entry to data address
    R_X86_64_JMP_SLOT = 7,      // wordclass, S, set GOT entry to code address
    R_X86_64_RELATIVE = 8,      // wordclass, B + A
    R_X86_64_GOTPCREL = 9,      // word32, G + GOT + A - P
    R_X86_64_32 = 10,           // word32 (zero extend), S + A
    R_X86_64_32S = 11,          // word32 (sign extend), S + A
    R_X86_64_16 = 12,           // word16, S + A
    R_X86_64_PC16 = 13,         // word16, S + A - P
    R_X86_64_8 = 14,            // word8, S + A
    R_X86_64_PC8 = 15,          // word8, S + A - P
    R_X86_64_DPTMOD64 = 16,     // word64, ID of module containing symbol
    R_X86_64_DTPOFF64 = 17,     // word64, offset in TLS block
    R_X86_64_TPOFF64 = 18,      // word64, offset in initial TLS block
    R_X86_64_TLSGD = 19,        // word32, PC-rel offset to GD GOT block
    R_X86_64_TLSLD = 20,        // word32, PC-rel offset to LD GOT block
    R_X86_64_DTPOFF32 = 21,     // word32, offset to TLS block
    R_X86_64_GOTTPOFF = 22,     // word32, PC-rel offset to IE GOT entry
    R_X86_64_TPOFF32 = 23,      // word32, offset in initial TLS block
    R_X86_64_PC64 = 24,         // word64, PC relative
    R_X86_64_GOTOFF64 = 25,     // word64, offset to GOT
    R_X86_64_GOTPC32 = 26,      // word32, signed pc relative to GOT
    R_X86_64_GOT64 = 27,        // word64, GOT entry offset
    R_X86_64_GOTPCREL64 = 28,   // word64, signed pc relative to GOT entry
    R_X86_64_GOTPC64 = 29,      // word64, signed pc relative to GOT
    R_X86_64_GOTPLT64 = 30,     // like GOT64, but indicates PLT entry needed
    R_X86_64_PLTOFF64 = 31,     // word64, GOT relative offset to PLT entry
    R_X86_64_GOTPC32_TLSDESC = 34, // GOT offset for TLS descriptor
    R_X86_64_TLSDESC_CALL = 35, // Marker for call through TLS descriptor
    R_X86_64_TLSDESC = 36,      // TLS descriptor
    R_X86_64_IRELATIVE = 37,    // wordclass, indirect (B + A)
    R_X86_64_RELATIVE64 = 38    // word64, B + A
};

typedef std::vector<SymbolRef> ElfSymtab;

}} // namespace yasm::objfmt

#endif

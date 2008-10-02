#ifndef ELF_H_INCLUDED
#define ELF_H_INCLUDED
//
// ELF object format helpers
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

#include <iosfwd>
#include <vector>

#include <libyasmx/assoc_data.h>
#include <libyasmx/bytes.h>
#include <libyasmx/intnum.h>
#include <libyasmx/marg_ostream_fwd.h>
#include <libyasmx/reloc.h>
#include <libyasmx/scoped_ptr.h>
#include <libyasmx/section.h>
#include <libyasmx/symbol.h>
#include <libyasmx/symbolref.h>


namespace yasm
{

class Errwarns;
class Expr;
class Object;
class Section;

namespace objfmt
{
namespace elf
{

class ElfMachine;

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

    SHT_LOOS = 0x60000000,      // reserved for environment specific use
    SHT_HIOS = 0x6fffffff,
    SHT_LOPROC = 0x70000000,    // reserved for processor specific semantics
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
    static_cast<ElfSymbolIndex>(((i) >> 32).get_uint())
#define ELF64_R_TYPE(i)             ((i.get_uint()) & 0xffffffff)
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
    R_386_TLS_TPOFF32 = 37      // Offset in static TLS block
};

enum ElfRelocationType_x86_64
{
    R_X86_64_NONE = 0,          // none
    R_X86_64_64 = 1,            // word64, S + A
    R_X86_64_PC32 = 2,          // word32, S + A - P
    R_X86_64_GOT32 = 3,         // word32, G + A
    R_X86_64_PLT32 = 4,         // word32, L + A - P
    R_X86_64_COPY = 5,          // none
    R_X86_64_GLOB_DAT = 6,      // word64, S, set GOT entry to data address
    R_X86_64_JMP_SLOT = 7,      // word64, S, set GOT entry to code address
    R_X86_64_RELATIVE = 8,      // word64, B + A
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
    R_X86_64_TPOFF32 = 23       // word32, offset in initial TLS block
};

typedef std::vector<SymbolRef> ElfSymtab;

class ElfStrtab
{
public:
    ElfStrtab();
    ~ElfStrtab() {}

    ElfStringIndex get_index(const std::string& str);
    std::string get_str(ElfStringIndex index) const;

    unsigned long write(std::ostream& os) const;
    void read(std::istream& is, unsigned long size);

private:
    std::vector<char> m_storage;
};

struct ElfConfig
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

    unsigned long proghead_get_size() const;
    bool proghead_read(std::istream& is);
    void proghead_write(std::ostream& os, Bytes& scratch);

    ElfSymbolIndex symtab_setindexes(Object& object, ElfSymbolIndex* nlocal)
        const;

    unsigned long symtab_write(std::ostream& os,
                               Object& object,
                               Errwarns& errwarns,
                               Bytes& scratch) const;
    bool symtab_read(std::istream&      is,
                     ElfSymtab&         symtab,
                     Object&            object,
                     unsigned long      size,
                     ElfSize            symsize,
                     const ElfStrtab&   strtab,
                     Section*           sections[]) const;

    std::string name_reloc_section(const std::string& basesect) const;

    bool setup_endian(Bytes& bytes) const
    {
        if (encoding == ELFDATA2LSB)
            bytes << little_endian;
        else if (encoding == ELFDATA2MSB)
            bytes << big_endian;
        else
            return false;
        return true;
    }
};

class ElfReloc : public Reloc
{
public:
    // Constructor that reads from file.  Assumes input stream is already
    // positioned at the beginning of the relocation.
    ElfReloc(const ElfConfig& config,
             const ElfSymtab& symtab,
             std::istream& is,
             bool rela);
    ElfReloc(SymbolRef sym, SymbolRef wrt, const IntNum& addr, size_t valsize);
    virtual ~ElfReloc();

    std::auto_ptr<Expr> get_value() const;
    virtual std::string get_type_name() const = 0;

    void handle_addend(IntNum* intn, const ElfConfig& config);
    void write(Bytes& bytes, const ElfConfig& config);

protected:
    ElfRelocationType   m_type;
    IntNum              m_addend;
};

class ElfSymbol : public AssocData
{
public:
    static const char* key;

    // Constructor that reads from bytes (e.g. from file)
    ElfSymbol(const ElfConfig&  config,
              Bytes&            bytes,
              ElfSymbolIndex    index,
              Section*          sections[]);

    ElfSymbol();
    ~ElfSymbol();

    SymbolRef create_symbol(Object& object, const ElfStrtab& strtab) const;

    void put(marg_ostream& os) const;

    void finalize(Symbol& sym, Errwarns& errwarns);
    void write(Bytes& bytes, const ElfConfig& config);

    void set_section(Section* sect) { m_sect = sect; }
    void set_name(ElfStringIndex index) { m_name_index = index; }
    void set_index(ElfSectionIndex index) { m_index = index; }
    void set_visibility(ElfSymbolVis vis) { m_vis = ELF_ST_VISIBILITY(vis); }
    void set_binding(ElfSymbolBinding bind) { m_bind = bind; }
    void set_type(ElfSymbolType type) { m_type = type; }
    void set_size(std::auto_ptr<Expr> size);
    void set_value(ElfAddress value) { m_value = value; }
    void set_symindex(ElfSymbolIndex symindex) { m_symindex = symindex; }
    ElfSymbolIndex get_symindex() const { return m_symindex; }

    bool is_local() const { return m_bind == STB_LOCAL; }

private:
    Section*            m_sect;
    ElfStringIndex      m_name_index;
    IntNum              m_value;
    util::scoped_ptr<Expr> m_xsize;
    IntNum              m_size;
    ElfSectionIndex     m_index;
    ElfSymbolBinding    m_bind;
    ElfSymbolType       m_type;
    ElfSymbolVis        m_vis;
    ElfSymbolIndex      m_symindex;
};

inline ElfSymbol*
get_elf(Symbol& sym)
{
    return static_cast<ElfSymbol*>(sym.get_assoc_data(ElfSymbol::key));
}

void insert_local_sym(Object& object,
                      std::auto_ptr<Symbol> sym,
                      std::auto_ptr<ElfSymbol> entry);
ElfSymbolIndex assign_sym_indices(Object& object);

class ElfSection : public AssocData
{
public:
    static const char* key;

    // Constructor that reads from file.  Assumes input stream is already
    // positioned at the beginning of the section header.
    ElfSection(const ElfConfig&     config,
               std::istream&        is,
               ElfSectionIndex      index);

    ElfSection(const ElfConfig&     config,
               ElfSectionType       type,
               ElfSectionFlags      flags,
               bool                 symtab = false);

    ~ElfSection();

    void put(marg_ostream& os) const;

    unsigned long write(std::ostream& os, Bytes& scratch) const;

    std::auto_ptr<Section> create_section(const ElfStrtab& shstrtab) const;
    void load_section_data(Section& sect, std::istream& is) const;

    ElfSectionType get_type() const { return m_type; }

    void set_name(ElfStringIndex index) { m_name_index = index; }
    ElfStringIndex get_name() const { return m_name_index; }

    void set_typeflags(ElfSectionType type, ElfSectionFlags flags)
    {
        m_type = type;
        m_flags = flags;
    }
    ElfSectionFlags get_flags() const { return m_flags; }

    bool is_empty() const { return m_size.is_zero(); }

    SymbolRef get_sym() const { return m_sym; }

    unsigned long get_align() const { return m_align; }
    void set_align(unsigned long align) { m_align = align; }

    ElfSectionIndex get_index() { return m_index; }

    void set_info(ElfSectionInfo info) { m_info = info; }
    ElfSectionInfo get_info() const { return m_info; }

    void set_index(ElfSectionIndex sectidx) { m_index = sectidx; }

    void set_link(ElfSectionIndex link) { m_link = link; }
    ElfSectionIndex get_link() const { return m_link; }

    void set_rel_index(ElfSectionIndex sectidx) { m_rel_index = sectidx; }
    void set_rel_name(ElfStringIndex nameidx) { m_rel_name_index = nameidx; }

    void set_entsize(ElfSize size) { m_entsize = size; }
    ElfSize get_entsize() const { return m_entsize; }

    void set_sym(SymbolRef sym) { m_sym = sym; }

    void add_size(const IntNum& size) { m_size += size; }
    void set_size(const IntNum& size) { m_size = size; }
    IntNum get_size() const { return m_size; }

    unsigned long write_rel(std::ostream& os,
                            ElfSectionIndex symtab,
                            Section& sect,
                            Bytes& scratch);
    unsigned long write_relocs(std::ostream& os,
                               Section& sect,
                               Errwarns& errwarns,
                               Bytes& scratch,
                               const ElfMachine& machine);
    bool read_relocs(std::istream& is,
                     Section& sect,
                     unsigned long size,
                     const ElfMachine& machine,
                     const ElfSymtab& symtab,
                     bool rela) const;

    unsigned long set_file_offset(unsigned long pos);
    unsigned long get_file_offset() const { return m_offset; }

private:
    const ElfConfig&    m_config;

    ElfSectionType      m_type;
    ElfSectionFlags     m_flags;
    IntNum              m_addr;
    ElfAddress          m_offset;
    IntNum              m_size;
    ElfSectionIndex     m_link;
    ElfSectionInfo      m_info;         // see note ESD1
    unsigned long       m_align;
    ElfSize             m_entsize;

    SymbolRef           m_sym;
    ElfStringIndex      m_name_index;
    ElfSectionIndex     m_index;

    ElfStringIndex      m_rel_name_index;
    ElfSectionIndex     m_rel_index;
    ElfAddress          m_rel_offset;
};

inline ElfSection*
get_elf(Section& sym)
{
    return static_cast<ElfSection*>(sym.get_assoc_data(ElfSection::key));
}

// Note ESD1:
//   for section types SHT_REL, SHT_RELA:
//     link -> index of associated symbol table
//     info -> index of relocated section
//   for section types SHT_SYMTAB, SHT_DYNSYM:
//     link -> index of associated string table
//     info -> 1+index of last "local symbol" (bind == STB_LOCAL)
//  (for section type SHT_DNAMIC:
//     link -> index of string table
//     info -> 0 )
//  (for section type SHT_HASH:
//     link -> index of symbol table to which hash applies
//     info -> 0 )
//   for all others:
//     link -> SHN_UNDEF
//     info -> 0

}}} // namespace yasm::objfmt::elf

#endif // ELF_H_INCLUDED

#ifndef COFF_DATA_H
#define COFF_DATA_H
//
// COFF object format data definitions
//
//  Copyright (C) 2002-2008  Peter Johnson
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
#include <string>
#include <vector>

#include <libyasmx/assoc_data.h>
#include <libyasmx/marg_ostream_fwd.h>
#include <libyasmx/reloc.h>
#include <libyasmx/section.h>
#include <libyasmx/symbol.h>
#include <libyasmx/symbolref.h>

namespace yasm
{

class Bytes;
class StringTable;

namespace objfmt
{
namespace coff
{

class CoffReloc : public Reloc
{
public:
    enum Type
    {
        ABSOLUTE = 0,           ///< absolute, no reloc needed

        // I386 relocations
        I386_ADDR16 = 0x1,      ///< 16-bit absolute reference
        I386_REL16 = 0x2,       ///< 16-bit PC-relative reference
        I386_ADDR32 = 0x6,      ///< 32-bit absolute reference
        I386_ADDR32NB = 0x7,    ///< 32-bit absolute ref w/o base
        I386_SEG12 = 0x9,       ///< 16-bit absolute segment ref
        I386_SECTION = 0xA,     ///< section index
        I386_SECREL = 0xB,      ///< offset from start of segment
        I386_TOKEN = 0xC,       ///< CLR metadata token
        I386_SECREL7 = 0xD,     ///< 7-bit offset from base of sect
        I386_REL32 = 0x14,      ///< 32-bit PC-relative reference

        // AMD64 relocations
        AMD64_ADDR64 = 0x1,     ///< 64-bit address (VA)
        AMD64_ADDR32 = 0x2,     ///< 32-bit address (VA)
        AMD64_ADDR32NB = 0x3,   ///< 32-bit address w/o base (RVA)
        AMD64_REL32 = 0x4,      ///< 32-bit relative (0 byte dist)
        AMD64_REL32_1 = 0x5,    ///< 32-bit relative (1 byte dist)
        AMD64_REL32_2 = 0x6,    ///< 32-bit relative (2 byte dist)
        AMD64_REL32_3 = 0x7,    ///< 32-bit relative (3 byte dist)
        AMD64_REL32_4 = 0x8,    ///< 32-bit relative (4 byte dist)
        AMD64_REL32_5 = 0x9,    ///< 32-bit relative (5 byte dist)
        AMD64_SECTION = 0xA,    ///< section index
        AMD64_SECREL = 0xB,     ///< 32-bit offset from base of sect
        AMD64_SECREL7 = 0xC,    ///< 7-bit offset from base of sect
        AMD64_TOKEN = 0xD       ///< CLR metadata token
    };

    CoffReloc(const IntNum& addr, SymbolRef sym, Type type);
    virtual ~CoffReloc();

    virtual Expr get_value() const;
    virtual std::string get_type_name() const = 0;

    void write(Bytes& bytes) const;

protected:
    Type m_type;    ///< type of relocation
};

class Coff32Reloc : public CoffReloc
{
public:
    Coff32Reloc(const IntNum& addr, SymbolRef sym, Type type)
        : CoffReloc(addr, sym, type)
    {}
    virtual ~Coff32Reloc();

    virtual std::string get_type_name() const;
};

class Coff64Reloc : public CoffReloc
{
public:
    Coff64Reloc(const IntNum& addr, SymbolRef sym, Type type)
        : CoffReloc(addr, sym, type)
    {}
    virtual ~Coff64Reloc();

    virtual std::string get_type_name() const;
};

struct CoffSection : public AssocData
{
    static const char* key;

    CoffSection(SymbolRef sym);
    ~CoffSection();
    void put(marg_ostream& os) const;
    void write(Bytes& bytes, const Section& sect) const;

    static const unsigned long TEXT;
    static const unsigned long DATA;
    static const unsigned long BSS;
    static const unsigned long INFO;
    static const unsigned long STD_MASK;
    static const unsigned long ALIGN_MASK;
    static const unsigned int ALIGN_SHIFT;

    // Win32-specific flags
    static const unsigned long NRELOC_OVFL;
    static const unsigned long DISCARD;
    static const unsigned long NOCACHE;
    static const unsigned long NOPAGE;
    static const unsigned long SHARED;
    static const unsigned long EXECUTE;
    static const unsigned long READ;
    static const unsigned long WRITE;
    static const unsigned long WIN32_MASK;

    SymbolRef m_sym;            ///< symbol created for this section
    unsigned int m_scnum;       ///< section number (1=first section)
    unsigned long m_flags;      ///< section flags (see above)
    unsigned long m_size;       ///< size of raw data (section data) in bytes
    unsigned long m_relptr;     ///< file ptr to relocation
    unsigned long m_strtab_name;///< strtab offset of name if name > 8 chars
    bool m_nobase;              ///< Use no-base (NB) relocs?
    bool m_isdebug;             ///< is a debug section?
    bool m_setalign;            ///< Set alignment in section flags
};

inline CoffSection*
get_coff(Section& sect)
{
    return static_cast<CoffSection*>(sect.get_assoc_data(CoffSection::key));
}

inline const CoffSection*
get_coff(const Section& sect)
{
    return static_cast<const CoffSection*>
        (sect.get_assoc_data(CoffSection::key));
}

struct CoffSymbol : public AssocData
{
    static const char* key;

    enum StorageClass
    {
        SCL_EFCN = 0xff,    ///< physical end of function
        SCL_NULL = 0,
        SCL_AUTO = 1,       ///< automatic variable
        SCL_EXT = 2,        ///< external symbol
        SCL_STAT = 3,       ///< static
        SCL_REG = 4,        ///< register variable
        SCL_EXTDEF = 5,     ///< external definition
        SCL_LABEL = 6,      ///< label
        SCL_ULABEL = 7,     ///< undefined label
        SCL_MOS = 8,        ///< member of structure
        SCL_ARG = 9,        ///< function argument
        SCL_STRTAG = 10,    ///< structure tag
        SCL_MOU = 11,       ///< member of union
        SCL_UNTAG = 12,     ///< union tag
        SCL_TPDEF = 13,     ///< type definition
        SCL_USTATIC = 14,   ///< undefined static
        SCL_ENTAG = 15,     ///< enumeration tag
        SCL_MOE = 16,       ///< member of enumeration
        SCL_REGPARM = 17,   ///< register parameter
        SCL_FIELD = 18,     ///< bit field
        SCL_AUTOARG = 19,   ///< auto argument
        SCL_LASTENT = 20,   ///< dummy entry (end of block)
        SCL_BLOCK = 100,    ///< ".bb" or ".eb"
        SCL_FCN = 101,      ///< ".bf" or ".ef"
        SCL_EOS = 102,      ///< end of structure
        SCL_FILE = 103,     ///< file name
        SCL_LINE = 104,     ///< line # reformatted as symbol table entry
        SCL_ALIAS = 105,    ///< duplicate tag
        SCL_HIDDEN = 106    ///< ext symbol in dmert public lib
    };

    struct AuxEntry
    {
        std::string fname;      ///< filename aux entry data
    };

    enum AuxType
    {
        AUX_NONE = 0,
        AUX_SECT,
        AUX_FILE
    };

    CoffSymbol(StorageClass sclass, AuxType auxtype = AUX_NONE);
    ~CoffSymbol();
    void put(marg_ostream& os) const;
    void write(Bytes& bytes,
               const Symbol& sym,
               Errwarns& errwarns,
               StringTable& strtab) const;

    unsigned long m_index;          ///< assigned COFF symbol table index
    StorageClass m_sclass;          ///< storage class
    AuxType m_auxtype;              ///< type of aux entries
    std::vector<AuxEntry> m_aux;    ///< aux entries
};

inline CoffSymbol*
get_coff(Symbol& sym)
{
    return static_cast<CoffSymbol*>(sym.get_assoc_data(CoffSymbol::key));
}

inline const CoffSymbol*
get_coff(const Symbol& sym)
{
    return static_cast<const CoffSymbol*>(sym.get_assoc_data(CoffSymbol::key));
}

}}} // namespace yasm::objfmt::coff

#endif

#ifndef YASM_ELFSECTION_H
#define YASM_ELFSECTION_H
//
// ELF object format section
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
#include <iosfwd>
#include <vector>

#include "yasmx/Config/export.h"
#include "yasmx/AssocData.h"
#include "yasmx/IntNum.h"
#include "yasmx/Section.h"
#include "yasmx/SymbolRef.h"

#include "ElfTypes.h"


namespace llvm { class MemoryBuffer; }

namespace yasm
{

class Bytes;
class Diagnostic;
class Section;
class StringTable;

namespace objfmt
{

class YASM_STD_EXPORT ElfSection : public AssocData
{
public:
    static const char* key;

    // Constructor that reads from memory buffer.
    ElfSection(const ElfConfig&             config,
               const llvm::MemoryBuffer&    in,
               ElfSectionIndex              index,
               Diagnostic&                  diags);

    ElfSection(const ElfConfig&     config,
               ElfSectionType       type,
               ElfSectionFlags      flags,
               bool                 symtab = false);

    ~ElfSection();

#ifdef WITH_XML
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    unsigned long Write(llvm::raw_ostream& os, Bytes& scratch) const;

    std::auto_ptr<Section> CreateSection(const StringTable& shstrtab) const;
    bool LoadSectionData(Section& sect,
                         const llvm::MemoryBuffer& in,
                         Diagnostic& diags) const;

    ElfSectionType getType() const { return m_type; }

    void setName(ElfStringIndex index) { m_name_index = index; }
    ElfStringIndex getName() const { return m_name_index; }

    void setTypeFlags(ElfSectionType type, ElfSectionFlags flags)
    {
        m_type = type;
        m_flags = flags;
    }
    ElfSectionFlags getFlags() const { return m_flags; }

    bool isEmpty() const { return m_size.isZero(); }

    unsigned long getAlign() const { return m_align; }
    void setAlign(unsigned long align) { m_align = align; }

    ElfSectionIndex getIndex() const { return m_index; }

    void setInfo(ElfSectionInfo info) { m_info = info; }
    ElfSectionInfo getInfo() const { return m_info; }

    void setIndex(ElfSectionIndex sectidx) { m_index = sectidx; }

    void setLink(ElfSectionIndex link) { m_link = link; }
    ElfSectionIndex getLink() const { return m_link; }

    void setRelIndex(ElfSectionIndex sectidx) { m_rel_index = sectidx; }
    void setRelName(ElfStringIndex nameidx) { m_rel_name_index = nameidx; }

    void setEntSize(ElfSize size) { m_entsize = size; }
    ElfSize getEntSize() const { return m_entsize; }

    void AddSize(const IntNum& size) { m_size += size; }
    void setSize(const IntNum& size) { m_size = size; }
    IntNum getSize() const { return m_size; }

    unsigned long WriteRel(llvm::raw_ostream& os,
                           ElfSectionIndex symtab,
                           Section& sect,
                           Bytes& scratch);
    unsigned long WriteRelocs(llvm::raw_ostream& os,
                              Section& sect,
                              Bytes& scratch,
                              const ElfMachine& machine,
                              Diagnostic& diags);
    void ReadRelocs(const llvm::MemoryBuffer& in,
                    const ElfSection& reloc_sect,
                    Section& sect,
                    const ElfMachine& machine,
                    const ElfSymtab& symtab,
                    bool rela) const;

    unsigned long setFileOffset(unsigned long pos);
    unsigned long getFileOffset() const { return m_offset; }

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

    ElfStringIndex      m_name_index;
    ElfSectionIndex     m_index;

    ElfStringIndex      m_rel_name_index;
    ElfSectionIndex     m_rel_index;
    ElfAddress          m_rel_offset;
};

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

}} // namespace yasm::objfmt

#endif

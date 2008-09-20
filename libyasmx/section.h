#ifndef YASM_SECTION_H
#define YASM_SECTION_H
///
/// @file
/// @brief Section interface.
///
/// @license
///  Copyright (C) 2001-2007  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
///  - Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
///  - Redistributions in binary form must reproduce the above copyright
///    notice, this list of conditions and the following disclaimer in the
///    documentation and/or other materials provided with the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
/// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
/// @endlicense
///
#include <memory>
#include <string>

#include "assoc_data.h"
#include "bc_container.h"
#include "export.h"
#include "expr.h"
#include "intnum.h"
#include "marg_ostream_fwd.h"
#include "ptr_vector.h"
#include "symbolref.h"


namespace yasm
{

class Bytecode;
class Errwarns;
class Object;
class Reloc;

/// A section.
class YASM_LIB_EXPORT Section
    : public AssocDataContainer,
      public BytecodeContainer
{
public:
    /// Create a new section.  The section
    /// is added to the object if there's not already a section by that name.
    /// @param name     section name
    /// @param code     if true, section is intended to contain code
    ///                 (e.g. alignment should be made with NOP instructions)
    /// @param bss      if true, section is intended to contain only
    ///                 uninitialized space
    /// @param isnew    output; set to true if section did not already exist
    /// @param line     virtual line of section declaration (ignored if
    ///                 section already exists)
    Section(const std::string& name, bool code, bool bss, unsigned long line);

    ~Section();

    /// BytecodeContainer override that returns this.
    /// @return This section.
    Section* as_section();
    const Section* as_section() const;

    /// Determine if a section is flagged to contain code.
    /// @return True if section is flagged to contain code.
    bool is_code() const { return m_code; }

    /// Set section code flag to a new value.
    /// @param code     new value of code flag
    void set_code(bool code = true) { m_code = code; }

    /// Determine if a section is flagged to only contain uninitialized space.
    /// @return True if section is flagged to only contain uninitialized space.
    bool is_bss() const { return m_bss; }

    /// Set uninitialized space flag to a new value.
    /// @param code     new value of uninitialized space flag
    void set_bss(bool bss = true) { m_bss = bss; }

    /// Determine if a section was declared as the "default" section (e.g.
    /// not created through a section directive).
    /// @param sect     section
    /// @return True if section was declared as default.
    bool is_default() const { return m_def; }

    /// Set section "default" flag to a new value.
    /// @param def      new value of default flag
    void set_default(bool def = true) { m_def = def; }

    /// Add a relocation to a section.
    /// @param reloc        relocation
    void add_reloc(std::auto_ptr<Reloc> reloc);

    typedef stdx::ptr_vector<Reloc> Relocs;
    typedef Relocs::iterator reloc_iterator;
    typedef Relocs::const_iterator const_reloc_iterator;

    Relocs& get_relocs() { return m_relocs; }
    const Relocs& get_relocs() const { return m_relocs; }

    reloc_iterator relocs_begin() { return m_relocs.begin(); }
    const_reloc_iterator relocs_begin() const { return m_relocs.begin(); }
    reloc_iterator relocs_end() { return m_relocs.end(); }
    const_reloc_iterator relocs_end() const { return m_relocs.end(); }

    /// Get name of a section.
    /// @return Section name.
    const std::string& get_name() const { return m_name; }

    /// Match name of a section.
    /// @param Section name.
    /// @return True if section name matches, false if not.
    bool is_name(const std::string& name) const { return m_name == name; }

    /// Change alignment of a section.
    /// @param align    alignment in bytes
    /// @param line     virtual line
    void set_align(unsigned long align) { m_align = align; }

    /// Get alignment of a section.
    /// @return Alignment in bytes (0 if none).
    unsigned long get_align() const { return m_align; }

    /// Get virtual memory address (VMA).
    /// @return VMA.
    IntNum get_vma() const { return m_vma; }

    /// Set virtual memory address (VMA).
    /// @param vma      VMA
    void set_vma(const IntNum& vma) { m_vma = vma; }

    /// Get load memory address (LMA).
    /// @return LMA.
    IntNum get_lma() const { return m_lma; }

    /// Set load memory address (LMA).
    /// @param lma      LMA
    void set_lma(const IntNum& lma) { m_lma = lma; }

    /// Get file position of section data.
    /// @return File position.
    unsigned long get_filepos() const { return m_filepos; }

    /// Set file position of section data.
    /// Generally should only be used by ObjectFormat.
    /// @param filepos  File position
    void set_filepos(unsigned long filepos) { m_filepos = filepos; }

    /// Print a section.  For debugging purposes.
    /// @param os           output stream
    /// @param with_bcs     if true, print bytecodes within section
    void put(marg_ostream& os, bool with_bcs=false) const;

private:
    std::string m_name;                 ///< name (given by user)

    IntNum m_vma;               ///< Virtual Memory Address (VMA)
    IntNum m_lma;               ///< Load Memory Address (LMA)

    unsigned long m_filepos;    ///< File position of section data

    unsigned long m_align;      ///< Section alignment

    bool m_code;                ///< section contains code (instructions)
    bool m_bss;         ///< section should contain only uninitialized space

    /// "Default" section, e.g. not specified by using section directive.
    bool m_def;

    /// The relocations for the section.
    Relocs m_relocs;
    stdx::ptr_vector_owner<Reloc> m_relocs_owner;
};

inline marg_ostream&
operator<< (marg_ostream& os, const Section& section)
{
    section.put(os);
    return os;
}

} // namespace yasm

#endif

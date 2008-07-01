#ifndef YASM_SECTION_H
#define YASM_SECTION_H
///
/// @file libyasm/section.h
/// @brief YASM section interface.
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

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include "assoc_data.h"
#include "bc_container.h"
#include "export.h"
#include "marg_ostream_fwd.h"
#include "ptr_vector.h"


namespace yasm
{

class Bytecode;
class Errwarns;
class IntNum;
class Object;
class Symbol;

/// Basic YASM relocation.  Object formats will need to extend this
/// structure with additional fields for relocation type, etc.
class YASM_LIB_EXPORT Reloc
{
public:
    Reloc(std::auto_ptr<IntNum> addr, Symbol* sym);
    virtual ~Reloc();

protected:
    boost::scoped_ptr<IntNum> m_addr;   ///< Offset (address) within section
    Symbol* m_sym;                      ///< Relocated symbol
};

/// A section.
class YASM_LIB_EXPORT Section
    : public AssocDataContainer,
      public BytecodeContainer
{
public:
    /// Create a new section.  The section
    /// is added to the object if there's not already a section by that name.
    /// @param name     section name
    /// @param align    alignment in bytes (0 if none)
    /// @param code     if true, section is intended to contain code
    ///                 (e.g. alignment should be made with NOP instructions)
    /// @param res_only if true, only space-reserving bytecodes are allowed
    ///                 in the section (ignored if section already exists)
    /// @param isnew    output; set to true if section did not already exist
    /// @param line     virtual line of section declaration (ignored if
    ///                 section already exists)
    Section(const std::string& name,
            unsigned long align,
            bool code,
            bool res_only,
            unsigned long line);

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
    void add_reloc(std::auto_ptr<Reloc> reloc)
    { m_relocs.push_back(reloc.release()); }

    typedef stdx::ptr_vector<Reloc>::iterator reloc_iterator;
    typedef stdx::ptr_vector<Reloc>::const_iterator const_reloc_iterator;

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

    /// Print a section.  For debugging purposes.
    /// @param os           output stream
    /// @param with_bcs     if true, print bytecodes within section
    void put(marg_ostream& os, bool with_bcs=false) const;

private:
    std::string m_name;                 ///< name (given by user)

    unsigned long m_align;      ///< Section alignment

    bool m_code;                ///< section contains code (instructions)
    bool m_res_only;            ///< allow only resb family of bytecodes?

    /// "Default" section, e.g. not specified by using section directive.
    bool m_def;

    /// The relocations for the section.
    stdx::ptr_vector<Reloc> m_relocs;
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

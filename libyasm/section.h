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
#include <iosfwd>
#include <memory>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include "assoc_data.h"
#include "ptr_vector.h"


namespace yasm {

class Bytecode;
class Errwarns;
class Expr;
class IntNum;
class Object;
class Symbol;

/// Basic YASM relocation.  Object formats will need to extend this
/// structure with additional fields for relocation type, etc.
class Reloc {
public:
    Reloc(std::auto_ptr<IntNum> addr, Symbol* sym);
    virtual ~Reloc();

protected:
    boost::scoped_ptr<IntNum> m_addr;   ///< Offset (address) within section
    Symbol* m_sym;                      ///< Relocated symbol
};

/// A section.
class Section : private boost::noncopyable, public AssocDataContainer {
    friend class Object;

public:
    /// Create a new section.  The section
    /// is added to the object if there's not already a section by that name.
    /// @param name     section name
    /// @param start    starting address (ignored if section already exists),
    ///                 NULL if 0 or don't care.
    /// @param align    alignment in bytes (0 if none)
    /// @param code     if true, section is intended to contain code
    ///                 (e.g. alignment should be made with NOP instructions)
    /// @param res_only if true, only space-reserving bytecodes are allowed
    ///                 in the section (ignored if section already exists)
    /// @param isnew    output; set to true if section did not already exist
    /// @param line     virtual line of section declaration (ignored if
    ///                 section already exists)
    Section(const std::string& name,
            std::auto_ptr<Expr> start,
            unsigned long align,
            bool code,
            bool res_only,
            unsigned long line);

    ~Section();

    /// Determine if a section is flagged to contain code.
    /// @return True if section is flagged to contain code.
    bool is_code() const { return m_code; }

    /// Determine if a section was declared as the "default" section (e.g.
    /// not created through a section directive).
    /// @param sect     section
    /// @return True if section was declared as default.
    bool is_default() const { return m_def; }

    /// Set section "default" flag to a new value.
    /// @param def      new value of default flag
    void set_default(bool def = true) { m_def = def; }

    /// Get object owner of a section.
    /// @return Object this section is a part of.
    Object* get_object() const { return m_object; }

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

    /// Add bytecode to the end of a section.
    /// @param bc       bytecode (may be NULL)
    void append_bytecode(/*@null@*/ std::auto_ptr<Bytecode> bc);

    /// Start a new bytecode at the end of a section.  Factory function.
    /// @return Reference to new bytecode.
    Bytecode& start_bytecode();

    typedef stdx::ptr_vector<Bytecode>::iterator bc_iterator;
    typedef stdx::ptr_vector<Bytecode>::const_iterator const_bc_iterator;

    bc_iterator bcs_begin() { return m_bcs.begin(); }
    const_bc_iterator bcs_begin() const { return m_bcs.begin(); }
    bc_iterator bcs_end() { return m_bcs.end(); }
    const_bc_iterator bcs_end() const { return m_bcs.end(); }

    Bytecode& bcs_first() { return m_bcs.front(); }
    const Bytecode& bcs_first() const { return m_bcs.front(); }
    Bytecode& bcs_last() { return m_bcs.back(); }
    const Bytecode& bcs_last() const { return m_bcs.back(); }

    /// Get name of a section.
    /// @return Section name.
    const std::string& get_name() const { return m_name; }

    /// Match name of a section.
    /// @param Section name.
    /// @return True if section name matches, false if not.
    bool is_name(const std::string& name) const { return m_name == name; }

    /// Change starting address of a section.
    /// @param start    starting address
    void set_start(std::auto_ptr<Expr> start);

    /// Get starting address of a section.
    /// @return Starting address.
    const Expr* get_start() const { return m_start.get(); }

    /// Change alignment of a section.
    /// @param align    alignment in bytes
    /// @param line     virtual line
    void set_align(unsigned long align) { m_align = align; }

    /// Get alignment of a section.
    /// @return Alignment in bytes (0 if none).
    unsigned long get_align() const { return m_align; }

    /// Print a section.  For debugging purposes.
    /// @param os           output stream
    /// @param indent_level indentation level
    /// @param with_bcs     if true, print bytecodes within section
    void put(std::ostream& os, int indent_level, bool with_bcs) const;

    /// Finalize a section after parsing.
    /// @param errwarns     error/warning set
    /// @note Errors/warnings are stored into errwarns.
    void finalize(Errwarns& errwarns);

    /// Updates all bytecode offsets in section.
    /// @param errwarns     error/warning set
    /// @note Errors/warnings are stored into errwarns.
    void update_bc_offsets(Errwarns& errwarns);

private:
    /*@dependent@*/ Object* m_object;   ///< Pointer to parent object

    std::string m_name;                 ///< name (given by user)

    /// Starting address of section contents.
    boost::scoped_ptr<Expr> m_start;

    unsigned long m_align;      ///< Section alignment

    bool m_code;                ///< section contains code (instructions)
    bool m_res_only;            ///< allow only resb family of bytecodes?

    /// "Default" section, e.g. not specified by using section directive.
    bool m_def;

    /// The bytecodes for the section's contents.
    stdx::ptr_vector<Bytecode> m_bcs;
    stdx::ptr_vector_owner<Bytecode> m_bcs_owner;

    /// The relocations for the section.
    stdx::ptr_vector<Reloc> m_relocs;
    stdx::ptr_vector_owner<Reloc> m_relocs_owner;
};

} // namespace yasm

#endif

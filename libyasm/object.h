#ifndef YASM_OBJECT_H
#define YASM_OBJECT_H
///
/// @file libyasm/object.h
/// @brief YASM object interface.
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
#include <iostream>
#include <memory>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include "ptr_vector.h"


namespace yasm {

class Arch;
class DebugFormat;
class Errwarns;
class ObjectFormat;
class Section;
class Symbol;

/// An object.  This is the internal representation of an object file.
class Object : private boost::noncopyable {
public:
    /// Constructor.  A default section is created as the first
    /// section.  An empty symbol table and line mapping are also
    /// automatically created.
    /// @param src_filename     source filename (e.g. "file.asm")
    /// @param obj_filename     object filename (e.g. "file.o")
    /// @param arch             architecture
    /// @param objfmt_module    object format module
    /// @param dbgfmt_module    debug format module
    Object(const std::string& src_filename,
           const std::string& obj_filename,
           std::auto_ptr<Arch> arch,
           const std::string& objfmt_keyword,
           const std::string& dbgfmt_keyword);

    /// Destructor.
    ~Object();

    /// Print an object.  For debugging purposes.
    /// @param os           output stream
    /// @param indent_level indentation level
    void put(std::ostream& os, int indent_level) const;

    /// Finalize an object after parsing.
    /// @param errwarns     error/warning set
    /// @note Errors/warnings are stored into errwarns.
    void finalize(Errwarns& errwarns);

    /// Change the source filename for an object.
    /// @param src_filename new source filename (e.g. "file.asm")
    void set_source_fn(const std::string& src_filename);

    /// Optimize an object.  Takes the unoptimized object and optimizes it.
    /// If successful, the object is ready for output to an object file.
    /// @param errwarns     error/warning set
    /// @note Optimization failures are stored into errwarns.
    void optimize(Errwarns& errwarns);

    /// Updates all bytecode offsets in object.
    /// @param errwarns     error/warning set
    /// @note Errors/warnings are stored into errwarns.
    void update_bc_offsets(Errwarns& errwarns);

    // Section functions

    /// Add a new section.  Does /not/ check to see if there's already
    /// an existing section in the object with that name.  The caller
    /// should first call find_section() if only unique names
    /// are desired.
    /// @param sect         section
    void append_section(std::auto_ptr<Section> sect);

    /// Find a general section in an object, based on its name.
    /// @param name         section name
    /// @return Section matching name, or NULL if no match found.
    /*@null@*/ Section* find_section(const std::string& name);

    typedef stdx::ptr_vector<Section>::iterator section_iterator;
    typedef stdx::ptr_vector<Section>::const_iterator const_section_iterator;

    section_iterator sections_begin() { return m_sections.begin(); }
    const_section_iterator sections_begin() const
    { return m_sections.begin(); }

    section_iterator sections_end() { return m_sections.end(); }
    const_section_iterator sections_end() const { return m_sections.end(); }

    // Symbol functions

    /// Get the object's "absolute" symbol.  This is
    /// essentially an EQU with no name and value 0, and is used for
    /// relocating absolute current-position-relative values.
    /// @see Value::set_curpos_rel().
    /// @return Absolute symbol.
    Symbol* get_abs_sym();

    /// Find a symbol by name.
    /// @param name         symbol name
    /// @return Symbol matching name, or NULL if no match found.
    /*@null@*/ Symbol* find_sym(const std::string& name);

    /// Get (creating if necessary) a symbol by name.
    /// @param name         symbol name
    /// @return Symbol matching name.
    Symbol* get_sym(const std::string& name);

    typedef stdx::ptr_vector<Symbol>::iterator symbol_iterator;
    typedef stdx::ptr_vector<Symbol>::const_iterator const_symbol_iterator;

    symbol_iterator begin() { return m_symbols.begin(); }
    const_symbol_iterator begin() const { return m_symbols.begin(); }

    symbol_iterator end() { return m_symbols.end(); }
    const_symbol_iterator end() const { return m_symbols.end(); }

    /// Add an arbitrary symbol to the end of the symbol table.
    /// @note Does /not/ index the symbol by name.
    /// @param sym      symbol
    void append_symbol(std::auto_ptr<Symbol> sym);

    /// Have the object manage an arbitrary symbol.  Useful for symbols
    /// that shouldn't be in the table, but need to have memory management
    /// tied up with the object (such as curpos symbols).
    /// @param sym      symbol
    void add_non_table_symbol(std::auto_ptr<Symbol> sym);

    /// Finalize symbol table after parsing stage.  Checks for symbols that
    /// are used but never defined or declared #EXTERN or #COMMON.
    /// @param errwarns     error/warning set
    /// @param undef_extern if true, all undef syms should be declared extern
    /// @note Errors/warnings are stored into errwarns.
    void symbols_finalize(Errwarns& errwarns, bool undef_extern);

private:
    std::string m_src_filename;         ///< Source filename
    std::string m_obj_filename;         ///< Object filename

    // /*@owned@*/ yasm_symtab *symtab;         ///< Symbol table
    boost::scoped_ptr<Arch> m_arch;             ///< Target architecture
    boost::scoped_ptr<ObjectFormat> m_objfmt;   ///< Object format
    boost::scoped_ptr<DebugFormat> m_dbgfmt;    ///< Debug format

    /// Currently active section.  Used by some directives.  NULL if no
    /// section active.
    /*@null@*/ Section* m_cur_section;

    /// Sections
    stdx::ptr_vector<Section> m_sections;
    stdx::ptr_vector_owner<Section> m_sections_owner;

    /// Symbols in the symbol table
    stdx::ptr_vector<Symbol> m_symbols;
    stdx::ptr_vector_owner<Symbol> m_symbols_owner;

    /// Non-table symbols
    stdx::ptr_vector<Symbol> m_non_table_syms;
    stdx::ptr_vector_owner<Symbol> m_non_table_syms_owner;

    /// Pimpl for symbol table hash trie.
    class Impl;
    boost::scoped_ptr<Impl> m_impl;
};

} // namespace yasm

#endif
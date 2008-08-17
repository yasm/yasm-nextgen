#ifndef YASM_OBJECT_H
#define YASM_OBJECT_H
///
/// @file
/// @brief Object interface.
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

#include "export.h"
#include "marg_ostream_fwd.h"
#include "ptr_vector.h"
#include "symbolref.h"


namespace yasm
{

class Arch;
class Errwarns;
class Section;
class Symbol;

/// An object.  This is the internal representation of an object file.
class YASM_LIB_EXPORT Object : private boost::noncopyable
{
    friend YASM_LIB_EXPORT
    marg_ostream& operator<< (marg_ostream& os, const Object& object);

public:
    /// Constructor.  A default section is created as the first
    /// section, and an empty symbol table is created.
    /// The object filename is initially unset (empty string).
    /// @param src_filename     source filename (e.g. "file.asm")
    /// @param obj_filename     object filename (e.g. "file.o")
    /// @param arch             architecture
    Object(const std::string& src_filename,
           const std::string& obj_filename,
           Arch* arch);

    /// Destructor.
    ~Object();

    /// Finalize an object after parsing.
    /// @param errwarns     error/warning set
    /// @note Errors/warnings are stored into errwarns.
    void finalize(Errwarns& errwarns);

    /// Change the source filename for an object.
    /// @param src_filename new source filename (e.g. "file.asm")
    void set_source_fn(const std::string& src_filename);

    /// Change the object filename for an object.
    /// @param obj_filename new object filename (e.g. "file.o")
    void set_object_fn(const std::string& obj_filename);

    /// Get the source filename for an object.
    /// @return Source filename.
    std::string get_source_fn() const { return m_src_filename; }

    /// Get the object filename for an object.
    /// @return Object filename.
    std::string get_object_fn() const { return m_obj_filename; }

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

    typedef stdx::ptr_vector<Section> Sections;
    typedef Sections::iterator section_iterator;
    typedef Sections::const_iterator const_section_iterator;

    /// Get a section by index.
    /// @param n            section index
    /// @return Section at index.
    /// Can raise std::out_of_range exception if index out of range.
    Section& get_section(Sections::size_type n) { return m_sections.at(n); }

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
    SymbolRef get_abs_sym();

    /// Find a symbol by name.
    /// @param name         symbol name
    /// @return Symbol matching name, or NULL if no match found.
    SymbolRef find_sym(const std::string& name);

    /// Get (creating if necessary) a symbol by name.
    /// @param name         symbol name
    /// @return Symbol matching name.
    SymbolRef get_sym(const std::string& name);

    typedef stdx::ptr_vector<Symbol> Symbols;
    typedef Symbols::iterator symbol_iterator;
    typedef Symbols::const_iterator const_symbol_iterator;

    /// Get a symbol by index.
    /// @param n            symbol index
    /// @return Symbol at index.
    /// Can raise std::out_of_range exception if index out of range.
    SymbolRef get_sym(Symbols::size_type n)
    { return SymbolRef(&(m_symbols.at(n))); }

    symbol_iterator symbols_begin() { return m_symbols.begin(); }
    const_symbol_iterator symbols_begin() const { return m_symbols.begin(); }

    symbol_iterator symbols_end() { return m_symbols.end(); }
    const_symbol_iterator symbols_end() const { return m_symbols.end(); }

    /// Add an arbitrary symbol to the end of the symbol table.
    /// @note Does /not/ index the symbol by name.
    /// @param sym      symbol
    /// @return Reference to symbol.
    SymbolRef append_symbol(std::auto_ptr<Symbol> sym);

    /// Have the object manage an arbitrary symbol.  Useful for symbols
    /// that shouldn't be in the table, but need to have memory management
    /// tied up with the object (such as curpos symbols).
    /// @param sym      symbol
    /// @return Reference to symbol.
    SymbolRef add_non_table_symbol(std::auto_ptr<Symbol> sym);

    /// Finalize symbol table after parsing stage.  Checks for symbols that
    /// are used but never defined or declared #EXTERN or #COMMON.
    /// @param errwarns     error/warning set
    /// @param undef_extern if true, all undef syms should be declared extern
    /// @note Errors/warnings are stored into errwarns.
    void symbols_finalize(Errwarns& errwarns, bool undef_extern);

    /// Add a special symbol.
    /// @param parser   parser
    /// @param sym      symbol
    /// @return Reference to symbol.
    SymbolRef add_special_sym(const std::string& parser,
                              std::auto_ptr<Symbol> sym);

    /// Find a special symbol.  Special symbols are generally used to generate
    /// special relocation types via the WRT mechanism.
    /// @note Default implementation always returns NULL.
    /// @param name         symbol name (not including any parser-specific
    ///                     prefix)
    /// @param parser       parser keyword
    /// @return NULL if unrecognized, otherwise special symbol.
    SymbolRef find_special_sym(const std::string& name,
                               const std::string& parser);

    /*@null@*/ Section* get_cur_section() { return m_cur_section; }
    const /*@null@*/ Section* get_cur_section() const { return m_cur_section; }
    void set_cur_section(/*@null@*/ Section* section)
    { m_cur_section = section; }

    Arch* get_arch() { return m_arch; }
    const Arch* get_arch() const { return m_arch; }

private:
    std::string m_src_filename;         ///< Source filename
    std::string m_obj_filename;         ///< Object filename

    Arch* m_arch;                       ///< Target architecture

    /// Currently active section.  Used by some directives.  NULL if no
    /// section active.
    /*@null@*/ Section* m_cur_section;

    /// Sections
    Sections m_sections;
    stdx::ptr_vector_owner<Section> m_sections_owner;

    /// Symbols in the symbol table
    Symbols m_symbols;
    stdx::ptr_vector_owner<Symbol> m_symbols_owner;

    /// Non-table symbols
    Symbols m_non_table_syms;
    stdx::ptr_vector_owner<Symbol> m_non_table_syms_owner;

    /// Pimpl for symbol table hash trie.
    class Impl;
    boost::scoped_ptr<Impl> m_impl;
};

/// Print an object.  For debugging purposes.
/// @param os           output stream
/// @param object       object
YASM_LIB_EXPORT
marg_ostream& operator<< (marg_ostream& os, const Object& object);

} // namespace yasm

#endif

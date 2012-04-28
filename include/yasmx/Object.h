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

#include "llvm/ADT/StringRef.h"
#include "yasmx/Config/export.h"
#include "yasmx/Support/ptr_vector.h"
#include "yasmx/Support/scoped_ptr.h"
#include "yasmx/DebugDumper.h"
#include "yasmx/Location.h"
#include "yasmx/SymbolRef.h"


namespace llvm { class Twine; }

namespace yasm
{

class Arch;
class Diagnostic;
class Section;
class Symbol;

/// An object.  This is the internal representation of an object file.
class YASM_LIB_EXPORT Object
{
public:
    /// Options to control behavior of various functions globally for
    /// this object.
    struct Options
    {
        /// Don't allow Value::SubRelative() to move global symbols into
        /// the absolute portion of the value.  This forces a relocation
        /// to be generated even if the symbol is in the same section as
        /// the value.  Defaults to false.
        bool DisableGlobalSubRelative;

        /// Alignment directives specify power-of-2.  Defaults to false.
        bool PowerOfTwoAlignment;
    };

    /// Generic object configuration.
    struct Config
    {
        /// Advise linker that stack should be executable.
        /// Defaults to false.
        bool ExecStack;

        /// Advise linker that stack should be non-executable.
        /// Defaults to false.
        bool NoExecStack;
    };

    /// Constructor.  A default section is created as the first
    /// section, and an empty symbol table is created.
    /// The object filename is initially unset (empty string).
    /// @param src_filename     source filename (e.g. "file.asm")
    /// @param obj_filename     object filename (e.g. "file.o")
    /// @param arch             architecture
    Object(llvm::StringRef src_filename,
           llvm::StringRef obj_filename,
           Arch* arch);

    /// Destructor.
    ~Object();

    /// Finalize an object after parsing.
    /// @param diags    diagnostic reporting
    void Finalize(Diagnostic& diags);

    /// Change the source filename for an object.
    /// @param src_filename new source filename (e.g. "file.asm")
    void setSourceFilename(llvm::StringRef src_filename);

    /// Change the object filename for an object.
    /// @param obj_filename new object filename (e.g. "file.o")
    void setObjectFilename(llvm::StringRef obj_filename);

    /// Get the source filename for an object.
    /// @return Source filename.
    llvm::StringRef getSourceFilename() const { return m_src_filename; }

    /// Get the object filename for an object.
    /// @return Object filename.
    llvm::StringRef getObjectFilename() const { return m_obj_filename; }

    Options& getOptions() { return m_options; }
    Config& getConfig() { return m_config; }

    /// Optimize an object.  Takes the unoptimized object and optimizes it.
    /// If successful, the object is ready for output to an object file.
    /// @param diags    diagnostic reporting
    void Optimize(Diagnostic& diags);

    /// Updates all bytecode offsets in object.
    /// @param diags    diagnostic reporting
    void UpdateBytecodeOffsets(Diagnostic& diags);

    // Section functions

    /// Add a new section.  Does /not/ check to see if there's already
    /// an existing section in the object with that name.  The caller
    /// should first call FindSection() if only unique names
    /// are desired.
    /// @param sect         section
    void AppendSection(std::auto_ptr<Section> sect);

    /// Find a general section in an object, based on its name.
    /// @param name         section name
    /// @return Section matching name, or NULL if no match found.
    /*@null@*/ Section* FindSection(llvm::StringRef name);

    typedef stdx::ptr_vector<Section> Sections;
    typedef Sections::iterator section_iterator;
    typedef Sections::const_iterator const_section_iterator;

    /// Get a section by index.
    /// @param n            section index
    /// @return Section at index.
    /// Can raise std::out_of_range exception if index out of range.
    Section& getSection(Sections::size_type n) { return m_sections.at(n); }

    /// Get number of sections.
    /// @return Number of sections.
    Sections::size_type getNumSections() const { return m_sections.size(); }

    section_iterator sections_begin() { return m_sections.begin(); }
    const_section_iterator sections_begin() const
    { return m_sections.begin(); }

    section_iterator sections_end() { return m_sections.end(); }
    const_section_iterator sections_end() const { return m_sections.end(); }

    // Symbol functions

    /// Get the object's "absolute" symbol.  This is
    /// essentially an EQU with no name and value 0, and is used for
    /// relocating subtractive relative values.
    /// @see Value::SubRelative().
    /// @return Absolute symbol.
    SymbolRef getAbsoluteSymbol();

    /// Find a symbol by name.
    /// @param name         symbol name
    /// @return Symbol matching name, or NULL if no match found.
    SymbolRef FindSymbol(llvm::StringRef name);

    /// Get (creating if necessary) a symbol by name.
    /// @param name         symbol name
    /// @return Symbol matching name.
    SymbolRef getSymbol(llvm::StringRef name);

    /// Get a symbol for an arbitrary location.  The symbol may already exist,
    /// or a new unnamed one may be created.
    /// @param loc          location
    SymbolRef getSymbol(Location loc);

    typedef stdx::ptr_vector<Symbol> Symbols;
    typedef Symbols::iterator symbol_iterator;
    typedef Symbols::const_iterator const_symbol_iterator;

    /// Get a symbol by index.
    /// @param n            symbol index
    /// @return Symbol at index.
    /// Can raise std::out_of_range exception if index out of range.
    SymbolRef getSymbol(Symbols::size_type n)
    { return SymbolRef(&(m_symbols.at(n))); }

    symbol_iterator symbols_begin() { return m_symbols.begin(); }
    const_symbol_iterator symbols_begin() const { return m_symbols.begin(); }

    symbol_iterator symbols_end() { return m_symbols.end(); }
    const_symbol_iterator symbols_end() const { return m_symbols.end(); }

    /// Add an arbitrary symbol to the end of the symbol table.
    /// @note Does /not/ index the symbol by name.
    /// @param name     symbol name
    /// @return Reference to symbol.
    SymbolRef AppendSymbol(llvm::StringRef name);

    /// Have the object manage an arbitrary symbol.
    /// @note Does /not/ index the symbol by name.
    /// @param name     symbol name
    /// @return Reference to symbol.
    SymbolRef AddNonTableSymbol(llvm::StringRef name);

    /// Rename a symbol.
    void RenameSymbol(SymbolRef sym, llvm::StringRef name);

    /// Declare all used but undefined symbols extern.
    void ExternUndefinedSymbols();

    /// Finalize symbol table after parsing stage.  Checks for symbols that
    /// are used but never defined or declared #Symbol::EXTERN or
    /// #Symbol::COMMON.
    /// @param diags        diagnostic reporting
    void FinalizeSymbols(Diagnostic& diags);

    /// Add a special symbol.
    /// @param sym      symbol name
    /// @return Reference to symbol.
    SymbolRef AddSpecialSymbol(llvm::StringRef name);

    /// Find a special symbol.  Special symbols are generally used to generate
    /// special relocation types via the WRT mechanism.
    /// @note Default implementation always returns NULL.
    /// @param name         symbol name (not including any parser-specific
    ///                     prefix)
    /// @return NULL if unrecognized, otherwise special symbol.
    SymbolRef FindSpecialSymbol(llvm::StringRef name);

    /*@null@*/ Section* getCurSection() { return m_cur_section; }
    const /*@null@*/ Section* getCurSection() const { return m_cur_section; }
    void setCurSection(/*@null@*/ Section* section)
    { m_cur_section = section; }

    Arch* getArch() { return m_arch; }
    const Arch* getArch() const { return m_arch; }

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

private:
    Object(const Object&);                  // not implemented
    const Object& operator=(const Object&); // not implemented

    std::string m_src_filename;         ///< Source filename
    std::string m_obj_filename;         ///< Object filename

    Options m_options;                  ///< Object options
    Config m_config;                    ///< Object configuration

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

    /// Pimpl for symbol table hash trie.
    class Impl;
    util::scoped_ptr<Impl> m_impl;
};

} // namespace yasm

#endif

#ifndef YASM_SYMBOL_H
#define YASM_SYMBOL_H
///
/// @file
/// @brief Symbol interface.
///
/// @license
///  Copyright (C) 2001-2007  Michael Urman, Peter Johnson
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
///  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
/// @endlicense
///
#include <string>

#include "llvm/ADT/StringRef.h"
#include "yasmx/Basic/LLVM.h"
#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Config/export.h"
#include "yasmx/Support/scoped_ptr.h"
#include "yasmx/AssocData.h"
#include "yasmx/Location.h"


namespace yasm
{

class DiagnosticsEngine;
class Expr;
class Object;

/// A symbol.
class YASM_LIB_EXPORT Symbol : public AssocDataContainer
{
    friend class Object;

public:
    /// Constructor.
    explicit Symbol(StringRef name);

    /// Destructor.
    ~Symbol();

    /// Symbol status.  #DEFINED is set by define_label(), define_equ(), or
    /// declare(), with a visibility of #EXTERN or #COMMON.
    enum Status
    {
        NOSTATUS = 0,           ///< no status
        USED = 1 << 0,          ///< for use before definition
        DEFINED = 1 << 1,       ///< once it's been defined in the file
        VALUED = 1 << 2         ///< once its value has been determined
    };

    /// Symbol record visibility.
    /// @note #EXTERN and #COMMON are mutually exclusive.
    enum Visibility
    {
        LOCAL = 0,              ///< Default, local only
        GLOBAL = 1 << 0,        ///< If symbol is declared GLOBAL
        COMMON = 1 << 1,        ///< If symbol is declared COMMON
        EXTERN = 1 << 2,        ///< If symbol is declared EXTERN
        DLOCAL = 1 << 3         ///< If symbol is explicitly declared LOCAL
    };

    /// Get the name of a symbol.
    /// @return Symbol name.
    StringRef getName() const { return m_name; }

    /// Get the visibility of a symbol.
    /// @return Symbol visibility.
    int getVisibility() const { return m_visibility; }

    bool isUsed() const { return (m_status & USED) ? true : false; }
    bool isDefined() const { return (m_status & DEFINED) ? true : false; }
    bool isValued() const { return (m_status & VALUED) ? true : false; }

    /// Set the source location where a symbol was first defined.
    /// @param source   source location
    void setDefSource(SourceLocation source) { m_def_source = source; }

    /// Set the source location where a symbol was first declared.
    /// @param source   source location
    void setDeclSource(SourceLocation source) { m_decl_source = source; }

    /// Get the source location where a symbol was first defined.
    /// @return Source location.
    SourceLocation getDefSource() const { return m_def_source; }

    /// Get the source location where a symbol was first declared.
    /// @return Source location.
    SourceLocation getDeclSource() const { return m_decl_source; }

    /// Get the source location where a symbol was first used.
    /// @return Source location.
    SourceLocation getUseSource() const { return m_use_source; }

    /// Get EQU value of a symbol.
    /// @return EQU value, or NULL if symbol is not an EQU or is not defined.
    /*@null@*/ const Expr* getEqu() const;

    /// Get the label location of a symbol.
    /// @param sym       symbol
    /// @param loc       label location (output)
    /// @return False if not symbol is not a label or if the symbol's
    ///         visibility is #EXTERN or #COMMON (not defined in the file).
    bool getLabel(/*@out@*/ Location* loc) const;

    /// Determine if symbol is the "absolute" symbol created by
    /// yasm_symtab_abs_sym().
    /// @return False if symbol is not the "absolute" symbol, true otherwise.
    bool isAbsoluteSymbol() const
    {
        return !m_def_source.isValid() && m_type == EQU && m_name.empty();
    }

    /// Determine if symbol is a special symbol.
    /// @return False if symbol is not a special symbol, true otherwise.
    bool isSpecial() const { return m_type == SPECIAL; }

    /// Mark the symbol as used.  The symbol does not necessarily need to
    /// be defined before it is used.
    /// @param source   source location
    void Use(SourceLocation source);

    /// Define as an EQU value.
    /// @note Asserts if already defined.
    /// @param e        EQU value (expression)
    /// @param source   source location
    void DefineEqu(const Expr& e);

    /// Define as an EQU value.  Reports diagnostics if already defined.
    /// @param e        EQU value (expression)
    /// @param source   source location
    /// @param diags    diagnostic reporting
    void CheckedDefineEqu(const Expr& e,
                          SourceLocation source,
                          DiagnosticsEngine& diags);

    /// Define as a label.
    /// @note Asserts if already defined.
    /// @param loc      location of label
    /// @param source   source location
    void DefineLabel(Location loc);

    /// Define as a label.  Reports diagnostics if already defined.
    /// @param loc      location of label
    /// @param source   source location
    void CheckedDefineLabel(Location loc,
                            SourceLocation source,
                            DiagnosticsEngine& diags);

    /// Define a special symbol.  Special symbols have no generic associated
    /// data (such as an expression or precbc).
    /// @note Asserts if already defined.
    /// @param vis      symbol visibility
    void DefineSpecial(Symbol::Visibility vis);

    /// Determine if external visibility is compatible with symbol status.
    /// @param vis      visibility
    bool okToDeclare(Visibility vis) const;

    /// Declare external visibility.  Asserts if incompatible visibility.
    /// @param vis      visibility
    /// @param source   source location
    void Declare(Visibility vis);

    /// Declare external visibility.  Reports diagnostics for incompatible
    /// visibility.
    /// @param vis      visibility
    /// @param source   source location
    /// @param diags    diagnostic reporting
    void CheckedDeclare(Visibility vis,
                        SourceLocation source,
                        DiagnosticsEngine& diags);

    /// Determine if symbol is used but is undefined.  Undefined symbols are
    /// those that are used but never defined or declared #EXTERN or #COMMON.
    bool isUndefined() const
    {
        return ((m_status & USED) && !(m_status & DEFINED) &&
                !(m_visibility & (EXTERN | COMMON)));
    }

    /// Declare a used but undefined symbol extern.
    /// Intended for use after parsing stage.
    void ExternUndefined()
    {
        if (isUndefined())
            m_visibility |= EXTERN;
    }

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

private:
    Symbol(const Symbol&);                  // not implemented
    const Symbol& operator=(const Symbol&); // not implemented

    enum Type
    {
        UNKNOWN,    ///< for unknown type (COMMON/EXTERN)
        EQU,        ///< for EQU defined symbols (expressions)
        LABEL,      ///< for labels

        /// For special symbols that need to be in the symbol table but
        /// otherwise have no purpose.
        SPECIAL
    };

    bool DefineCheck(SourceLocation source, DiagnosticsEngine& diags) const;

    std::string m_name;
    Type m_type;
    int m_status;
    int m_visibility;
    SourceLocation m_def_source;    ///< where symbol was first defined
    SourceLocation m_decl_source;   ///< where symbol was first declared
    SourceLocation m_use_source;    ///< where symbol was first used

    // Possible data

    util::scoped_ptr<Expr> m_equ;   ///< EQU value

    /// Label location
    Location m_loc;
};

inline void
Symbol::Use(SourceLocation source)
{
    if (!m_use_source.isValid())
        m_use_source = source;      // set source location of first use
    m_status |= USED;
}

inline const Expr*
Symbol::getEqu() const
{
    if (m_type == EQU && (m_status & VALUED))
        return m_equ.get();
    return 0;
}

} // namespace yasm

#endif

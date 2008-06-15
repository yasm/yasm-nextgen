#ifndef YASM_SYMBOL_H
#define YASM_SYMBOL_H
///
/// @file libyasm/symbol.h
/// @brief YASM symbol interface.
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

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include "assoc_data.h"
#include "location.h"
#include "marg_ostream_fwd.h"


namespace yasm
{

class Bytecode;
class Expr;
class NameValues;

class Symbol : private boost::noncopyable, public AssocDataContainer
{
    friend marg_ostream& operator<< (marg_ostream& os, const Symbol& sym);

public:
    /// Constructor.
    explicit Symbol(const std::string& name);

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
    const std::string& get_name() const { return m_name; }

    /// Get the visibility of a symbol.
    /// @return Symbol visibility.
    int get_visibility() const { return m_visibility; }

    /// Get the status of a symbol.
    /// @return Symbol status.
    int get_status() const { return m_status; }

    /// Get the virtual line of where a symbol was first defined.
    /// @return Virtual line.
    unsigned long get_def_line() const { return m_def_line; }

    /// Get the virtual line of where a symbol was first declared.
    /// @return Virtual line.
    unsigned long get_decl_line() const { return m_decl_line; }

    /// Get the virtual line of where a symbol was first used.
    /// @return Virtual line.
    unsigned long get_use_line() const { return m_use_line; }

    /// Get EQU value of a symbol.
    /// @return EQU value, or NULL if symbol is not an EQU or is not defined.
    /*@null@*/ const Expr* get_equ() const;

    /// Get the label location of a symbol.
    /// @param sym       symbol
    /// @param loc       label location (output)
    /// @return False if not symbol is not a label or if the symbol's
    ///         visibility is #EXTERN or #COMMON (not defined in the file).
    bool get_label(/*@out@*/ Location* loc) const;

    /// Determine if symbol is the "absolute" symbol created by
    /// yasm_symtab_abs_sym().
    /// @return False if symbol is not the "absolute" symbol, true otherwise.
    bool is_abs() const
    {
        return m_def_line == 0 && m_type == EQU && m_name.length() == 0;
    }

    /// Determine if symbol is a special symbol.
    /// @return False if symbol is not a special symbol, true otherwise.
    bool is_special() const { return m_type == SPECIAL; }

    /// Determine if symbol is a label representing the current assembly
    /// position.
    /// @param sym       symbol
    /// @return False if symbol is not a current position label, true
    ///         otherwise.
    bool is_curpos() const { return m_type == CURPOS; }

    /// Mark the symbol as used.  The symbol does not necessarily need to
    /// be defined before it is used.
    /// @param line     virtual line where referenced
    /// @return Symbol (this).
    Symbol& use(unsigned long line);

    /// Define as an EQU value.
    /// @param e        EQU value (expression)
    /// @param line     virtual line of EQU
    /// @return Symbol (this).
    Symbol& define_equ(std::auto_ptr<Expr> e, unsigned long line);

    /// Define as a label.
    /// @param loc      location of label
    /// @param line     virtual line of label
    /// @return Symbol (this).
    Symbol& define_label(Location loc, unsigned long line);

    /// Define as a label representing the current assembly position.
    /// This should be used for this purpose instead of define_label()
    /// as Value::finalize_scan() looks for usage of this symbol type for
    /// special handling.
    /// @param loc      location of label
    /// @param line     virtual line of label
    /// @return Symbol (this).
    Symbol& define_curpos(Location loc, unsigned long line);

    /// Define a special symbol.  Special symbols have no generic associated
    /// data (such as an expression or precbc).
    /// @param vis      symbol visibility
    /// @return Symbol (this).
    Symbol& define_special(Symbol::Visibility vis, unsigned long line=0);

    /// Declare external visibility.
    /// @note Not all visibility combinations are allowed.
    /// @param vis      visibility
    /// @param line     virtual line of visibility-setting
    /// @return Symbol (this).
    Symbol& declare(Visibility vis, unsigned long line);

    /// Set object-extended name/values.
    /// @param objext_namevals  object-extended name/values
    /// @return Symbol (this).
    Symbol& set_objext_namevals(std::auto_ptr<NameValues> objext_namevals);

    /// Get object-extended name/values, if any, associated with symbol's
    /// declaration.
    /// @param sym      symbol
    /// @return Object-extended name/values (NULL if none).
    /*@null@*/ const NameValues* get_objext_namevals() const;

    /// Set common size of symbol.
    /// @param common_size  common size expression
    /// @return Symbol (this).
    Symbol& set_common_size(std::auto_ptr<Expr> common_size);

    /// Get common size of symbol, if symbol is declared #COMMON and a
    /// size was set for it.
    /// @return Common size (NULL if none).
    /*@null@*/ Expr* get_common_size();

    /// Finalize symbol after parsing stage.  Errors on symbols that
    /// are used but never defined or declared #EXTERN or #COMMON.
    /// @param undef_extern if true, all undef syms should be declared extern
    void finalize(bool undef_extern);

private:
    enum Type
    {
        UNKNOWN,    ///< for unknown type (COMMON/EXTERN)
        EQU,        ///< for EQU defined symbols (expressions)
        LABEL,      ///< for labels

        /// For labels representing the current assembly position.
        CURPOS,

        /// For special symbols that need to be in the symbol table but
        /// otherwise have no purpose.
        SPECIAL
    };

    void define(Type type, unsigned long line);

    std::string m_name;
    Type m_type;
    int m_status;
    int m_visibility;
    unsigned long m_def_line;   ///< line where symbol was first defined
    unsigned long m_decl_line;  ///< line where symbol was first declared
    unsigned long m_use_line;   ///< line where symbol was first used

    // Possible data

    boost::scoped_ptr<Expr> m_equ;  ///< EQU value

    /// Label location
    Location m_loc;
};

inline Symbol&
Symbol::use(unsigned long line)
{
    if (m_use_line == 0)
        m_use_line = line;      // set line number of first use
    m_status |= USED;
    return *this;
}

inline const Expr*
Symbol::get_equ() const
{
    if (m_type == EQU && (m_status & VALUED))
        return m_equ.get();
    return 0;
}

/// Print a symbol.  For debugging purposes.
/// @param os           output stream
/// @param sym          symbol
marg_ostream& operator<< (marg_ostream& os, const Symbol& sym);

} // namespace yasm

#endif

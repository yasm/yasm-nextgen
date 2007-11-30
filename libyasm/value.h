#ifndef YASM_VALUE_H
#define YASM_VALUE_H
///
/// @file libyasm/value.h
/// @brief YASM value interface.
///
/// @license
///  Copyright (C) 2006-2007  Peter Johnson
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

#include "location.h"


namespace yasm {

class Arch;
class Bytecode;
class Bytes;
class Expr;
class IntNum;
class Symbol;

/// A value.  May be absolute or relative.  Outside the parser, #Expr
/// should only be used for absolute exprs.  Anything that could contain
/// a relocatable value should use this structure instead.
class Value {
public:
    explicit Value(unsigned int size);

    /// Initialize a value with just an expression.  No processing is
    /// performed, the expression is simply stuck into #abs and the other
    /// fields are initialized.  Use extract_value() to perform "smart"
    /// processing into a value.  This function is intended for use during
    /// parsing simply to ensure all fields of the value are initialized;
    /// after the parse is complete, yasm_value_extract() should be called
    /// to finalize the value.  The value defaults to unsigned.
    /// @param e        expression (kept)
    /// @param size     value size (in bits)
    Value(unsigned int size, std::auto_ptr<Expr> e);

    /// Initialize with just a symrec.  No processing is performed,
    /// the symrec is simply stuck into value.rel and the other fields are
    /// initialized.
    /// @param sym      symrec
    /// @param size     value size (in bits)
    Value(unsigned int size, /*@null@*/ Symbol* sym);

    /// Copy constructor.
    /// @param oth      original value
    Value(const Value& oth);
    Value& operator= (const Value& rhs);

    /// Destructor.
    ~Value();

    /// Clear the value.  This is needed as values are commonly stored by
    /// value in other objects.
    void clear();

    /// Set a value to be relative to the current assembly position rather
    /// than relative to the section start.
    /// @param bc       bytecode containing value
    /// @param ip_rel   if nonzero, indicates IP-relative data relocation,
    ///                 sometimes used to generate special relocations
    /// @note If value is just an absolute value, will get an absolute symrec
    ///       to reference to (via bc's symbol table).
    void set_curpos_rel(const Bytecode& bc, bool ip_rel);

    /// Break an #Expr into a #Value constituent parts.  Extracts the
    /// relative portion of the value, SEG and WRT portions, and top-level
    /// right shift, if any.  Places any remaining expr into the absolute
    /// portion of the value.  First expands references to symrecs in
    /// absolute sections by expanding with the absolute section start plus
    /// the symrec offset within the absolute section.
    /// @param loc      location of value
    /// @return True if the expr could not be split into a value for some
    ///         reason (e.g. the relative portion was not added, but
    ///         multiplied etc).
    /// @note This should only be called after the parse is complete.
    ///       Calling before the parse is complete will usually result in
    ///       an error return.
    bool finalize(Location loc);

    /// Get value if absolute or PC-relative section-local relative.
    /// Returns NULL otherwise.
    /// @param loc          current location (for PC-relative calculation);
    /// @param calc_bc_dist if nonzero, calculates bytecode distances in
    ///                     absolute portion of value
    /// @note Adds in value.rel (correctly) if PC-relative and in the same
    ///       section as bc (and there is no WRT or SEG).
    /// @return Intnum if can be resolved to integer value, otherwise NULL.
    /*@null@*/ std::auto_ptr<IntNum> get_intnum(Location loc,
                                                bool calc_bc_dist);

    /// Get value if absolute section-local relative.
    /// Returns NULL otherwise.
    /// @param calc_bc_dist if nonzero, calculates bytecode distances in
    ///                     absolute portion of value
    /// @return Intnum if can be resolved to integer value, otherwise NULL.
    /*@null@*/ std::auto_ptr<IntNum> get_intnum(bool calc_bc_dist);

    /// Output value if constant or PC-relative section-local.  This should
    /// be used from objfmt output_value_func() functions.
    /// @param bytes        storage for byte representation
    /// @param destsize     destination size (in bytes)
    /// @param loc          location of value
    /// @param warn         enables standard warnings: zero for none;
    ///                     nonzero for overflow/underflow floating point and
    ///                     integer warnings
    /// @param arch         architecture
    /// @note Adds in value.rel (correctly) if PC-relative and in the same
    ///      section as bc (and there is no WRT or SEG); if this is not the
    ///      desired behavior, e.g. a reloc is needed in this case, don't
    ///      use this function!
    /// @return False if no value output due to value needing relocation;
    ///         true if value output.
    bool output_basic(Bytes& bytes, size_t destsize, Location loc, int warn,
                      const Arch& arch);

    /// Print a value.  For debugging purposes.
    /// @param os           output stream
    /// @param indent_level indentation level
    void put(std::ostream& os, int indent_level) const;

    /// Get the absolute portion of the value.
    /// @return Absolute expression, or NULL if there is no absolute portion.
    Expr* get_abs() { return m_abs; }
    const Expr* get_abs() const { return m_abs; }

    /// Determine if the value has an absolute portion.
    /// @return True if value has absolute portion, false if not.
    bool has_abs() const { return m_abs != 0; }

    /// Add integer to the absolute portion of the value.
    void add_abs(std::auto_ptr<IntNum> delta);

    /// Add expression to the absolute portion of the value.
    void add_abs(std::auto_ptr<Expr> delta);

    /// Determine if the value is relative.
    /// @return True if value has relative portions, false if not.
    bool is_relative() const { return m_rel != 0; }

    /// Determine if the value is WRT anything.
    /// @return True if value has WRT portion, false if not.
    bool is_wrt() const { return m_wrt != 0; }

    /// Maximum value of #m_rshift.
    static const unsigned int RSHIFT_MAX = 127;

private:
    bool finalize_scan(Expr* e, Location expr_loc, bool ssym_not_ok);

    /// The absolute portion of the value.  May contain *differences* between
    /// symrecs but not standalone symrecs.  May be NULL if there is no
    /// absolute portion (e.g. the absolute portion is 0).
    Expr* m_abs;    // can't use scoped_ptr as we want to internally modify

public:
    /// The relative portion of the value.  This is the portion that may
    /// need to generate a relocation.  May be NULL if no relative portion.
    /*@null@*/ /*@dependent@*/ Symbol* m_rel;

    /// What the relative portion is in reference to.  NULL if the default.
    /*@null@*/ /*@dependent@*/ Symbol* m_wrt;

    /// If the segment of the relative portion should be used, not the
    /// relative portion itself.  Boolean.
    unsigned int m_seg_of : 1;

    /// If the relative portion of the value should be shifted right
    /// (supported only by a few object formats).  If just the absolute
    /// portion should be shifted, that must be in the abs expr, not here!
    unsigned int m_rshift : 7;

    /// Indicates the relative portion of the value should be relocated
    /// relative to the current assembly position rather than relative to the
    /// section start.  "Current assembly position" here refers to the
    /// starting address of the bytecode containing this value.  Boolean.
    unsigned int m_curpos_rel : 1;

    /// Indicates that curpos_rel was set due to IP-relative relocation;
    /// in some objfmt/arch combinations (e.g. win64/x86-amd64) this info
    /// is needed to generate special relocations.
    unsigned int m_ip_rel : 1;

    /// Indicates the value is a jump target address (rather than a simple
    /// data address).  In some objfmt/arch combinations (e.g. macho/amd64)
    /// this info is needed to generate special relocations.
    unsigned int m_jump_target : 1;

    /// Indicates the relative portion of the value should be relocated
    /// relative to its own section start rather than relative to the
    /// section start of the bytecode containing this value.  E.g. the value
    /// resulting from the relative portion should be the offset from its
    /// section start.  Boolean.
    unsigned int m_section_rel : 1;

    /// Sign of the value.  Nonzero if the final value should be treated as
    /// signed, 0 if it should be treated as signed.
    unsigned int m_sign : 1;

    /// Size of the value, in bits.
    unsigned int m_size : 8;
};

} // namespace yasm

#endif

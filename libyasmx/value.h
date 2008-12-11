#ifndef YASM_VALUE_H
#define YASM_VALUE_H
///
/// @file
/// @brief Value interface.
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
#include <algorithm>
#include <memory>

#include "export.h"
#include "intnum.h"
#include "location.h"
#include "marg_ostream_fwd.h"
#include "symbolref.h"


namespace yasm
{

class Arch;
class Bytes;
class Expr;
class Object;

/// A value.  May be absolute or relative.  Outside the parser, #Expr
/// should only be used for absolute exprs.  Anything that could contain
/// a relocatable value should use this structure instead.
class YASM_LIB_EXPORT Value
{
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
    Value(unsigned int size, SymbolRef sym);

    /// Copy constructor.
    /// @param oth      original value
    Value(const Value& oth);
    Value& operator= (const Value& rhs);

    /// Destructor.
    ~Value();

    /// Exchanges this value with another value.
    /// @param oth      other value
    void swap(Value& oth);

    /// Clear the value.  This is needed as values are commonly stored by
    /// value in other objects.
    void clear();

    /// Clear just the relative portion of the value.  Clears additive and
    /// subtractive relative portions, WRT symbol, m_seg_of, m_rshift,
    /// m_ip_rel, and m_section_rel.
    void clear_rel();

    /// Add a subtractive relative to the value.  Commonly used to subtract
    /// the current assembly position.
    /// Creates absolute symbol to refer to if no existing additive relative
    /// portion.
    /// @param object   object
    /// @param sub      symbol to subtract
    void sub_rel(Object* object, SymbolRef sub);

    /// Break an #Expr into a #Value constituent parts.  Extracts the
    /// relative portion of the value, SEG and WRT portions, and top-level
    /// right shift, if any.  Places any remaining expr into the absolute
    /// portion of the value.  First expands references to symrecs in
    /// absolute sections by expanding with the absolute section start plus
    /// the symrec offset within the absolute section.
    /// @return True if the expr could not be split into a value for some
    ///         reason (e.g. the relative portion was not added, but
    ///         multiplied etc).
    /// @note This should only be called after the parse is complete.
    ///       Calling before the parse is complete will usually result in
    ///       an error return.
    bool finalize();

    /// Determine if subtractive relative portion can be treated as
    /// PC relative, and if so, calculate its needed integer fixup.
    /// This can only be done if the subtractive portion is in the value's
    /// segment.
    /// @param out          (output) integer additive fixup
    /// @param loc          location of value in output
    /// @return True if can be treated as PC relative; out valid.
    ///         False if not; out=0.
    /// @note In cases when this function returns false, there still may
    ///       be a subtractive relative portion, so users should handle
    ///       or error as necessary.
    bool calc_pcrel_sub(/*@out@*/ IntNum* out, Location loc) const;

    /// Get integer value if absolute constant (no relative portion).
    /// @param out          (output) integer
    /// @param calc_bc_dist if nonzero, calculates bytecode distances in
    ///                     absolute portion of value
    /// @return True (and out set) if can be resolved to integer value.
    bool get_intnum(/*@out@*/ IntNum* out, bool calc_bc_dist);

    /// Output value if absolute constant (no relative portion).  This should
    /// be used from BytecodeOutput::output_value() implementations.
    /// @param bytes        storage for byte representation
    /// @param warn         enables standard warnings: zero for none;
    ///                     nonzero for overflow/underflow floating point and
    ///                     integer warnings
    /// @param arch         architecture
    /// @return False if no value output due to value needing relocation;
    ///         true if value output.
    bool output_basic(Bytes& bytes, int warn, const Arch& arch);

    /// Get the absolute portion of the value.
    /// @return Absolute expression, or NULL if there is no absolute portion.
    Expr* get_abs() { return m_abs; }
    const Expr* get_abs() const { return m_abs; }

    /// Determine if the value has an absolute portion.
    /// @return True if value has absolute portion, false if not.
    bool has_abs() const { return m_abs != 0; }

    /// Add integer to the absolute portion of the value.
    void add_abs(const IntNum& delta);

    /// Add expression to the absolute portion of the value.
    void add_abs(std::auto_ptr<Expr> delta);

    /// Get the additive relative portion of the value.
    /// @return Relative symbol, or NULL if there is no relative portion.
    SymbolRef get_rel() const { return m_rel; }

    /// Get the WRT portion of the value.
    /// @return WRT symbol, or NULL if there isn't one.
    SymbolRef get_wrt() const { return m_wrt; }

    /// Get the subtractive relative portion of the value.
    /// @return Subtractive symbol, or NULL if there isn't one.
    SymbolRef get_sub() const { return m_sub; }

    /// Determine if the value is relative.
    /// @return True if value has a relative portion, false if not.
    bool is_relative() const { return m_rel != 0; }

    /// Determine if the value is WRT anything.
    /// @return True if value has WRT portion, false if not.
    bool is_wrt() const { return m_wrt != 0; }

    /// Determine if there is a subtractive relative portion of the value.
    /// @return True if value has a subtractive relative portion, false if not.
    bool has_sub() const { return m_sub != 0; }

    /// Set line number.
    /// @param line     Virtual line number
    void set_line(unsigned long line) { m_line = line; }

    /// Get line number.
    /// @return Virtual line number.
    unsigned long get_line() const { return m_line; }

    /// Maximum value of #m_rshift.
    static const unsigned int RSHIFT_MAX = 127;

private:
    bool finalize_scan(Expr* e, bool ssym_not_ok);

    /// The absolute portion of the value.  May contain *differences* between
    /// symrecs but not standalone symrecs.  May be NULL if there is no
    /// absolute portion (e.g. the absolute portion is 0).
    Expr* m_abs;    // can't use scoped_ptr as we want to internally modify

    /// The relative portion of the value.  This is the portion that may
    /// need to generate a relocation.  May be NULL if no relative portion.
    SymbolRef m_rel;

    /// What the relative portion is in reference to.  NULL if the default.
    SymbolRef m_wrt;

    /// Subtractive relative element.  NULL if none.
    /// Usually in a different section than m_rel; unless there's a WRT portion
    /// or other modification of m_rel, differences in the same section should
    /// be in m_abs.
    SymbolRef m_sub;

    /// Line number.
    unsigned long m_line;

public:
    /// Distance from the end of the value to the next instruction, in bytes.
    /// Used to generate special relocations in some object formats.
    /// Only needs to be set for IP-relative values (m_ip_rel is true).
    unsigned int m_next_insn : 4;

    /// If the segment of the relative portion should be used, not the
    /// relative portion itself.  Boolean.
    unsigned int m_seg_of : 1;

    /// If the relative portion of the value should be shifted right
    /// (supported only by a few object formats).  If just the absolute
    /// portion should be shifted, that must be in the abs expr, not here!
    unsigned int m_rshift : 7;

    /// Indicates that the value should be treated as a IP-relative relocation;
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

    /// Indicates overflow warnings have been disabled for this value.
    unsigned int m_no_warn : 1;

    /// Sign of the value.  Nonzero if the final value should be treated as
    /// signed, 0 if it should be treated as signed.
    unsigned int m_sign : 1;

    /// Size of the value, in bits.
    unsigned int m_size : 8;
};

/// Print a value.  For debugging purposes.
/// @param os           output stream
/// @param value        value
YASM_LIB_EXPORT
marg_ostream& operator<< (marg_ostream& os, const Value& value);

/// Specialized swap for algorithms.
inline void
swap(Value& left, Value& right)
{
    left.swap(right);
}

} // namespace yasm

namespace std
{

/// Specialized std::swap.
template <>
inline void
swap(yasm::Value& left, yasm::Value& right)
{
    left.swap(right);
}

} // namespace std

#endif

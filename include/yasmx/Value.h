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

#include <boost/scoped_ptr.hpp>

#include "yasmx/Basic/DiagnosticKinds.h"
#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Config/export.h"
#include "yasmx/DebugDumper.h"
#include "yasmx/IntNum.h"
#include "yasmx/Location.h"
#include "yasmx/SymbolRef.h"


class ValueTest;

namespace yasm
{

class Arch;
class Bytes;
class Diagnostic;
class Expr;
class NumericOutput;
class Object;

/// A value.  May be absolute or relative.  Outside the parser, #Expr
/// should only be used for absolute exprs.  Anything that could contain
/// a relocatable value should use this structure instead.
class YASM_LIB_EXPORT Value
{
    friend class ::ValueTest;

public:
    explicit Value(unsigned int size);

    /// Initialize a value with just an expression.  No processing is
    /// performed, the expression is simply stuck into m_abs.
    /// After the parse is complete, Finalize() should be called
    /// to finalize the value.  The value defaults to unsigned.
    /// @param e        expression (kept)
    /// @param size     value size (in bits)
    Value(unsigned int size, std::auto_ptr<Expr> e);

    /// Initialize with just a symrec.  No processing is performed,
    /// the symrec is simply stuck into m_rel.
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
    void Clear();

    /// Clear just the relative portion of the value.  Clears additive and
    /// subtractive relative portions, WRT symbol, m_seg_of, m_rshift,
    /// m_ip_rel, and m_section_rel.
    void ClearRelative();

    /// Subtract a relative location from the value.  Commonly used to subtract
    /// the current assembly position.
    /// Creates absolute symbol to refer to if no existing additive relative
    /// portion.
    /// @param object   object
    /// @param sub      location to subtract
    /// @return False if error occurred.
    bool SubRelative(Object* object, Location sub);

    /// Break m_abs into its constituent parts.  Extracts the
    /// relative portion of the value, SEG and WRT portions, and top-level
    /// right shift, if any.  Leaves any remaining expr in the absolute
    /// portion of the value.  First expands references to symrecs in
    /// absolute sections by expanding with the absolute section start plus
    /// the symrec offset within the absolute section.
    /// @param diags            diagnostic reporting
    /// @param err_too_complex  error to report if the expr could not be
    ///                         split into a value (e.g. the relative portion
    ///                         was not added, but multiplied etc).
    /// @return False if an error was reported.
    /// @note This should only be called after the parse is complete.
    ///       Calling before the parse is complete will usually result in
    ///       an error.
    bool Finalize(Diagnostic& diags, unsigned int err_too_complex =
                  diag::err_too_complex_expression);

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
    bool CalcPCRelSub(/*@out@*/ IntNum* out, Location loc) const;

    /// Get integer value if absolute constant (no relative portion).
    /// @param out          (output) integer
    /// @param calc_bc_dist if nonzero, calculates bytecode distances in
    ///                     absolute portion of value
    /// @param diags        diagnostic reporting
    /// @return True (and out set) if can be resolved to integer value.
    bool getIntNum(/*@out@*/ IntNum* out, bool calc_bc_dist, Diagnostic& diags);

    /// Configure numeric output based on value settings.
    /// @param num_out      numeric output
    void ConfigureOutput(NumericOutput* num_out) const;

    /// Output value if absolute constant (no relative portion).  This should
    /// be used from BytecodeOutput::ConvertValueToBytes() implementations.
    /// @param num_out      numeric output
    /// @param outval       output integer value (set on false return)
    /// @param diags        diagnostic reporting
    /// @return False if no value output due to value needing relocation;
    ///         true if value output.
    bool OutputBasic(NumericOutput& num_out, IntNum* outval, Diagnostic& diags);

    /// Get the absolute portion of the value.
    /// @return Absolute expression, or NULL if there is no absolute portion.
    Expr* getAbs() { return m_abs.get(); }
    const Expr* getAbs() const { return m_abs.get(); }

    /// Determine if the value has an absolute portion.
    /// @return True if value has absolute portion, false if not.
    bool hasAbs() const { return m_abs.get() != 0; }

    /// Add integer to the absolute portion of the value.
    void AddAbs(const IntNum& delta);

    /// Add expression to the absolute portion of the value.
    void AddAbs(const Expr& delta);

    /// Get the additive relative portion of the value.
    /// @return Relative symbol, or NULL if there is no relative portion.
    SymbolRef getRelative() const { return m_rel; }

    /// Get the WRT portion of the value.
    /// @return WRT symbol, or NULL if there isn't one.
    SymbolRef getWRT() const { return m_wrt; }

    /// Get the subtractive relative portion of the value as a symbol.
    /// @return Subtractive symbol, or NULL if there isn't one.
    SymbolRef getSubSymbol() const
    { return SymbolRef(m_sub_sym ? m_sub.sym : 0); }

    /// Get the subtractive relative portion of the value as a location.
    /// Handles both locations and labels.
    /// @param loc      Subtractive location (output)
    /// @return True if loc set, false if not.
    bool getSubLocation(/*@out@*/ Location* loc) const;

    /// Determine if the value is relative.
    /// @return True if value has a relative portion, false if not.
    bool isRelative() const { return m_rel != 0; }

    /// Determine if value's relative portion is "complex".
    /// A relative is considered to be complex if its segment should be used
    /// (isSegOf() is true), it has a right shift (getRShift() > 0), it has
    /// a left shift (getShift() > 0), or if it is section relative
    /// (isSectionRelative() is true).
    bool isComplexRelative() const
    {
        return (m_seg_of != 0 || m_rshift > 0 || m_shift > 0 ||
                m_section_rel != 0);
    }

    /// Determine if the segment of the relative portion should be used.
    /// @return True if segment should be used, false if not.
    bool isSegOf() const { return m_seg_of; }

    /// Get the amount the relative portion should be shifted right by.
    /// @return Right shift amount, in bits.
    unsigned int getRShift() const { return m_rshift; }

    /// Indicate the value should be treated as IP-relative.
    /// @param ip_rel   true if value should be treated as IP-relative
    void setIPRelative(bool ip_rel = true) { m_ip_rel = ip_rel; }

    /// Determine if the value should be treated as an IP-relative relocation.
    /// @return True if value is IP-relative, false if not.
    bool isIPRelative() const { return m_ip_rel; }

    /// Indicate the value is a jump target address.
    /// @param jump_target  true if value is a jump target address
    void setJumpTarget(bool jump_target = true)
    { m_jump_target = jump_target; }

    /// Determine if the value is a jump target address.
    /// @return True if value is a jump target address, false if not.
    bool isJumpTarget() const { return m_jump_target; }

    /// Indicate the value should be treated as section-relative.
    /// @param sect_rel     true if value should be treated as section-relative
    void setSectionRelative(bool sect_rel = true) { m_section_rel = sect_rel; }

    /// Determine if the value should be relocated relative to its relative
    /// portion's section rather than the section containing the value.
    /// @return True if value is section-relative, false if not.
    bool isSectionRelative() const { return m_section_rel; }

    /// Determine if the value is WRT anything.
    /// @return True if value has WRT portion, false if not.
    bool isWRT() const { return m_wrt != 0; }

    /// Determine if there is a subtractive relative portion of the value.
    /// @return True if value has a subtractive relative portion, false if not.
    bool hasSubRelative() const { return m_sub_sym != 0 || m_sub_loc != 0; }

    /// Enable overflow warnings for this value.
    void EnableWarn() { m_no_warn = 0; }

    /// Disable overflow warnings for this value.
    void DisableWarn() { m_no_warn = 1; }

    /// Determine if overflow warnings are enabled for this value.
    /// @return True if overflow warnings are enabled, false if not.
    bool isWarnEnabled() const { return !m_no_warn; }

    /// Set sign of the value.
    /// @param sign     true if signed, false if unsigned.
    void setSigned(bool sign = true) { m_sign = sign; }

    /// Get sign of the value.
    /// @return True if signed, false if unsigned.
    bool isSigned() const { return m_sign; }

    /// Set value size.
    /// @param size     size of the value, in bits
    void setSize(unsigned int size) { m_size = size; }

    /// Get value size.
    /// @return Size of the value, in bits.
    unsigned int getSize() const { return m_size; }

    /// Set value shift.
    /// @param shift    left shift for the value result, in bits
    void setShift(unsigned int shift) { m_shift = shift; }

    /// Get value shift
    /// @return Left shift for the value result, in bits.
    unsigned int getShift() const { return m_shift; }

    /// Set distance from the start of the instruction to the start of the
    /// value.
    /// @param dist     distance in bytes
    void setInsnStart(unsigned int dist) { m_insn_start = dist; }

    /// Get distance from the start of the instruction to the start of the
    /// value.
    /// @return Distance in bytes.
    unsigned int getInsnStart() const { return m_insn_start; }

    /// Set distance from the end of the value to the next instruction.
    /// @param dist     distance in bytes
    void setNextInsn(unsigned int dist) { m_next_insn = dist; }

    /// Get distance from the end of the value to the next instruction.
    /// @return Distance in bytes.
    unsigned int getNextInsn() const { return m_next_insn; }

    /// Set source range.
    /// @param range    Source range
    void setSource(SourceRange range) { m_source = range; }

    /// Get source range.
    /// @return Source range.
    SourceRange getSource() const { return m_source; }

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    /// Maximum value of right shift.
    enum { RSHIFT_MAX = 127 };

private:
    bool FinalizeScan(Expr& e, bool ssym_ok, int* pos);

    /// The absolute portion of the value.  May contain *differences* between
    /// symrecs but not standalone symrecs.  May be NULL if there is no
    /// absolute portion (e.g. the absolute portion is 0).
    boost::scoped_ptr<Expr> m_abs;

    /// The relative portion of the value.  This is the portion that may
    /// need to generate a relocation.  May be NULL if no relative portion.
    SymbolRef m_rel;

    /// What the relative portion is in reference to.  NULL if the default.
    SymbolRef m_wrt;

    /// Subtractive relative element.
    /// Usually in a different section than m_rel; unless there's a WRT portion
    /// or other modification of m_rel, differences in the same section should
    /// be in m_abs.
    union
    {
        Symbol* sym;        ///< Symbol if m_sub_sym is nonzero.
        Location loc;       ///< Location if m_sub_loc is nonzero.
    } m_sub;

    /// Source range.
    SourceRange m_source;

    /// If m_sub is a symbol.  Boolean.
    /// Should not be set if m_sub_loc is set.
    unsigned int m_sub_sym : 1;

    /// If m_sub is a location.  Boolean.
    /// Should not be set if m_sub_sym is set.
    unsigned int m_sub_loc : 1;

    /// Distance from the start of the machine instruction, in bytes.
    /// Used to make special relocation adjustments in some object formats.
    /// Should be 0 for non-instructions.
    unsigned int m_insn_start : 4;

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

    /// If the value result should be shifted some number of bits to the left
    /// when being output.  Usually only supported by specific object format
    /// and architecture combinations.
    unsigned int m_shift : 6;

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

    /// Indicates if overflow warnings are disabled for this value.
    unsigned int m_no_warn : 1;

    /// Sign of the value.  Nonzero if the final value should be treated as
    /// signed, 0 if it should be treated as signed.
    unsigned int m_sign : 1;

    /// Size of the value, in bits.
    unsigned int m_size : 8;
};

/// Specialized swap for Value.
inline void
swap(Value& left, Value& right)
{
    left.swap(right);
}

} // namespace yasm

namespace std
{

/// Specialized std::swap for Value.
template <>
inline void
swap(yasm::Value& left, yasm::Value& right)
{
    left.swap(right);
}

} // namespace std

#endif

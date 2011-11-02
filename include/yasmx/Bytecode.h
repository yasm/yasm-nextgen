#ifndef YASM_BYTECODE_H
#define YASM_BYTECODE_H
///
/// @file
/// @brief Bytecode interface.
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
#include <algorithm>
#include <memory>
#include <vector>

#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Config/export.h"
#include "yasmx/Config/functional.h"
#include "yasmx/Support/scoped_ptr.h"
#include "yasmx/Bytes.h"
#include "yasmx/DebugDumper.h"
#include "yasmx/SymbolRef.h"
#include "yasmx/Value.h"


namespace yasm
{

class BytecodeContainer;
class BytecodeOutput;
class Diagnostic;
class Expr;

/// A bytecode.
class YASM_LIB_EXPORT Bytecode
{
    friend class BytecodeContainer;

public:
    typedef std::vector<SymbolRef> SymbolRefs;
    typedef SymbolRefs::iterator symbolref_iterator;
    typedef SymbolRefs::const_iterator const_symbolref_iterator;

    /// Add a dependent span for a bytecode.
    /// @param bc           bytecode containing span
    /// @param id           non-zero identifier for span; may be any
    ///                     non-zero value
    ///                     if <0, expand is called for any change;
    ///                     if >0, expand is only called when exceeds
    ///                     threshold
    /// @param value        dependent value for bytecode expansion
    /// @param neg_thres    negative threshold for long/short decision
    /// @param pos_thres    positive threshold for long/short decision
    typedef TR1::function<void (Bytecode& bc,
                                int id,
                                const Value& value,
                                long neg_thres,
                                long pos_thres)>
        AddSpanFunc;

    typedef std::auto_ptr<Bytecode> Ptr;

    /// Bytecode contents (abstract base class).  Any implementation of a
    /// specific bytecode must implement a class derived from this one.
    /// The bytecode implementation-specific data is stored in #m_contents.
    class YASM_LIB_EXPORT Contents
    {
    public:
        typedef std::auto_ptr<Contents> Ptr;

        Contents();
        virtual ~Contents();

        /// Finalizes the bytecode after parsing.
        /// Called from Bytecode::Finalize().
        /// @param bc           bytecode
        /// @param diags        diagnostic reporting
        /// @return False if an error occurred.
        virtual bool Finalize(Bytecode& bc, Diagnostic& diags) = 0;

        /// Calculates the minimum size of a bytecode.
        /// Called from Bytecode::CalcLen().
        ///
        /// @param bc           bytecode
        /// @param len          length in bytes (output)
        /// @param add_span     function to call to add a span
        /// @param diags        diagnostic reporting
        /// @return False if an error occurred.
        /// @note May store to bytecode updated expressions.
        virtual bool CalcLen(Bytecode& bc,
                             /*@out@*/ unsigned long* len,
                             const Bytecode::AddSpanFunc& add_span,
                             Diagnostic& diags)
            = 0;

        /// Recalculates the bytecode's length based on an expanded span
        /// length.  Called from Bytecode::Expand().
        /// The base version of this function internal errors when called,
        /// so if calc_len() ever adds a span, this function should be
        /// overridden!
        /// This function should simply add to the len parameter to increase the
        /// length by a delta amount.
        /// @param bc           bytecode
        /// @param len          length (update this)
        /// @param span         span ID (as given to add_span in calc_len)
        /// @param old_val      previous span value
        /// @param new_val      new span value
        /// @param keep         function should set to true if bc size may
        ///                     increase further based on the new neg_thres
        ///                     and pos_thres returned (returned)
        /// @param neg_thres    negative threshold for long/short decision
        ///                     (returned)
        /// @param pos_thres    positive threshold for long/short decision
        ///                     (returned)
        /// @param diags        diagnostic reporting
        /// @return False if an error occurred.
        /// @note May store to bytecode updated expressions.
        virtual bool Expand(Bytecode& bc,
                            unsigned long* len,
                            int span,
                            long old_val,
                            long new_val,
                            bool* keep,
                            /*@out@*/ long* neg_thres,
                            /*@out@*/ long* pos_thres,
                            Diagnostic& diags);

        /// Output a bytecode.
        /// Called from Bytecode::Output().
        /// @param bc           bytecode
        /// @param bc_out       bytecode output interface
        /// @return False if an error occurred.
        /// @note May result in non-reversible changes to the bytecode, but
        ///       it's preferable if calling this function twice would result
        ///       in the same output.
        virtual bool Output(Bytecode& bc, BytecodeOutput& bc_out) = 0;

        /// Special bytecode classifications.  Most bytecode types should
        /// simply not override the getSpecial() function (which returns
        /// #SPECIAL_NONE).  Other return values cause special handling to
        /// kick in in various parts of yasm.
        enum SpecialType
        {
            /// No special handling.
            SPECIAL_NONE = 0,

            /// Adjusts offset instead of calculating len.
            SPECIAL_OFFSET
        };

        virtual SpecialType getSpecial() const;

        /// Get the type name of the bytecode contents.
        /// Implementations should return a known unique identifying name.
        virtual llvm::StringRef getType() const = 0;

        virtual Contents* clone() const = 0;

#ifdef WITH_XML
        /// Write an XML representation.  For debugging purposes.
        /// Called by Bytecode::Write(pugi::xml_node).
        /// @param out          XML node
        /// @return Root node.
        virtual pugi::xml_node Write(pugi::xml_node out) const = 0;
#endif // WITH_XML

    protected:
        /// Copy constructor so that derived classes can sanely have one.
        Contents(const Contents&);

    private:
        const Contents& operator=(const Contents&);
    };

    /// Create a bytecode of given contents.
    /// @param contents     type-specific data
    /// @param source       source location
    Bytecode(Contents::Ptr contents, SourceLocation source);

    /// Create a bytecode of no type.
    Bytecode();

    Bytecode(const Bytecode& oth);
    Bytecode& operator= (const Bytecode& rhs);

    /// Exchanges this bytecode with another one.
    /// @param oth      other bytecode
    void swap(Bytecode& oth);

    /// Transform a bytecode of any type into a different type.
    /// @param contents     new type-specific data
    void Transform(Contents::Ptr contents);

    /// Return if bytecode has contents.
    /// @return True if bytecode has contents.
    bool hasContents() const { return m_contents.get() != 0; }

    /// Get the current contents.
    /// @return Bytecode contents.
    Contents& getContents() { return *m_contents.get(); }
    const Contents& getContents() const { return *m_contents.get(); }

    /// Set source location of bytecode.
    /// @param source   source location
    void setSource(SourceLocation source) { m_source = source; }

    /// Get the container the bytecode resides in.
    /// @param bc   bytecode
    /// @return Bytecode container containing bc.
    const BytecodeContainer* getContainer() const { return m_container; }
    BytecodeContainer* getContainer() { return m_container; }

    /// Destructor.
    ~Bytecode();

    /// Finalize a bytecode after parsing.
    /// @param diags        diagnostic reporting
    /// @return False if an error occurred.
    bool Finalize(Diagnostic& diags);

    /// Get the offset of the bytecode.
    /// @return Offset of the bytecode in bytes.
    /// @warning Only valid /after/ optimization.
    unsigned long getOffset() const { return m_offset; }

    /// Set the offset of the bytecode.
    /// @internal Should be used by Object::optimize() only.
    /// @param offset       new offset of the bytecode
    void setOffset(unsigned long offset) { m_offset = offset; }

    /// Get the offset of the start of the tail of the bytecode.
    /// @return Offset of the tail in bytes.
    unsigned long getTailOffset() const { return m_offset + getFixedLen(); }

    /// Get the offset of the next bytecode (the next bytecode doesn't have to
    /// actually exist).
    /// @return Offset of the next bytecode in bytes.
    /// @warning Only valid /after/ optimization.
    unsigned long getNextOffset() const
    { return m_offset + getTotalLen(); }

    /// Get the total length of the bytecode.
    /// @return Total length of the bytecode in bytes.
    /// @warning Only valid /after/ optimization.
    unsigned long getTotalLen() const
    { return static_cast<unsigned long>(m_fixed.size()) + m_len; }

    /// Get the fixed length of the bytecode.
    /// @return Length in bytes.
    unsigned long getFixedLen() const
    { return static_cast<unsigned long>(m_fixed.size()); }

    /// Get the tail (dynamic) length of the bytecode.
    /// @return Length of the bytecode in bytes.
    /// @warning Only valid /after/ optimization.
    unsigned long getTailLen() const { return m_len; }

    /// Resolve EQUs in a bytecode and calculate its minimum size.
    /// Generates dependent bytecode spans for cases where, if the length
    /// spanned increases, it could cause the bytecode size to increase.
    /// @param add_span     function to call to add a span
    /// @param diags        diagnostic reporting
    /// @return False if an error occurred.
    /// @note May store to bytecode updated expressions and the short length.
    bool CalcLen(const AddSpanFunc& add_span, Diagnostic& diags);

    /// Recalculate a bytecode's length based on an expanded span length.
    /// @param span         span ID (as given to add_span in CalcLen())
    /// @param old_val      previous span value
    /// @param new_val      new span value
    /// @param keep         set to true if bc size may increase further based on
    ///                     the new neg_thres and pos_thres returned (returned)
    /// @param neg_thres    negative threshold for long/short decision
    ///                     (returned)
    /// @param pos_thres    positive threshold for long/short decision
    ///                     (returned)
    /// @param diags        diagnostic reporting
    /// @return False if an error occurred.
    /// @note May store to bytecode updated expressions and the updated
    ///       length.
    bool Expand(int span,
                long old_val,
                long new_val,
                bool* keep,
                /*@out@*/ long* neg_thres,
                /*@out@*/ long* pos_thres,
                Diagnostic& diags);

    /// Output a bytecode.
    /// @param bc_out       bytecode output interface
    /// @return False if an error occurred.
    /// @note Calling twice on the same bytecode may \em not produce the same
    ///       results on the second call, as calling this function may result
    ///       in non-reversible changes to the bytecode.
    bool Output(BytecodeOutput& bc_out);

    /// Updates bytecode offset.
    /// @note For offset-based bytecodes, calls Expand() to determine new
    ///       length.
    /// @param offset       offset to set this bytecode to
    /// @param diags        diagnostic reporting
    /// @return Offset of next bytecode.
    unsigned long UpdateOffset(unsigned long offset, Diagnostic& diags);

    SourceLocation getSource() const { return m_source; }

    unsigned long getIndex() const { return m_index; }
    void setIndex(unsigned long idx) { m_index = idx; }

    Contents::SpecialType getSpecial() const;

    Bytes& getFixed() { return m_fixed; }
    const Bytes& getFixed() const { return m_fixed; }

    void AppendFixed(const Value& val);
    void AppendFixed(std::auto_ptr<Value> val);
    Value& AppendFixed(unsigned int size,
                       std::auto_ptr<Expr> e,
                       SourceLocation source);

    /// A fixup consists of a value+offset combination.  0's need to be stored
    /// in m_fixed as placeholders.
    class YASM_LIB_EXPORT Fixup : public Value
    {
    public:
        Fixup(unsigned int off, const Value& val);
        Fixup(unsigned int off, std::auto_ptr<Value> val);
        Fixup(unsigned int off,
              unsigned int size,
              std::auto_ptr<Expr> e,
              SourceLocation source);
        void swap(Fixup& oth);
        unsigned int getOffset() const { return m_off; }

#ifdef WITH_XML
        /// Write an XML representation.  For debugging purposes.
        /// @param out          XML node
        /// @return Root node.
        pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    private:
        unsigned int m_off;
    };

    void AppendFixup(const Fixup& fixup) { m_fixed_fixups.push_back(fixup); }

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

private:
    /// Fixed data that comes before the possibly dynamic length data generated
    /// by the implementation-specific tail in m_contents.
    Bytes m_fixed;

    /// To allow combination of more complex values, fixups can be specified.
    std::vector<Fixup> m_fixed_fixups;

    /// Implementation-specific tail.
    util::scoped_ptr<Contents> m_contents;

    /// Pointer to container containing bytecode.
    /*@dependent@*/ BytecodeContainer* m_container;

    /// Total length of tail contents (not including multiple copies).
    unsigned long m_len;

    /// Source location where bytecode tail was defined.
    SourceLocation m_source;

    /// Offset of bytecode from beginning of its section.
    /// 0-based, ~0UL (e.g. all 1 bits) if unknown.
    unsigned long m_offset;

    /// Unique integer index of bytecode.  Used during optimization.
    unsigned long m_index;
};

inline Bytecode&
Bytecode::operator= (const Bytecode& rhs)
{
    if (this != &rhs)
        Bytecode(rhs).swap(*this);
    return *this;
}

inline Bytecode::Contents::SpecialType
Bytecode::getSpecial() const
{
    if (m_contents.get() == 0)
        return Contents::SPECIAL_NONE;
    return m_contents->getSpecial();
}

/// Specialized swap for algorithms.
inline void
swap(Bytecode& left, Bytecode& right)
{
    left.swap(right);
}

inline void
swap(Bytecode::Fixup& left, Bytecode::Fixup& right)
{
    left.swap(right);
}

} // namespace yasm

namespace std
{

// Specialized std::swap.
template <>
inline void
swap(yasm::Bytecode& left, yasm::Bytecode& right)
{
    left.swap(right);
}

template <>
inline void
swap(yasm::Bytecode::Fixup& left, yasm::Bytecode::Fixup& right)
{
    left.swap(right);
}

} // namespace std

#endif

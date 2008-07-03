#ifndef YASM_BYTECODE_H
#define YASM_BYTECODE_H
///
/// @file
/// @brief YASM bytecode interface.
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
#include <vector>

#include "functional.h"
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include "bytes.h"
#include "export.h"
#include "location.h"
#include "marg_ostream_fwd.h"
#include "value.h"


namespace yasm
{

class BytecodeContainer;
class Expr;
class Symbol;

/// Bytecode output interface.
/// Object formats should implement this interface for output of bytecodes.
/// value_to_bytes() and reloc_to_bytes() are called to convert values and
/// relocations into byte format first, then output_bytes() is called to
/// actually output the bytes to the object file.  The function output_gap()
/// will be called for gaps in the output.
class YASM_LIB_EXPORT BytecodeOutput : private boost::noncopyable
{
public:
    /// Constructor.
    BytecodeOutput();

    /// Destructor.
    virtual ~BytecodeOutput();

    /// Get a scratch empty Bytes object.  Should only be used by callers of
    /// this interface, not by implementations.  Only one scratch object is
    /// available, and calling this function again will clear all previous
    /// usages.
    /// @return Shared scratch buffer.
    Bytes& get_scratch()
    {
        m_scratch.resize(0);
        return m_scratch;
    }

    /// Output value.  Usual implementations will keep track of relocations
    /// and verify legal expressions.
    ///
    /// Implementations must put the value into the least significant bits of
    /// the destination, unless shifted into more significant bits by
    /// value.m_shift.
    ///
    /// The bytes parameter is the destination; it is sized prior to this call
    /// to the correct number of bytes to be output, and may contain non-zero
    /// bits.  Implementations should only overwrite the bits specified by the
    /// value.
    ///
    /// May raise an exception if an error occurs.
    ///
    /// @param value        value
    /// @param bytes        storage for byte representation
    /// @param loc          location of the expr contents (needed for relative)
    /// @param warn         enables standard warnings.  Zero for none; nonzero
    ///                     for overflow/underflow floating point warnings;
    ///                     negative for signed integer warnings,
    ///                     positive for unsigned integer warnings
    virtual void output(Value& value,
                        Bytes& bytes,
                        Location loc,
                        int warn) = 0;

    /// Convert a symbol reference to its byte representation.  Usual
    /// implementations will keep track of relocations generated this way.
    ///
    /// The bytes parameter is the destination; it is sized prior to this call
    /// to the correct number of bytes to be output, and may contain non-zero
    /// bits.  Implementations should only overwrite the bits specified by
    /// valsize.
    ///
    /// May raise an exception if an error occurs.
    ///
    /// The default implementation just calls output(Bytes).
    ///
    /// @param sym          symbol
    /// @param bytes        storage for byte representation
    /// @param bc           current bytecode (usually passed into higher-level
    ///                     calling function)
    /// @param valsize      size (in bits)
    /// @param warn         enables standard warnings.  Zero for none; nonzero
    ///                     for overflow/underflow floating point warnings;
    ///                     negative for signed integer warnings,
    ///                     positive for unsigned integer warnings
    virtual void output(Symbol* sym,
                        Bytes& bytes,
                        Bytecode& bc,
                        unsigned int valsize,
                        int warn);

    /// Output a "gap" in the object file: the data does not really need to
    /// exist in the object file, but should be initialized to 0 when the
    /// program is run.
    /// @param size         gap size, in bytes
    virtual void output_gap(unsigned int size) = 0;

    /// Output a sequence of bytes.
    /// @param bytes        bytes to output
    virtual void output(const Bytes& bytes) = 0;

private:
    Bytes m_scratch;
};

/// A bytecode.
class YASM_LIB_EXPORT Bytecode
{
    friend YASM_LIB_EXPORT
    marg_ostream& operator<< (marg_ostream &os, const Bytecode &bc);
    friend class BytecodeContainer;

public:
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
    typedef
        FUNCTION::function<void (Bytecode& bc,
                                 int id,
                                 const Value& value,
                                 long neg_thres,
                                 long pos_thres)>
        AddSpanFunc;

    typedef std::auto_ptr<Bytecode> Ptr;

    /// Bytecode contents (abstract base class).  Any implementation of a
    /// specific bytecode must implement a class derived from this one.
    /// The bytecode implementation-specific data is stored in #contents.
    class YASM_LIB_EXPORT Contents
    {
    public:
        typedef std::auto_ptr<Contents> Ptr;

        Contents();
        virtual ~Contents();

        /// Prints the implementation-specific data (for debugging purposes).
        /// Called from Bytecode::put().
        /// @param os           output stream
        virtual void put(marg_ostream& os) const = 0;

        /// Finalizes the bytecode after parsing.
        /// Called from Bytecode::finalize().
        /// @param bc           bytecode
        virtual void finalize(Bytecode& bc) = 0;

        /// Calculates the minimum size of a bytecode.
        /// Called from Bytecode::calc_len().
        ///
        /// @param bc           bytecode
        /// @param add_span     function to call to add a span
        /// @return Length in bytes.
        /// @note May store to bytecode updated expressions.
        virtual unsigned long calc_len(Bytecode& bc,
                                       Bytecode::AddSpanFunc add_span) = 0;

        /// Recalculates the bytecode's length based on an expanded span
        /// length.  Called from Bytecode::expand().
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
        /// @param neg_thres    negative threshold for long/short decision
        ///                     (returned)
        /// @param pos_thres    positive threshold for long/short decision
        ///                     (returned)
        /// @return False if bc no longer dependent on this span's length,
        ///         or true if bc size may increase for this span further
        ///         based on the new negative and positive thresholds
        ///         returned.
        /// @note May store to bytecode updated expressions.
        virtual bool expand(Bytecode& bc,
                            unsigned long& len,
                            int span,
                            long old_val,
                            long new_val,
                            /*@out@*/ long& neg_thres,
                            /*@out@*/ long& pos_thres);

        /// Output a bytecode.
        /// Called from Bytecode::output().
        /// @param bc           bytecode
        /// @param bc_out       bytecode output interface
        /// @note May result in non-reversible changes to the bytecode, but
        ///       it's preferable if calling this function twice would result
        ///       in the same output.
        virtual void output(Bytecode& bc, BytecodeOutput& bc_out) = 0;

        /// Special bytecode classifications.  Most bytecode types should
        /// simply not override the get_special() function (which returns
        /// #SPECIAL_NONE).  Other return values cause special handling to
        /// kick in in various parts of yasm.
        enum SpecialType
        {
            /// No special handling.
            SPECIAL_NONE = 0,

            /// Adjusts offset instead of calculating len.
            SPECIAL_OFFSET
        };

        virtual SpecialType get_special() const;

        virtual Contents* clone() const = 0;

    protected:
        /// Copy constructor so that derived classes can sanely have one.
        Contents(const Contents&);

    private:
        const Contents& operator=(const Contents&);
    };

    /// Create a bytecode of given contents.
    /// @param contents     type-specific data
    /// @param line         virtual line (from Linemap)
    Bytecode(Contents::Ptr contents, unsigned long line);

    /// Create a bytecode of no type.
    Bytecode();

    Bytecode(const Bytecode& oth);
    Bytecode& operator= (const Bytecode& oth);

    /// Transform a bytecode of any type into a different type.
    /// @param contents     new type-specific data
    void transform(Contents::Ptr contents);

    /// Return if bytecode has contents.
    /// @return True if bytecode has contents.
    bool has_contents() const { return m_contents.get() != 0; }

    /// Set line number of bytecode.
    /// @param line     virtual line (from Linemap)
    void set_line(unsigned long line) { m_line = line; }

    /// Get the container the bytecode resides in.
    /// @param bc   bytecode
    /// @return Bytecode container containing bc.
    const BytecodeContainer* get_container() const { return m_container; }
    BytecodeContainer* get_container() { return m_container; }

    /// Add to the list of symbols that reference a bytecode.
    /// @note Intended for #Symbol use only.
    /// @param sym  symbol
    void add_symbol(/*@dependent@*/ Symbol* sym) { m_symbols.push_back(sym); }

    /// Destructor.
    ~Bytecode();

    /// Finalize a bytecode after parsing.
    void finalize();

    /// Get the offset of the bytecode.
    /// @return Offset of the bytecode in bytes.
    /// @warning Only valid /after/ optimization.
    unsigned long get_offset() const { return m_offset; }

    /// Set the offset of the bytecode.
    /// @internal Should be used by Object::optimize() only.
    /// @param offset       new offset of the bytecode
    void set_offset(unsigned long offset) { m_offset = offset; }

    /// Get the offset of the start of the tail of the bytecode.
    /// @return Offset of the tail in bytes.
    unsigned long tail_offset() const { return m_offset + get_fixed_len(); }

    /// Get the offset of the next bytecode (the next bytecode doesn't have to
    /// actually exist).
    /// @return Offset of the next bytecode in bytes.
    /// @warning Only valid /after/ optimization.
    unsigned long next_offset() const
    { return m_offset + get_total_len(); }

    /// Get the total length of the bytecode.
    /// @return Total length of the bytecode in bytes.
    /// @warning Only valid /after/ optimization.
    unsigned long get_total_len() const
    { return m_fixed.size() + m_len; }

    /// Get the fixed length of the bytecode.
    /// @return Length in bytes.
    unsigned long get_fixed_len() const { return m_fixed.size(); }

    /// Get the tail (dynamic) length of the bytecode.
    /// @return Length of the bytecode in bytes.
    /// @warning Only valid /after/ optimization.
    unsigned long get_tail_len() const { return m_len; }

    /// Resolve EQUs in a bytecode and calculate its minimum size.
    /// Generates dependent bytecode spans for cases where, if the length
    /// spanned increases, it could cause the bytecode size to increase.
    /// @param add_span     function to call to add a span
    /// @note May store to bytecode updated expressions and the short length.
    void calc_len(AddSpanFunc add_span);

    /// Recalculate a bytecode's length based on an expanded span length.
    /// @param span         span ID (as given to yasm_bc_add_span_func in
    ///                     yasm_bc_calc_len)
    /// @param old_val      previous span value
    /// @param new_val      new span value
    /// @param neg_thres    negative threshold for long/short decision
    ///                     (returned)
    /// @param pos_thres    positive threshold for long/short decision
    ///                     (returned)
    /// @return False if bc no longer dependent on this span's length,
    ///         or true if bc size may increase for this span further
    ///         based on the new negative and positive thresholds returned.
    /// @note May store to bytecode updated expressions and the updated
    ///       length.
    bool expand(int span,
                long old_val,
                long new_val,
                /*@out@*/ long& neg_thres,
                /*@out@*/ long& pos_thres);

    /// Output a bytecode.
    /// @param bc_out       bytecode output interface
    /// @note Calling twice on the same bytecode may \em not produce the same
    ///       results on the second call, as calling this function may result
    ///       in non-reversible changes to the bytecode.
    void output(BytecodeOutput& bc_out);

    /// Updates bytecode offset.
    /// @note For offset-based bytecodes, calls expand() to determine new
    ///       length.
    /// @param offset       offset to set this bytecode to
    /// @param errwarns     error/warning set
    /// @return Offset of next bytecode.
    unsigned long update_offset(unsigned long offset);

    unsigned long get_line() const { return m_line; }

    unsigned long get_index() const { return m_index; }
    void set_index(unsigned long idx) { m_index = idx; }

    Contents::SpecialType get_special() const
    {
        if (m_contents.get() == 0)
            return Contents::SPECIAL_NONE;
        return m_contents->get_special();
    }

    Bytes& get_fixed() { return m_fixed; }
    const Bytes& get_fixed() const { return m_fixed; }

    void append_fixed(const Value& val);
    void append_fixed(unsigned int size, std::auto_ptr<Expr> e);
private:
    /// Fixed data that comes before the possibly dynamic length data generated
    /// by the implementation-specific tail in m_contents.
    Bytes m_fixed;

    /// To allow combination of more complex values, fixups can be specified.
    /// A fixup consists of a value+offset combination.  0's need to be stored
    /// in m_fixed as placeholders.
    class YASM_LIB_EXPORT Fixup : public Value
    {
    public:
        Fixup(unsigned int off, const Value& val, unsigned long line);
        unsigned long get_line() const { return m_line; }
        unsigned int get_off() const { return m_off; }
    private:
        unsigned long m_line;
        unsigned int m_off;
    };

    std::vector<Fixup> m_fixed_fixups;

    /// Implementation-specific tail.
    boost::scoped_ptr<Contents> m_contents;

    /// Pointer to container containing bytecode.
    /*@dependent@*/ BytecodeContainer* m_container;

    /// Total length of tail contents (not including multiple copies).
    unsigned long m_len;

    /// Line number where bytecode tail was defined.
    unsigned long m_line;

    /// Offset of bytecode from beginning of its section.
    /// 0-based, ~0UL (e.g. all 1 bits) if unknown.
    unsigned long m_offset;

    /// Unique integer index of bytecode.  Used during optimization.
    unsigned long m_index;

    /// Labels that point to this bytecode.
    std::vector<Symbol*> m_symbols;
};

inline marg_ostream&
operator<< (marg_ostream& os, const Bytecode::Contents& contents)
{
    contents.put(os);
    return os;
}

YASM_LIB_EXPORT
marg_ostream& operator<< (marg_ostream &os, const Bytecode& bc);

} // namespace yasm

#endif

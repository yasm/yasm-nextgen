#ifndef YASM_BYTECODEOUTPUT_H
#define YASM_BYTECODEOUTPUT_H
///
/// @file
/// @brief Bytecode output interfaces.
///
/// @license
///  Copyright (C) 2001-2008  Peter Johnson
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
#include "yasmx/Config/export.h"

#include "yasmx/Bytes.h"
#include "yasmx/Location.h"
#include "yasmx/SymbolRef.h"


namespace llvm { class raw_ostream; }

namespace yasm
{

class Bytecode;
class Value;

/// Bytecode output interface.
///
/// Object formats should implement this interface for output of bytecodes.
/// Implementator notes: value_to_bytes() and sym_to_bytes() are called to
/// convert values and relocations into byte format first, then
/// do_output_bytes() is called.  It is assumed that do_output_bytes()
/// actually outputs the bytes to the object file.  The implementation
/// function do_output_gap() will be called for gaps in the output.
class YASM_LIB_EXPORT BytecodeOutput
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
    Bytes& getScratch()
    {
        m_scratch.resize(0);
        return m_scratch;
    }

    /// Reset the number of bytes output to zero.
    void ResetNumOutput() { m_num_output = 0; }

    /// Get the total number of bytes (and gap) output.
    /// @return Number of bytes output.
    unsigned long getNumOutput() const { return m_num_output; }

    /// Output a value.
    ///
    /// The value is put into the least significant bits of the destination,
    /// unless shifted into more significant bits by value.m_shift.
    ///
    /// The bytes parameter is the destination; callers must size this prior
    /// to calling this function to the correct number of bytes to be output.
    /// It may contain non-zero bits.
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
    void Output(Value& value, Bytes& bytes, Location loc, int warn)
    {
        ConvertValueToBytes(value, bytes, loc, warn);
        Output(bytes);
    }

    /// Output a symbol reference.
    ///
    /// The bytes parameter is the destination; callers must size this prior
    /// to calling this function to the correct number of bytes to be output.
    /// It may contain non-zero bits.
    ///
    /// May raise an exception if an error occurs.
    ///
    /// @param sym          symbol
    /// @param bytes        storage for byte representation
    /// @param loc          location of the symbol reference
    /// @param valsize      size (in bits)
    /// @param warn         enables standard warnings.  Zero for none; nonzero
    ///                     for overflow/underflow floating point warnings;
    ///                     negative for signed integer warnings,
    ///                     positive for unsigned integer warnings
    void Output(SymbolRef sym,
                Bytes& bytes,
                Location loc,
                unsigned int valsize,
                int warn)
    {
        ConvertSymbolToBytes(sym, bytes, loc, valsize, warn);
        Output(bytes);
    }

    /// Output a "gap" in the object file: the data does not really need to
    /// exist in the object file, but should be initialized to 0 when the
    /// program is run.
    /// @param size         gap size, in bytes
    void OutputGap(unsigned int size)
    {
        DoOutputGap(size);
        m_num_output += size;
    }

    /// Output a sequence of bytes.
    /// @param bytes        bytes to output
    void Output(const Bytes& bytes)
    {
        DoOutputBytes(bytes);
        m_num_output += static_cast<unsigned long>(bytes.size());
    }

protected:
    /// Convert a value to bytes.  Called by output_value() so that
    /// implementations can keep track of relocations and verify legal
    /// expressions.
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
    /// Implementations may raise an exception if an error occurs.
    ///
    /// @param value        value
    /// @param bytes        storage for byte representation
    /// @param loc          location of the expr contents (needed for relative)
    /// @param warn         enables standard warnings.  Zero for none; nonzero
    ///                     for overflow/underflow floating point warnings;
    ///                     negative for signed integer warnings,
    ///                     positive for unsigned integer warnings
    virtual void ConvertValueToBytes(Value& value,
                                     Bytes& bytes,
                                     Location loc,
                                     int warn) = 0;

    /// Convert a symbol to bytes.  Called by output_sym() so that
    /// implementations may keep track of relocations generated this way.
    ///
    /// The bytes parameter is the destination; it is sized prior to this call
    /// to the correct number of bytes to be output, and may contain non-zero
    /// bits.  Implementations should only overwrite the bits specified by
    /// valsize.
    ///
    /// Implementations may raise an exception if an error occurs.
    ///
    /// The default implementation does nothing.
    ///
    /// @param sym          symbol
    /// @param bytes        storage for byte representation
    /// @param loc          location of the symbol reference
    /// @param valsize      size (in bits)
    /// @param warn         enables standard warnings.  Zero for none; nonzero
    ///                     for overflow/underflow floating point warnings;
    ///                     negative for signed integer warnings,
    ///                     positive for unsigned integer warnings
    virtual void ConvertSymbolToBytes(SymbolRef sym,
                                      Bytes& bytes,
                                      Location loc,
                                      unsigned int valsize,
                                      int warn);

    /// Overrideable implementation of output_gap().
    /// @param size         gap size, in bytes
    virtual void DoOutputGap(unsigned int size) = 0;

    /// Overrideable implementation of output_bytes().
    /// @param bytes        bytes to output
    virtual void DoOutputBytes(const Bytes& bytes) = 0;

private:
    BytecodeOutput(const BytecodeOutput&);                  // not implemented
    const BytecodeOutput& operator=(const BytecodeOutput&); // not implemented

    Bytes m_scratch;            ///< Reusable scratch area
    unsigned long m_num_output; ///< Total number of bytes+gap output
};

/// No-output specialization of BytecodeOutput.
/// Warns on all attempts to output non-gaps.
class YASM_LIB_EXPORT BytecodeNoOutput : public BytecodeOutput
{
public:
    BytecodeNoOutput() {}
    ~BytecodeNoOutput();

protected:
    void ConvertValueToBytes(Value& value,
                             Bytes& bytes,
                             Location loc,
                             int warn);
    void DoOutputGap(unsigned int size);
    void DoOutputBytes(const Bytes& bytes);
};

/// Stream output specialization of BytecodeOutput.
/// Handles gaps by converting to 0 and generating a warning.
/// This does not implement value_to_bytes(), so it's still a virtual
/// base class.
class YASM_LIB_EXPORT BytecodeStreamOutput : public BytecodeOutput
{
public:
    BytecodeStreamOutput(llvm::raw_ostream& os) : m_os(os) {}
    ~BytecodeStreamOutput();

protected:
    void DoOutputGap(unsigned int size);
    void DoOutputBytes(const Bytes& bytes);

    llvm::raw_ostream& m_os;
};

} // namespace yasm

#endif

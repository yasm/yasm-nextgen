#ifndef YASM_BC_OUTPUT_H
#define YASM_BC_OUTPUT_H
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
#include <iosfwd>

#include <boost/noncopyable.hpp>

#include "bytes.h"
#include "export.h"
#include "location.h"
#include "symbolref.h"


namespace yasm
{

class Bytecode;
class Value;

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
    /// @param loc          location of the symbol reference
    /// @param valsize      size (in bits)
    /// @param warn         enables standard warnings.  Zero for none; nonzero
    ///                     for overflow/underflow floating point warnings;
    ///                     negative for signed integer warnings,
    ///                     positive for unsigned integer warnings
    virtual void output(SymbolRef sym,
                        Bytes& bytes,
                        Location loc,
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

/// No-output specialization of BytecodeOutput.
/// Warns on all attempts to output non-gaps.
class YASM_LIB_EXPORT BytecodeNoOutput : public BytecodeOutput
{
public:
    BytecodeNoOutput() {}
    ~BytecodeNoOutput();

    using BytecodeOutput::output;
    void output(Value& value, Bytes& bytes, Location loc, int warn);
    void output_gap(unsigned int size);
    void output(const Bytes& bytes);
};

/// Stream output specialization of BytecodeOutput.
/// Handles gaps by converting to 0 and generating a warning.
/// This does not implement Value output function, so it's still a virtual
/// base class.
class YASM_LIB_EXPORT BytecodeStreamOutput : public BytecodeOutput
{
public:
    BytecodeStreamOutput(std::ostream& os) : m_os(os) {}
    ~BytecodeStreamOutput();

    using BytecodeOutput::output;
    void output_gap(unsigned int size);
    void output(const Bytes& bytes);

protected:
    std::ostream& m_os;
};

} // namespace yasm

#endif

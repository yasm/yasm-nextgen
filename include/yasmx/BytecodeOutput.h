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
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Basic/LLVM.h"
#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Config/export.h"
#include "yasmx/Bytes.h"
#include "yasmx/Location.h"
#include "yasmx/NumericOutput.h"
#include "yasmx/SymbolRef.h"
#include "yasmx/Value.h"


namespace yasm
{

class Bytecode;

/// Bytecode output interface.
///
/// Object formats should implement this interface for output of bytecodes.
/// Implementator notes: ConvertValueToBytes() and ConvertSymbolToBytes() are
/// called to convert values and relocations into byte format first, then
/// DoOutputBytes() is called.  It is assumed that DoOutputBytes()
/// actually outputs the bytes to the object file.  The implementation
/// function DoOutputGap() will be called for gaps in the output.
class YASM_LIB_EXPORT BytecodeOutput
{
public:
    /// Constructor.
    BytecodeOutput(DiagnosticsEngine& diags);

    /// Destructor.
    virtual ~BytecodeOutput();

    /// Get a scratch empty Bytes object.  Should only be used by callers of
    /// this interface, not by implementations.  Only one scratch object is
    /// available, and calling this function again will clear all previous
    /// usages.
    /// @return Shared scratch buffer.
    Bytes& getScratch();

    /// Get the diagnostic reporter.
    DiagnosticsEngine& getDiagnostics() const { return m_diags; }

    /// Diagnostic reporting wrapper.
    DiagnosticBuilder Diag(SourceLocation pos, unsigned diag_id)
    { return m_diags.Report(pos, diag_id); }

    /// Does this output to actual bits?  Base implementation returns true.
    virtual bool isBits() const;

    /// Reset the number of bytes output to zero.
    void ResetNumOutput() { m_num_output = 0; }

    /// Get the total number of bytes (and gap) output.
    /// @return Number of bytes output.
    unsigned long getNumOutput() const { return m_num_output; }

    /// Output a value.
    ///
    /// @param value        value
    /// @param loc          location of the expr contents (needed for relative)
    /// @param num_out      numeric output
    /// @return False if an error occurred.
    inline bool OutputValue(Value& value, Location loc, NumericOutput& num_out);

    /// Output a symbol reference.
    ///
    /// @param sym          symbol
    /// @param loc          location of the symbol reference
    /// @param num_out      numeric output
    /// @return False if an error occurred.
    inline bool OutputSymbol(SymbolRef sym,
                             Location loc,
                             NumericOutput& num_out);

    /// Output a "gap" in the object file: the data does not really need to
    /// exist in the object file, but should be initialized to 0 when the
    /// program is run.
    /// @param size         gap size, in bytes
    /// @param source       source location
    inline void OutputGap(unsigned long size, SourceLocation source);

    /// Output a sequence of bytes.
    /// @param bytes        bytes to output
    /// @param source       source location
    inline void OutputBytes(const Bytes& bytes, SourceLocation source);

    /// Convert a value to bytes.  Called by OutputValue() so that
    /// implementations can keep track of relocations and verify legal
    /// expressions.
    ///
    /// @param value        value
    /// @param loc          location of the expr contents (needed for relative)
    /// @param num_out      numeric output
    /// @return False if an error occurred.
    virtual bool ConvertValueToBytes(Value& value,
                                     Location loc,
                                     NumericOutput& num_out) = 0;

    /// Convert a symbol to bytes.  Called by OutputSymbol() so that
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
    /// @param loc          location of the symbol reference
    /// @param num_out      numeric output
    /// @return False if an error occurred.
    virtual bool ConvertSymbolToBytes(SymbolRef sym,
                                      Location loc,
                                      NumericOutput& num_out);

protected:
    /// Overrideable implementation of OutputGap().
    /// @param size         gap size, in bytes
    /// @param source       source location
    virtual void DoOutputGap(unsigned long size,
                             SourceLocation source) = 0;

    /// Overrideable implementation of OutputBytes().
    /// @param bytes        bytes to output
    /// @param source       source location
    virtual void DoOutputBytes(const Bytes& bytes,
                               SourceLocation source) = 0;

private:
    friend class Bytecode;

    BytecodeOutput(const BytecodeOutput&);                  // not implemented
    const BytecodeOutput& operator=(const BytecodeOutput&); // not implemented

    DiagnosticsEngine& m_diags; ///< Diagnostic reporting
    Bytes m_scratch;            ///< Reusable scratch area
    Bytes m_bc_scratch;         ///< Reusable scratch area for Bytecode class
    unsigned long m_num_output; ///< Total number of bytes+gap output
};

inline Bytes&
BytecodeOutput::getScratch()
{
    m_scratch.resize(0);
    return m_scratch;
}

inline bool
BytecodeOutput::OutputValue(Value& value, Location loc, NumericOutput& num_out)
{
    if (!ConvertValueToBytes(value, loc, num_out))
        return false;
    OutputBytes(num_out.getBytes(), value.getSource().getBegin());
    num_out.EmitWarnings(m_diags);
    num_out.ClearWarnings();
    return true;
}

inline bool
BytecodeOutput::OutputSymbol(SymbolRef sym,
                             Location loc,
                             NumericOutput& num_out)
{
    if (!ConvertSymbolToBytes(sym, loc, num_out))
        return false;
    OutputBytes(num_out.getBytes(), num_out.getSource());
    num_out.EmitWarnings(m_diags);
    num_out.ClearWarnings();
    return true;
}

inline void
BytecodeOutput::OutputGap(unsigned long size, SourceLocation source)
{
    DoOutputGap(size, source);
    m_num_output += size;
}

inline void
BytecodeOutput::OutputBytes(const Bytes& bytes, SourceLocation source)
{
    DoOutputBytes(bytes, source);
    m_num_output += static_cast<unsigned long>(bytes.size());
}

/// No-output specialization of BytecodeOutput.
/// Warns on all attempts to output non-gaps.
class YASM_LIB_EXPORT BytecodeNoOutput : public BytecodeOutput
{
public:
    BytecodeNoOutput(DiagnosticsEngine& diags) : BytecodeOutput(diags) {}
    ~BytecodeNoOutput();

    /// Returns false.
    bool isBits() const;

protected:
    bool ConvertValueToBytes(Value& value,
                             Location loc,
                             NumericOutput& num_out);
    void DoOutputGap(unsigned long size, SourceLocation source);
    void DoOutputBytes(const Bytes& bytes, SourceLocation source);
};

/// Stream output specialization of BytecodeOutput.
/// Handles gaps by converting to 0 and generating a warning.
/// This does not implement ConvertValueToBytes(), so it's still a virtual
/// base class.
class YASM_LIB_EXPORT BytecodeStreamOutput : public BytecodeOutput
{
public:
    BytecodeStreamOutput(raw_ostream& os, DiagnosticsEngine& diags)
        : BytecodeOutput(diags), m_os(os)
    {}
    ~BytecodeStreamOutput();

protected:
    void DoOutputGap(unsigned long size, SourceLocation source);
    void DoOutputBytes(const Bytes& bytes, SourceLocation source);

    raw_ostream& m_os;
};

} // namespace yasm

#endif

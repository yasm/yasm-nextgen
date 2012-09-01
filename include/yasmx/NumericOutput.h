#ifndef YASM_NUMERICOUTPUT_H
#define YASM_NUMERICOUTPUT_H
//
// Numeric output handling class
//
//  Copyright (C) 2010  Peter Johnson
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#include <cctype>

#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Config/export.h"


namespace llvm
{
class APFloat;
class APInt;
}

namespace yasm
{
class Bytes;
class DiagnosticsEngine;
class IntNum;

/// This performs output of both integer and floating point numbers.
/// It takes care of checking for under/overflow/truncation and reporting
/// of warnings for such.
///
/// The number is written into the least significant bits of the destination,
/// buffer, or may be shifted into more significant bits by using setShift().
/// The destination bits are cleared before being set.
/// The number may also have a right shift applied; a warning will be generated
/// for truncation if any 1 bits are present in this portion of the number.
///
/// Respects endianness of bytes buffer, so [0] should be the first byte output
/// to the file.
class YASM_LIB_EXPORT NumericOutput
{
public:
    /// @param bytes    destination
    ///
    /// The bytes parameter is the destination; callers must size this prior
    /// to calling Output*() functions to the correct number of bytes to be
    /// output.  It may contain non-zero bits.
    NumericOutput(Bytes& bytes);

    Bytes& getBytes() { return m_bytes; }

    void setSource(SourceLocation source) { m_source = source; }
    SourceLocation getSource() const { return m_source; }

    /// @param size     output size (in bits)
    void setSize(unsigned int size)     { m_size = size; }

    /// Get output size.
    /// @return Output size, in bits.
    unsigned int getSize() const { return m_size; }

    /// @param shift    left shift (in bits)
    void setShift(unsigned int shift)   { m_shift = shift; }

    /// @param rshift   right shift (in bits)
    void setRShift(unsigned int rshift) { m_rshift = rshift; }

    /// Set signedness of output.
    /// @parma sign     true for signed, false for unsigned
    void setSign(bool sign)             { m_sign = sign; }

    /// Enable standard warnings when outputting.
    void EnableWarnings()   { m_warns_enabled = true; }

    /// Disable standard warnings (default).
    void DisableWarnings()  { m_warns_enabled = false; }

    /// Clear all detected warnings.
    void ClearWarnings()    { m_warns = 0; }

    void OutputInteger(const llvm::APInt& intn);
    void OutputInteger(const IntNum& intn);
    void OutputFloat(const llvm::APFloat& flt);

    void EmitWarnings(DiagnosticsEngine& diags) const;

private:
    enum WarnFlag
    {
        INT_OVERFLOW    = 1<<0,
        FP_UNDERFLOW    = 1<<1,
        FP_OVERFLOW     = 1<<2,
        TRUNCATED       = 1<<3
    };

    Bytes& m_bytes;
    SourceLocation m_source;
    unsigned int m_size;
    unsigned int m_shift;
    unsigned int m_rshift;
    bool m_sign;
    bool m_warns_enabled;
    unsigned int m_warns;   // WarnFlags
};

} // namespace yasm

#endif

//
// YASM bytecode output implementation
//
//  Copyright (C) 2001-2008  Peter Johnson
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
#include "yasmx/BytecodeOutput.h"

#include "llvm/Support/raw_ostream.h"
#include "yasmx/Basic/Diagnostic.h"


using namespace yasm;

BytecodeOutput::BytecodeOutput(DiagnosticsEngine& diags)
    : m_diags(diags), m_num_output(0)
{
}

BytecodeOutput::~BytecodeOutput()
{
}

bool
BytecodeOutput::isBits() const
{
    return true;
}

bool
BytecodeOutput::ConvertSymbolToBytes(SymbolRef sym,
                                     Location loc,
                                     NumericOutput& num_out)
{
    return true;
}

BytecodeNoOutput::~BytecodeNoOutput()
{
}

bool
BytecodeNoOutput::isBits() const
{
    return false;
}

bool
BytecodeNoOutput::ConvertValueToBytes(Value& value,
                                      Location loc,
                                      NumericOutput& num_out)
{
    // unnecessary; we don't actually output it anyway
    return true;
}

void
BytecodeNoOutput::DoOutputGap(unsigned long size, SourceLocation source)
{
    // expected
}

void
BytecodeNoOutput::DoOutputBytes(const Bytes& bytes, SourceLocation source)
{
    if (bytes.empty())
        return;
    Diag(source, diag::warn_nobits_data);
}

BytecodeStreamOutput::~BytecodeStreamOutput()
{
}

void
BytecodeStreamOutput::DoOutputGap(unsigned long size, SourceLocation source)
{
    // Warn that gaps are converted to 0 and write out the 0's.
    static const unsigned long BLOCK_SIZE = 4096;

    if (size == 0)
        return;

    Diag(source, diag::warn_uninit_zero);

    // Write out in chunks
    Bytes& bytes = getScratch();
    bytes.resize(BLOCK_SIZE);
    while (size > BLOCK_SIZE)
    {
        m_os << bytes;
        size -= BLOCK_SIZE;
    }
    bytes.resize(size);
    m_os << bytes;
}

void
BytecodeStreamOutput::DoOutputBytes(const Bytes& bytes, SourceLocation source)
{
    // Output bytes to file
    m_os << bytes;
}

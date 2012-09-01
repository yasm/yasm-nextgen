//
// Binary Offset Diagnostic Client (for binary files)
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
#include "yasmx/Frontend/OffsetDiagnosticPrinter.h"

#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/STLExtras.h"
#include "yasmx/Basic/SourceManager.h"


using namespace yasm;

static const enum llvm::raw_ostream::Colors noteColor =
    llvm::raw_ostream::BLACK;
static const enum llvm::raw_ostream::Colors warningColor =
    llvm::raw_ostream::MAGENTA;
static const enum llvm::raw_ostream::Colors errorColor = llvm::raw_ostream::RED;
static const enum llvm::raw_ostream::Colors fatalColor = llvm::raw_ostream::RED;
// used for changing only the bold attribute
static const enum llvm::raw_ostream::Colors savedColor =
    llvm::raw_ostream::SAVEDCOLOR;

void
OffsetDiagnosticPrinter::HandleDiagnostic(DiagnosticsEngine::Level level,
                                          const Diagnostic& info)
{
    // If the location is specified, print out offset.
    if (info.getLocation().isValid())
    {
        const SourceManager& sm = info.getSourceManager();
        FileID fid;
        unsigned int offset;

        llvm::tie(fid, offset) = sm.getDecomposedLoc(info.getLocation());

        if (m_use_colors)
            m_os.changeColor(savedColor, true);
        m_os << sm.getBuffer(fid)->getBufferIdentifier() << ':';
        if (offset != 0)
             m_os << offset << ':';

        m_os << ' ';
        if (m_use_colors)
            m_os.resetColor();
    }
    else if (!m_prefix.empty())
        m_os << m_prefix << ": ";

    if (m_use_colors)
    {
        // Print diagnostic category in bold and color
        switch (level)
        {
            case DiagnosticsEngine::Ignored: assert(0 && "Invalid diagnostic type");
            case DiagnosticsEngine::Note:    m_os.changeColor(noteColor, true); break;
            case DiagnosticsEngine::Warning: m_os.changeColor(warningColor, true); break;
            case DiagnosticsEngine::Error:   m_os.changeColor(errorColor, true); break;
            case DiagnosticsEngine::Fatal:   m_os.changeColor(fatalColor, true); break;
        }
    }

    switch (level)
    {
        case DiagnosticsEngine::Ignored: assert(0 && "Invalid diagnostic type");
        case DiagnosticsEngine::Note:    m_os << "note: "; break;
        case DiagnosticsEngine::Warning: m_os << "warning: "; break;
        case DiagnosticsEngine::Error:   m_os << "error: "; break;
        case DiagnosticsEngine::Fatal:   m_os << "fatal error: "; break;
    }

    if (m_use_colors)
        m_os.resetColor();

    SmallString<100> out_str;
    info.FormatDiagnostic(out_str);

    if (m_print_diagnostic_option)
    {
        StringRef opt = DiagnosticIDs::getWarningOptionForDiag(info.getID());
        if (!opt.empty())
        {
            out_str += " [-W";
            out_str += opt;
            out_str += ']';
        }
    }

    if (m_use_colors)
    {
        // Print warnings, errors and fatal errors in bold, no color
        switch (level)
        {
            case DiagnosticsEngine::Warning: m_os.changeColor(savedColor, true); break;
            case DiagnosticsEngine::Error:   m_os.changeColor(savedColor, true); break;
            case DiagnosticsEngine::Fatal:   m_os.changeColor(savedColor, true); break;
            default: break; //don't bold notes
        }
    }

    m_os.write(out_str.begin(), out_str.size());
    m_os << '\n';
    if (m_use_colors)
        m_os.resetColor();

    m_os.flush();
}

DiagnosticConsumer*
OffsetDiagnosticPrinter::clone(DiagnosticsEngine& diags) const
{
    return new OffsetDiagnosticPrinter(m_os, m_print_diagnostic_option,
                                       m_use_colors);
}

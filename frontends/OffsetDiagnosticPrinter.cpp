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
#include "OffsetDiagnosticPrinter.h"

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
OffsetDiagnosticPrinter::HandleDiagnostic(Diagnostic::Level level,
                                          const DiagnosticInfo& info)
{
    // If the location is specified, print out offset.
    if (info.getLocation().isValid())
    {
        const SourceManager& sm = info.getLocation().getManager();
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

    if (m_use_colors)
    {
        // Print diagnostic category in bold and color
        switch (level)
        {
            case Diagnostic::Ignored: assert(0 && "Invalid diagnostic type");
            case Diagnostic::Note:    m_os.changeColor(noteColor, true); break;
            case Diagnostic::Warning: m_os.changeColor(warningColor, true); break;
            case Diagnostic::Error:   m_os.changeColor(errorColor, true); break;
            case Diagnostic::Fatal:   m_os.changeColor(fatalColor, true); break;
        }
    }

    switch (level)
    {
        case Diagnostic::Ignored: assert(0 && "Invalid diagnostic type");
        case Diagnostic::Note:    m_os << "note: "; break;
        case Diagnostic::Warning: m_os << "warning: "; break;
        case Diagnostic::Error:   m_os << "error: "; break;
        case Diagnostic::Fatal:   m_os << "fatal error: "; break;
    }

    if (m_use_colors)
        m_os.resetColor();

    llvm::SmallString<100> out_str;
    info.FormatDiagnostic(out_str);

    if (m_print_diagnostic_option)
        if (const char* opt = Diagnostic::getWarningOptionForDiag(info.getID()))
        {
            out_str += " [-W";
            out_str += opt;
            out_str += ']';
        }

    if (m_use_colors)
    {
        // Print warnings, errors and fatal errors in bold, no color
        switch (level)
        {
            case Diagnostic::Warning: m_os.changeColor(savedColor, true); break;
            case Diagnostic::Error:   m_os.changeColor(savedColor, true); break;
            case Diagnostic::Fatal:   m_os.changeColor(savedColor, true); break;
            default: break; //don't bold notes
        }
    }

    m_os.write(out_str.begin(), out_str.size());
    m_os << '\n';
    if (m_use_colors)
        m_os.resetColor();

    m_os.flush();
}

#ifndef YASM_OFFSET_DIAGNOSTIC_PRINTER_H
#define YASM_OFFSET_DIAGNOSTIC_PRINTER_H
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
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Basic/LLVM.h"
#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Config/export.h"

namespace yasm
{

class YASM_LIB_EXPORT OffsetDiagnosticPrinter : public DiagnosticConsumer
{
    raw_ostream& m_os;
    bool m_print_diagnostic_option;
    bool m_use_colors;

    std::string m_prefix;

public:
    OffsetDiagnosticPrinter(raw_ostream &os,
                            bool print_diagnostic_option = true,
                            bool use_colors = false)
        : m_os(os)
        , m_print_diagnostic_option(print_diagnostic_option)
        , m_use_colors(use_colors)
    {}

    void setPrefix(std::string value) { m_prefix = value; }

    virtual void HandleDiagnostic(DiagnosticsEngine::Level level,
                                  const Diagnostic& info);
    virtual DiagnosticConsumer* clone(DiagnosticsEngine& diags) const;
};

} // end namspace yasm

#endif

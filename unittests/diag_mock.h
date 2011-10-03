#ifndef DIAG_MOCK_H
#define DIAG_MOCK_H
///
/// @file
/// @brief Diagnostic mocking.
///
/// @license
///  Copyright (C) 2010  Peter Johnson
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
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "yasmx/Basic/Diagnostic.h"

#include <gmock/gmock.h>


namespace yasmunit
{

class MockDiagnosticConsumer : public yasm::DiagnosticConsumer
{
public:
    MOCK_METHOD2(HandleDiagnostic, void(yasm::DiagnosticsEngine::Level DiagLevel,
                                        const yasm::Diagnostic& Info));
    yasm::DiagnosticConsumer* clone(yasm::DiagnosticsEngine& diags) const
    {
        return new MockDiagnosticConsumer;
    }
};

class MockDiagnosticString : public yasm::DiagnosticConsumer
{
public:
    void HandleDiagnostic(yasm::DiagnosticsEngine::Level level,
                          const yasm::Diagnostic& info)
    {
        llvm::SmallString<100> str;
        switch (level)
        {
            case yasm::DiagnosticsEngine::Ignored: assert(0 && "Invalid diagnostic type");
            case yasm::DiagnosticsEngine::Note:    str += "note: "; break;
            case yasm::DiagnosticsEngine::Warning: str += "warning: "; break;
            case yasm::DiagnosticsEngine::Error:   str += "error: "; break;
            case yasm::DiagnosticsEngine::Fatal:   str += "fatal error: "; break;
        }
        info.FormatDiagnostic(str);
        DiagString(str);
    }

    MOCK_METHOD1(DiagString, void(llvm::StringRef str));

    yasm::DiagnosticConsumer* clone(yasm::DiagnosticsEngine& diags) const
    {
        return new MockDiagnosticString;
    }
};

class MockDiagnosticId : public yasm::DiagnosticConsumer
{
public:
    void HandleDiagnostic(yasm::DiagnosticsEngine::Level level,
                          const yasm::Diagnostic& info)
    {
        DiagId(info.getID());
    }

    MOCK_METHOD1(DiagId, void(unsigned int id));

    yasm::DiagnosticConsumer* clone(yasm::DiagnosticsEngine& diags) const
    {
        return new MockDiagnosticId;
    }
};

} // namespace yasmunit

#endif

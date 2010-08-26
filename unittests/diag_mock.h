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

class MockDiagnosticClient : public yasm::DiagnosticClient
{
public:
    MOCK_METHOD2(HandleDiagnostic, void(yasm::Diagnostic::Level DiagLevel,
                                        const yasm::DiagnosticInfo& Info));
};

class MockDiagnosticString : public yasm::DiagnosticClient
{
public:
    void HandleDiagnostic(yasm::Diagnostic::Level level,
                          const yasm::DiagnosticInfo& info)
    {
        llvm::SmallString<100> str;
        switch (level)
        {
            case yasm::Diagnostic::Ignored: assert(0 && "Invalid diagnostic type");
            case yasm::Diagnostic::Note:    str += "note: "; break;
            case yasm::Diagnostic::Warning: str += "warning: "; break;
            case yasm::Diagnostic::Error:   str += "error: "; break;
            case yasm::Diagnostic::Fatal:   str += "fatal error: "; break;
        }
        info.FormatDiagnostic(str);
        DiagString(str);
    }

    MOCK_METHOD1(DiagString, void(llvm::StringRef str));
};

class MockDiagnosticId : public yasm::DiagnosticClient
{
public:
    void HandleDiagnostic(yasm::Diagnostic::Level level,
                          const yasm::DiagnosticInfo& info)
    {
        DiagId(info.getID());
    }

    MOCK_METHOD1(DiagId, void(unsigned int id));
};

} // namespace yasmunit

#endif

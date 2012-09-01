#ifndef YASM_OBJECT_UTIL_H
#define YASM_OBJECT_UTIL_H
///
/// @file
/// @brief Object utility functions.
///
/// @license
///  Copyright (C) 2008  Peter Johnson
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
#include "yasmx/Basic/LLVM.h"
#include "yasmx/Config/export.h"


namespace yasm
{

class DiagnosticsEngine;
class DirectiveInfo;
class ObjectFormat;

/// Ident directive handler.  This is a common implementation that matches
/// GAS behavior: initial 0 byte, each ident string 0-terminated.
/// Only string values are accepted.  The directive flags may be set to
/// Directives::ANY.
/// @param objfmt       Object format
/// @param sectname     Ident section name
/// @param info         Directive info
/// @param diags        Diagnostic reporting
YASM_LIB_EXPORT
void DirIdentCommon(ObjectFormat& objfmt,
                    StringRef sectname,
                    DirectiveInfo& info,
                    DiagnosticsEngine& diags);

} // namespace yasm

#endif

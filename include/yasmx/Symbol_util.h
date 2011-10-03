#ifndef YASM_SYMBOL_UTIL_H
#define YASM_SYMBOL_UTIL_H
///
/// @file
/// @brief Symbol utility interface.
///
/// @license
///  Copyright (C) 2001-2008  Michael Urman, Peter Johnson
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
///  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
/// @endlicense
///
#include "yasmx/Config/export.h"


namespace yasm
{

class DiagnosticsEngine;
class DirectiveInfo;
class Expr;
class NameValues;
class Object;
class SourceLocation;
class Symbol;

/// Set object-extended name/values.
/// @param sym              symbol
/// @param objext_namevals  object-extended name/values
/// @note object_namevals are moved, not copied.
YASM_LIB_EXPORT
void setObjextNameValues(Symbol& sym, NameValues& objext_namevals);

/// Get object-extended name/values, if any, associated with symbol's
/// declaration.
/// @param sym          symbol
/// @return Object-extended name/values (NULL if none).
YASM_LIB_EXPORT
/*@null@*/ const NameValues* getObjextNameValues(const Symbol& sym);

/// Get object-extended name/values, if any, associated with symbol's
/// declaration (non-const version).
/// @param sym          symbol
/// @return Object-extended name/values (NULL if none).
YASM_LIB_EXPORT
/*@null@*/ NameValues* getObjextNameValues(Symbol& sym);

/// Set common size of symbol.
/// @param sym          symbol
/// @param common_size  common size expression
/// @return Symbol (this).
YASM_LIB_EXPORT
void setCommonSize(Symbol& sym, const Expr& common_size);

/// Get common size of symbol, if symbol is declared #COMMON and a
/// size was set for it.
/// @param sym          symbol
/// @return Common size (NULL if none).
YASM_LIB_EXPORT
/*@null@*/ const Expr* getCommonSize(const Symbol& sym);

/// Get common size of symbol, if symbol is declared #COMMON and a
/// size was set for it.
/// @param sym          symbol
/// @return Common size (NULL if none).
YASM_LIB_EXPORT
/*@null@*/ Expr* getCommonSize(Symbol& sym);

/// Extern directive handler.  Sets symbol visibility to Symbol::EXTERN and
/// saves objext_namevals as associated symbol data.
/// @param info     directive info
/// @param diags    diagnostic reporter
YASM_LIB_EXPORT
void DirExtern(DirectiveInfo& info, DiagnosticsEngine& diags);

/// Extern multiple directive handler.  Sets each symbol's visibility to
/// Symbol::EXTERN.
/// @param info     directive info
/// @param diags    diagnostic reporter
YASM_LIB_EXPORT
void DirExternMulti(DirectiveInfo& info, DiagnosticsEngine& diags);

/// Global directive handler.  Sets symbol visibility to Symbol::GLOBAL and
/// saves objext_namevals as associated symbol data.
/// @param info     directive info
/// @param diags    diagnostic reporter
YASM_LIB_EXPORT
void DirGlobal(DirectiveInfo& info, DiagnosticsEngine& diags);

/// Global multiple directive handler.  Sets each symbol's visibility to
/// Symbol::GLOBAL.
/// @param info     directive info
/// @param diags    diagnostic reporter
YASM_LIB_EXPORT
void DirGlobalMulti(DirectiveInfo& info, DiagnosticsEngine& diags);

/// Common directive handler.  Sets symbol visibility to Symbol::COMMON and
/// saves common size and objext_namevals as associated symbol data.
/// @param info     directive info
/// @param diags    diagnostic reporter
YASM_LIB_EXPORT
void DirCommon(DirectiveInfo& info, DiagnosticsEngine& diags);

} // namespace yasm

#endif

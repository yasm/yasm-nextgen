#ifndef YASM_SYMBOLREF_H
#define YASM_SYMBOLREF_H
///
/// @file
/// @brief Symbol reference interface/implementation.
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
///  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
/// @endlicense
///
#include <cassert>

#include "yasmx/Config/export.h"

namespace YAML { class Emitter; }

namespace yasm
{

class Symbol;

/// Reference to a symbol.  References are not counted, so the reference
/// becomes invalid when the referred-to Symbol is destroyed.
class SymbolRef
{
public:
    SymbolRef() : m_sym(0) {}
    explicit SymbolRef(Symbol* sym) : m_sym(sym) {}
    operator Symbol* () const { return m_sym; }
    Symbol& operator*() const
    {
        assert(m_sym && "deref of null symbol reference");
        return *m_sym;
    }
    Symbol* operator->() const
    {
        assert(m_sym && "deref of null symbol reference");
        return m_sym;
    }

private:
    Symbol* m_sym;
};

/// Dump a YAML representation of a symbol reference.  For debugging purposes.
/// @param out          YAML emitter
/// @param sym          symbol reference
/// @return Emitter.
YASM_LIB_EXPORT
YAML::Emitter& operator<< (YAML::Emitter& out, const SymbolRef& sym);

} // namespace yasm

#endif

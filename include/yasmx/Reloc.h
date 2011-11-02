#ifndef YASM_RELOC_H
#define YASM_RELOC_H
///
/// @file
/// @brief Relocation interface.
///
/// @license
///  Copyright (C) 2001-2007  Peter Johnson
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
#include <memory>
#include <string>

#include "yasmx/Config/export.h"
#include "yasmx/DebugDumper.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/SymbolRef.h"


namespace yasm
{

/// Basic YASM relocation.  Object formats will need to extend this
/// structure with additional fields for relocation type, etc.
class YASM_LIB_EXPORT Reloc
{
public:
    Reloc(const IntNum& addr, SymbolRef sym);
    virtual ~Reloc();

    SymbolRef getSymbol() { return m_sym; }
    const SymbolRef getSymbol() const { return m_sym; }

    const IntNum& getAddress() const { return m_addr; }

    /// Get the relocated value as an expression.
    /// Should be overloaded by derived classes that have addends.
    /// The default implementation simply returns the symbol as the value.
    /// @return Relocated value.
    virtual Expr getValue() const;

    /// Get the name of the relocation type (a string).
    /// @return Type name.
    virtual std::string getTypeName() const = 0;

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

protected:
    IntNum m_addr;      ///< Offset (address) within section
    SymbolRef m_sym;    ///< Relocated symbol

#ifdef WITH_XML
    /// Write derived class XML representation.  For debugging purposes.
    /// Default implementation outputs a simple null.
    /// @param out          XML node
    /// @return Root node.
    virtual pugi::xml_node DoWrite(pugi::xml_node out) const;
#endif // WITH_XML

private:
    Reloc(const Reloc&);                    // not implemented
    const Reloc& operator=(const Reloc&);   // not implemented
};

} // namespace yasm

#endif

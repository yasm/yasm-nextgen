#ifndef YASM_XDFRELOC_H
#define YASM_XDFRELOC_H
//
// Extended Dynamic Object format relocation
//
//  Copyright (C) 2004-2007  Peter Johnson
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
#include <string>

#include "yasmx/Config/export.h"
#include "yasmx/Reloc.h"
#include "yasmx/SymbolRef.h"


namespace yasm
{

class Bytes;
class IntNum;
class Value;

namespace objfmt
{

class YASM_STD_EXPORT XdfReloc : public Reloc
{
public:
    enum Type
    {
        XDF_REL = 1,        //< relative to segment
        XDF_WRT = 2,        //< relative to symbol
        XDF_RIP = 4,        //< RIP-relative
        XDF_SEG = 8         //< segment containing symbol
    };
    enum Size
    {
        XDF_8  = 1,
        XDF_16 = 2,
        XDF_32 = 4,
        XDF_64 = 8
    };

    XdfReloc(const IntNum& addr,
             SymbolRef sym,
             SymbolRef base,
             Type type,
             Size size,
             unsigned int shift);
    XdfReloc(const IntNum& addr, const Value& value, bool ip_rel);
    ~XdfReloc();

    Expr getValue() const;
    std::string getTypeName() const;

    Type getType() const { return m_type; }

    void Write(Bytes& bytes) const;

protected:
#ifdef WITH_XML
    pugi::xml_node DoWrite(pugi::xml_node out) const;
#endif // WITH_XML

private:
    SymbolRef m_base;       //< base symbol (for WRT)
    Type m_type;            //< type of relocation
    Size m_size;            //< size of relocation
    unsigned int m_shift;   //< relocation shift (0,4,8,16,24,32)
};

}} // namespace yasm::objfmt

#endif

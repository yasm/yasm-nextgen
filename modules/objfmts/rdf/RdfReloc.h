#ifndef YASM_RDFRELOC_H
#define YASM_RDFRELOC_H
//
// Relocatable Dynamic Object (RDOFF) format relocation
//
//  Copyright (C) 2006-2007  Peter Johnson
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
#include "yasmx/Config/export.h"
#include "yasmx/Reloc.h"


namespace yasm
{

class Bytes;

namespace objfmt
{

class YASM_STD_EXPORT RdfReloc : public Reloc
{
public:
    enum Type
    {
        RDF_NORM,           ///< normal
        RDF_REL,            ///< relative to current position
        RDF_SEG             ///< segment containing symbol
    };

    RdfReloc(const IntNum& addr,
             SymbolRef sym,
             Type type,
             unsigned int size,
             unsigned int refseg);
    ~RdfReloc();

    Expr getValue() const;
    std::string getTypeName() const;

    Type getType() const { return m_type; }

    void Write(Bytes& bytes, unsigned int scnum) const;

protected:
#ifdef WITH_XML
    pugi::xml_node DoWrite(pugi::xml_node out) const;
#endif // WITH_XML

private:
    Type m_type;            ///< type of relocation
    unsigned int m_size;    ///< size of relocation
    unsigned int m_refseg;  ///< relocated symbol "segment number"
};

}} // namespace yasm::objfmt

#endif

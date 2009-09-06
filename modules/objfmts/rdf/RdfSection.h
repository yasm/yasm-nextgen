#ifndef YASM_RDFSECTION_H
#define YASM_RDFSECTION_H
//
// Relocatable Dynamic Object (RDOFF) format section
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
#include "yasmx/AssocData.h"
#include "yasmx/Bytes.h"
#include "yasmx/SymbolRef.h"


namespace yasm
{

class IntNum;
class InputBuffer;
class Section;

namespace objfmt
{
namespace rdf
{

struct RdfSection : public AssocData
{
    static const char* key;

    enum { SECTHEAD_SIZE = 10 };

    enum Type
    {
        RDF_BSS = 0,
        RDF_CODE = 1,
        RDF_DATA = 2,
        RDF_COMMENT = 3,
        RDF_LCOMMENT = 4,
        RDF_PCOMMENT = 5,
        RDF_SYMDEBUG = 6,
        RDF_LINEDEBUG = 7,
        RDF_UNKNOWN = 0xffff
    };

    RdfSection(Type type_, SymbolRef sym_);
    ~RdfSection();
    void Write(YAML::Emitter& out) const;

    /// Write section header to bytes.
    void Write(Bytes& bytes, const Section& sect) const;

    /// Read section header from bytes.
    /// @param size     section size (output)
    /// @param name     section name (manufactured from type and scnum)
    void Read(InputBuffer& inbuf, unsigned long* size, std::string* name);

    SymbolRef sym;          ///< symbol created for this section
    unsigned int scnum;     ///< section number (0=first section)
    Type type;              ///< section type
    unsigned int reserved;  ///< reserved data
    Bytes raw_data;         ///< raw section data, only used during output
};

}}} // namespace yasm::objfmt::rdf

#endif

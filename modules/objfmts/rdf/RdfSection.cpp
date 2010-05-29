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
#include "RdfSection.h"

#include "llvm/Support/raw_ostream.h"
#include "YAML/emitter.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/InputBuffer.h"


using namespace yasm;
using namespace yasm::objfmt;

const char* RdfSection::key = "objfmt::rdf::RdfSection";

RdfSection::RdfSection(Type type_, SymbolRef sym_)
    : sym(sym_)
    , scnum(0)
    , type(type_)
    , reserved(0)
{
}

RdfSection::~RdfSection()
{
}

void
RdfSection::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << key;
    out << YAML::Key << "sym" << YAML::Value << sym;
    out << YAML::Key << "scnum" << YAML::Value << scnum;
    out << YAML::Key << "reserved" << YAML::Value << reserved;
    out << YAML::Key << "raw data" << YAML::Value << raw_data;
    out << YAML::EndMap;
}

void
RdfSection::Write(Bytes& bytes, const Section& sect) const
{
    bytes.setLittleEndian();
    Write16(bytes, type);               // type
    Write16(bytes, scnum);              // number
    Write16(bytes, reserved);           // reserved
    Write32(bytes, raw_data.size());    // length
}

void
RdfSection::Read(InputBuffer& inbuf, unsigned long* size, std::string* name)
{
    inbuf.setLittleEndian();
    type = static_cast<Type>(ReadU16(inbuf));   // type
    scnum = ReadU16(inbuf);                     // number
    reserved = ReadU16(inbuf);                  // reserved
    *size = ReadU32(inbuf);                     // length

    // Manufacture a name based on the type and scnum
    name->clear();
    llvm::raw_string_ostream oss(*name);
    switch (type)
    {
        case RDF_BSS:       oss << ".bss"; break;
        case RDF_CODE:      oss << ".text"; break;
        case RDF_DATA:      oss << ".data"; break;
        case RDF_COMMENT:   oss << ".comment"; break;
        case RDF_LCOMMENT:  oss << ".lcomment"; break;
        case RDF_PCOMMENT:  oss << ".pcomment"; break;
        case RDF_SYMDEBUG:  oss << ".symdebug"; break;
        case RDF_LINEDEBUG: oss << ".linedebug"; break;
        default:            oss << ".unknown"; break;
    }
    // Don't suffix the most common combinations
    if (type == RDF_BSS || scnum+1 != static_cast<unsigned int>(type))
        oss << scnum;
    oss.flush();
}

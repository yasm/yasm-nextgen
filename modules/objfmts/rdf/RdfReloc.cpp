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
#include "RdfReloc.h"

#include "llvm/Support/raw_ostream.h"
#include "YAML/emitter.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"

#include "RdfRecords.h"


using namespace yasm;
using namespace objfmt;

RdfReloc::RdfReloc(const IntNum& addr,
                   SymbolRef sym,
                   Type type,
                   unsigned int size,
                   unsigned int refseg)
    : Reloc(addr, sym)
    , m_type(type)
    , m_size(size)
    , m_refseg(refseg)
{
}

RdfReloc::~RdfReloc()
{
}

Expr
RdfReloc::getValue() const
{
    Expr e(m_sym);
    if (m_type == RDF_SEG)
        e.Calc(Op::SEG);

    return e;
}

std::string
RdfReloc::getTypeName() const
{
    std::string s;
    llvm::raw_string_ostream oss(s);

    switch (m_type)
    {
        case RDF_NORM: oss << "NORM_"; break;
        case RDF_REL: oss << "REL_"; break;
        case RDF_SEG: oss << "SEG_"; break;
    }

    oss << m_size;

    oss.flush();
    return s;
}

void
RdfReloc::Write(Bytes& bytes, unsigned int scnum) const
{
    if (m_type == RDF_SEG)
        Write8(bytes, RDFREC_SEGRELOC);
    else
        Write8(bytes, RDFREC_RELOC);

    Write8(bytes, 8);               // record length

    // Section number, +0x40 if relative reloc
    Write8(bytes, scnum + (m_type == RDF_REL ? 0x40 : 0));

    bytes.setLittleEndian();
    Write32(bytes, m_addr);         // offset of relocation
    Write8(bytes, m_size);          // size of relocation
    Write16(bytes, m_refseg);       // relocated symbol
}

void
RdfReloc::DoWrite(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << "RdfReloc";
    out << YAML::Key << "size" << YAML::Value << m_size;
    out << YAML::Key << "refseg" << YAML::Value << m_refseg;
    out << YAML::EndMap;
}

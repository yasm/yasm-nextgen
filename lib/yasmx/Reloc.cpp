//
// Relocation implementation.
//
//  Copyright (C) 2001-2007  Peter Johnson
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
#include "yasmx/Reloc.h"

#include "llvm/Support/raw_ostream.h"
#include "YAML/emitter.h"


namespace yasm
{

Reloc::Reloc(const IntNum& addr, SymbolRef sym)
    : m_addr(addr),
      m_sym(sym)
{
}

Reloc::~Reloc()
{
}

Expr
Reloc::getValue() const
{
    return Expr(m_sym);
}

void
Reloc::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "reloc type" << YAML::Value << getTypeName();
    out << YAML::Key << "addr" << YAML::Value << m_addr;
    out << YAML::Key << "sym" << YAML::Value << m_sym;
    out << YAML::Key << "implementation" << YAML::Value;
    DoWrite(out);
    out << YAML::EndMap;
}

void
Reloc::Dump() const
{
    YAML::Emitter out;
    Write(out);
    llvm::errs() << out.c_str() << '\n';
}

void
Reloc::DoWrite(YAML::Emitter& out) const
{
    out << YAML::Null;
}

} // namespace yasm

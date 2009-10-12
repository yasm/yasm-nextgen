//
// Section implementation.
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
#include "yasmx/Section.h"

#include "util.h"

#include "clang/Basic/SourceLocation.h"
#include "llvm/Support/raw_ostream.h"
#include "YAML/emitter.h"
#include "yasmx/IntNum.h"
#include "yasmx/Reloc.h"


namespace yasm
{

Section::Section(const llvm::StringRef& name,
                 bool code,
                 bool bss,
                 clang::SourceLocation source)
    : m_name(name),
      m_vma(0),
      m_lma(0),
      m_filepos(0),
      m_align(0),
      m_code(code),
      m_bss(bss),
      m_def(false),
      m_relocs_owner(m_relocs)
{
}

Section::~Section()
{
}

Section*
Section::AsSection()
{
    return this;
}

const Section*
Section::AsSection() const
{
    return this;
}

void
Section::AddReloc(std::auto_ptr<Reloc> reloc)
{
    m_relocs.push_back(reloc.release());
}

void
Section::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << m_name;
    out << YAML::Key << "vma" << YAML::Value << m_vma;
    out << YAML::Key << "lma" << YAML::Value << m_lma;
    out << YAML::Key << "filepos" << YAML::Value << m_filepos;
    out << YAML::Key << "align" << YAML::Value << m_align;
    out << YAML::Key << "code" << YAML::Value << m_code;
    out << YAML::Key << "bss" << YAML::Value << m_bss;
    out << YAML::Key << "default" << YAML::Value << m_def;

    out << YAML::Key << "assoc data" << YAML::Value;
    AssocDataContainer::Write(out);

    out << YAML::Key << "bytecodes" << YAML::Value;
    BytecodeContainer::Write(out);

    out << YAML::Key << "relocs" << YAML::Value;
    if (m_relocs.empty())
        out << YAML::Flow;
    out << YAML::BeginSeq;
    for (Relocs::const_iterator i=m_relocs.begin(), end=m_relocs.end();
         i != end; ++i)
        out << *i;
    out << YAML::EndSeq;
    out << YAML::EndMap;
}

void
Section::Dump() const
{
    YAML::Emitter out;
    Write(out);
    llvm::errs() << out.c_str() << '\n';
}

} // namespace yasm

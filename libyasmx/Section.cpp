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

#include "yasmx/Support/marg_ostream.h"

#include "yasmx/IntNum.h"
#include "yasmx/Reloc.h"


namespace yasm
{

Section::Section(const std::string& name,
                 bool code,
                 bool bss,
                 unsigned long line)
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
Section::as_section()
{
    return this;
}

const Section*
Section::as_section() const
{
    return this;
}

void
Section::add_reloc(std::auto_ptr<Reloc> reloc)
{
    m_relocs.push_back(reloc.release());
}

marg_ostream&
operator<< (marg_ostream& os, const Section& section)
{
    os << "name=" << section.get_name() << '\n';
    os << "vma=" << section.get_vma() << '\n';
    os << "lma=" << section.get_lma() << '\n';
    os << "filepos=" << section.get_filepos() << '\n';
    os << "align=" << section.get_align() << '\n';
    os << "code=" << section.is_code() << '\n';
    os << "bss=" << section.is_bss() << '\n';
    os << "default=" << section.is_default() << '\n';

    os << "Associated data:\n";
    ++os;
    os << static_cast<const AssocDataContainer&>(section);
    --os;

    os << static_cast<const BytecodeContainer&>(section);

    // TODO: relocs

    return os;
}

} // namespace yasm

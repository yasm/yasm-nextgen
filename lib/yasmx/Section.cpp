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

#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/IntNum.h"
#include "yasmx/Reloc.h"


using namespace yasm;

Section::Section(StringRef name, bool code, bool bss, SourceLocation source)
    : BytecodeContainer(this)
    , m_name(name)
    , m_object(0)
    , m_sym(0)
    , m_vma(0)
    , m_lma(0)
    , m_filepos(0)
    , m_align(0)
    , m_code(code)
    , m_bss(bss)
    , m_def(false)
    , m_relocs_owner(m_relocs)
{
}

Section::~Section()
{
}

void
Section::AddReloc(std::auto_ptr<Reloc> reloc)
{
    m_relocs.push_back(reloc.release());
}

#ifdef WITH_XML
pugi::xml_node
Section::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("Section");
    root.append_attribute("id") = m_name.c_str();
    append_child(root, "Name", m_name);
    append_child(root, "Sym", m_sym);
    append_child(root, "VMA", m_vma);
    append_child(root, "LMA", m_lma);
    append_child(root, "FilePos", m_filepos);
    append_child(root, "Align", m_align);
    if (m_code)
        root.append_attribute("code") = true;
    if (m_bss)
        root.append_attribute("bss") = true;
    if (m_def)
        root.append_attribute("default") = true;

    AssocDataContainer::Write(root);
    BytecodeContainer::Write(root);

    for (Relocs::const_iterator i=m_relocs.begin(), end=m_relocs.end();
         i != end; ++i)
        append_data(root, *i);
    return root;
}
#endif // WITH_XML

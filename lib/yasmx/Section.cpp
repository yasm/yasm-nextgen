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

Section::Section(llvm::StringRef name,
                 bool code,
                 bool bss,
                 SourceLocation source)
    : m_name(name),
      m_sym(0),
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

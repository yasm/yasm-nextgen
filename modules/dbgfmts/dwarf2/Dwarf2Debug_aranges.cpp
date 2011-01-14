//
// DWARF2 debugging format - address range table
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
#include "Dwarf2Debug.h"

#include "yasmx/Bytecode.h"
#include "yasmx/BytecodeContainer.h"
#include "yasmx/Expr.h"
#include "yasmx/Object.h"
#include "yasmx/ObjectFormat.h"
#include "yasmx/Section.h"

#include "Dwarf2Section.h"


using namespace yasm;
using namespace yasm::dbgfmt;

Section&
Dwarf2Debug::Generate_aranges(Section& debug_info)
{
    Section* debug_aranges = m_object.FindSection(".debug_aranges");

    // align ranges to 2x address size (range size)
    if (debug_aranges)
    {
        AppendAlign(*debug_aranges, Expr(2*m_sizeof_address), Expr(), Expr(), 0,
                    SourceLocation());
    }
    else
    {
        debug_aranges = m_objfmt->AppendSection(".debug_aranges",
                                                SourceLocation(),
                                                *m_diags);
        debug_aranges->setAlign(2*m_sizeof_address);
    }

    // header
    Location head = AppendHead(*debug_aranges, &debug_info, true, true);
    AppendAlign(*debug_aranges, Expr(m_sizeof_address), Expr(), Expr(), 0,
                SourceLocation());

    for (Object::section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        Dwarf2Section* dwarf2sect = i->getAssocData<Dwarf2Section>();
        if (!dwarf2sect)
            continue;   // no line data for this section

        // Create address range descriptor
        Expr::Ptr start(new Expr(i->getSymbol()));
        AppendData(*debug_aranges, start, m_sizeof_address, *m_object.getArch(),
                   SourceLocation(), *m_diags);

        IntNum length;
        CalcDist(i->getBeginLoc(), i->getEndLoc(), &length);
        AppendData(*debug_aranges, length, m_sizeof_address,
                   *m_object.getArch());
    }

    // Terminate with empty address range descriptor
    AppendData(*debug_aranges, 0, m_sizeof_address, *m_object.getArch());
    AppendData(*debug_aranges, 0, m_sizeof_address, *m_object.getArch());

    // mark end of aranges information
    debug_aranges->UpdateOffsets(*m_diags);
    setHeadEnd(head, debug_aranges->getEndLoc());

    return *debug_aranges;
}


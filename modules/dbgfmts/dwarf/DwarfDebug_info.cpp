//
// DWARF debugging format - info and abbreviation tables
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
#include "DwarfDebug.h"

#include "config.h"
#include "llvm/Support/Path.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_leb128.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/Expr.h"
#include "yasmx/Object.h"
#include "yasmx/ObjectFormat.h"
#include "yasmx/Section.h"


using namespace yasm;
using namespace yasm::dbgfmt;

static void
AppendAbbrevHeader(Bytes& bytes,
                   unsigned long id,
                   DwarfTag tag,
                   bool has_children)
{
    WriteULEB128(bytes, id);
    WriteULEB128(bytes, tag);
    Write8(bytes, has_children ? 1 : 0);
}

static void
AppendAbbrevAttr(Bytes& bytes, DwarfAttribute name, DwarfForm form)
{
    WriteULEB128(bytes, name);
    WriteULEB128(bytes, form);
}

static void
AppendAbbrevTail(Bytes& bytes)
{
    Write8(bytes, 0);
    Write8(bytes, 0);
}

Section&
DwarfDebug::Generate_info(Section& debug_line, Section* main_code)
{
    Section& debug_abbrev =
        *m_objfmt->AppendSection(".debug_abbrev", SourceLocation(), *m_diags);
    Section& debug_info =
        *m_objfmt->AppendSection(".debug_info", SourceLocation(), *m_diags);

    debug_abbrev.setAlign(0);
    debug_info.setAlign(0);

    // Create abbreviation table entry for compilation unit
    Bytes& abbrev = debug_abbrev.FreshBytecode().getFixed();
    AppendAbbrevHeader(abbrev, 1, DW_TAG_compile_unit, false);

    // info header
    Location head = AppendHead(debug_info, &debug_abbrev, true, false);

    // Generate abbreviations at the same time as info (since they're linked
    // and we're only generating one piece of info).

    // generating info using abbrev 1
    AppendLEB128(debug_info, 1, false, SourceLocation(), *m_diags);

    // statement list (line numbers)
    AppendAbbrevAttr(abbrev, DW_AT_stmt_list, DW_FORM_data4);
    AppendData(debug_info, Expr::Ptr(new Expr(debug_line.getSymbol())),
               m_sizeof_offset, *m_object.getArch(), SourceLocation(),
               *m_diags);

    if (main_code)
    {
        SymbolRef first = main_code->getSymbol();
        // All code is contiguous in one section
        AppendAbbrevAttr(abbrev, DW_AT_low_pc, DW_FORM_addr);
        AppendData(debug_info, Expr::Ptr(new Expr(first)), m_sizeof_address,
                   *m_object.getArch(), SourceLocation(), *m_diags);

        AppendAbbrevAttr(abbrev, DW_AT_high_pc, DW_FORM_addr);
        Expr::Ptr last(new Expr(first));
        last->Calc(Op::ADD, (main_code->bytecodes_back().getTailOffset() -
                             main_code->bytecodes_front().getOffset()));
        AppendData(debug_info, last, m_sizeof_address, *m_object.getArch(),
                   SourceLocation(), *m_diags);
    }

    // input filename: use file 1 if specified, otherwise use source filename
    AppendAbbrevAttr(abbrev, DW_AT_name, DW_FORM_string);
    if (!m_filenames.empty() && !m_filenames[0].pathname.empty())
        AppendData(debug_info, m_filenames[0].pathname, true);
    else
        AppendData(debug_info, m_object.getSourceFilename(), true);

    // compile directory (current working directory)
    AppendAbbrevAttr(abbrev, DW_AT_comp_dir, DW_FORM_string);
    AppendData(debug_info, llvm::sys::Path::GetCurrentDirectory().str(), true);

    // producer - assembler name
    AppendAbbrevAttr(abbrev, DW_AT_producer, DW_FORM_string);
    AppendData(debug_info, PACKAGE_NAME " " PACKAGE_VERSION, true);

    // language - no standard code for assembler, use MIPS as a substitute
    AppendAbbrevAttr(abbrev, DW_AT_language, DW_FORM_data2);
    AppendData(debug_info, static_cast<unsigned int>(DW_LANG_Mips_Assembler),
               2, *m_object.getArch());

    // Terminate list of abbreviations
    AppendAbbrevTail(abbrev);
    Write8(abbrev, 0);

    // mark end of info
    debug_info.UpdateOffsets(*m_diags);
    setHeadEnd(head, debug_info.getEndLoc());

    return debug_info;
}

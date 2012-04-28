//
// DWARF debugging format
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

#include <algorithm>

#include "yasmx/Parse/Directive.h"
#include "yasmx/Support/registry.h"
#include "yasmx/Arch.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/Object.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"


using namespace yasm;
using namespace yasm::dbgfmt;

DwarfDebug::DwarfDebug(const DebugFormatModule& module, Object& object)
    : DebugFormat(module, object)
    , m_format(FORMAT_32BIT)    // TODO: flexible?
    , m_sizeof_address(object.getArch()->getAddressSize()/8)
    , m_min_insn_len(object.getArch()->getModule().getMinInsnLen())
    , m_fdes_owner(m_fdes)
    , m_cur_fde(0)
{
    switch (m_format)
    {
        case FORMAT_32BIT: m_sizeof_offset = 4; break;
        case FORMAT_64BIT: m_sizeof_offset = 8; break;
    }
    InitCfi(*object.getArch());
}

DwarfDebug::~DwarfDebug()
{
}

void
DwarfDebug::GenerateDebug(ObjectFormat& objfmt,
                          SourceManager& smgr,
                          Diagnostic& diags)
{
    m_objfmt = &objfmt;
    m_diags = &diags;

    size_t num_line_sections = 0;
    Section* main_code = 0;

    // If we don't have any .file directives, generate line information
    // based on the asm source.
    Section& debug_line =
        Generate_line(smgr, !gotFile(), &main_code, &num_line_sections);
    debug_line.Finalize(diags);

    // If we don't have a .debug_info (or it's empty), generate the minimal
    // set of .debug_info, .debug_aranges, and .debug_abbrev so that the
    // .debug_line we're generating is actually useful.
    Section* debug_info = m_object.FindSection(".debug_info");
    if (num_line_sections > 0 &&
        (!debug_info ||
         debug_info->bytecodes_begin() == debug_info->bytecodes_end()))
    {
        debug_info = &Generate_info(debug_line, main_code);
        debug_info->Finalize(diags);
        Section& aranges = Generate_aranges(*debug_info);
        aranges.Finalize(diags);
        //Generate_pubnames(*debug_info);
    }
}

void
DwarfDebug::Generate(ObjectFormat& objfmt,
                     SourceManager& smgr,
                     Diagnostic& diags)
{
    GenerateCfi(objfmt, smgr, diags);
    GenerateDebug(objfmt, smgr, diags);
}

DwarfPassDebug::~DwarfPassDebug()
{
}

void
DwarfPassDebug::Generate(ObjectFormat& objfmt,
                         SourceManager& smgr,
                         Diagnostic& diags)
{
    // Always generate CFI.
    GenerateCfi(objfmt, smgr, diags);

    // If we don't have any .file directives, don't generate any debug info.
    if (!gotFile())
        return;
    GenerateDebug(objfmt, smgr, diags);
}

Location
DwarfDebug::AppendHead(Section& sect,
                       /*@null@*/ Section* debug_ptr,
                       bool with_address,
                       bool with_segment)
{
    if (m_format == FORMAT_64BIT)
    {
        AppendByte(sect, 0xff);
        AppendByte(sect, 0xff);
        AppendByte(sect, 0xff);
        AppendByte(sect, 0xff);
    }

    // Total length of aranges info (following this field).
    // Note this needs to be fixed up by setHeadLength().
    Location loc = sect.getEndLoc();
    AppendData(sect, 0, m_sizeof_offset, *m_object.getArch());

    // DWARF version
    AppendData(sect, 2, 2, *m_object.getArch());

    // Pointer to another debug section
    if (debug_ptr)
    {
        AppendData(sect, Expr::Ptr(new Expr(debug_ptr->getSymbol())),
                   m_sizeof_offset, *m_object.getArch(), SourceLocation(),
                   *m_diags);
    }

    // Size of the offset portion of the address
    if (with_address)
        AppendByte(sect, m_sizeof_address);

    // Size of a segment descriptor.  0 = flat address space
    if (with_segment)
        AppendByte(sect, 0);

    return loc;
}

void
DwarfDebug::setHeadEnd(Location head, Location tail)
{
    assert(head.bc->getContainer() == tail.bc->getContainer());

    IntNum size;
    CalcDist(head, tail, &size);
    size -= m_sizeof_offset;

    Bytes bytes;
    WriteN(bytes, size, m_sizeof_offset*8);
    std::copy(bytes.begin(), bytes.end(), head.bc->getFixed().begin()+head.off);
}

void
DwarfDebug::AddDebugDirectives(Directives& dirs, llvm::StringRef parser)
{
    static const Directives::Init<DwarfDebug> nasm_dirs[] =
    {
        {"loc",     &DwarfDebug::DirLoc,   Directives::ARG_REQUIRED},
        {"file",    &DwarfDebug::DirFile,  Directives::ARG_REQUIRED},
    };
    static const Directives::Init<DwarfDebug> gas_dirs[] =
    {
        {".loc",    &DwarfDebug::DirLoc,   Directives::ARG_REQUIRED},
        {".file",   &DwarfDebug::DirFile,  Directives::ARG_REQUIRED},
    };

    if (parser.equals_lower("nasm"))
        dirs.AddArray(this, nasm_dirs);
    else if (parser.equals_lower("gas") || parser.equals_lower("gnu"))
        dirs.AddArray(this, gas_dirs);
}

void
DwarfDebug::AddDirectives(Directives& dirs, llvm::StringRef parser)
{
    AddDebugDirectives(dirs, parser);
    AddCfiDirectives(dirs, parser);
}

CfiDebug::~CfiDebug()
{
}

void
CfiDebug::AddDirectives(Directives& dirs, llvm::StringRef parser)
{
    AddCfiDirectives(dirs, parser);
}

void
CfiDebug::Generate(ObjectFormat& objfmt,
                   SourceManager& smgr,
                   Diagnostic& diags)
{
    GenerateCfi(objfmt, smgr, diags);
}

void
yasm_dbgfmt_dwarf_DoRegister()
{
    RegisterModule<DebugFormatModule,
                   DebugFormatModuleImpl<DwarfDebug> >("dwarf");
    RegisterModule<DebugFormatModule,
                   DebugFormatModuleImpl<DwarfPassDebug> >("dwarfpass");
    RegisterModule<DebugFormatModule,
                   DebugFormatModuleImpl<DwarfDebug> >("dwarf2");
    RegisterModule<DebugFormatModule,
                   DebugFormatModuleImpl<DwarfPassDebug> >("dwarf2pass");
    RegisterModule<DebugFormatModule,
                   DebugFormatModuleImpl<CfiDebug> >("cfi");
}

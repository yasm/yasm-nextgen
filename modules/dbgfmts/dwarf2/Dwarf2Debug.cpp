//
// DWARF2 debugging format
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

#include <algorithm>

#include "yasmx/Support/registry.h"
#include "yasmx/Arch.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/Directive.h"
#include "yasmx/Object.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"

#include "util.h"


using namespace yasm;
using namespace yasm::dbgfmt;

Dwarf2Debug::Dwarf2Debug(const DebugFormatModule& module, Object& object)
    : DebugFormat(module, object)
    , m_format(FORMAT_32BIT)    // TODO: flexible?
    , m_sizeof_address(object.getArch()->getAddressSize()/8)
    , m_min_insn_len(object.getArch()->getModule().getMinInsnLen())
{
    switch (m_format)
    {
        case FORMAT_32BIT: m_sizeof_offset = 4; break;
        case FORMAT_64BIT: m_sizeof_offset = 8; break;
    }
}

Dwarf2Debug::~Dwarf2Debug()
{
}

void
Dwarf2Debug::Generate(ObjectFormat& objfmt,
                      clang::SourceManager& smgr,
                      Diagnostic& diags)
{
    m_objfmt = &objfmt;
    m_diags = &diags;

    size_t num_line_sections = 0;
    Section* main_code = 0;

    // If we don't have any .file directives, generate line information
    // based on the asm source.
    Section& debug_line =
        Generate_line(smgr, m_filenames.empty(), &main_code,
                      &num_line_sections);

    // If we don't have a .debug_info (or it's empty), generate the minimal
    // set of .debug_info, .debug_aranges, and .debug_abbrev so that the
    // .debug_line we're generating is actually useful.
    Section* debug_info = m_object.FindSection(".debug_info");
    if (num_line_sections > 0 &&
        (!debug_info ||
         debug_info->bytecodes_begin() == debug_info->bytecodes_end()))
    {
        debug_info = &Generate_info(debug_line, main_code);
        Generate_aranges(*debug_info);
        //Generate_pubnames(*debug_info);
    }
}

Location
Dwarf2Debug::AppendHead(Section& sect,
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
                   m_sizeof_offset, *m_object.getArch(),
                   clang::SourceLocation());
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
Dwarf2Debug::setHeadEnd(Location head, Location tail)
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
Dwarf2Debug::AddDirectives(Directives& dirs, llvm::StringRef parser)
{
    static const Directives::Init<Dwarf2Debug> nasm_dirs[] =
    {
        {"loc",     &Dwarf2Debug::DirLoc,   Directives::ARG_REQUIRED},
        {"file",    &Dwarf2Debug::DirFile,  Directives::ARG_REQUIRED},
    };
    static const Directives::Init<Dwarf2Debug> gas_dirs[] =
    {
        {".loc",    &Dwarf2Debug::DirLoc,   Directives::ARG_REQUIRED},
        {".file",   &Dwarf2Debug::DirFile,  Directives::ARG_REQUIRED},
    };

    if (parser.equals_lower("nasm"))
        dirs.AddArray(this, nasm_dirs, NELEMS(nasm_dirs));
    else if (parser.equals_lower("gas") || parser.equals_lower("gnu"))
        dirs.AddArray(this, gas_dirs, NELEMS(gas_dirs));
}

void
yasm_dbgfmt_dwarf2_DoRegister()
{
    RegisterModule<DebugFormatModule,
                   DebugFormatModuleImpl<Dwarf2Debug> >("dwarf2");
}

//
// DWARF debugging format - line information
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
// 3. Neither the name of the author nor the names of other contributors
//    may be used to endorse or promote products derived from this
//    software without specific prior written permission.
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

#include "llvm/Support/Path.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Basic/FileManager.h"
#include "yasmx/Basic/SourceManager.h"
#include "yasmx/Parse/Directive.h"
#include "yasmx/Parse/NameValue.h"
#include "yasmx/Bytecode.h"
#include "yasmx/BytecodeContainer.h"
#include "yasmx/Bytes_leb128.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/Section.h"
#include "yasmx/Object.h"
#include "yasmx/ObjectFormat.h"

#include "DwarfSection.h"


#ifndef NELEMS
#define NELEMS(array)   (sizeof(array) / sizeof(array[0]))
#endif

using namespace yasm;
using namespace yasm::dbgfmt;

// # of LEB128 operands needed for each of the above opcodes
static unsigned char line_opcode_num_operands[DWARF_LINE_OPCODE_BASE-1] =
{
    0,  // DW_LNS_copy
    1,  // DW_LNS_advance_pc
    1,  // DW_LNS_advance_line
    1,  // DW_LNS_set_file
    1,  // DW_LNS_set_column
    0,  // DW_LNS_negate_stmt
    0,  // DW_LNS_set_basic_block
    0,  // DW_LNS_const_add_pc
    1,  // DW_LNS_fixed_advance_pc
#ifdef WITH_DWARF3
    0,  // DW_LNS_set_prologue_end
    0,  // DW_LNS_set_epilogue_begin
    1   // DW_LNS_set_isa
#endif
};

// Base and range for line offsets in special opcodes
static const int DWARF_LINE_BASE = -5;
static const int DWARF_LINE_RANGE = 14;

#define DWARF_MAX_SPECIAL_ADDR_DELTA   \
    (((255-DWARF_LINE_OPCODE_BASE)/DWARF_LINE_RANGE)*m_min_insn_len)

// Initial value of is_stmt register
#define DWARF_LINE_DEFAULT_IS_STMT      1

namespace yasm { namespace dbgfmt {
// Line number state machine register state
struct DwarfLineState
{
    // DWARF state machine registers
    unsigned long address;
    unsigned long file;
    unsigned long line;
    unsigned long column;
    unsigned long isa;
    bool is_stmt;

    // other state information
    Location prevloc;
};
}} // namespace yasm::dbgfmt

namespace {
class MatchFileDir
{
public:
    MatchFileDir(StringRef name, unsigned long dir)
        : m_name(name), m_dir(dir)
    {}

    bool operator() (const DwarfDebug::Filename& file)
    {
        if (file.filename.empty())
            return true;
        return (m_dir == file.dir && m_name == file.filename);
    }

private:
    StringRef m_name;
    unsigned long m_dir;
};
} // anonymous namespace

unsigned long
DwarfDebug::AddDir(StringRef dirname)
{
    // Put the directory into the directory table (checking for duplicates)
    Dirs::iterator d =
        std::find(m_dirs.begin(), m_dirs.end(), dirname);
    if (d != m_dirs.end())
        return d-m_dirs.begin();

    m_dirs.push_back(dirname);
    return m_dirs.size()-1;
}

size_t
DwarfDebug::AddFile(const FileEntry* file)
{
    unsigned long dir = AddDir(file->getDir()->getName());

    // Put the filename into the filename table (checking for duplicates)
    Filenames::iterator f = std::find_if(m_filenames.begin(), m_filenames.end(),
                                         MatchFileDir(file->getName(), dir));

    size_t filenum;
    if (f != m_filenames.end())
    {
        filenum = f-m_filenames.begin();
        if (!f->filename.empty())
            return filenum;
    }
    else
    {
        filenum = m_filenames.size();
        m_filenames.push_back(Filename());
    }
    m_filenames[filenum].filename = file->getName();
    m_filenames[filenum].dir = dir;
    m_filenames[filenum].time = file->getModificationTime();
    m_filenames[filenum].length = file->getSize();
    return filenum;
}

size_t
DwarfDebug::AddFile(unsigned long filenum, StringRef pathname)
{
    StringRef dirname = llvm::sys::path::parent_path(pathname);
    if (dirname.empty())
        dirname = ".";
    unsigned long dir = AddDir(dirname);

    // Put the filename into the filename table
    assert(filenum != 0);
    filenum--;      // array index is 0-based

    // Ensure table is sufficient size
    if (filenum >= m_filenames.size())
        m_filenames.resize(filenum+1);

    // Save in table
    m_filenames[filenum].pathname = pathname;
    m_filenames[filenum].filename = llvm::sys::path::filename(pathname);
    m_filenames[filenum].dir = dir;
    m_filenames[filenum].time = 0;
    m_filenames[filenum].length = 0;

    return filenum;
}

// Create and add a new line opcode to a section.
void
DwarfDebug::AppendLineOp(BytecodeContainer& container,
                          unsigned int opcode)
{
    AppendByte(container, opcode);
}

void
DwarfDebug::AppendLineOp(BytecodeContainer& container,
                          unsigned int opcode,
                          const IntNum& operand)
{
    AppendByte(container, opcode);
    AppendLEB128(container, operand, opcode == DW_LNS_advance_line,
                 SourceLocation(), *m_diags);
}

// Create and add a new extended line opcode to a section.
void
DwarfDebug::AppendLineExtOp(BytecodeContainer& container,
                            DwarfLineNumberExtOp ext_opcode)
{
    AppendByte(container, DW_LNS_extended_op);
    AppendLEB128(container, 1, false, SourceLocation(), *m_diags);
    AppendByte(container, ext_opcode);
}

void
DwarfDebug::AppendLineExtOp(BytecodeContainer& container,
                            DwarfLineNumberExtOp ext_opcode,
                            const IntNum& operand)
{
    AppendByte(container, DW_LNS_extended_op);
    AppendLEB128(container, 1 + SizeLEB128(operand, false), false,
                 SourceLocation(), *m_diags);
    AppendByte(container, ext_opcode);
    AppendLEB128(container, operand, false, SourceLocation(), *m_diags);
}

void
DwarfDebug::AppendLineExtOp(BytecodeContainer& container,
                            DwarfLineNumberExtOp ext_opcode,
                            unsigned long ext_operandsize,
                            SymbolRef ext_operand)
{
    AppendByte(container, DW_LNS_extended_op);
    AppendLEB128(container, ext_operandsize+1, false, SourceLocation(),
                 *m_diags);
    AppendByte(container, ext_opcode);
    AppendData(container, Expr::Ptr(new Expr(ext_operand)), ext_operandsize,
               *m_object.getArch(), SourceLocation(), *m_diags);
}

void
DwarfDebug::GenerateLineOp(Section& debug_line,
                           DwarfLineState* state,
                           const DwarfLoc& loc,
                           const DwarfLoc* nextloc)
{
    if (state->file != loc.file)
    {
        state->file = loc.file;
        AppendLineOp(debug_line, DW_LNS_set_file, state->file);
    }
    if (state->column != loc.column)
    {
        state->column = loc.column;
        AppendLineOp(debug_line, DW_LNS_set_column, state->column);
    }
    if (loc.discriminator != 0)
    {
        AppendLineExtOp(debug_line, DW_LNE_set_discriminator,
                        loc.discriminator);
    }
#ifdef WITH_DWARF3
    if (loc.isa_change)
    {
        state->isa = loc.isa;
        AppendLineOp(debug_line, DW_LNS_set_isa, state->isa);
    }
#endif
    if (!state->is_stmt && loc.is_stmt == DwarfLoc::IS_STMT_SET)
    {
        state->is_stmt = true;
        AppendLineOp(debug_line, DW_LNS_negate_stmt);
    }
    else if (state->is_stmt && loc.is_stmt == DwarfLoc::IS_STMT_CLEAR)
    {
        state->is_stmt = false;
        AppendLineOp(debug_line, DW_LNS_negate_stmt);
    }
    if (loc.basic_block)
    {
        AppendLineOp(debug_line, DW_LNS_set_basic_block);
    }
#ifdef WITH_DWARF3
    if (loc.prologue_end)
    {
        AppendLineOp(debug_line, DW_LNS_set_prologue_end);
    }
    if (loc.epilogue_begin)
    {
        AppendLineOp(debug_line, DW_LNS_set_epilogue_begin);
    }
#endif

    IntNum addr_delta;
    if (!state->prevloc.bc)
    {
        addr_delta.Zero();
    }
    else
    {
        CalcDist(state->prevloc, loc.loc, &addr_delta);
        assert(addr_delta.getSign() >= 0 && "dwarf2 address went backwards");
    }

    // Generate appropriate opcode(s).  Address can only increment,
    // whereas line number can go backwards.
    IntNum line_delta = loc.line;
    line_delta -= state->line;
    state->line = loc.line;

    // First handle the line delta
    if (line_delta < DWARF_LINE_BASE
        || line_delta >= DWARF_LINE_BASE+DWARF_LINE_RANGE)
    {
        // Won't fit in special opcode, use (signed) line advance
        AppendLineOp(debug_line, DW_LNS_advance_line, line_delta);
        line_delta.Zero();
    }

    // Next handle the address delta
    int opcode1, opcode2;
    opcode1 = opcode2 =
        DWARF_LINE_OPCODE_BASE + line_delta.getInt() - DWARF_LINE_BASE;
    opcode1 += DWARF_LINE_RANGE * (addr_delta.getUInt() / m_min_insn_len);
    opcode2 += DWARF_LINE_RANGE *
        ((addr_delta.getUInt() - DWARF_MAX_SPECIAL_ADDR_DELTA) / m_min_insn_len);
    if (line_delta.isZero() && addr_delta.isZero())
    {
        // Both line and addr deltas are 0: do DW_LNS_copy
        AppendLineOp(debug_line, DW_LNS_copy);
    }
    else if (addr_delta.getUInt() <= DWARF_MAX_SPECIAL_ADDR_DELTA &&
             opcode1 <= 255)
    {
        // Addr delta in range of special opcode
        AppendLineOp(debug_line, opcode1);
    }
    else if (addr_delta.getUInt() <= 2*DWARF_MAX_SPECIAL_ADDR_DELTA &&
             opcode2 <= 255)
    {
        // Addr delta in range of const_add_pc + special
        AppendLineOp(debug_line, DW_LNS_const_add_pc);
        AppendLineOp(debug_line, opcode2);
    }
    else
    {
        // Need advance_pc
        AppendLineOp(debug_line, DW_LNS_advance_pc, addr_delta);
        // Take care of any remaining line_delta and add entry to matrix
        if (line_delta.isZero())
            AppendLineOp(debug_line, DW_LNS_copy);
        else
        {
            AppendLineOp(debug_line, DWARF_LINE_OPCODE_BASE +
                                     line_delta.getInt() - DWARF_LINE_BASE);
        }
    }
    state->prevloc = loc.loc;
}
#if 0
void
DwarfDebug::GenerateLineBC(Section& debug_line,
                           SourceManager& smgr,
                           DwarfLineState* state,
                           Bytecode& bc,
                           DwarfLoc* loc,
                           unsigned long* lastfile)
{
    unsigned long i;
    const char *filename;
    /*@null@*/ yasm_bytecode *nextbc = yasm_bc__next(bc);

    if (nextbc && bc->offset == nextbc->offset)
        return 0;

    info->loc.vline = bc->line;
    info->loc.bc = bc;

    yasm_linemap_lookup(info->linemap, bc->line, &filename, &info->loc.line);
    // Find file index; just linear search it unless it was the last used
    if (*lastfile > 0 && filename == m_filenames[*lastfile-1].pathname)
        loc->file = *lastfile;
    else
    {
        for (i=0; i<dbgfmt_dwarf2->filenames_size; i++)
        {
            if (strcmp(filename, dbgfmt_dwarf2->filenames[i].pathname) == 0)
                break;
        }
        if (i >= dbgfmt_dwarf2->filenames_size)
            yasm_internal_error(N_("could not find filename in table"));
        info->loc.file = i+1;
        info->lastfile = i+1;
    }
    if (GenerateLineOp(debug_line, state, loc, NULL))
        return 1;
    return 0;
}
#endif
void
DwarfDebug::GenerateLineSection(Section& sect,
                                Section& debug_line,
                                SourceManager& smgr,
                                bool asm_source,
                                Section** last_code,
                                size_t* num_line_sections)
{
    DwarfSection* dwarf2sect = sect.getAssocData<DwarfSection>();
    if (!dwarf2sect)
    {
        if (!asm_source || !sect.isCode())
            return;     // no line data for this section
        // Create line data for asm code sections
        dwarf2sect = new DwarfSection;
        sect.AddAssocData(std::auto_ptr<DwarfSection>(dwarf2sect));
    }

    ++(*num_line_sections);
    *last_code = &sect;

    // initialize state machine registers for each sequence
    DwarfLineState state;
    state.address = 0;
    state.file = 1;
    state.line = 1;
    state.column = 0;
    state.isa = 0;
    state.is_stmt = DWARF_LINE_DEFAULT_IS_STMT;
    state.prevloc.bc = NULL;

    // Set the starting address for the section
    AppendLineExtOp(debug_line, DW_LNE_set_address, m_sizeof_address,
                    sect.getSymbol());

    if (asm_source)
    {
#if 0
        unsigned long lastfile = 0;
        DwarfLoc loc;

        for (Section::bc_iterator i=sect.bytecodes_begin(),
             end=sect.bytecodes_end(); i != end; ++i)
        {
            GenerateLineBC(debug_line, smgr, &state, *i, &loc, &lastfile);
        }
#endif
    }
    else
    {
        for (DwarfSection::Locs::const_iterator i=dwarf2sect->locs.begin(),
             end=dwarf2sect->locs.end(); i != end; ++i)
        {
            DwarfSection::Locs::const_iterator next = i+1;
            GenerateLineOp(debug_line, &state, *i, next != end ? &*next : 0);
        }
    }

    // End sequence: bring address to end of section, then output end
    // sequence opcode.  Don't use a special opcode to do this as we don't
    // want an extra entry in the line matrix.
    if (!state.prevloc.bc)
        state.prevloc = sect.getBeginLoc();
    IntNum addr_delta;
    CalcDist(state.prevloc, sect.getEndLoc(), &addr_delta);
    if (addr_delta == DWARF_MAX_SPECIAL_ADDR_DELTA)
        AppendLineOp(debug_line, DW_LNS_const_add_pc);
    else if (addr_delta > 0)
        AppendLineOp(debug_line, DW_LNS_advance_pc, addr_delta);
    AppendLineExtOp(debug_line, DW_LNE_end_sequence);
}

Section&
DwarfDebug::Generate_line(SourceManager& smgr,
                          bool asm_source,
                          /*@out@*/ Section** main_code,
                          /*@out@*/ size_t* num_line_sections)
{
    if (asm_source)
    {
        // Generate dirs and filenames based on smgr
        for (SourceManager::fileinfo_iterator i=smgr.fileinfo_begin(),
             end=smgr.fileinfo_end(); i != end; ++i)
        {
            AddFile(i->first);
        }
    }

    Section* debug_line = m_object.FindSection(".debug_line");
    if (!debug_line)
    {
        debug_line =
            m_objfmt->AppendSection(".debug_line", SourceLocation(), *m_diags);
        debug_line->setAlign(0);
    }

    // header
    Location head = AppendHead(*debug_line, NULL, false, false);

    // statement program prologue
    AppendSPP(*debug_line);

    // statement program
    *num_line_sections = 0;
    Section* last_code = NULL;
    for (Object::section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        GenerateLineSection(*i, *debug_line, smgr, asm_source, &last_code,
                            num_line_sections);
    }

    // mark end of line information
    debug_line->UpdateOffsets(*m_diags);
    setHeadEnd(head, debug_line->getEndLoc());

    if (*num_line_sections == 1)
        *main_code = last_code;
    else
        *main_code = NULL;
    return *debug_line;
}

void
DwarfDebug::AppendSPP(BytecodeContainer& container)
{
    Bytes bytes;

    Write8(bytes, m_min_insn_len);                  // minimum_instr_len
    Write8(bytes, DWARF_LINE_DEFAULT_IS_STMT);      // default_is_stmt
    Write8(bytes, DWARF_LINE_BASE);                 // line_base
    Write8(bytes, DWARF_LINE_RANGE);                // line_range
    Write8(bytes, DWARF_LINE_OPCODE_BASE);          // opcode_base

    // Standard opcode # operands array
    for (unsigned int i=0; i<NELEMS(line_opcode_num_operands); ++i)
        Write8(bytes, line_opcode_num_operands[i]);

    // directory list
    for (Dirs::const_iterator i=m_dirs.begin(), end=m_dirs.end(); i != end; ++i)
    {
        bytes.WriteString(*i);
        Write8(bytes, 0);
    }
    // finish with single 0 byte
    Write8(bytes, 0);

    // filename list
    for (Filenames::const_iterator i=m_filenames.begin(), end=m_filenames.end();
         i != end; ++i)
    {
        if (i->filename.empty())
        {
            m_diags->Report(SourceLocation(), diag::err_file_number_unassigned)
                << static_cast<unsigned int>((i-m_filenames.begin())+1);
            continue;
        }
        bytes.WriteString(i->filename);
        Write8(bytes, 0);

        WriteULEB128(bytes, i->dir+1);  // dir
        WriteULEB128(bytes, i->time);   // time
        WriteULEB128(bytes, i->length); // length
    }
    // finish with single 0 byte
    Write8(bytes, 0);

    // Prologue length (following this field)
    AppendData(container, bytes.size(), m_sizeof_offset, *m_object.getArch());

    // Prologue data
    Bytes& fixed = container.FreshBytecode().getFixed();
    fixed.insert(fixed.end(), bytes.begin(), bytes.end());
}

void
DwarfDebug::DirLoc(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    NameValues::const_iterator nv = info.getNameValues().begin();
    NameValues::const_iterator end = info.getNameValues().end();

    // File number (required)
    if (!nv->isExpr())
    {
        diags.Report(nv->getValueRange().getBegin(),
                     diag::err_loc_file_number_missing);
        return;
    }
    Expr file_e = nv->getExpr(m_object);
    if (!file_e.isIntNum())
    {
        diags.Report(nv->getValueRange().getBegin(),
                     diag::err_loc_file_number_not_integer);
        return;
    }
    IntNum file = file_e.getIntNum();
    if (file.getSign() != 1)
    {
        diags.Report(nv->getValueRange().getBegin(),
                     diag::err_loc_file_number_invalid);
        return;
    }

    // Line number (required)
    ++nv;
    if (nv == end || !nv->isExpr())
    {
        diags.Report(nv->getValueRange().getBegin(),
                     diag::err_loc_line_number_missing);
        return;
    }
    Expr line_e = nv->getExpr(m_object);
    if (!line_e.isIntNum())
    {
        diags.Report(nv->getValueRange().getBegin(),
                     diag::err_loc_line_number_not_integer);
        return;
    }
    IntNum line = line_e.getIntNum();

    // Generate new section data if it doesn't already exist
    Section* section = m_object.getCurSection();
    if (!section)
    {
        diags.Report(info.getSource(), diag::err_loc_must_be_in_section);
        return;
    }

    DwarfSection* dwarf2sect = section->getAssocData<DwarfSection>();
    if (!dwarf2sect)
    {
        dwarf2sect = new DwarfSection;
        section->AddAssocData(std::auto_ptr<DwarfSection>(dwarf2sect));
    }

    // Defaults for optional settings
    Bytecode& herebc = info.getObject().getCurSection()->FreshBytecode();
    Location here = { &herebc, herebc.getFixedLen() };
    std::auto_ptr<DwarfLoc> loc(new DwarfLoc(here, info.getSource(),
                                             file.getUInt(), line.getUInt()));

    // Optional column number
    ++nv;
    if (nv != end && nv->isExpr())
    {
        Expr col_e = nv->getExpr(m_object);
        if (!col_e.isIntNum())
        {
            diags.Report(nv->getValueRange().getBegin(),
                         diag::err_loc_column_number_not_integer);
            return;
        }
        loc->column = col_e.getIntNum().getUInt();
        ++nv;
    }

    // Other options; note for GAS compatibility we need to support both:
    // is_stmt=1 (NASM) and
    // is_stmt 1 (GAS)
    bool in_is_stmt = false, in_isa = false, in_discriminator = false;
    while (nv != end)
    {
        StringRef name = nv->getName();

restart:
        if (in_is_stmt)
        {
            in_is_stmt = false;
            if (!nv->isExpr())
            {
                diags.Report(nv->getValueRange().getBegin(),
                             diag::err_loc_is_stmt_not_zero_or_one);
                return;
            }
            Expr is_stmt_e = nv->getExpr(m_object);
            if (!is_stmt_e.isIntNum())
            {
                diags.Report(nv->getValueRange().getBegin(),
                             diag::err_loc_is_stmt_not_zero_or_one);
                return;
            }
            IntNum is_stmt = is_stmt_e.getIntNum();
            if (is_stmt.isZero())
                loc->is_stmt = DwarfLoc::IS_STMT_SET;
            else if (is_stmt.isPos1())
                loc->is_stmt = DwarfLoc::IS_STMT_CLEAR;
            else
            {
                diags.Report(nv->getValueRange().getBegin(),
                             diag::err_loc_is_stmt_not_zero_or_one);
                return;
            }
        }
        else if (in_isa)
        {
            in_isa = false;
            if (!nv->isExpr())
            {
                diags.Report(nv->getValueRange().getBegin(),
                             diag::err_loc_isa_not_integer);
                return;
            }
            Expr isa_e = nv->getExpr(m_object);
            if (!isa_e.isIntNum())
            {
                diags.Report(nv->getValueRange().getBegin(),
                             diag::err_loc_isa_not_integer);
                return;
            }
            IntNum isa = isa_e.getIntNum();
            if (isa.getSign() < 0)
            {
                diags.Report(nv->getValueRange().getBegin(),
                             diag::err_loc_isa_less_than_zero);
                return;
            }
            loc->isa_change = true;
            loc->isa = isa.getUInt();
        }
        else if (in_discriminator)
        {
            in_discriminator = false;
            if (!nv->isExpr())
            {
                diags.Report(nv->getValueRange().getBegin(),
                             diag::err_loc_discriminator_not_integer);
                return;
            }
            Expr discriminator_e = nv->getExpr(m_object);
            if (!discriminator_e.isIntNum())
            {
                diags.Report(nv->getValueRange().getBegin(),
                             diag::err_loc_discriminator_not_integer);
                return;
            }
            IntNum discriminator = discriminator_e.getIntNum();
            if (discriminator.getSign() < 0)
            {
                diags.Report(nv->getValueRange().getBegin(),
                             diag::err_loc_discriminator_less_than_zero);
                return;
            }
            loc->discriminator = discriminator;
        }
        else if (name.empty() && nv->isId())
        {
            StringRef s = nv->getId();
            if (s.equals_lower("is_stmt"))
                in_is_stmt = true;
            else if (s.equals_lower("isa"))
                in_isa = true;
            else if (s.equals_lower("discriminator"))
                in_discriminator = true;
            else if (s.equals_lower("basic_block"))
                loc->basic_block = true;
            else if (s.equals_lower("prologue_end"))
                loc->prologue_end = true;
            else if (s.equals_lower("epilogue_begin"))
                loc->epilogue_begin = true;
            else
                diags.Report(nv->getValueRange().getBegin(),
                             diag::warn_unrecognized_loc_option) << s;
        }
        else if (name.empty())
        {
            diags.Report(nv->getValueRange().getBegin(),
                         diag::warn_unrecognized_numeric_qualifier);
        }
        else if (name.equals_lower("is_stmt"))
        {
            in_is_stmt = true;
            goto restart;   // don't go to the next nameval
        }
        else if (name.equals_lower("isa"))
        {
            in_isa = true;
            goto restart;   // don't go to the next nameval
        }
        else if (name.equals_lower("discriminator"))
        {
            in_discriminator = true;
            goto restart;   // don't go to the next nameval
        }
        else
            diags.Report(nv->getNameSource(),
                         diag::warn_unrecognized_loc_option) << name;
        ++nv;
    }

    if (in_is_stmt || in_isa || in_discriminator)
    {
        diags.Report(info.getSource(), diag::err_loc_option_requires_value)
            << (in_is_stmt ? "is_stmt" : (in_isa ? "isa" : "discriminator"));
        return;
    }

    // Append new location
    dwarf2sect->locs.push_back(loc.release());
}

void
DwarfDebug::DirFile(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    NameValues& nvs = info.getNameValues();
    assert(!nvs.empty());

    NameValue& nv = nvs.front();
    if (nv.isString())
    {
        // Just a bare filename
        m_object.setSourceFilename(nv.getString());
        return;
    }

    // Otherwise.. first nv is the file number
    if (!nv.isExpr() || !nv.getExpr(m_object).isIntNum())
    {
        diags.Report(nv.getValueRange().getBegin(),
                     diag::err_loc_file_number_not_integer);
        return;
    }
    IntNum filenum = nv.getExpr(m_object).getIntNum();

    if (nvs.size() < 2 || !nvs[1].isString())
    {
        diags.Report(nv.getValueRange().getBegin(),
                     diag::err_loc_missing_filename);
        return;
    }

    AddFile(filenum.getUInt(), nvs[1].getString());
}

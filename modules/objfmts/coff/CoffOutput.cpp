//
// COFF (DJGPP) object format writer
//
//  Copyright (C) 2002-2008  Peter Johnson
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
#include "CoffObject.h"

#include <cstdlib>
#include <ctime>
#include <vector>

#include "llvm/Support/raw_ostream.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Arch.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/DebugFormat.h"
#include "yasmx/IntNum.h"
#include "yasmx/Location_util.h"
#include "yasmx/Object.h"
#include "yasmx/Reloc.h"
#include "yasmx/Section.h"
#include "yasmx/StringTable.h"
#include "yasmx/Symbol.h"
#include "yasmx/Symbol_util.h"

#include "CoffReloc.h"
#include "CoffSection.h"
#include "CoffSymbol.h"


using namespace yasm;
using namespace yasm::objfmt;

namespace {
class CoffOutput : public BytecodeStreamOutput
{
public:
    CoffOutput(llvm::raw_ostream& os,
               CoffObject& objfmt,
               Object& object,
               bool all_syms,
               Diagnostic& diags);
    ~CoffOutput();

    bool OutputSection(Section& sect);
    void OutputSectionHeader(const Section& sect);
    unsigned long CountSymbols();
    void OutputSymbolTable();
    void OutputStringTable();

    // OutputBytecode overrides
    bool ConvertValueToBytes(Value& value,
                             Location loc,
                             NumericOutput& num_out);

private:
    CoffObject& m_objfmt;
    CoffSection* m_coffsect;
    Object& m_object;
    bool m_all_syms;
    StringTable m_strtab;
    BytecodeNoOutput m_no_output;
};
} // anonymous namespace

CoffOutput::CoffOutput(llvm::raw_ostream& os,
                       CoffObject& objfmt,
                       Object& object,
                       bool all_syms,
                       Diagnostic& diags)
    : BytecodeStreamOutput(os, diags)
    , m_objfmt(objfmt)
    , m_object(object)
    , m_all_syms(all_syms)
    , m_strtab(4)   // first 4 bytes in string table are length
    , m_no_output(diags)
{
}

CoffOutput::~CoffOutput()
{
}

bool
CoffOutput::ConvertValueToBytes(Value& value,
                                Location loc,
                                NumericOutput& num_out)
{
    m_object.getArch()->setEndian(num_out.getBytes());

    IntNum base(0);
    if (value.OutputBasic(num_out, &base, getDiagnostics()))
        return true;

    IntNum intn(0);
    IntNum dist(0);
    if (value.isRelative())
    {
        // We can't handle these types of values
        if (value.getRShift() > 0 || value.getShift() > 0
            || (value.isSegOf() && (value.isWRT() || value.hasSubRelative()))
            || (value.isSectionRelative()
                && (value.isWRT() || value.hasSubRelative())))
        {
            Diag(value.getSource().getBegin(), diag::err_reloc_too_complex);
            return false;
        }

        SymbolRef sym = value.getRelative();
        SymbolRef wrt = value.getWRT();

        // Sometimes we want the relocation to be generated against one
        // symbol but the value generated correspond to a different symbol.
        // This is done through (sym being referenced) WRT (sym used for
        // reloc).  Note both syms need to be in the same section!
        if (wrt)
        {
            Location wrt_loc, rel_loc;
            if (!sym->getLabel(&rel_loc) || !wrt->getLabel(&wrt_loc))
            {
                Diag(value.getSource().getBegin(), diag::err_wrt_too_complex);
                return false;
            }
            if (!CalcDist(wrt_loc, rel_loc, &dist))
            {
                Diag(value.getSource().getBegin(),
                     diag::err_wrt_across_sections);
                return false;
            }
            sym = wrt;
        }

        int vis = sym->getVisibility();
        if (vis & Symbol::COMMON)
        {
            // In standard COFF, COMMON symbols have their length added in
            if (!m_objfmt.isWin32())
            {
                assert(getCommonSize(*sym) != 0);
                Expr csize_expr(*getCommonSize(*sym));
                SimplifyCalcDist(csize_expr, getDiagnostics());
                if (!csize_expr.isIntNum())
                {
                    Diag(value.getSource().getBegin(),
                         diag::err_common_size_too_complex);
                    return false;
                }

                IntNum common_size = csize_expr.getIntNum();
                if (common_size.getSign() < 0)
                {
                    Diag(value.getSource().getBegin(),
                         diag::err_common_size_negative);
                    return false;
                }

                intn += common_size;
            }
        }
        else if (!(vis & Symbol::EXTERN) && !m_objfmt.isWin64())
        {
            // Local symbols need relocation to their section's start
            Location symloc;
            if (sym->getLabel(&symloc))
            {
                Section* sym_sect = symloc.bc->getContainer()->getSection();
                CoffSection* coffsect = sym_sect->getAssocData<CoffSection>();
                assert(coffsect != 0);
                sym = coffsect->m_sym;

                intn = symloc.getOffset();
                intn += sym_sect->getVMA();
            }
        }

        bool pc_rel = false;
        IntNum intn2;
        if (value.CalcPCRelSub(&intn2, loc))
        {
            // Create PC-relative relocation type and fix up absolute portion.
            pc_rel = true;
            intn += intn2;
        }
        else if (value.hasSubRelative())
        {
            Diag(value.getSource().getBegin(), diag::err_reloc_too_complex);
            return false;
        }

        if (pc_rel)
        {
            // For standard COFF, need to adjust to start of section, e.g.
            // subtract out the value location.
            // For Win32 COFF, adjust by value size.
            // For Win64 COFF, adjust to next instruction;
            // the delta is taken care of by special relocation types.
            if (m_objfmt.isWin64())
                intn += value.getNextInsn();
            else if (m_objfmt.isWin32())
                intn += value.getSize()/8;
            else
                intn -= loc.getOffset();
        }

        // Zero value for segment generation.
        if (value.isSegOf())
            intn = 0;

        // Generate reloc
        CoffObject::Machine machine = m_objfmt.getMachine();
        CoffReloc::Type rtype = CoffReloc::ABSOLUTE;
        IntNum addr = loc.getOffset();
        addr += loc.bc->getContainer()->getSection()->getVMA();

        if (machine == CoffObject::MACHINE_I386)
        {
            if (pc_rel)
            {
                if (value.getSize() == 32)
                    rtype = CoffReloc::I386_REL32;
                else
                {
                    Diag(value.getSource().getBegin(),
                         diag::err_reloc_invalid_size);
                    return false;
                }
            }
            else if (value.isSegOf())
                rtype = CoffReloc::I386_SECTION;
            else if (value.isSectionRelative())
                rtype = CoffReloc::I386_SECREL;
            else
            {
                if (m_coffsect->m_nobase)
                    rtype = CoffReloc::I386_ADDR32NB;
                else
                    rtype = CoffReloc::I386_ADDR32;
            }
        }
        else if (machine == CoffObject::MACHINE_AMD64)
        {
            if (pc_rel)
            {
                if (value.getSize() != 32)
                {
                    Diag(value.getSource().getBegin(),
                         diag::err_reloc_invalid_size);
                    return false;
                }
                switch (value.getNextInsn())
                {
                    case 0: rtype = CoffReloc::AMD64_REL32; break;
                    case 1: rtype = CoffReloc::AMD64_REL32_1; break;
                    case 2: rtype = CoffReloc::AMD64_REL32_2; break;
                    case 3: rtype = CoffReloc::AMD64_REL32_3; break;
                    case 4: rtype = CoffReloc::AMD64_REL32_4; break;
                    case 5: rtype = CoffReloc::AMD64_REL32_5; break;
                    default:
                        Diag(value.getSource().getBegin(),
                             diag::err_reloc_invalid_size);
                        return false;
                }
            }
            else if (value.isSegOf())
                rtype = CoffReloc::AMD64_SECTION;
            else if (value.isSectionRelative())
                rtype = CoffReloc::AMD64_SECREL;
            else
            {
                unsigned int size = value.getSize();
                if (size == 32)
                {
                    if (m_coffsect->m_nobase)
                        rtype = CoffReloc::AMD64_ADDR32NB;
                    else
                        rtype = CoffReloc::AMD64_ADDR32;
                }
                else if (size == 64)
                    rtype = CoffReloc::AMD64_ADDR64;
                else
                {
                    Diag(value.getSource().getBegin(),
                         diag::err_reloc_invalid_size);
                    return false;
                }
            }
        }
        else
            assert(false);  // unrecognized machine

        Section* sect = loc.bc->getContainer()->getSection();
        Reloc* reloc = 0;
        if (machine == CoffObject::MACHINE_I386)
            reloc = new Coff32Reloc(addr, sym, rtype);
        else if (machine == CoffObject::MACHINE_AMD64)
            reloc = new Coff64Reloc(addr, sym, rtype);
        else
            assert(false && "nonexistent machine");
        sect->AddReloc(std::auto_ptr<Reloc>(reloc));
    }

    intn += base;
    intn += dist;

    num_out.OutputInteger(intn);
    return true;
}

bool
CoffOutput::OutputSection(Section& sect)
{
    BytecodeOutput* outputter = this;

    CoffSection* coffsect = sect.getAssocData<CoffSection>();
    assert(coffsect != 0);
    m_coffsect = coffsect;

    // Add to strtab if in win32 format and name > 8 chars
    if (m_objfmt.isWin32())
    {
        size_t namelen = sect.getName().size();
        if (namelen > 8)
            coffsect->m_strtab_name = m_strtab.getIndex(sect.getName());
    }

    long pos;
    if (sect.isBSS())
    {
        // Don't output BSS sections.
        outputter = &m_no_output;
        pos = 0;    // position = 0 because it's not in the file
    }
    else
    {
        if (sect.bytecodes_back().getNextOffset() == 0)
            return true;

        pos = m_os.tell();
        if (pos < 0)
        {
            Diag(SourceLocation(), diag::err_file_output_position);
            return false;
        }
    }
    sect.setFilePos(static_cast<unsigned long>(pos));
    coffsect->m_size = 0;

    // Output bytecodes
    for (Section::bc_iterator i=sect.bytecodes_begin(),
         end=sect.bytecodes_end(); i != end; ++i)
    {
        if (i->Output(*outputter))
            coffsect->m_size += i->getTotalLen();
    }

    if (getDiagnostics().hasErrorOccurred())
        return false;

    // Sanity check final section size
    assert(coffsect->m_size == sect.bytecodes_back().getNextOffset());

    // No relocations to output?  Go on to next section
    if (sect.getRelocs().size() == 0)
        return true;

    pos = m_os.tell();
    if (pos < 0)
    {
        Diag(SourceLocation(), diag::err_file_output_position);
        return false;
    }
    coffsect->m_relptr = static_cast<unsigned long>(pos);

    // If >=64K relocs (for Win32/64), we set a flag in the section header
    // (NRELOC_OVFL) and the first relocation contains the number of relocs.
    if (sect.getRelocs().size() >= 64*1024)
    {
#if 0
        if (m_objfmt.isWin32())
        {
            coffsect->m_flags |= CoffSection::NRELOC_OVFL;
            Bytes& bytes = getScratch();
            bytes << little_endian;
            Write32(bytes, sect.getRelocs().size()+1);  // address (# relocs)
            Write32(bytes, 0);                          // relocated symbol
            Write16(bytes, 0);                          // type of relocation
            m_os << bytes;
        }
        else
#endif
        {
            Diag(SourceLocation(), diag::err_too_many_relocs)
                << sect.getName();
        }
    }

    for (Section::const_reloc_iterator i=sect.relocs_begin(),
         end=sect.relocs_end(); i != end; ++i)
    {
        const CoffReloc& reloc = static_cast<const CoffReloc&>(*i);
        Bytes& scratch = getScratch();
        reloc.Write(scratch);
        assert(scratch.size() == 10);
        m_os << scratch;
    }
    return true;
}

unsigned long
CoffOutput::CountSymbols()
{
    unsigned long indx = 0;

    for (Object::symbol_iterator i = m_object.symbols_begin(),
         end = m_object.symbols_end(); i != end; ++i)
    {
        int vis = i->getVisibility();
        CoffSymbol* coffsym = i->getAssocData<CoffSymbol>();

        // Don't output local syms unless outputting all syms
        if (!m_all_syms && vis == Symbol::LOCAL && !i->isAbsoluteSymbol()
            && !(coffsym && coffsym->m_forcevis))
            continue;

        // Create basic coff symbol data if it doesn't already exist
        if (!coffsym)
        {
            coffsym = new CoffSymbol(CoffSymbol::SCL_NULL);
            i->AddAssocData(std::auto_ptr<CoffSymbol>(coffsym));
        }
        // Update storage class based on visibility if not otherwise set.
        if (coffsym->m_sclass == CoffSymbol::SCL_NULL)
        {
            if (vis & (Symbol::EXTERN|Symbol::GLOBAL|Symbol::COMMON))
                coffsym->m_sclass = CoffSymbol::SCL_EXT;
            else
                coffsym->m_sclass = CoffSymbol::SCL_STAT;
        }
        coffsym->m_index = indx;

        indx += coffsym->m_aux.size() + 1;
    }

    return indx;
}

void
CoffOutput::OutputSymbolTable()
{
    for (Object::const_symbol_iterator i = m_object.symbols_begin(),
         end = m_object.symbols_end(); i != end; ++i)
    {
        const CoffSymbol* coffsym = i->getAssocData<CoffSymbol>();

        // Don't output local syms unless outputting all syms
        if (!m_all_syms && i->getVisibility() == Symbol::LOCAL
            && !i->isAbsoluteSymbol() && !(coffsym && coffsym->m_forcevis))
            continue;

        Bytes& bytes = getScratch();
        assert(coffsym != 0);
        coffsym->Write(bytes, *i, getDiagnostics(), m_strtab);
        m_os << bytes;
    }
}

void
CoffOutput::OutputStringTable()
{
    Bytes& bytes = getScratch();
    bytes.setLittleEndian();
    Write32(bytes, m_strtab.getSize()+4);   // total length
    m_os << bytes;
    m_strtab.Write(m_os);                   // strings
}

void
CoffOutput::OutputSectionHeader(const Section& sect)
{
    Bytes& bytes = getScratch();
    const CoffSection* coffsect = sect.getAssocData<CoffSection>();
    coffsect->Write(bytes, sect);
    m_os << bytes;
}

void
CoffObject::Output(llvm::raw_fd_ostream& os,
                   bool all_syms,
                   DebugFormat& dbgfmt,
                   Diagnostic& diags)
{
    // Update file symbol filename
    assert(m_file_coffsym != 0);
    m_file_coffsym->m_aux.resize(1);
    m_file_coffsym->m_aux[0].fname = m_object.getSourceFilename();

    // Number sections and determine each section's addr values.
    // The latter is needed in VMA case before actually outputting
    // relocations, as a relocation's section address is added into the
    // addends in the generated code.
    unsigned int scnum = 1;
    unsigned long addr = 0;
    for (Object::section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        CoffSection* coffsect = i->getAssocData<CoffSection>();
        coffsect->m_scnum = scnum++;

        if (coffsect->m_isdebug)
        {
            i->setLMA(0);
            i->setVMA(0);
        }
        else
        {
            i->setLMA(addr);
            if (m_set_vma)
                i->setVMA(addr);
            else
                i->setVMA(0);
            addr += i->bytecodes_back().getNextOffset();
        }
    }

    // Allocate space for headers by seeking forward.
    os.seek(20+40*(scnum-1));
    if (os.has_error())
    {
        diags.Report(SourceLocation(), diag::err_file_output_seek);
        return;
    }

    CoffOutput out(os, *this, m_object, all_syms, diags);

    // Finalize symbol table (assign index to each symbol).
    unsigned long symtab_count = out.CountSymbols();

    // Section data/relocs
    for (Object::section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        if (!out.OutputSection(*i))
            return;
    }

    // Symbol table
    long pos = os.tell();
    if (pos < 0)
    {
        diags.Report(SourceLocation(), diag::err_file_output_position);
        return;
    }
    unsigned long symtab_pos = static_cast<unsigned long>(pos);
    out.OutputSymbolTable();

    // String table
    out.OutputStringTable();

    // Write headers
    os.seek(0);
    if (os.has_error())
    {
        diags.Report(SourceLocation(), diag::err_file_output_seek);
        return;
    }

    // Write file header
    Bytes& bytes = out.getScratch();
    bytes.setLittleEndian();
    Write16(bytes, m_machine);          // magic number
    Write16(bytes, scnum-1);            // number of sects
    unsigned long ts;
    if (std::getenv("YASM_TEST_SUITE"))
        ts = 0;
    else
        ts = static_cast<unsigned long>(std::time(NULL));
    Write32(bytes, ts);                 // time/date stamp
    Write32(bytes, symtab_pos);         // file ptr to symtab
    Write32(bytes, symtab_count);       // number of symtabs
    Write16(bytes, 0);                  // size of optional header (none)

    // flags
    unsigned int flags = 0;
    if (dbgfmt.getModule().getKeyword().equals_lower("null"))
        flags |= F_LNNO;
    if (!all_syms)
        flags |= F_LSYMS;
    if (m_machine != MACHINE_AMD64)
        flags |= F_AR32WR;
    Write16(bytes, flags);
    os << bytes;

    // Section headers
    for (Object::section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        out.OutputSectionHeader(*i);
    }
}

//
// COFF (DJGPP) object format
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

#include <util.h>

#include <cstdlib>
#include <ctime>
#include <vector>

#include <yasmx/Support/bitcount.h>
#include <yasmx/Support/Compose.h>
#include <yasmx/Support/errwarn.h>
#include <yasmx/Support/nocase.h>
#include <yasmx/Support/registry.h>
#include <yasmx/Arch.h>
#include <yasmx/BytecodeOutput.h>
#include <yasmx/Bytecode.h>
#include <yasmx/Bytes.h>
#include <yasmx/Bytes_util.h>
#include <yasmx/Directive.h>
#include <yasmx/DirHelpers.h>
#include <yasmx/Errwarns.h>
#include <yasmx/IntNum.h>
#include <yasmx/Location_util.h>
#include <yasmx/NameValue.h>
#include <yasmx/Object.h>
#include <yasmx/Object_util.h>
#include <yasmx/Reloc.h>
#include <yasmx/Section.h>
#include <yasmx/StringTable.h>
#include <yasmx/Symbol.h>
#include <yasmx/Symbol_util.h>

#include "CoffReloc.h"
#include "CoffSection.h"
#include "CoffSymbol.h"


namespace yasm
{
namespace objfmt
{
namespace coff
{

CoffObject::CoffObject(const ObjectFormatModule& module,
                       Object& object,
                       bool set_vma,
                       bool win32,
                       bool win64)
    : ObjectFormat(module, object)
    , m_set_vma(set_vma)
    , m_win32(win32)
    , m_win64(win64)
    , m_machine(MACHINE_UNKNOWN)
    , m_file_coffsym(0)
{
    // Support x86 and amd64 machines of x86 arch
    if (String::NocaseEqual(m_object.getArch()->getMachine(), "x86"))
        m_machine = MACHINE_I386;
    else if (String::NocaseEqual(m_object.getArch()->getMachine(), "amd64"))
        m_machine = MACHINE_AMD64;
}

std::vector<const char*>
CoffObject::getDebugFormatKeywords()
{
    static const char* keywords[] = {"null", "dwarf2"};
    return std::vector<const char*>(keywords, keywords+NELEMS(keywords));
}

bool
CoffObject::isOkObject(Object& object)
{
    // Support x86 and amd64 machines of x86 arch
    Arch* arch = object.getArch();
    if (!String::NocaseEqual(arch->getModule().getKeyword(), "x86"))
        return false;

    if (String::NocaseEqual(arch->getMachine(), "x86"))
        return true;
    if (String::NocaseEqual(arch->getMachine(), "amd64"))
        return true;
    return false;
}

void
CoffObject::InitSymbols(const char* parser)
{
    // Add .file symbol
    SymbolRef filesym = m_object.AppendSymbol(".file");
    filesym->DefineSpecial(Symbol::GLOBAL);

    std::auto_ptr<CoffSymbol> coffsym(
        new CoffSymbol(CoffSymbol::SCL_FILE, CoffSymbol::AUX_FILE));
    m_file_coffsym = coffsym.get();

    filesym->AddAssocData(coffsym);
}

class CoffOutput : public BytecodeStreamOutput
{
public:
    CoffOutput(std::ostream& os,
               CoffObject& objfmt,
               Object& object,
               bool all_syms);
    ~CoffOutput();

    void OutputSection(Section& sect, Errwarns& errwarns);
    void OutputSectionHeader(const Section& sect);
    unsigned long CountSymbols();
    void OutputSymbolTable(Errwarns& errwarns);
    void OutputStringTable();

    // OutputBytecode overrides
    void ConvertValueToBytes(Value& value,
                             Bytes& bytes,
                             Location loc,
                             int warn);

private:
    CoffObject& m_objfmt;
    CoffSection* m_coffsect;
    Object& m_object;
    bool m_all_syms;
    StringTable m_strtab;
    BytecodeNoOutput m_no_output;
};

CoffOutput::CoffOutput(std::ostream& os,
                       CoffObject& objfmt,
                       Object& object,
                       bool all_syms)
    : BytecodeStreamOutput(os)
    , m_objfmt(objfmt)
    , m_object(object)
    , m_all_syms(all_syms)
    , m_strtab(4)   // first 4 bytes in string table are length
{
}

CoffOutput::~CoffOutput()
{
}

void
CoffOutput::ConvertValueToBytes(Value& value,
                                Bytes& bytes,
                                Location loc,
                                int warn)
{
    if (Expr* abs = value.getAbs())
        SimplifyCalcDist(*abs);

    // Try to output constant and PC-relative section-local first.
    // Note this does NOT output any value with a SEG, WRT, external,
    // cross-section, or non-PC-relative reference (those are handled below).
    if (value.OutputBasic(bytes, warn, *m_object.getArch()))
        return;

    // Handle other expressions, with relocation if necessary
    if (value.getRShift() > 0
        || (value.isSegOf() && (value.isWRT() || value.hasSubRelative()))
        || (value.isSectionRelative()
            && (value.isWRT() || value.hasSubRelative())))
    {
        throw TooComplexError(N_("coff: relocation too complex"));
    }

    IntNum intn(0);
    IntNum dist(0);
    if (value.isRelative())
    {
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
                throw TooComplexError(N_("coff: wrt expression too complex"));
            if (!CalcDist(wrt_loc, rel_loc, &dist))
                throw TooComplexError(N_("coff: cannot wrt across sections"));
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
                SimplifyCalcDist(csize_expr);
                if (!csize_expr.isIntNum())
                    throw TooComplexError(N_("coff: common size too complex"));

                IntNum common_size = csize_expr.getIntNum();
                if (common_size.getSign() < 0)
                    throw ValueError(N_("coff: common size is negative"));

                intn += common_size;
            }
        }
        else if (!(vis & Symbol::EXTERN) && !m_objfmt.isWin64())
        {
            // Local symbols need relocation to their section's start
            Location symloc;
            if (sym->getLabel(&symloc))
            {
                Section* sym_sect = symloc.bc->getContainer()->AsSection();
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
            throw TooComplexError(N_("elf: relocation too complex"));

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

        // Zero value for segment or section-relative generation.
        if (value.isSegOf() || value.isSectionRelative())
            intn = 0;

        // Generate reloc
        CoffObject::Machine machine = m_objfmt.getMachine();
        CoffReloc::Type rtype = CoffReloc::ABSOLUTE;
        IntNum addr = loc.getOffset();
        addr += loc.bc->getContainer()->AsSection()->getVMA();

        if (machine == CoffObject::MACHINE_I386)
        {
            if (pc_rel)
            {
                if (value.getSize() == 32)
                    rtype = CoffReloc::I386_REL32;
                else
                    throw TypeError(N_("coff: invalid relocation size"));
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
                    throw TypeError(N_("coff: invalid relocation size"));
                switch (value.getNextInsn())
                {
                    case 0: rtype = CoffReloc::AMD64_REL32; break;
                    case 1: rtype = CoffReloc::AMD64_REL32_1; break;
                    case 2: rtype = CoffReloc::AMD64_REL32_2; break;
                    case 3: rtype = CoffReloc::AMD64_REL32_3; break;
                    case 4: rtype = CoffReloc::AMD64_REL32_4; break;
                    case 5: rtype = CoffReloc::AMD64_REL32_5; break;
                    default:
                        throw TypeError(N_("coff: invalid relocation size"));
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
                    throw TypeError(N_("coff: invalid relocation size"));
            }
        }
        else
            assert(false);  // unrecognized machine

        Section* sect = loc.bc->getContainer()->AsSection();
        Reloc* reloc = 0;
        if (machine == CoffObject::MACHINE_I386)
            reloc = new Coff32Reloc(addr, sym, rtype);
        else if (machine == CoffObject::MACHINE_AMD64)
            reloc = new Coff64Reloc(addr, sym, rtype);
        else
            assert(false && "nonexistent machine");
        sect->AddReloc(std::auto_ptr<Reloc>(reloc));
    }

    if (Expr* abs = value.getAbs())
    {
        if (!abs->isIntNum())
            throw TooComplexError(N_("coff: relocation too complex"));
        intn += abs->getIntNum();
    }

    intn += dist;

    m_object.getArch()->ToBytes(intn, bytes, value.getSize(), 0, warn);
}

void
CoffOutput::OutputSection(Section& sect, Errwarns& errwarns)
{
    BytecodeOutput* outputter = this;

    CoffSection* coffsect = sect.getAssocData<CoffSection>();
    assert(coffsect != 0);
    m_coffsect = coffsect;

    // Add to strtab if in win32 format and name > 8 chars
    if (m_objfmt.isWin32())
    {
        size_t namelen = sect.getName().length();
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
        if (sect.bytecodes_last().getNextOffset() == 0)
            return;

        pos = m_os.tellp();
        if (pos < 0)
            throw IOError(N_("could not get file position on output file"));
    }
    sect.setFilePos(static_cast<unsigned long>(pos));
    coffsect->m_size = 0;

    // Output bytecodes
    for (Section::bc_iterator i=sect.bytecodes_begin(),
         end=sect.bytecodes_end(); i != end; ++i)
    {
        try
        {
            i->Output(*outputter);
            coffsect->m_size += i->getTotalLen();
        }
        catch (Error& err)
        {
            errwarns.Propagate(i->getLine(), err);
        }
        errwarns.Propagate(i->getLine());   // propagate warnings
    }

    // Sanity check final section size
    assert(coffsect->m_size == sect.bytecodes_last().getNextOffset());

    // No relocations to output?  Go on to next section
    if (sect.getRelocs().size() == 0)
        return;

    pos = m_os.tellp();
    if (pos < 0)
        throw IOError(N_("could not get file position on output file"));
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
            setWarn(WARN_GENERAL,
                    String::Compose(N_("too many relocations in section `%1'"),
                                    sect.getName()));
            errwarns.Propagate(0);
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
}

unsigned long
CoffOutput::CountSymbols()
{
    unsigned long indx = 0;

    for (Object::symbol_iterator i = m_object.symbols_begin(),
         end = m_object.symbols_end(); i != end; ++i)
    {
        int vis = i->getVisibility();

        // Don't output local syms unless outputting all syms
        if (!m_all_syms && vis == Symbol::LOCAL && !i->isAbsoluteSymbol())
            continue;

        CoffSymbol* coffsym = i->getAssocData<CoffSymbol>();

        // Create basic coff symbol data if it doesn't already exist
        if (!coffsym)
        {
            if (vis & (Symbol::EXTERN|Symbol::GLOBAL|Symbol::COMMON))
                coffsym = new CoffSymbol(CoffSymbol::SCL_EXT);
            else
                coffsym = new CoffSymbol(CoffSymbol::SCL_STAT);
            i->AddAssocData(std::auto_ptr<CoffSymbol>(coffsym));
        }
        coffsym->m_index = indx;

        indx += coffsym->m_aux.size() + 1;
    }

    return indx;
}

void
CoffOutput::OutputSymbolTable(Errwarns& errwarns)
{
    for (Object::const_symbol_iterator i = m_object.symbols_begin(),
         end = m_object.symbols_end(); i != end; ++i)
    {
        // Don't output local syms unless outputting all syms
        if (!m_all_syms && i->getVisibility() == Symbol::LOCAL
            && !i->isAbsoluteSymbol())
            continue;

        // Get symrec data
        const CoffSymbol* coffsym = i->getAssocData<CoffSymbol>();
        assert(coffsym != 0);

        Bytes& bytes = getScratch();
        coffsym->Write(bytes, *i, errwarns, m_strtab);
        m_os << bytes;
    }
}

void
CoffOutput::OutputStringTable()
{
    Bytes& bytes = getScratch();
    bytes << little_endian;
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
CoffObject::Output(std::ostream& os, bool all_syms, Errwarns& errwarns)
{
    // Update file symbol filename
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
            addr += i->bytecodes_last().getNextOffset();
        }
    }

    // Allocate space for headers by seeking forward.
    os.seekp(20+40*(scnum-1));
    if (!os)
        throw IOError(N_("could not seek on output file"));

    CoffOutput out(os, *this, m_object, all_syms);

    // Finalize symbol table (assign index to each symbol).
    unsigned long symtab_count = out.CountSymbols();

    // Section data/relocs
    for (Object::section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        out.OutputSection(*i, errwarns);
    }

    // Symbol table
    long pos = os.tellp();
    if (pos < 0)
        throw IOError(N_("could not get file position on output file"));
    unsigned long symtab_pos = static_cast<unsigned long>(pos);
    out.OutputSymbolTable(errwarns);

    // String table
    out.OutputStringTable();

    // Write headers
    os.seekp(0);
    if (!os)
        throw IOError(N_("could not seek on output file"));

    // Write file header
    Bytes& bytes = out.getScratch();
    bytes << little_endian;
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
#if 0
    if (String::NocaseEqual(object->dbgfmt, "null"))
        flags |= F_LNNO;
#endif
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

CoffObject::~CoffObject()
{
}

Section*
CoffObject::AddDefaultSection()
{
    Section* section = AppendSection(".text", 0);
    section->setDefault(true);
    return section;
}

bool
CoffObject::InitSection(const std::string& name,
                        Section& section,
                        CoffSection* coffsect)
{
    unsigned long flags = 0;

    if (name == ".data")
        flags = CoffSection::DATA;
    else if (name == ".bss")
    {
        flags = CoffSection::BSS;
        section.setBSS(true);
    }
    else if (name == ".text")
    {
        flags = CoffSection::TEXT;
        section.setCode(true);
    }
    else if (name == ".rdata"
             || (name.length() >= 7 && name.compare(0, 7, ".rodata") == 0)
             || (name.length() >= 7 && name.compare(0, 7, ".rdata$") == 0))
    {
        flags = CoffSection::DATA;
        setWarn(WARN_GENERAL,
                N_("Standard COFF does not support read-only data sections"));
    }
    else if (name == ".drectve")
        flags = CoffSection::INFO;
    else if (name == ".comment")
        flags = CoffSection::INFO;
    else if (String::NocaseEqual(name, ".debug", 6))
        flags = CoffSection::DATA;
    else
    {
        // Default to code (NASM default; note GAS has different default.
        flags = CoffSection::TEXT;
        section.setCode(true);
        return false;
    }
    coffsect->m_flags = flags;
    return true;
}

Section*
CoffObject::AppendSection(const std::string& name, unsigned long line)
{
    Section* section = new Section(name, false, false, line);
    m_object.AppendSection(std::auto_ptr<Section>(section));

    // Define a label for the start of the section
    Location start = {&section->bytecodes_first(), 0};
    SymbolRef sym = m_object.getSymbol(name);
    sym->DefineLabel(start, line);
    sym->Declare(Symbol::GLOBAL, line);
    sym->AddAssocData(
        std::auto_ptr<CoffSymbol>(new CoffSymbol(CoffSymbol::SCL_STAT,
                                                CoffSymbol::AUX_SECT)));

    // Add COFF data to the section
    CoffSection* coffsect = new CoffSection(sym);
    section->AddAssocData(std::auto_ptr<CoffSection>(coffsect));
    InitSection(name, *section, coffsect);

    return section;
}

void
CoffObject::DirGasSection(Object& object,
                          NameValues& nvs,
                          NameValues& objext_nvs,
                          unsigned long line)
{
    assert(&object == &m_object);

    if (!nvs.front().isString())
        throw Error(N_("section name must be a string"));
    std::string sectname = nvs.front().getString();

    if (sectname.length() > 8 && !m_win32)
    {
        // win32 format supports >8 character section names in object
        // files via "/nnnn" (where nnnn is decimal offset into string table),
        // so only warn for regular COFF.
        setWarn(WARN_GENERAL,
                N_("COFF section names limited to 8 characters: truncating"));
        sectname.resize(8);
    }

    Section* sect = m_object.FindSection(sectname);
    bool first = true;
    if (sect)
        first = sect->isDefault();
    else
        sect = AppendSection(sectname, line);

    CoffSection* coffsect = sect->getAssocData<CoffSection>();
    assert(coffsect != 0);

    m_object.setCurSection(sect);
    sect->setDefault(false);

    // Default to read/write data
    if (first)
    {
        coffsect->m_flags =
            CoffSection::TEXT | CoffSection::READ | CoffSection::WRITE;
    }

    // No flags, so nothing more to do
    if (nvs.size() <= 1)
        return;

    // Section flags must be a string.
    if (!nvs[1].isString())
        throw SyntaxError(N_("flag string expected"));

    // Parse section flags
    bool alloc = false, load = false, readonly = false, code = false;
    bool datasect = false, shared = false;
    std::string flagstr = nvs[1].getString();

    for (std::string::size_type i=0; i<flagstr.length(); ++i)
    {
        switch (flagstr[i])
        {
            case 'a':
                break;
            case 'b':
                alloc = true;
                load = false;
                break;
            case 'n':
                load = false;
                break;
            case 's':
                shared = true;
                /*@fallthrough@*/
            case 'd':
                datasect = true;
                load = true;
                readonly = false;
            case 'x':
                code = true;
                load = true;
                break;
            case 'r':
                datasect = true;
                load = true;
                readonly = true;
                break;
            case 'w':
                readonly = false;
                break;
            default:
                setWarn(WARN_GENERAL, String::Compose(
                    N_("unrecognized section attribute: `%1'"), flagstr[i]));
        }
    }

    if (code)
    {
        coffsect->m_flags =
            CoffSection::TEXT | CoffSection::EXECUTE | CoffSection::READ;
    }
    else if (datasect)
    {
        coffsect->m_flags =
            CoffSection::DATA | CoffSection::READ | CoffSection::WRITE;
    }
    else if (readonly)
        coffsect->m_flags = CoffSection::DATA | CoffSection::READ;
    else if (load)
        coffsect->m_flags = CoffSection::TEXT;
    else if (alloc)
        coffsect->m_flags = CoffSection::BSS;

    if (shared)
        coffsect->m_flags |= CoffSection::SHARED;

    sect->setBSS((coffsect->m_flags & CoffSection::BSS) != 0);
    sect->setCode((coffsect->m_flags & CoffSection::EXECUTE) != 0);

    if (!m_win32)
        coffsect->m_flags &= ~CoffSection::WIN32_MASK;
}

void
CoffObject::DirSectionInitHelpers(DirHelpers& helpers,
                                  CoffSection* coffsect,
                                  IntNum* align,
                                  bool* has_align,
                                  unsigned long line)
{
    helpers.Add("code", false,
                BIND::bind(&DirResetFlag, _1, &coffsect->m_flags,
                           CoffSection::TEXT |
                           CoffSection::EXECUTE |
                           CoffSection::READ));
    helpers.Add("text", false,
                BIND::bind(&DirResetFlag, _1, &coffsect->m_flags,
                           CoffSection::TEXT |
                           CoffSection::EXECUTE |
                           CoffSection::READ));
    helpers.Add("data", false,
                BIND::bind(&DirResetFlag, _1, &coffsect->m_flags,
                           CoffSection::DATA |
                           CoffSection::READ |
                           CoffSection::WRITE));
    helpers.Add("rdata", false,
                BIND::bind(&DirResetFlag, _1, &coffsect->m_flags,
                           CoffSection::DATA | CoffSection::READ));
    helpers.Add("bss", false,
                BIND::bind(&DirResetFlag, _1, &coffsect->m_flags,
                           CoffSection::BSS |
                           CoffSection::READ |
                           CoffSection::WRITE));
    helpers.Add("info", false,
                BIND::bind(&DirResetFlag, _1, &coffsect->m_flags,
                           CoffSection::INFO |
                           CoffSection::DISCARD |
                           CoffSection::READ));
    helpers.Add("align", true,
                BIND::bind(&DirIntNum, _1, &m_object, line, align, has_align));
}

void
CoffObject::DirSection(Object& object,
                       NameValues& nvs,
                       NameValues& objext_nvs,
                       unsigned long line)
{
    assert(&object == &m_object);

    if (!nvs.front().isString())
        throw Error(N_("section name must be a string"));
    std::string sectname = nvs.front().getString();

    if (sectname.length() > 8 && !m_win32)
    {
        // win32 format supports >8 character section names in object
        // files via "/nnnn" (where nnnn is decimal offset into string table),
        // so only warn for regular COFF.
        setWarn(WARN_GENERAL,
                N_("COFF section names limited to 8 characters: truncating"));
        sectname.resize(8);
    }

    Section* sect = m_object.FindSection(sectname);
    bool first = true;
    if (sect)
        first = sect->isDefault();
    else
        sect = AppendSection(sectname, line);

    CoffSection* coffsect = sect->getAssocData<CoffSection>();
    assert(coffsect != 0);

    m_object.setCurSection(sect);
    sect->setDefault(false);

    // No name/values, so nothing more to do
    if (nvs.size() <= 1)
        return;

    // Ignore flags if we've seen this section before
    if (!first)
    {
        setWarn(WARN_GENERAL,
                N_("section flags ignored on section redeclaration"));
        return;
    }

    // Parse section flags
    IntNum align;
    bool has_align = false;

    DirHelpers helpers;
    DirSectionInitHelpers(helpers, coffsect, &align, &has_align, line);
    helpers(++nvs.begin(), nvs.end(), DirNameValueWarn);

    sect->setBSS((coffsect->m_flags & CoffSection::BSS) != 0);
    sect->setCode((coffsect->m_flags & CoffSection::EXECUTE) != 0);

    if (!m_win32)
        coffsect->m_flags &= ~CoffSection::WIN32_MASK;

    if (has_align)
    {
        unsigned long aligni = align.getUInt();

        // Alignments must be a power of two.
        if (!isExp2(aligni))
        {
            throw ValueError(String::Compose(
                N_("argument to `%1' is not a power of two"), "align"));
        }

        // Check to see if alignment is supported size
        if (aligni > 8192)
            throw ValueError(N_("Win32 does not support alignments > 8192"));

        sect->setAlign(aligni);
    }
}

void
CoffObject::DirIdent(Object& object,
                     NameValues& namevals,
                     NameValues& objext_namevals,
                     unsigned long line)
{
    assert(&m_object == &object);
    DirIdentCommon(*this, ".comment", object, namevals, objext_namevals, line);
}

void
CoffObject::AddDirectives(Directives& dirs, const char* parser)
{
    static const Directives::Init<CoffObject> nasm_dirs[] =
    {
        {"section", &CoffObject::DirSection, Directives::ARG_REQUIRED},
        {"segment", &CoffObject::DirSection, Directives::ARG_REQUIRED},
        {"ident",   &CoffObject::DirIdent, Directives::ANY},
    };
    static const Directives::Init<CoffObject> gas_dirs[] =
    {
        {".section", &CoffObject::DirGasSection, Directives::ARG_REQUIRED},
        {".ident",   &CoffObject::DirIdent, Directives::ANY},
    };

    if (String::NocaseEqual(parser, "nasm"))
        dirs.AddArray(this, nasm_dirs, NELEMS(nasm_dirs));
    else if (String::NocaseEqual(parser, "gas"))
        dirs.AddArray(this, gas_dirs, NELEMS(gas_dirs));
}

void
DoRegister()
{
    RegisterModule<ObjectFormatModule,
                   ObjectFormatModuleImpl<CoffObject> >("coff");
}

}}} // namespace yasm::objfmt::coff

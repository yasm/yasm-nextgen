//
// Mach-O object format writer
//
//  Copyright (C) 2002-2012  Peter Johnson
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
#include "MachObject.h"

#include "llvm/Support/raw_ostream.h"

#include "yasmx/Arch.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/DebugFormat.h"
#include "yasmx/IntNum.h"
#include "yasmx/Location_util.h"
#include "yasmx/Object.h"
#include "yasmx/StringTable.h"
#include "yasmx/Symbol.h"

#include "modules/arch/x86/X86General.h"

#include "MachReloc.h"
#include "MachSection.h"
#include "MachSymbol.h"

// Mach-O file header values
#define MH_MAGIC                0xfeedface
#define MH_MAGIC_64             0xfeedfacf

// Mach-O in-file header structure sizes
#define SYMCMD_SIZE     24
#define DYSYMCMD_SIZE   80
#define RELINFO_SIZE    8

// 32 bit sizes
#define HEADER32_SIZE   28
#define SEGCMD32_SIZE   56
#define SECTCMD32_SIZE  68
#define NLIST32_SIZE    12

// 64 bit sizes
#define HEADER64_SIZE   32
#define SEGCMD64_SIZE   72
#define SECTCMD64_SIZE  80
#define NLIST64_SIZE    16

// CPU machine type
#define CPU_TYPE_I386           7       // x86 platform
#define CPU_TYPE_X86_64         (CPU_TYPE_I386|CPU_ARCH_ABI64)
#define CPU_ARCH_ABI64          0x01000000      // 64 bit ABI

// CPU machine subtype, e.g. processor
#define CPU_SUBTYPE_I386_ALL    3       // all-x86 compatible
#define CPU_SUBTYPE_X86_64_ALL  CPU_SUBTYPE_I386_ALL
#define CPU_SUBTYPE_386         3
#define CPU_SUBTYPE_486         4
#define CPU_SUBTYPE_486SX       (4 + 128)
#define CPU_SUBTYPE_586         5
#define CPU_SUBTYPE_INTEL(f, m) ((f) + ((m) << 4))
#define CPU_SUBTYPE_PENT        CPU_SUBTYPE_INTEL(5, 0)
#define CPU_SUBTYPE_PENTPRO     CPU_SUBTYPE_INTEL(6, 1)
#define CPU_SUBTYPE_PENTII_M3   CPU_SUBTYPE_INTEL(6, 3)
#define CPU_SUBTYPE_PENTII_M5   CPU_SUBTYPE_INTEL(6, 5)
#define CPU_SUBTYPE_PENTIUM_4   CPU_SUBTYPE_INTEL(10, 0)

#define CPU_SUBTYPE_INTEL_FAMILY(x)     ((x) & 15)
#define CPU_SUBTYPE_INTEL_FAMILY_MAX    15

#define CPU_SUBTYPE_INTEL_MODEL(x)      ((x) >> 4)
#define CPU_SUBTYPE_INTEL_MODEL_ALL     0

#define MH_OBJECT                   0x1     // object file
#define MH_SUBSECTIONS_VIA_SYMBOLS  0x2000

#define LC_SEGMENT              0x1     // segment load command
#define LC_SYMTAB               0x2     // symbol table load command
#define LC_DYSYMTAB             0xb     // dynamic symbol table load command
#define LC_SEGMENT_64           0x19    // segment load command

#define VM_PROT_NONE            0x00
#define VM_PROT_READ            0x01
#define VM_PROT_WRITE           0x02
#define VM_PROT_EXECUTE         0x04

#define VM_PROT_DEFAULT (VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE)
#define VM_PROT_ALL     (VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE)

// macho references symbols in different ways whether they are linked at
// runtime (LAZY, read library functions) or at link time (NON_LAZY, mostly
// data)
//
// TODO: proper support for dynamically linkable modules would require the
// __import sections as well as the dsymtab command
#define REFERENCE_FLAG_UNDEFINED_NON_LAZY 0x0
#define REFERENCE_FLAG_UNDEFINED_LAZY     0x1

#define alignxy(x, y) \
    (((x) + (y) - 1) & ~((y) - 1))  // align x to multiple of y

#define align32(x) \
    alignxy(x, 4)                   // align x to 32 bit boundary

#define macho_MAGIC     0x87654322


using namespace yasm;
using namespace yasm::objfmt;

namespace {
class MachOutput : public BytecodeStreamOutput
{
public:
    MachOutput(llvm::raw_ostream& os,
               Object& object,
               Diagnostic& diags,
               SymbolRef gotpcrel_sym,
               bool is64,
               bool all_syms);
    ~MachOutput();

    bool OutputSection(Section& sect);
    void OutputSectionRelocs(Section& sect, unsigned long* relocs_offset);
    void OutputFileHeader(unsigned long flags);
    void OutputSegmentCommand(unsigned long total_vmsize,
                              unsigned long total_filesize);

    void EnumerateSymbols();
    void OutputSymbolTable();
    void OutputStringTable();
    void OutputSymtabCommand();
    void OutputDysymtabCommand();

    // BytecodeOutput overrides
    bool ConvertValueToBytes(Value& value,
                             Location loc,
                             NumericOutput& num_out);

    unsigned long getHeadSize() const { return m_head_size; }

private:
    Object& m_object;
    BytecodeNoOutput m_no_output;

    // configuration
    SymbolRef m_gotpcrel_sym;
    bool m_is64;
    bool m_all_syms;
    unsigned long m_head_size;
    int m_longint_size;
    unsigned int m_segcmd_size, m_sectcmd_size, m_nlist_size;
    unsigned int m_segcmd;

    // symbol table info
    StringTable m_strtab;
    unsigned long m_symtab_offset;
    unsigned long m_symtab_count;
    unsigned long m_strtab_offset;
    unsigned long m_localsym_index;
    unsigned long m_localsym_count;
    unsigned long m_extdefsym_index;
    unsigned long m_extdefsym_count;
    unsigned long m_undefsym_index;
    unsigned long m_undefsym_count;

    // current section
    MachSection* m_machsect;
};
} // anonymous namespace

static bool
isSectionLabel(const Symbol& sym)
{
    Location loc;
    if (!sym.getLabel(&loc))
        return false;   // not a label

    BytecodeContainer* container = loc.bc->getContainer();
    if (!container)
        return false;   // no section

    Section* sect = container->getSection();
    if (!sect)
        return false;   // no section

    if (&(*sect->getSymbol()) != &sym)
        return false;

    return true;
}

MachOutput::MachOutput(llvm::raw_ostream& os,
                       Object& object,
                       Diagnostic& diags,
                       SymbolRef gotpcrel_sym,
                       bool is64,
                       bool all_syms)
    : BytecodeStreamOutput(os, diags)
    , m_object(object)
    , m_no_output(diags)
    , m_gotpcrel_sym(gotpcrel_sym)
    , m_is64(is64)
    , m_all_syms(all_syms)
    , m_symtab_offset(0)
    , m_symtab_count(0)
    , m_strtab_offset(0)
    , m_localsym_index(0)
    , m_localsym_count(0)
    , m_extdefsym_index(0)
    , m_extdefsym_count(0)
    , m_undefsym_index(0)
    , m_undefsym_count(0)
    , m_machsect(0)
{
    // Size MACH-O Header, Seg CMD, Sect CMDs, Sym Tab, Reloc Data
    if (is64)
    {
        m_head_size =
            HEADER64_SIZE + SEGCMD64_SIZE +
            (SECTCMD64_SIZE * object.getNumSections()) +
            SYMCMD_SIZE + DYSYMCMD_SIZE;
        m_segcmd = LC_SEGMENT_64;
        m_segcmd_size = SEGCMD64_SIZE;
        m_sectcmd_size = SECTCMD64_SIZE;
        m_nlist_size = NLIST64_SIZE;
        m_longint_size = 64;
    }
    else
    {
        m_head_size =
            HEADER32_SIZE + SEGCMD32_SIZE +
            (SECTCMD32_SIZE * object.getNumSections()) +
            SYMCMD_SIZE + DYSYMCMD_SIZE;
        m_segcmd = LC_SEGMENT;
        m_segcmd_size = SEGCMD32_SIZE;
        m_sectcmd_size = SECTCMD32_SIZE;
        m_nlist_size = NLIST32_SIZE;
        m_longint_size = 32;
    }
}

MachOutput::~MachOutput()
{
}

bool
MachOutput::ConvertValueToBytes(Value& value,
                                Location loc,
                                NumericOutput& num_out)
{
    m_object.getArch()->setEndian(num_out.getBytes());

    IntNum intn(0);
    if (value.OutputBasic(num_out, &intn, getDiagnostics()))
        return true;

    if (value.isRelative())
    {
        // We can't handle these types of values
        if (value.getRShift() > 0 || value.getShift() > 0 || value.isSegOf()
            || value.isSectionRelative())
        {
            Diag(value.getSource().getBegin(), diag::err_reloc_too_complex);
            return false;
        }

        SymbolRef sym = value.getRelative();
        SymbolRef wrt = value.getWRT();
        SymbolRef sub_sym = value.getSubSymbol();

        Section* sect = loc.bc->getContainer()->getSection();
        int vis = sym->getVisibility();

        // Generate reloc
        MachReloc::Type rtype = MachReloc::GENERIC_RELOC_VANILLA;
        IntNum addr = loc.getOffset();
        unsigned int length = 2;
        switch (value.getSize())
        {
            case 64: length = 3; break;
            case 32: length = 2; break;
            case 16: length = 1; break;
            case 8:  length = 0; break;
            default:
                Diag(value.getSource().getBegin(),
                     diag::err_reloc_invalid_size);
                return false;
        }
        bool pcrel = false;
        bool ext = false;
        // R_ABS

        if (wrt && wrt == m_gotpcrel_sym)
        {
            rtype = MachReloc::X86_64_RELOC_GOT;
            wrt = SymbolRef(0);
        }
        else if (wrt)
            Diag(value.getSource().getBegin(), diag::err_invalid_wrt);

        if ((vis & Symbol::EXTERN) || (vis & Symbol::COMMON))
            ext = true;

        IntNum intn2;
        if (length == 2 && value.CalcPCRelSub(&intn2, loc))
        {
            // Create PC-relative relocation type and fix up absolute portion.
            pcrel = true;
            intn += intn2;
        }

        if (m_is64)
        {
            // It seems that x86-64 objects need to have all extern relocs?
            ext = true;

            if (pcrel)
            {
                intn += value.getSize()/8;
                if (rtype == MachReloc::X86_64_RELOC_GOT)
                {
                    // XXX: This is the cleanest way currently to do this...
                    const Bytecode::Contents& contents = loc.bc->getContents();
                    if (contents.getType() == "yasm::arch::X86General")
                    {
                        const arch::X86General& general =
                            static_cast<const arch::X86General&>(contents);
                        if (general.getOpcode().get(0) == 0x8B)
                            rtype = MachReloc::X86_64_RELOC_GOT_LOAD;
                    }
                }
                else if (value.isJumpTarget())
                    rtype = MachReloc::X86_64_RELOC_BRANCH;
                else
                    rtype = MachReloc::X86_64_RELOC_SIGNED;
            }
            else if (value.hasSubRelative())
            {
                if (!sub_sym)
                {
                    // XXX: any need to handle location?
                    Diag(value.getSource().getBegin(),
                         diag::err_reloc_too_complex);
                    return false;
                }

                // build and add a subtractor reloc
                MachReloc* sub_reloc;
                sub_reloc = new Mach64Reloc(addr, sub_sym,
                                            MachReloc::X86_64_RELOC_SUBTRACTOR,
                                            false, length, ext);
                sect->AddReloc(std::auto_ptr<Reloc>(sub_reloc));

                // this reloc is unsigned
                rtype = MachReloc::X86_64_RELOC_UNSIGNED;
            }
            else
            {
                if (length != 3)
                {
                    Diag(value.getSource().getBegin(),
                         diag::err_macho_no_32_absolute_reloc_in_64);
                    return false;
                }
                rtype = MachReloc::X86_64_RELOC_UNSIGNED;
            }
        }
        else
        {
            if (pcrel)
            {
                // Adjust to start of section by subtracting value location.
                intn -= loc.getOffset();
            }
            else if (value.hasSubRelative())
            {
                // FIXME: add handling for this
                Diag(value.getSource().getBegin(), diag::err_reloc_too_complex);
                return false;
            }

            if (!ext)
            {
                // Local symbols need valued to their actual address
                Location sym_loc;
                if (sym->getLabel(&sym_loc))
                {
                    intn += sym_loc.bc->getContainer()->getSection()->getVMA();
                    intn += sym_loc.getOffset();
                }
            }
        }

        if (ext)
        {
            m_machsect->extreloc = true; // section has external relocations

            // external relocations must be in the symbol table
            if (sub_sym)
            {
                MachSymbol* msubsym = sub_sym->getAssocData<MachSymbol>();
                if (!msubsym)
                {
                    msubsym = new MachSymbol;
                    sub_sym->AddAssocData(std::auto_ptr<MachSymbol>(msubsym));
                }
                msubsym->m_required = true;
            }

            if (sym)
            {
                MachSymbol* msym = sym->getAssocData<MachSymbol>();
                if (!msym)
                {
                    msym = new MachSymbol;
                    sym->AddAssocData(std::auto_ptr<MachSymbol>(msym));
                }
                msym->m_required = true;
            }
        }

        MachReloc* reloc;
        if (m_is64)
            reloc = new Mach32Reloc(addr, sym, rtype, pcrel, length, ext);
        else
            reloc = new Mach64Reloc(addr, sym, rtype, pcrel, length, ext);
        sect->AddReloc(std::auto_ptr<Reloc>(reloc));
    }
    num_out.OutputInteger(intn);
    return true;
}

bool
MachOutput::OutputSection(Section& sect)
{
    BytecodeOutput* outputter = this;

    MachSection* msect = sect.getAssocData<MachSection>();
    assert(msect != NULL);
    m_machsect = msect;

    if (sect.isBSS())
    {
        // Don't output BSS sections.
        outputter = &m_no_output;
    }
    else
    {
        unsigned long pos = m_os.tell();
        if (m_os.has_error() || pos > sect.getFilePos())
        {
            Diag(SourceLocation(), diag::err_file_output_position);
            return false;
        }
        // pad with zeros to section start
        Bytes& scratch = getScratch();
        scratch.Write(sect.getFilePos() - pos, 0);
        m_os << scratch;
    }

    // Output bytecodes
    unsigned long size = 0;
    for (Section::bc_iterator i=sect.bytecodes_begin(),
         end=sect.bytecodes_end(); i != end; ++i)
    {
        if (i->Output(*outputter))
            size += i->getTotalLen();
    }

    if (getDiagnostics().hasErrorOccurred())
        return false;

    // Sanity check final section size
    assert(size == sect.bytecodes_back().getNextOffset());

    return true;
}


void
MachOutput::OutputSectionRelocs(Section& sect, unsigned long* relocs_offset)
{
    MachSection* msect = sect.getAssocData<MachSection>();
    assert(msect != 0);

    for (Section::const_reloc_iterator i=sect.relocs_begin(),
         end=sect.relocs_end(); i != end; ++i)
    {
        const MachReloc& reloc = static_cast<const MachReloc&>(*i);
        Bytes& scratch = getScratch();
        reloc.Write(scratch);
        assert(scratch.size() == RELINFO_SIZE);
        m_os << scratch;
    }

    msect->reloff = *relocs_offset;
    *relocs_offset += RELINFO_SIZE * sect.getRelocs().size();
}

void
MachOutput::OutputFileHeader(unsigned long flags)
{
    Bytes& scratch = getScratch();
    scratch.setLittleEndian();

    // size is common to 32 bit and 64 bit variants
    if (m_is64)
    {
        Write32(scratch, MH_MAGIC_64); // magic number
        // i386 64-bit ABI
        Write32(scratch, CPU_ARCH_ABI64 | CPU_TYPE_I386);
    }
    else
    {
        Write32(scratch, MH_MAGIC);         // magic number
        Write32(scratch, CPU_TYPE_I386);    // i386 32-bit ABI
    }
    // i386 all cpu subtype compatible
    Write32(scratch, CPU_SUBTYPE_I386_ALL);
    Write32(scratch, MH_OBJECT);            // MACH file type

    // calculate number of commands and their size, put to stream
    unsigned int head_ncmds = 0;
    unsigned int head_sizeofcmds = 0;
    if (m_object.getNumSections() > 0)
    {
        head_ncmds++;
        head_sizeofcmds += m_segcmd_size +
            m_sectcmd_size * m_object.getNumSections();
    }
    if (m_symtab_count > 0)
    {
        head_ncmds += 2;
        head_sizeofcmds += SYMCMD_SIZE;
        head_sizeofcmds += DYSYMCMD_SIZE;
    }

    Write32(scratch, head_ncmds);
    Write32(scratch, head_sizeofcmds);
    Write32(scratch, flags);    // flags

    if (m_is64)
        Write32(scratch, 0);    // reserved in 64 bit

    if (m_is64)
        assert(scratch.size() == HEADER64_SIZE);
    else
        assert(scratch.size() == HEADER32_SIZE);
    m_os << scratch;
}

void
MachOutput::OutputSegmentCommand(unsigned long total_vmsize,
                                 unsigned long total_filesize)
{
    Bytes& scratch = getScratch();
    scratch.setLittleEndian();

    Write32(scratch, m_segcmd);     // command LC_SEGMENT
    // size of load command including section load commands
    Write32(scratch,
            m_segcmd_size + m_sectcmd_size * m_object.getNumSections());
    // in an MH_OBJECT file all sections are in one unnamed (all zeros) segment
    scratch.Write(16, 0);

    // in-memory offset, in-memory size
    WriteN(scratch, 0, m_longint_size);             // offset in memory (vmaddr)
    WriteN(scratch, total_vmsize, m_longint_size);  // size in memory (vmsize)
    // offset in file to first section
    WriteN(scratch, m_object.sections_begin()->getFilePos(), m_longint_size);
    WriteN(scratch, total_filesize, m_longint_size);// overall size in file

    Write32(scratch, VM_PROT_DEFAULT); // VM protection, maximum
    Write32(scratch, VM_PROT_DEFAULT); // VM protection, initial

    // number of sections
    Write32(scratch, m_object.getNumSections());
    Write32(scratch, 0);    // no flags

    // write header and segment command to file
    assert(scratch.size() == m_segcmd_size);
    m_os << scratch;

    // section headers
    for (Object::const_section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        const MachSection* msect = i->getAssocData<MachSection>();
        assert(msect != 0);

        scratch.resize(0);
        msect->Write(scratch, *i, m_longint_size);
        assert(scratch.size() == m_sectcmd_size);
        m_os << scratch;
    }
}

static inline bool
MachSymbolIsInTable(const Symbol& sym)
{
    const MachSymbol* msym = sym.getAssocData<MachSymbol>();
    return msym && msym->m_required;
}

static inline bool
MachSymbolIsLocal(const Symbol& sym)
{
    const MachSymbol* msym = sym.getAssocData<MachSymbol>();
    assert(msym);
    return (msym->getType() & MachSymbol::N_EXT) == 0;
}

static inline bool
MachSymbolIsDefined(const Symbol& sym)
{
    const MachSymbol* msym = sym.getAssocData<MachSymbol>();
    assert(msym);
    return (msym->getType() & MachSymbol::N_TYPE) != MachSymbol::N_UNDF;
}

void
MachOutput::EnumerateSymbols()
{
    m_symtab_count = 0;

    // finalize symbols (to determine type field, which is used for sorting)
    for (Object::symbol_iterator i = m_object.symbols_begin(),
         end = m_object.symbols_end(); i != end; ++i)
    {
        MachSymbol* msym = i->getAssocData<MachSymbol>();

        if (!msym || !msym->m_required)
        {
            if (!m_all_syms && i->getVisibility() == Symbol::LOCAL
                && !i->isAbsoluteSymbol())
                continue;

            if (isSectionLabel(*i))
                continue;
        }

        // Save index in symrec data
        if (!msym)
        {
            msym = new MachSymbol;
            i->AddAssocData(std::auto_ptr<MachSymbol>(msym));
        }
        msym->m_required = true;
        msym->Finalize(*i, getDiagnostics());
    }

    // Order symbols based on type field.  This also gives us the indexes
    // and counts needed for OutputDysymtabCommand().
    // 1) put table symbols before non-table
    // 2) put local symbols before external
    // 3) put external defined symbols before undefined
    Object::symbol_iterator table_end =
        stdx::stable_partition(m_object.symbols_begin(), m_object.symbols_end(),
                               MachSymbolIsInTable);
    Object::symbol_iterator extdef_begin =
        stdx::stable_partition(m_object.symbols_begin(), table_end,
                               MachSymbolIsLocal);
    Object::symbol_iterator undef_begin =
        stdx::stable_partition(extdef_begin, table_end, MachSymbolIsDefined);

    m_localsym_index = 0;
    m_localsym_count = extdef_begin - m_object.symbols_begin();
    m_extdefsym_index = m_localsym_count;
    m_extdefsym_count = undef_begin - extdef_begin;
    m_undefsym_index = undef_begin - m_object.symbols_begin();
    m_undefsym_count = table_end - undef_begin;

    // number symbols
    for (Object::symbol_iterator i = m_object.symbols_begin(),
         end = m_object.symbols_end(); i != end; ++i)
    {
        MachSymbol* msym = i->getAssocData<MachSymbol>();
        if (!msym || !msym->m_required)
            continue;

        msym->m_index = m_symtab_count++;
    }
}

void
MachOutput::OutputSymbolTable()
{
    m_symtab_offset = m_os.tell();
    for (Object::symbol_iterator i = m_object.symbols_begin(),
         end = m_object.symbols_end(); i != end; ++i)
    {
        const MachSymbol* msym = i->getAssocData<MachSymbol>();
        if (!msym || !msym->m_required)
            continue;

        Bytes& scratch = getScratch();
        msym->Write(scratch, *i, m_strtab, m_longint_size);
        assert(scratch.size() == m_nlist_size);
        m_os << scratch;
    }
}

void
MachOutput::OutputStringTable()
{
    m_strtab_offset = m_os.tell();
    m_strtab.Write(m_os);
}

void
MachOutput::OutputSymtabCommand()
{
    if (m_symtab_count == 0)
        return;

    Bytes& scratch = getScratch();
    scratch.setLittleEndian();

    Write32(scratch, LC_SYMTAB);            // command
    Write32(scratch, SYMCMD_SIZE);
    Write32(scratch, m_symtab_offset);      // symbol table offset
    Write32(scratch, m_symtab_count);       // number of symbols

    Write32(scratch, m_strtab_offset);      // string table offset
    Write32(scratch, m_strtab.getSize());   // string table size

    // write to file
    assert(scratch.size() == SYMCMD_SIZE);
    m_os << scratch;
}

void
MachOutput::OutputDysymtabCommand()
{
    if (m_symtab_count == 0)
        return;

    Bytes& scratch = getScratch();
    scratch.setLittleEndian();

    Write32(scratch, LC_DYSYMTAB);      // command
    Write32(scratch, DYSYMCMD_SIZE);
    Write32(scratch, m_localsym_index); // index to local symbols
    Write32(scratch, m_localsym_count); // number of local symbols
    Write32(scratch, m_extdefsym_index);// index to externally defined symbols
    Write32(scratch, m_extdefsym_count);// number of externally defined symbols
    Write32(scratch, m_undefsym_index); // index to undefined symbols
    Write32(scratch, m_undefsym_count); // number of undefined symbols
    scratch.Write(12*4, 0);             // other fields unused

    // write to file
    assert(scratch.size() == DYSYMCMD_SIZE);
    m_os << scratch;
}

static inline bool
MachIsNotZeroFill(const Section& sect)
{
    const MachSection* msect = sect.getAssocData<MachSection>();
    assert(msect != NULL);
    return (msect->flags & MachSection::SECTION_TYPE) !=
        MachSection::S_ZEROFILL;
}

void
MachObject::Output(llvm::raw_fd_ostream& os,
                   bool all_syms,
                   DebugFormat& dbgfmt,
                   Diagnostic& diags)
{
    const char pad_data[3] = {0, 0, 0};

    // XXX: ugly workaround to prevent all_syms from kicking in
    if (dbgfmt.getModule().getKeyword() == "cfi")
        all_syms = false;

    // partition sections to put zerofill sections last
    stdx::stable_partition(m_object.sections_begin(), m_object.sections_end(),
                           MachIsNotZeroFill);

    MachOutput out(os, m_object, diags, m_gotpcrel_sym, m_bits == 64, all_syms);

    // write raw section data first
    os.seek((long)out.getHeadSize());
    if (os.has_error())
    {
        diags.Report(SourceLocation(), diag::err_file_output_seek);
        return;
    }

    // enumerate and get size of sections in memory (including BSS) and sizes
    // of sections in file (without BSS)
    long scnum = 0;
    unsigned long vmsize = 0;
    unsigned long filesize = 0;
    unsigned long offset = out.getHeadSize();
    for (Object::section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        MachSection* msect = i->getAssocData<MachSection>();
        assert(msect != NULL);
        msect->scnum = scnum++;

        msect->size = i->bytecodes_back().getNextOffset();
        // align start of section
        unsigned long align = i->getAlign();
        if (align > 1)
        {
            unsigned long delta;
            // vm start
            delta = vmsize % align;
            if (delta > 0)
                vmsize += align - delta;
            // offset in file
            delta = offset % align;
            if (delta > 0)
            {
                offset += align - delta;
                filesize += align - delta; // also bump up file size
            }
            // align size as well?
            //delta = msect->size % align;
            //if (delta > 0)
            //    msect->size += align - delta;
        }

        // accumulate size in memory
        i->setVMA(vmsize);
        vmsize += msect->size;

        // accumulate size in file
        if (!i->isBSS())
        {
            i->setFilePos(offset);
            offset += msect->size;
            filesize += msect->size;
        }
    }

    // output sections to file
    for (Object::section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        if (!out.OutputSection(*i))
            return;
    }

    // number symbols before generating relocations
    out.EnumerateSymbols();

    // pad to long boundary
    unsigned long reloc_start_offset = os.tell();
    unsigned long reloc_cur_offset = align32((long)reloc_start_offset);
    if ((reloc_cur_offset - reloc_start_offset) > 0)
        os.write(pad_data, reloc_cur_offset - reloc_start_offset);
    reloc_start_offset = reloc_cur_offset;

    // write relocations
    for (Object::section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        out.OutputSectionRelocs(*i, &reloc_cur_offset);
    }

    // write symbol table and strings
    out.OutputSymbolTable();
    out.OutputStringTable();

    // Write file headers
    os.seek(0);
    if (os.has_error())
    {
        diags.Report(SourceLocation(), diag::err_file_output_seek);
        return;
    }

    unsigned long flags = 0;
    if (m_subsections_via_symbols)
        flags |= MH_SUBSECTIONS_VIA_SYMBOLS;
    out.OutputFileHeader(flags);
    out.OutputSegmentCommand(vmsize, filesize);
    out.OutputSymtabCommand();
    out.OutputDysymtabCommand();
}

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
#include <yasmx/Support/marg_ostream.h>
#include <yasmx/Support/nocase.h>
#include <yasmx/Support/registry.h>
#include <yasmx/Arch.h>
#include <yasmx/BytecodeOutput.h>
#include <yasmx/Bytecode.h>
#include <yasmx/Bytes.h>
#include <yasmx/Bytes_util.h>
#include <yasmx/Compose.h>
#include <yasmx/Directive.h>
#include <yasmx/DirHelpers.h>
#include <yasmx/errwarn.h>
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

CoffObject::CoffObject(bool set_vma, bool win32, bool win64)
    : m_set_vma(set_vma)
    , m_win32(win32)
    , m_win64(win64)
    , m_machine(MACHINE_UNKNOWN)
    , m_file_coffsym(0)
{
}

std::string
CoffObject::get_name() const
{
    return "COFF (DJGPP)";
}

std::string
CoffObject::get_keyword() const
{
    return "coff";
}

std::string
CoffObject::get_extension() const
{
    return ".o";
}

unsigned int
CoffObject::get_default_x86_mode_bits() const
{
    return 32;
}

std::vector<std::string>
CoffObject::get_dbgfmt_keywords() const
{
    static const char* keywords[] = {"null", "dwarf2"};
    return std::vector<std::string>(keywords, keywords+NELEMS(keywords));
}

std::string
CoffObject::get_default_dbgfmt_keyword() const
{
    return "null";
}

bool
CoffObject::ok_object(Object* object) const
{
    // Support x86 and amd64 machines of x86 arch
    Arch* arch = object->get_arch();
    if (!String::nocase_equal(arch->get_keyword(), "x86"))
        return false;

    if (String::nocase_equal(arch->get_machine(), "x86"))
        return true;
    if (String::nocase_equal(arch->get_machine(), "amd64"))
        return true;
    return false;
}

void
CoffObject::initialize()
{
    // Support x86 and amd64 machines of x86 arch
    if (String::nocase_equal(m_object->get_arch()->get_machine(), "x86"))
        m_machine = MACHINE_I386;
    else if (String::nocase_equal(m_object->get_arch()->get_machine(), "amd64"))
        m_machine = MACHINE_AMD64;
}

void
CoffObject::init_symbols(const std::string& parser)
{
    // Add .file symbol
    SymbolRef filesym = m_object->append_symbol(".file");
    filesym->define_special(Symbol::GLOBAL);

    std::auto_ptr<CoffSymbol> coffsym(
        new CoffSymbol(CoffSymbol::SCL_FILE, CoffSymbol::AUX_FILE));
    m_file_coffsym = coffsym.get();

    filesym->add_assoc_data(CoffSymbol::key,
                            std::auto_ptr<AssocData>(coffsym.release()));
}

class Output : public BytecodeStreamOutput
{
public:
    Output(std::ostream& os, CoffObject& objfmt, Object& object, bool all_syms);
    ~Output();

    void output_section(Section& sect, Errwarns& errwarns);
    void output_secthead(const Section& sect);
    unsigned long count_syms();
    void output_symtab(Errwarns& errwarns);
    void output_strtab();

    // OutputBytecode overrides
    void value_to_bytes(Value& value, Bytes& bytes, Location loc, int warn);

private:
    CoffObject& m_objfmt;
    CoffSection* m_coffsect;
    Object& m_object;
    bool m_all_syms;
    StringTable m_strtab;
    BytecodeNoOutput m_no_output;
};

Output::Output(std::ostream& os,
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

Output::~Output()
{
}

void
Output::value_to_bytes(Value& value, Bytes& bytes, Location loc, int warn)
{
    if (Expr* abs = value.get_abs())
        simplify_calc_dist(*abs);

    // Try to output constant and PC-relative section-local first.
    // Note this does NOT output any value with a SEG, WRT, external,
    // cross-section, or non-PC-relative reference (those are handled below).
    if (value.output_basic(bytes, warn, *m_object.get_arch()))
        return;

    // Handle other expressions, with relocation if necessary
    if (value.get_rshift() > 0
        || (value.is_seg_of() && (value.is_wrt() || value.has_sub()))
        || (value.is_section_rel() && (value.is_wrt() || value.has_sub())))
    {
        throw TooComplexError(N_("coff: relocation too complex"));
    }

    IntNum intn(0);
    IntNum dist(0);
    if (value.is_relative())
    {
        SymbolRef sym = value.get_rel();
        SymbolRef wrt = value.get_wrt();

        // Sometimes we want the relocation to be generated against one
        // symbol but the value generated correspond to a different symbol.
        // This is done through (sym being referenced) WRT (sym used for
        // reloc).  Note both syms need to be in the same section!
        if (wrt)
        {
            Location wrt_loc, rel_loc;
            if (!sym->get_label(&rel_loc) || !wrt->get_label(&wrt_loc))
                throw TooComplexError(N_("coff: wrt expression too complex"));
            if (!calc_dist(wrt_loc, rel_loc, &dist))
                throw TooComplexError(N_("coff: cannot wrt across sections"));
            sym = wrt;
        }

        int vis = sym->get_visibility();
        if (vis & Symbol::COMMON)
        {
            // In standard COFF, COMMON symbols have their length added in
            if (!m_objfmt.is_win32())
            {
                assert(get_common_size(*sym) != 0);
                Expr csize_expr(*get_common_size(*sym));
                simplify_calc_dist(csize_expr);
                const IntNum* common_size = csize_expr.get_intnum();
                if (!common_size)
                    throw TooComplexError(N_("coff: common size too complex"));

                if (common_size->sign() < 0)
                    throw ValueError(N_("coff: common size is negative"));

                intn += *common_size;
            }
        }
        else if (!(vis & Symbol::EXTERN) && !m_objfmt.is_win64())
        {
            // Local symbols need relocation to their section's start
            Location symloc;
            if (sym->get_label(&symloc))
            {
                Section* sym_sect = symloc.bc->get_container()->as_section();
                CoffSection* coffsect = get_coff(*sym_sect);
                assert(coffsect != 0);
                sym = coffsect->m_sym;

                intn = symloc.get_offset();
                intn += sym_sect->get_vma();
            }
        }

        bool pc_rel = false;
        IntNum intn2;
        if (value.calc_pcrel_sub(&intn2, loc))
        {
            // Create PC-relative relocation type and fix up absolute portion.
            pc_rel = true;
            intn += intn2;
        }
        else if (value.has_sub())
            throw TooComplexError(N_("elf: relocation too complex"));

        if (pc_rel)
        {
            // For standard COFF, need to adjust to start of section, e.g.
            // subtract out the value location.
            // For Win32 COFF, adjust by value size.
            // For Win64 COFF, adjust to next instruction;
            // the delta is taken care of by special relocation types.
            if (m_objfmt.is_win64())
                intn += value.get_next_insn();
            else if (m_objfmt.is_win32())
                intn += value.get_size()/8;
            else
                intn -= loc.get_offset();
        }

        // Zero value for segment or section-relative generation.
        if (value.is_seg_of() || value.is_section_rel())
            intn = 0;

        // Generate reloc
        CoffObject::Machine machine = m_objfmt.get_machine();
        CoffReloc::Type rtype = CoffReloc::ABSOLUTE;
        IntNum addr = loc.get_offset();
        addr += loc.bc->get_container()->as_section()->get_vma();

        if (machine == CoffObject::MACHINE_I386)
        {
            if (pc_rel)
            {
                if (value.get_size() == 32)
                    rtype = CoffReloc::I386_REL32;
                else
                    throw TypeError(N_("coff: invalid relocation size"));
            }
            else if (value.is_seg_of())
                rtype = CoffReloc::I386_SECTION;
            else if (value.is_section_rel())
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
                if (value.get_size() != 32)
                    throw TypeError(N_("coff: invalid relocation size"));
                switch (value.get_next_insn())
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
            else if (value.is_seg_of())
                rtype = CoffReloc::AMD64_SECTION;
            else if (value.is_section_rel())
                rtype = CoffReloc::AMD64_SECREL;
            else
            {
                unsigned int size = value.get_size();
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

        Section* sect = loc.bc->get_container()->as_section();
        Reloc* reloc = 0;
        if (machine == CoffObject::MACHINE_I386)
            reloc = new Coff32Reloc(addr, sym, rtype);
        else if (machine == CoffObject::MACHINE_AMD64)
            reloc = new Coff64Reloc(addr, sym, rtype);
        else
            assert(false && "nonexistent machine");
        sect->add_reloc(std::auto_ptr<Reloc>(reloc));
    }

    if (Expr* abs = value.get_abs())
    {
        IntNum* intn2 = abs->get_intnum();
        if (!intn2)
            throw TooComplexError(N_("coff: relocation too complex"));
        intn += *intn2;
    }

    intn += dist;

    m_object.get_arch()->tobytes(intn, bytes, value.get_size(), 0, warn);
}

void
Output::output_section(Section& sect, Errwarns& errwarns)
{
    BytecodeOutput* outputter = this;

    CoffSection* coffsect = get_coff(sect);
    assert(coffsect != 0);
    m_coffsect = coffsect;

    // Add to strtab if in win32 format and name > 8 chars
    if (m_objfmt.is_win32())
    {
        size_t namelen = sect.get_name().length();
        if (namelen > 8)
            coffsect->m_strtab_name = m_strtab.get_index(sect.get_name());
    }

    long pos;
    if (sect.is_bss())
    {
        // Don't output BSS sections.
        outputter = &m_no_output;
        pos = 0;    // position = 0 because it's not in the file
    }
    else
    {
        if (sect.bcs_last().next_offset() == 0)
            return;

        pos = m_os.tellp();
        if (pos < 0)
            throw IOError(N_("could not get file position on output file"));
    }
    sect.set_filepos(static_cast<unsigned long>(pos));
    coffsect->m_size = 0;

    // Output bytecodes
    for (Section::bc_iterator i=sect.bcs_begin(), end=sect.bcs_end();
         i != end; ++i)
    {
        try
        {
            i->output(*outputter);
            coffsect->m_size += i->get_total_len();
        }
        catch (Error& err)
        {
            errwarns.propagate(i->get_line(), err);
        }
        errwarns.propagate(i->get_line());  // propagate warnings
    }

    // Sanity check final section size
    assert(coffsect->m_size == sect.bcs_last().next_offset());

    // No relocations to output?  Go on to next section
    if (sect.get_relocs().size() == 0)
        return;

    pos = m_os.tellp();
    if (pos < 0)
        throw IOError(N_("could not get file position on output file"));
    coffsect->m_relptr = static_cast<unsigned long>(pos);

    // If >=64K relocs (for Win32/64), we set a flag in the section header
    // (NRELOC_OVFL) and the first relocation contains the number of relocs.
    if (sect.get_relocs().size() >= 64*1024)
    {
#if 0
        if (m_objfmt.is_win32())
        {
            coffsect->m_flags |= CoffSection::NRELOC_OVFL;
            Bytes& bytes = get_scratch();
            bytes << little_endian;
            write_32(bytes, sect.get_relocs().size()+1);// address (# relocs)
            write_32(bytes, 0);                         // relocated symbol
            write_16(bytes, 0);                         // type of relocation
            m_os << bytes;
        }
        else
#endif
        {
            warn_set(WARN_GENERAL,
                     String::compose(N_("too many relocations in section `%1'"),
                                     sect.get_name()));
            errwarns.propagate(0);
        }
    }

    for (Section::const_reloc_iterator i=sect.relocs_begin(),
         end=sect.relocs_end(); i != end; ++i)
    {
        const CoffReloc& reloc = static_cast<const CoffReloc&>(*i);
        Bytes& scratch = get_scratch();
        reloc.write(scratch);
        assert(scratch.size() == 10);
        m_os << scratch;
    }
}

unsigned long
Output::count_syms()
{
    unsigned long indx = 0;

    for (Object::symbol_iterator i = m_object.symbols_begin(),
         end = m_object.symbols_end(); i != end; ++i)
    {
        int vis = i->get_visibility();

        // Don't output local syms unless outputting all syms
        if (!m_all_syms && vis == Symbol::LOCAL && !i->is_abs())
            continue;

        CoffSymbol* coffsym = get_coff(*i);

        // Create basic coff symbol data if it doesn't already exist
        if (!coffsym)
        {
            if (vis & (Symbol::EXTERN|Symbol::GLOBAL|Symbol::COMMON))
                coffsym = new CoffSymbol(CoffSymbol::SCL_EXT);
            else
                coffsym = new CoffSymbol(CoffSymbol::SCL_STAT);
            i->add_assoc_data(CoffSymbol::key,
                              std::auto_ptr<AssocData>(coffsym));
        }
        coffsym->m_index = indx;

        indx += coffsym->m_aux.size() + 1;
    }

    return indx;
}

void
Output::output_symtab(Errwarns& errwarns)
{
    for (Object::const_symbol_iterator i = m_object.symbols_begin(),
         end = m_object.symbols_end(); i != end; ++i)
    {
        // Don't output local syms unless outputting all syms
        if (!m_all_syms && i->get_visibility() == Symbol::LOCAL && !i->is_abs())
            continue;

        // Get symrec data
        const CoffSymbol* coffsym = get_coff(*i);
        assert(coffsym != 0);

        Bytes& bytes = get_scratch();
        coffsym->write(bytes, *i, errwarns, m_strtab);
        m_os << bytes;
    }
}

void
Output::output_strtab()
{
    Bytes& bytes = get_scratch();
    bytes << little_endian;
    write_32(bytes, m_strtab.get_size()+4); // total length
    m_os << bytes;
    m_strtab.write(m_os);                   // strings
}

void
Output::output_secthead(const Section& sect)
{
    Bytes& bytes = get_scratch();
    const CoffSection* coffsect = get_coff(sect);
    coffsect->write(bytes, sect);
    m_os << bytes;
}

void
CoffObject::output(std::ostream& os, bool all_syms, Errwarns& errwarns)
{
    // Update file symbol filename
    m_file_coffsym->m_aux.resize(1);
    m_file_coffsym->m_aux[0].fname = m_object->get_source_fn();

    // Number sections and determine each section's addr values.
    // The latter is needed in VMA case before actually outputting
    // relocations, as a relocation's section address is added into the
    // addends in the generated code.
    unsigned int scnum = 1;
    unsigned long addr = 0;
    for (Object::section_iterator i=m_object->sections_begin(),
         end=m_object->sections_end(); i != end; ++i)
    {
        CoffSection* coffsect = get_coff(*i);
        coffsect->m_scnum = scnum++;

        if (coffsect->m_isdebug)
        {
            i->set_lma(0);
            i->set_vma(0);
        }
        else
        {
            i->set_lma(addr);
            if (m_set_vma)
                i->set_vma(addr);
            else
                i->set_vma(0);
            addr += i->bcs_last().next_offset();
        }
    }

    // Allocate space for headers by seeking forward.
    os.seekp(20+40*(scnum-1));
    if (!os)
        throw IOError(N_("could not seek on output file"));

    Output out(os, *this, *m_object, all_syms);

    // Finalize symbol table (assign index to each symbol).
    unsigned long symtab_count = out.count_syms();

    // Section data/relocs
    for (Object::section_iterator i=m_object->sections_begin(),
         end=m_object->sections_end(); i != end; ++i)
    {
        out.output_section(*i, errwarns);
    }

    // Symbol table
    long pos = os.tellp();
    if (pos < 0)
        throw IOError(N_("could not get file position on output file"));
    unsigned long symtab_pos = static_cast<unsigned long>(pos);
    out.output_symtab(errwarns);

    // String table
    out.output_strtab();

    // Write headers
    os.seekp(0);
    if (!os)
        throw IOError(N_("could not seek on output file"));

    // Write file header
    Bytes& bytes = out.get_scratch();
    bytes << little_endian;
    write_16(bytes, m_machine);         // magic number
    write_16(bytes, scnum-1);           // number of sects
    unsigned long ts;
    if (std::getenv("YASM_TEST_SUITE"))
        ts = 0;
    else
        ts = static_cast<unsigned long>(std::time(NULL));
    write_32(bytes, ts);                // time/date stamp
    write_32(bytes, symtab_pos);        // file ptr to symtab
    write_32(bytes, symtab_count);      // number of symtabs
    write_16(bytes, 0);                 // size of optional header (none)

    // flags
    unsigned int flags = 0;
#if 0
    if (String::nocase_equal(object->dbgfmt, "null"))
        flags |= F_LNNO;
#endif
    if (!all_syms)
        flags |= F_LSYMS;
    if (m_machine != MACHINE_AMD64)
        flags |= F_AR32WR;
    write_16(bytes, flags);
    os << bytes;

    // Section headers
    for (Object::section_iterator i=m_object->sections_begin(),
         end=m_object->sections_end(); i != end; ++i)
    {
        out.output_secthead(*i);
    }
}

CoffObject::~CoffObject()
{
}

Section*
CoffObject::add_default_section()
{
    Section* section = append_section(".text", 0);
    section->set_default(true);
    return section;
}

bool
CoffObject::init_section(const std::string& name,
                         Section& section,
                         CoffSection* coffsect)
{
    unsigned long flags = 0;

    if (name == ".data")
        flags = CoffSection::DATA;
    else if (name == ".bss")
    {
        flags = CoffSection::BSS;
        section.set_bss(true);
    }
    else if (name == ".text")
    {
        flags = CoffSection::TEXT;
        section.set_code(true);
    }
    else if (name == ".rdata"
             || (name.length() >= 7 && name.compare(0, 7, ".rodata") == 0)
             || (name.length() >= 7 && name.compare(0, 7, ".rdata$") == 0))
    {
        flags = CoffSection::DATA;
        warn_set(WARN_GENERAL,
                 N_("Standard COFF does not support read-only data sections"));
    }
    else if (name == ".drectve")
        flags = CoffSection::INFO;
    else if (name == ".comment")
        flags = CoffSection::INFO;
    else if (String::nocase_equal(name, ".debug", 6))
        flags = CoffSection::DATA;
    else
    {
        // Default to code (NASM default; note GAS has different default.
        flags = CoffSection::TEXT;
        section.set_code(true);
        return false;
    }
    coffsect->m_flags = flags;
    return true;
}

Section*
CoffObject::append_section(const std::string& name, unsigned long line)
{
    Section* section = new Section(name, false, false, line);
    m_object->append_section(std::auto_ptr<Section>(section));

    // Define a label for the start of the section
    Location start = {&section->bcs_first(), 0};
    SymbolRef sym = m_object->get_symbol(name);
    sym->define_label(start, line);
    sym->declare(Symbol::GLOBAL, line);
    sym->add_assoc_data(CoffSymbol::key,
        std::auto_ptr<AssocData>(new CoffSymbol(CoffSymbol::SCL_STAT,
                                                CoffSymbol::AUX_SECT)));

    // Add COFF data to the section
    CoffSection* coffsect = new CoffSection(sym);
    section->add_assoc_data(CoffSection::key,
                            std::auto_ptr<AssocData>(coffsect));
    init_section(name, *section, coffsect);

    return section;
}
#if 0
/* GAS-style flags */
static int
coff_helper_gasflags(void *obj, yasm_valparam *vp, unsigned long line, void *d,
                     /*@unused@*/ uintptr_t arg)
{
    struct coff_section_switch_data *data =
        (struct coff_section_switch_data *)d;
    int alloc = 0, load = 0, readonly = 0, code = 0, datasect = 0;
    int shared = 0;
    const char *s = yasm_vp_string(vp);
    size_t i;

    if (!s) {
        yasm_error_set(YASM_ERROR_VALUE, N_("non-string section attribute"));
        return -1;
    }

    // For GAS, default to read/write data
    if (data->isdefault)
        data->flags = COFF_STYP_TEXT | COFF_STYP_READ | COFF_STYP_WRITE;

    for (i=0; i<strlen(s); i++)
    {
        switch (s[i])
        {
            case 'a':
                break;
            case 'b':
                alloc = 1;
                load = 0;
                break;
            case 'n':
                load = 0;
                break;
            case 's':
                shared = 1;
                /*@fallthrough@*/
            case 'd':
                datasect = 1;
                load = 1;
                readonly = 0;
            case 'x':
                code = 1;
                load = 1;
                break;
            case 'r':
                datasect = 1;
                load = 1;
                readonly = 1;
                break;
            case 'w':
                readonly = 0;
                break;
            default:
                warn_set(WARN_GENERAL, String::compose(
                    N_("unrecognized section attribute: `%1'"), s[i]));
        }
    }

    if (code)
        data->flags = COFF_STYP_TEXT | COFF_STYP_EXECUTE | COFF_STYP_READ;
    else if (datasect)
        data->flags = COFF_STYP_DATA | COFF_STYP_READ | COFF_STYP_WRITE;
    else if (readonly)
        data->flags = COFF_STYP_DATA | COFF_STYP_READ;
    else if (load)
        data->flags = COFF_STYP_TEXT;
    else if (alloc)
        data->flags = COFF_STYP_BSS;

    if (shared)
        data->flags |= COFF_STYP_SHARED;

    data->gasflags = 1;
    return 0;
}
#endif
void
CoffObject::dir_section_init_helpers(DirHelpers& helpers,
                                     CoffSection* coffsect,
                                     IntNum* align,
                                     bool* has_align,
                                     unsigned long line)
{
    helpers.add("code", false,
                BIND::bind(&dir_flag_reset, _1, &coffsect->m_flags,
                           CoffSection::TEXT |
                           CoffSection::EXECUTE |
                           CoffSection::READ));
    helpers.add("text", false,
                BIND::bind(&dir_flag_reset, _1, &coffsect->m_flags,
                           CoffSection::TEXT |
                           CoffSection::EXECUTE |
                           CoffSection::READ));
    helpers.add("data", false,
                BIND::bind(&dir_flag_reset, _1, &coffsect->m_flags,
                           CoffSection::DATA |
                           CoffSection::READ |
                           CoffSection::WRITE));
    helpers.add("rdata", false,
                BIND::bind(&dir_flag_reset, _1, &coffsect->m_flags,
                           CoffSection::DATA | CoffSection::READ));
    helpers.add("bss", false,
                BIND::bind(&dir_flag_reset, _1, &coffsect->m_flags,
                           CoffSection::BSS |
                           CoffSection::READ |
                           CoffSection::WRITE));
    helpers.add("info", false,
                BIND::bind(&dir_flag_reset, _1, &coffsect->m_flags,
                           CoffSection::INFO |
                           CoffSection::DISCARD |
                           CoffSection::READ));
    helpers.add("align", true,
                BIND::bind(&dir_intn, _1, m_object, line, align, has_align));
}

void
CoffObject::dir_section(Object& object,
                        NameValues& nvs,
                        NameValues& objext_nvs,
                        unsigned long line)
{
    assert(&object == m_object);

    if (!nvs.front().is_string())
        throw Error(N_("section name must be a string"));
    std::string sectname = nvs.front().get_string();

    if (sectname.length() > 8 && !m_win32)
    {
        // win32 format supports >8 character section names in object
        // files via "/nnnn" (where nnnn is decimal offset into string table),
        // so only warn for regular COFF.
        warn_set(WARN_GENERAL,
                 N_("COFF section names limited to 8 characters: truncating"));
        sectname.resize(8);
    }

    Section* sect = m_object->find_section(sectname);
    bool first = true;
    if (sect)
        first = sect->is_default();
    else
        sect = append_section(sectname, line);

    CoffSection* coffsect = get_coff(*sect);
    assert(coffsect != 0);

    m_object->set_cur_section(sect);
    sect->set_default(false);

    // No name/values, so nothing more to do
    if (nvs.size() <= 1)
        return;

    // Ignore flags if we've seen this section before
    if (!first)
    {
        warn_set(WARN_GENERAL,
                 N_("section flags ignored on section redeclaration"));
        return;
    }

    // Parse section flags
    IntNum align;
    bool has_align = false;

    DirHelpers helpers;
    dir_section_init_helpers(helpers, coffsect, &align, &has_align, line);
    helpers(++nvs.begin(), nvs.end(), dir_nameval_warn);

    sect->set_bss((coffsect->m_flags & CoffSection::BSS) != 0);
    sect->set_code((coffsect->m_flags & CoffSection::EXECUTE) != 0);

    if (!m_win32)
        coffsect->m_flags &= ~CoffSection::WIN32_MASK;

    if (has_align)
    {
        unsigned long aligni = align.get_uint();

        // Alignments must be a power of two.
        if (!is_exp2(aligni))
        {
            throw ValueError(String::compose(
                N_("argument to `%1' is not a power of two"), "align"));
        }

        // Check to see if alignment is supported size
        if (aligni > 8192)
            throw ValueError(N_("Win32 does not support alignments > 8192"));

        sect->set_align(aligni);
    }
}

void
CoffObject::dir_ident(Object& object,
                      NameValues& namevals,
                      NameValues& objext_namevals,
                      unsigned long line)
{
    assert(m_object == &object);
    dir_ident_common(*this, ".comment", object, namevals, objext_namevals,
                     line);
}

void
CoffObject::add_directives(Directives& dirs, const std::string& parser)
{
    static const Directives::Init<CoffObject> nasm_dirs[] =
    {
        {"section", &CoffObject::dir_section, Directives::ARG_REQUIRED},
        {"segment", &CoffObject::dir_section, Directives::ARG_REQUIRED},
        {"ident",   &CoffObject::dir_ident, Directives::ANY},
    };
    static const Directives::Init<CoffObject> gas_dirs[] =
    {
        {".section", &CoffObject::dir_section, Directives::ARG_REQUIRED},
        {".ident",   &CoffObject::dir_ident, Directives::ANY},
    };

    if (String::nocase_equal(parser, "nasm"))
        dirs.add_array(this, nasm_dirs, NELEMS(nasm_dirs));
    else if (String::nocase_equal(parser, "gas"))
        dirs.add_array(this, gas_dirs, NELEMS(gas_dirs));
}

void
do_register()
{
    register_module<ObjectFormat, CoffObject>("coff");
}

}}} // namespace yasm::objfmt::coff

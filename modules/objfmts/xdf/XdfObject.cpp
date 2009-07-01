//
// Extended Dynamic Object format
//
//  Copyright (C) 2004-2007  Peter Johnson
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
#include <util.h>

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
#include <yasmx/ObjectFormat.h>
#include <yasmx/Section.h>
#include <yasmx/Symbol.h>

#include "XdfReloc.h"
#include "XdfSection.h"
#include "XdfSymbol.h"


namespace yasm
{
namespace objfmt
{
namespace xdf
{

static const unsigned long XDF_MAGIC = 0x87654322;
static const unsigned int FILEHEAD_SIZE = 16;
static const unsigned int SECTHEAD_SIZE = 40;
static const unsigned int SYMBOL_SIZE = 16;
static const unsigned int RELOC_SIZE = 16;

class XdfObject : public ObjectFormat
{
public:
    /// Constructor.
    /// To make object format truly usable, set_object()
    /// needs to be called.
    XdfObject(const ObjectFormatModule& module, Object& object)
        : ObjectFormat(module, object)
    {}

    /// Destructor.
    ~XdfObject() {}

    void add_directives(Directives& dirs, const char* parser);

    void read(std::istream& is);
    void output(std::ostream& os, bool all_syms, Errwarns& errwarns);

    Section* add_default_section();
    Section* append_section(const std::string& name, unsigned long line);

    static const char* get_name() { return "Extended Dynamic Object"; }
    static const char* get_keyword() { return "xdf"; }
    static const char* get_extension() { return ".xdf"; }
    static unsigned int get_default_x86_mode_bits() { return 32; }
    static const char* get_default_dbgfmt_keyword() { return "null"; }
    static std::vector<const char*> get_dbgfmt_keywords();
    static bool ok_object(Object& object);
    static bool taste(std::istream& is,
                      /*@out@*/ std::string* arch_keyword,
                      /*@out@*/ std::string* machine);

private:
    void dir_section(Object& object,
                     NameValues& namevals,
                     NameValues& objext_namevals,
                     unsigned long line);
};

bool
XdfObject::ok_object(Object& object)
{
    // Only support x86 arch
    if (!String::nocase_equal(object.get_arch()->get_module().get_keyword(),
                              "x86"))
        return false;

    // Support x86 and amd64 machines of x86 arch
    if (!String::nocase_equal(object.get_arch()->get_machine(), "x86") &&
        !String::nocase_equal(object.get_arch()->get_machine(), "amd64"))
    {
        return false;
    }

    return true;
}

std::vector<const char*>
XdfObject::get_dbgfmt_keywords()
{
    static const char* keywords[] = {"null"};
    return std::vector<const char*>(keywords, keywords+NELEMS(keywords));
}


class Output : public BytecodeStreamOutput
{
public:
    Output(std::ostream& os, Object& object);
    ~Output();

    void output_section(Section& sect, Errwarns& errwarns);
    void output_sym(const Symbol& sym,
                    Errwarns& errwarns,
                    bool all_syms,
                    unsigned long* strtab_offset);

    // OutputBytecode overrides
    void value_to_bytes(Value& value, Bytes& bytes, Location loc, int warn);

private:
    Object& m_object;
    BytecodeNoOutput m_no_output;
};

Output::Output(std::ostream& os, Object& object)
    : BytecodeStreamOutput(os)
    , m_object(object)
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

    if (value.is_section_rel())
        throw TooComplexError(N_("xdf: relocation too complex"));

    IntNum intn(0);
    if (value.is_relative())
    {
        bool pc_rel = false;
        IntNum intn2;
        if (value.calc_pcrel_sub(&intn2, loc))
        {
            // Create PC-relative relocation type and fix up absolute portion.
            pc_rel = true;
            intn += intn2;
        }
        else if (value.has_sub())
            throw TooComplexError(N_("xdf: relocation too complex"));

        std::auto_ptr<XdfReloc>
            reloc(new XdfReloc(loc.get_offset(), value, pc_rel));
        if (pc_rel)
            intn -= loc.get_offset();   // Adjust to start of section
        Section* sect = loc.bc->get_container()->as_section();
        sect->add_reloc(std::auto_ptr<Reloc>(reloc.release()));
    }

    if (Expr* abs = value.get_abs())
    {
        if (!abs->is_intnum())
            throw TooComplexError(N_("xdf: relocation too complex"));
        intn += abs->get_intnum();
    }

    m_object.get_arch()->tobytes(intn, bytes, value.get_size(), 0, warn);
}

void
Output::output_section(Section& sect, Errwarns& errwarns)
{
    BytecodeOutput* outputter = this;

    XdfSection* xsect = get_xdf(sect);
    assert(xsect != NULL);

    std::streampos pos;
    if (sect.is_bss())
    {
        // Don't output BSS sections.
        outputter = &m_no_output;
        pos = 0;    // position = 0 because it's not in the file
        xsect->size = sect.bcs_last().next_offset();
    }
    else
    {
        pos = m_os.tellp();
        if (pos < 0)
            throw Fatal(N_("could not get file position on output file"));
    }

    // Output bytecodes
    for (Section::bc_iterator i=sect.bcs_begin(), end=sect.bcs_end();
         i != end; ++i)
    {
        try
        {
            i->output(*outputter);
            xsect->size += i->get_total_len();
        }
        catch (Error& err)
        {
            errwarns.propagate(i->get_line(), err);
        }
        errwarns.propagate(i->get_line());  // propagate warnings
    }

    // Sanity check final section size
    assert(xsect->size == sect.bcs_last().next_offset());

    // Empty?  Go on to next section
    if (xsect->size == 0)
        return;

    sect.set_filepos(static_cast<unsigned long>(pos));

    // No relocations to output?  Go on to next section
    if (sect.get_relocs().size() == 0)
        return;

    pos = m_os.tellp();
    if (pos < 0)
        throw Fatal(N_("could not get file position on output file"));
    xsect->relptr = static_cast<unsigned long>(pos);

    for (Section::const_reloc_iterator i=sect.relocs_begin(),
         end=sect.relocs_end(); i != end; ++i)
    {
        const XdfReloc& reloc = static_cast<const XdfReloc&>(*i);
        Bytes& scratch = get_scratch();
        reloc.write(scratch);
        assert(scratch.size() == RELOC_SIZE);
        m_os << scratch;
    }
}

void
Output::output_sym(const Symbol& sym,
                   Errwarns& errwarns,
                   bool all_syms,
                   unsigned long* strtab_offset)
{
    int vis = sym.get_visibility();

    if (!all_syms && vis == Symbol::LOCAL)
        return;

    unsigned long flags = 0;

    if (vis & Symbol::GLOBAL)
        flags = XdfSymbol::XDF_GLOBAL;

    unsigned long value = 0;
    long scnum = -3;        // -3 = debugging symbol

    // Look at symrec for value/scnum/etc.
    Location loc;
    if (sym.get_label(&loc))
    {
        const Section* sect = 0;
        if (loc.bc)
            sect = loc.bc->get_container()->as_section();
        // it's a label: get value and offset.
        // If there is not a section, leave as debugging symbol.
        if (sect)
        {
            const XdfSection* xsect = get_xdf(*sect);
            assert(xsect != 0);
            scnum = xsect->scnum;
            value += loc.get_offset();
        }
    }
    else if (const Expr* equ_val = sym.get_equ())
    {
        Expr::Ptr equ_val_copy(equ_val->clone());
        equ_val_copy->simplify();
        if (!equ_val_copy->is_intnum())
        {
            if (vis & Symbol::GLOBAL)
            {
                errwarns.propagate(sym.get_def_line(),
                    NotConstantError(
                        N_("global EQU value not an integer expression")));
            }
        }
        else
            value = equ_val_copy->get_intnum().get_uint();

        flags |= XdfSymbol::XDF_EQU;
        scnum = -2;     // -2 = absolute symbol
    }
    else if (vis & Symbol::EXTERN)
    {
        flags = XdfSymbol::XDF_EXTERN;
        scnum = -1;
    }

    Bytes& scratch = get_scratch();
    scratch << little_endian;

    write_32(scratch, scnum);       // section number
    write_32(scratch, value);       // value
    write_32(scratch, *strtab_offset);
    write_32(scratch, flags);       // flags
    assert(scratch.size() == SYMBOL_SIZE);
    m_os << scratch;

    *strtab_offset += static_cast<unsigned long>(sym.get_name().length()+1);
}

void
XdfObject::output(std::ostream& os, bool all_syms, Errwarns& errwarns)
{
    all_syms = true;   // force all syms into symbol table

    // Get number of symbols and set symbol index in symbol data.
    unsigned long symtab_count = 0;
    for (Object::symbol_iterator sym = m_object.symbols_begin(),
         end = m_object.symbols_end(); sym != end; ++sym)
    {
        int vis = sym->get_visibility();
        if (vis & Symbol::COMMON)
        {
            errwarns.propagate(sym->get_decl_line(),
                Error(N_("XDF object format does not support common variables")));
            continue;
        }
        if (all_syms || (vis != Symbol::LOCAL && !(vis & Symbol::DLOCAL)))
        {
            // Save index in symrec data
            sym->add_assoc_data(XdfSymbol::key,
                std::auto_ptr<AssocData>(new XdfSymbol(symtab_count)));

            ++symtab_count;
        }
    }

    // Number sections
    long scnum = 0;
    for (Object::section_iterator i = m_object.sections_begin(),
         end = m_object.sections_end(); i != end; ++i)
    {
        XdfSection* xsect = get_xdf(*i);
        assert(xsect != 0);
        xsect->scnum = scnum++;
    }

    // Allocate space for headers by seeking forward
    os.seekp(FILEHEAD_SIZE+SECTHEAD_SIZE*scnum);
    if (!os)
        throw Fatal(N_("could not seek on output file"));

    Output out(os, m_object);

    // Get file offset of start of string table
    unsigned long strtab_offset =
        FILEHEAD_SIZE + SECTHEAD_SIZE*scnum + SYMBOL_SIZE*symtab_count;

    // Output symbol table
    for (Object::const_symbol_iterator sym = m_object.symbols_begin(),
         end = m_object.symbols_end(); sym != end; ++sym)
    {
        out.output_sym(*sym, errwarns, all_syms, &strtab_offset);
    }

    // Output string table
    for (Object::const_symbol_iterator sym = m_object.symbols_begin(),
         end = m_object.symbols_end(); sym != end; ++sym)
    {
        if (all_syms || sym->get_visibility() != Symbol::LOCAL)
        {
            const std::string& name = sym->get_name();
            os << name << '\0';
        }
    }

    // Output section data/relocs
    for (Object::section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        out.output_section(*i, errwarns);
    }

    // Write headers
    os.seekp(0);
    if (!os)
        throw Fatal(N_("could not seek on output file"));

    // Output object header
    Bytes& scratch = out.get_scratch();
    scratch << little_endian;
    write_32(scratch, XDF_MAGIC);       // magic number
    write_32(scratch, scnum);           // number of sects
    write_32(scratch, symtab_count);    // number of symtabs
    // size of sect headers + symbol table + strings
    write_32(scratch, strtab_offset-FILEHEAD_SIZE);
    assert(scratch.size() == FILEHEAD_SIZE);
    os << scratch;

    // Output section headers
    for (Object::const_section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        const XdfSection* xsect = get_xdf(*i);
        assert(xsect != 0);

        Bytes& scratch2 = out.get_scratch();
        xsect->write(scratch2, *i);
        assert(scratch2.size() == SECTHEAD_SIZE);
        os << scratch2;
    }
}

bool
XdfObject::taste(std::istream& is,
                 /*@out@*/ std::string* arch_keyword,
                 /*@out@*/ std::string* machine)
{
    Bytes bytes;

    // Check for XDF magic number in header
    is.seekg(0);
    bytes.write(is, FILEHEAD_SIZE);
    if (!is)
        return false;
    bytes << little_endian;
    unsigned long magic = read_u32(bytes);
    if (magic != XDF_MAGIC)
        return false;

    // all XDF files are x86/x86 or amd64 (can't tell which)
    arch_keyword->assign("x86");
    machine->assign("x86");
    return true;
}

void
XdfObject::read(std::istream& is)
{
    Bytes bytes;

    // Read object header
    is.seekg(0);
    bytes.write(is, FILEHEAD_SIZE);
    if (!is)
        throw Error(N_("could not read object header"));
    bytes << little_endian;
    unsigned long magic = read_u32(bytes);
    if (magic != XDF_MAGIC)
        throw Error(N_("not an XDF file"));
    unsigned long scnum = read_u32(bytes);
    unsigned long symnum = read_u32(bytes);
    unsigned long headers_len = read_u32(bytes);

    // Read section headers, symbol table, and strings raw data
    bytes.resize(0);
    bytes.set_readpos(0);
    bytes.write(is, scnum*SECTHEAD_SIZE);
    if (!is)
        throw Error(N_("could not read section table"));

    Bytes symtab;
    symtab.write(is, symnum*SYMBOL_SIZE);
    if (!is)
        throw Error(N_("could not read symbol table"));

    unsigned strtab_offset =
        FILEHEAD_SIZE + SECTHEAD_SIZE*scnum + SYMBOL_SIZE*symnum;
    Bytes strtab;
    strtab.write(is, FILEHEAD_SIZE+headers_len-strtab_offset);
    strtab.push_back(0);        // add extra 0 in case of data corruption
    if (!is)
        throw Error(N_("could not read string table"));

    // Storage for nrelocs, indexed by section number
    std::vector<unsigned long> sects_nrelocs;
    sects_nrelocs.reserve(scnum);

    // Create sections
    for (unsigned long i=0; i<scnum; ++i)
    {
        // Start with symbol=0 as it's not created yet; updated later.
        std::auto_ptr<XdfSection> xsect(new XdfSection(SymbolRef(0)));
        unsigned long name_sym_index;
        IntNum lma, vma;
        unsigned long align;
        bool bss;
        unsigned long filepos;
        unsigned long nrelocs;
        xsect->read(bytes, &name_sym_index, &lma, &vma, &align, &bss, &filepos,
                    &nrelocs);
        xsect->scnum = i;

        symtab.set_readpos(name_sym_index*SYMBOL_SIZE+8);   // strtab offset
        unsigned long name_strtab_off = read_u32(symtab) - strtab_offset;
        std::string sectname =
            reinterpret_cast<const char*>(&strtab.at(name_strtab_off));

        std::auto_ptr<Section> section(
            new Section(sectname, xsect->bits != 0, bss, 0));

        section->set_filepos(filepos);
        section->set_vma(vma);
        section->set_lma(lma);

        if (bss)
        {
            Bytecode& gap = section->append_gap(xsect->size, 0);
            gap.calc_len(0);    // force length calculation of gap
        }
        else
        {
            // Read section data
            is.seekg(filepos);
            if (!is)
                throw Error(String::compose(
                    N_("could not read seek to section `%1'"), sectname));

            section->bcs_first().get_fixed().write(is, xsect->size);
            if (!is)
                throw Error(String::compose(
                    N_("could not read section `%1' data"), sectname));
        }

        // Associate section data with section
        section->add_assoc_data(XdfSection::key,
                                std::auto_ptr<AssocData>(xsect.release()));

        // Add section to object
        m_object.append_section(section);

        sects_nrelocs.push_back(nrelocs);
    }

    // Create symbols
    symtab.set_readpos(0);
    symtab << little_endian;
    for (unsigned long i=0; i<symnum; ++i)
    {
        unsigned long sym_scnum = read_u32(symtab);     // section number
        unsigned long value = read_u32(symtab);         // value
        unsigned long name_strtab_off = read_u32(symtab) - strtab_offset;
        unsigned long flags = read_u32(symtab);         // flags

        std::string symname =
            reinterpret_cast<const char*>(&strtab.at(name_strtab_off));

        SymbolRef sym = m_object.get_symbol(symname);
        if ((flags & XdfSymbol::XDF_GLOBAL) != 0)
            sym->declare(Symbol::GLOBAL, 0);
        else if ((flags & XdfSymbol::XDF_EXTERN) != 0)
            sym->declare(Symbol::EXTERN, 0);

        if ((flags & XdfSymbol::XDF_EQU) != 0)
            sym->define_equ(Expr(value), 0);
        else if (sym_scnum < scnum)
        {
            Section& sect = m_object.get_section(sym_scnum);
            Location loc = {&sect.bcs_first(), value};
            sym->define_label(loc, 0);
        }

        // Save index in symrec data
        sym->add_assoc_data(XdfSymbol::key,
                            std::auto_ptr<AssocData>(new XdfSymbol(i)));
    }

    // Update section symbol info, and create section relocations
    Bytes relocs;
    std::vector<unsigned long>::iterator nrelocsi = sects_nrelocs.begin();
    for (Object::section_iterator sect=m_object.sections_begin(),
         end=m_object.sections_end(); sect != end; ++sect, ++nrelocsi)
    {
        XdfSection* xsect = get_xdf(*sect);
        assert(xsect != 0);

        // Read relocations
        is.seekg(xsect->relptr);
        if (!is)
            throw Error(String::compose(
                N_("could not read seek to section `%1' relocs"),
                sect->get_name()));

        relocs.resize(0);
        relocs.set_readpos(0);
        relocs.write(is, (*nrelocsi) * RELOC_SIZE);
        if (!is)
            throw Error(String::compose(
                N_("could not read section `%1' relocs"),
                sect->get_name()));

        relocs << little_endian;
        for (unsigned long i=0; i<(*nrelocsi); ++i)
        {
            unsigned long addr = read_u32(relocs);
            unsigned long sym_index = read_u32(relocs);
            unsigned long basesym_index = read_u32(relocs);
            XdfReloc::Type type = static_cast<XdfReloc::Type>(read_u8(relocs));
            XdfReloc::Size size = static_cast<XdfReloc::Size>(read_u8(relocs));
            unsigned char shift = read_u8(relocs);
            read_u8(relocs);    // flags; ignored
            SymbolRef sym = m_object.get_symbol(sym_index);
            SymbolRef basesym(0);
            if (type == XdfReloc::XDF_WRT)
                basesym = m_object.get_symbol(basesym_index);
            sect->add_reloc(std::auto_ptr<Reloc>(
                new XdfReloc(addr, sym, basesym, type, size, shift)));
        }
    }
}

Section*
XdfObject::add_default_section()
{
    Section* section = append_section(".text", 0);
    section->set_default(true);
    return section;
}

Section*
XdfObject::append_section(const std::string& name, unsigned long line)
{
    bool code = (name == ".text");
    Section* section = new Section(name, code, false, line);
    m_object.append_section(std::auto_ptr<Section>(section));

    // Define a label for the start of the section
    Location start = {&section->bcs_first(), 0};
    SymbolRef sym = m_object.get_symbol(name);
    sym->define_label(start, line);

    // Add XDF data to the section
    section->add_assoc_data(XdfSection::key,
                            std::auto_ptr<AssocData>(new XdfSection(sym)));

    return section;
}

void
XdfObject::dir_section(Object& object,
                       NameValues& nvs,
                       NameValues& objext_nvs,
                       unsigned long line)
{
    assert(&object == &m_object);

    if (!nvs.front().is_string())
        throw Error(N_("section name must be a string"));
    std::string sectname = nvs.front().get_string();

    Section* sect = m_object.find_section(sectname);
    bool first = true;
    if (sect)
        first = sect->is_default();
    else
        sect = append_section(sectname, line);

    XdfSection* xsect = get_xdf(*sect);
    assert(xsect != 0);

    m_object.set_cur_section(sect);
    sect->set_default(false);
    m_object.get_arch()->set_var("mode_bits", xsect->bits);     // reapply

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

    unsigned long bss = sect->is_bss();
    unsigned long code = sect->is_code();
    IntNum vma = sect->get_vma();
    IntNum lma = sect->get_lma();
    unsigned long flat = xsect->flat;


    DirHelpers helpers;
    helpers.add("use16", false,
                BIND::bind(&dir_flag_reset, _1, &xsect->bits, 16));
    helpers.add("use32", false,
                BIND::bind(&dir_flag_reset, _1, &xsect->bits, 32));
    helpers.add("use64", false,
                BIND::bind(&dir_flag_reset, _1, &xsect->bits, 64));
    helpers.add("bss", false, BIND::bind(&dir_flag_set, _1, &bss, 1));
    helpers.add("nobss", false, BIND::bind(&dir_flag_clear, _1, &bss, 1));
    helpers.add("code", false, BIND::bind(&dir_flag_set, _1, &code, 1));
    helpers.add("data", false, BIND::bind(&dir_flag_clear, _1, &code, 1));
    helpers.add("flat", false, BIND::bind(&dir_flag_set, _1, &flat, 1));
    helpers.add("noflat", false, BIND::bind(&dir_flag_clear, _1, &flat, 1));
    helpers.add("absolute", true,
                BIND::bind(&dir_intn, _1, &m_object, line, &lma,
                           &xsect->has_addr));
    helpers.add("virtual", true,
                BIND::bind(&dir_intn, _1, &m_object, line, &vma,
                           &xsect->has_vaddr));
    helpers.add("align", true,
                BIND::bind(&dir_intn, _1, &m_object, line, &align, &has_align));

    helpers(++nvs.begin(), nvs.end(), dir_nameval_warn);

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
        if (aligni > 4096)
            throw ValueError(N_("XDF does not support alignments > 4096"));

        sect->set_align(aligni);
    }

    sect->set_bss(bss);
    sect->set_code(code);
    sect->set_vma(vma);
    sect->set_lma(lma);
    xsect->flat = flat;
    m_object.get_arch()->set_var("mode_bits", xsect->bits);
}

void
XdfObject::add_directives(Directives& dirs, const char* parser)
{
    static const Directives::Init<XdfObject> nasm_dirs[] =
    {
        {"section", &XdfObject::dir_section, Directives::ARG_REQUIRED},
        {"segment", &XdfObject::dir_section, Directives::ARG_REQUIRED},
    };

    if (String::nocase_equal(parser, "nasm"))
        dirs.add_array(this, nasm_dirs, NELEMS(nasm_dirs));
}

void
do_register()
{
    register_module<ObjectFormatModule,
                    ObjectFormatModuleImpl<XdfObject> >("xdf");
}

}}} // namespace yasm::objfmt::xdf

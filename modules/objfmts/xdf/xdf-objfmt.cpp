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

#include <fstream>

#include <libyasmx/arch.h>
#include <libyasmx/bc_output.h>
#include <libyasmx/bitcount.h>
#include <libyasmx/bytecode.h>
#include <libyasmx/bytes.h>
#include <libyasmx/bytes_util.h>
#include <libyasmx/compose.h>
#include <libyasmx/directive.h>
#include <libyasmx/errwarn.h>
#include <libyasmx/errwarns.h>
#include <libyasmx/intnum.h>
#include <libyasmx/marg_ostream.h>
#include <libyasmx/name_value.h>
#include <libyasmx/nocase.h>
#include <libyasmx/object.h>
#include <libyasmx/object_format.h>
#include <libyasmx/registry.h>
#include <libyasmx/reloc.h>
#include <libyasmx/section.h>
#include <libyasmx/symbol.h>


namespace yasm
{
namespace objfmt
{
namespace xdf
{

static const unsigned long XDF_MAGIC = 0x87654322;

struct XdfSymbolData : public AssocData
{
    static const char* key;

    XdfSymbolData(unsigned long index_) : index(index_) {}
    ~XdfSymbolData() {}
    void put(marg_ostream& os) const;

    enum Flags
    {
        XDF_EXTERN = 1,
        XDF_GLOBAL = 2,
        XDF_EQU    = 4
    };

    unsigned long index;                //< assigned XDF symbol table index
};

const char* XdfSymbolData::key = "objfmt::xdf::XdfSymbolData";

void
XdfSymbolData::put(marg_ostream& os) const
{
    os << "symtab index=" << index << '\n';
}

inline XdfSymbolData*
get_xdf(Symbol& sym)
{
    return static_cast<XdfSymbolData*>
        (sym.get_assoc_data(XdfSymbolData::key));
}

inline XdfSymbolData*
get_xdf(SymbolRef sym)
{
    return static_cast<XdfSymbolData*>
        (sym->get_assoc_data(XdfSymbolData::key));
}


struct XdfSectionData : public AssocData
{
    static const char* key;

    XdfSectionData(SymbolRef sym_);
    ~XdfSectionData() {}
    void put(marg_ostream& os) const;

    void write(Bytes& bytes, const Section& sect) const;

    // Section flags.
    // Not used directly in this class, but used in file representation.
    enum Flags
    {
        XDF_ABSOLUTE = 0x01,
        XDF_FLAT = 0x02,
        XDF_BSS = 0x04,
        XDF_EQU = 0x08,             // unused
        XDF_USE_16 = 0x10,
        XDF_USE_32 = 0x20,
        XDF_USE_64 = 0x40
    };

    SymbolRef sym;          //< symbol created for this section

    bool has_addr;          //< absolute address set by user?
    bool has_vaddr;         //< virtual address set by user?

    long scnum;             //< section number (0=first section)

    bool flat;              //< specified by user as "flat" section
    unsigned long bits;     //< "bits" (aka use16/use32/use64) of section

    unsigned long size;     //< size of raw data (section data) in bytes
    unsigned long relptr;   //< file ptr to relocation
};

const char* XdfSectionData::key = "objfmt::xdf::XdfSectionData";

XdfSectionData::XdfSectionData(SymbolRef sym_)
    : sym(sym_)
    , has_addr(false)
    , has_vaddr(false)
    , scnum(0)
    , flat(false)
    , bits(0)
    , size(0)
    , relptr(0)
{
}

void
XdfSectionData::put(marg_ostream& os) const
{
    os << "sym=\n";
    ++os;
    os << *sym;
    --os;
    os << "has_addr=" << has_addr << '\n';
    os << "has_vaddr=" << has_vaddr << '\n';
    os << "scnum=" << scnum << '\n';
    os << "flat=" << flat << '\n';
    os << "bits=" << bits << '\n';
    os << "size=" << size << '\n';
    os << "relptr=0x" << std::hex << relptr << std::dec << '\n';
}

void
XdfSectionData::write(Bytes& bytes, const Section& sect) const
{
    const XdfSymbolData* xsymd = get_xdf(sym);
    assert(xsymd != 0);

    bytes << little_endian;

    write_32(bytes, xsymd->index);      // section name symbol
    write_64(bytes, sect.get_lma());    // physical address

    if (has_vaddr)
        write_64(bytes, sect.get_vma());// virtual address
    else
        write_64(bytes, sect.get_lma());// virt=phys if unspecified

    write_16(bytes, sect.get_align());  // alignment

    // flags
    unsigned long flags = 0;
    if (has_addr)
        flags |= XDF_ABSOLUTE;
    if (flat)
        flags |= XDF_FLAT;
    if (sect.is_bss())
        flags |= XDF_BSS;
    switch (bits)
    {
        case 16: flags |= XDF_USE_16; break;
        case 32: flags |= XDF_USE_32; break;
        case 64: flags |= XDF_USE_64; break;
    }
    write_16(bytes, flags);

    write_32(bytes, sect.get_filepos());// file ptr to data
    write_32(bytes, size);              // section size
    write_32(bytes, relptr);            // file ptr to relocs
    write_32(bytes, sect.get_relocs().size());// num of relocation entries
}

inline XdfSectionData*
get_xdf(Section& sect)
{
    return static_cast<XdfSectionData*>
        (sect.get_assoc_data(XdfSectionData::key));
}

inline const XdfSectionData*
get_xdf(const Section& sect)
{
    return static_cast<const XdfSectionData*>
        (sect.get_assoc_data(XdfSectionData::key));
}


class XdfReloc : public Reloc
{
public:
    enum Type
    {
        XDF_REL = 1,        //< relative to segment
        XDF_WRT = 2,        //< relative to symbol
        XDF_RIP = 4,        //< RIP-relative
        XDF_SEG = 8         //< segment containing symbol
    };

    XdfReloc(Value& value, Location loc);
    ~XdfReloc() {}

    std::auto_ptr<Expr> get_value() const;
    std::string get_type_name() const;

    Type get_type() const { return m_type; }

    void write(Bytes& bytes) const;

private:
    SymbolRef m_base;       //< base symbol (for WRT)
    Type m_type;            //< type of relocation
    enum Size
    {
        XDF_8  = 1,
        XDF_16 = 2,
        XDF_32 = 4,
        XDF_64 = 8
    };
    Size m_size;            //< size of relocation
    unsigned int m_shift;   //< relocation shift (0,4,8,16,24,32)
};

inline
XdfReloc::XdfReloc(Value& value, Location loc)
    : Reloc(loc.get_offset(), value.m_rel)
    , m_base(0)
    , m_size(static_cast<Size>(value.m_size/8))
    , m_shift(value.m_rshift)
{
    if (value.m_seg_of)
        m_type = XDF_SEG;
    else if (value.m_wrt)
    {
        m_base = value.m_wrt;
        m_type = XDF_WRT;
    }
    else if (value.m_curpos_rel)
        m_type = XDF_RIP;
    else
        m_type = XDF_REL;
}

std::auto_ptr<Expr>
XdfReloc::get_value() const
{
    std::auto_ptr<Expr> e;
    if (m_type == XDF_WRT)
        e.reset(new Expr(m_sym, Op::WRT, m_base, 0));
    else
        e.reset(new Expr(m_sym, 0));

    if (m_shift > 0)
        e.reset(new Expr(e, Op::SHR, IntNum(m_shift), 0));

    return e;
}

std::string
XdfReloc::get_type_name() const
{
    std::string s;

    switch (m_type)
    {
        case XDF_REL: s += "REL_"; break;
        case XDF_WRT: s += "WRT_"; break;
        case XDF_RIP: s += "RIP_"; break;
        case XDF_SEG: s += "SEG_"; break;
    }

    switch (m_size)
    {
        case XDF_8: s += "8"; break;
        case XDF_16: s += "16"; break;
        case XDF_32: s += "32"; break;
        case XDF_64: s += "64"; break;
    }

    return s;
}

void
XdfReloc::write(Bytes& bytes) const
{
    const XdfSymbolData* xsymd = get_xdf(m_sym);
    assert(xsymd != 0);     // need symbol data for relocated symbol

    bytes << little_endian;

    write_32(bytes, m_addr);
    write_32(bytes, xsymd->index);      // relocated symbol

    if (m_base)
    {
        xsymd = get_xdf(m_base);
        assert(xsymd != 0); // need symbol data for relocated base symbol
        write_32(bytes, xsymd->index);  // base symbol
    }
    else
    {
        // need base symbol for WRT relocation
        assert(m_type != XDF_WRT);
        write_32(bytes, 0);             // no base symbol
    }

    write_8(bytes, m_type);             // type of relocation
    write_8(bytes, m_size);             // size of relocation
    write_8(bytes, m_shift);            // relocation shift
    write_8(bytes, 0);                  // flags
}


class XdfObject : public ObjectFormat
{
public:
    /// Constructor.
    /// To make object format truly usable, set_object()
    /// needs to be called.
    XdfObject() {}

    /// Destructor.
    ~XdfObject() {}

    std::string get_name() const { return "Extended Dynamic Object"; }
    std::string get_keyword() const { return "xdf"; }
    void add_directives(Directives& dirs, const std::string& parser);

    bool ok_object(Object* object) const;
    std::string get_extension() const { return ".xdf"; }
    unsigned int get_default_x86_mode_bits() const { return 32; }

    std::vector<std::string> get_dbgfmt_keywords() const;
    std::string get_default_dbgfmt_keyword() const { return "null"; }

    void output(std::ostream& os, bool all_syms, Errwarns& errwarns);

    Section* add_default_section();
    Section* append_section(const std::string& name, unsigned long line);

private:
    void dir_section(Object& object,
                     NameValues& namevals,
                     NameValues& objext_namevals,
                     unsigned long line);
};

bool
XdfObject::ok_object(Object* object) const
{
    // Only support x86 arch
    if (!String::nocase_equal(object->get_arch()->get_keyword(), "x86"))
        return false;

    // Support x86 and amd64 machines of x86 arch
    if (!String::nocase_equal(object->get_arch()->get_machine(), "x86") &&
        !String::nocase_equal(object->get_arch()->get_machine(), "amd64"))
    {
        return false;
    }

    return true;
}

std::vector<std::string>
XdfObject::get_dbgfmt_keywords() const
{
    static const char* keywords[] = {"null"};
    return std::vector<std::string>(keywords, keywords+NELEMS(keywords));
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
    using BytecodeStreamOutput::output;
    void output(Value& value, Bytes& bytes, Location loc, int warn);

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
Output::output(Value& value, Bytes& bytes, Location loc, int warn)
{
    if (Expr* abs = value.get_abs())
        abs->simplify();

    // Try to output constant and PC-relative section-local first.
    // Note this does NOT output any value with a SEG, WRT, external,
    // cross-section, or non-PC-relative reference (those are handled below).
    if (value.output_basic(bytes, loc, warn, *m_object.get_arch()))
    {
        m_os << bytes;
        return;
    }

    if (value.m_section_rel)
        throw TooComplexError(N_("xdf: relocation too complex"));

    IntNum intn(0);
    if (value.is_relative())
    {
        std::auto_ptr<XdfReloc> reloc(new XdfReloc(value, loc));
        if (reloc->get_type() == XdfReloc::XDF_RIP)
            intn -= loc.get_offset();   // Adjust to start of section
        Section* sect = loc.bc->get_container()->as_section();
        sect->add_reloc(std::auto_ptr<Reloc>(reloc.release()));
    }

    if (Expr* abs = value.get_abs())
    {
        IntNum* intn2 = abs->get_intnum();
        if (!intn2)
            throw TooComplexError(N_("xdf: relocation too complex"));
        intn += *intn2;
    }

    m_object.get_arch()->tobytes(intn, bytes, value.m_size, 0, warn);
    m_os << bytes;
}

void
Output::output_section(Section& sect, Errwarns& errwarns)
{
    BytecodeOutput* outputter = this;

    XdfSectionData* xsd = get_xdf(sect);
    assert(xsd != NULL);

    std::streampos pos;
    if (sect.is_bss())
    {
        // Don't output BSS sections.
        outputter = &m_no_output;
        pos = 0;    // position = 0 because it's not in the file
        xsd->size = sect.bcs_last().next_offset();
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
            xsd->size += i->get_total_len();
        }
        catch (Error& err)
        {
            errwarns.propagate(i->get_line(), err);
        }
        errwarns.propagate(i->get_line());  // propagate warnings
    }

    // Sanity check final section size
    assert(xsd->size == sect.bcs_last().next_offset());

    // Empty?  Go on to next section
    if (xsd->size == 0)
        return;

    sect.set_filepos(static_cast<unsigned long>(pos));

    // No relocations to output?  Go on to next section
    if (sect.get_relocs().size() == 0)
        return;

    pos = m_os.tellp();
    if (pos < 0)
        throw Fatal(N_("could not get file position on output file"));
    xsd->relptr = static_cast<unsigned long>(pos);

    for (Section::const_reloc_iterator i=sect.relocs_begin(),
         end=sect.relocs_end(); i != end; ++i)
    {
        const XdfReloc& reloc = static_cast<const XdfReloc&>(*i);
        Bytes& scratch = get_scratch();
        reloc.write(scratch);
        assert(scratch.size() == 16);
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
        flags = XdfSymbolData::XDF_GLOBAL;

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
            const XdfSectionData* csectd = get_xdf(*sect);
            assert(csectd != 0);
            scnum = csectd->scnum;
            value += loc.get_offset();
        }
    }
    else if (const Expr* equ_val = sym.get_equ())
    {
        Expr::Ptr equ_val_copy(equ_val->clone());
        equ_val_copy->simplify();
        const IntNum* intn = equ_val_copy->get_intnum();
        if (!intn)
        {
            if (vis & Symbol::GLOBAL)
            {
                errwarns.propagate(equ_val->get_line(),
                    NotConstantError(
                        N_("global EQU value not an integer expression")));
            }
        }
        else
            value = intn->get_uint();

        flags |= XdfSymbolData::XDF_EQU;
        scnum = -2;     // -2 = absolute symbol
    }
    else if (vis & Symbol::EXTERN)
    {
        flags = XdfSymbolData::XDF_EXTERN;
        scnum = -1;
    }

    Bytes& scratch = get_scratch();
    scratch << little_endian;

    write_32(scratch, scnum);       // section number
    write_32(scratch, value);       // value
    write_32(scratch, *strtab_offset);
    write_32(scratch, flags);       // flags
    assert(scratch.size() == 16);
    m_os << scratch;

    *strtab_offset += static_cast<unsigned long>(sym.get_name().length()+1);
}

void
XdfObject::output(std::ostream& os, bool all_syms, Errwarns& errwarns)
{
    all_syms = true;   // force all syms into symbol table

    // Get number of symbols and set symbol index in symbol data.
    unsigned long symtab_count = 0;
    for (Object::symbol_iterator sym = m_object->symbols_begin(),
         end = m_object->symbols_end(); sym != end; ++sym)
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
            sym->add_assoc_data(XdfSymbolData::key,
                std::auto_ptr<AssocData>(new XdfSymbolData(symtab_count)));

            ++symtab_count;
        }
    }

    // Number sections
    long scnum = 0;
    for (Object::section_iterator i = m_object->sections_begin(),
         end = m_object->sections_end(); i != end; ++i)
    {
        XdfSectionData* xsd = get_xdf(*i);
        assert(xsd != 0);
        xsd->scnum = scnum++;
    }

    // Allocate space for headers by seeking forward
    os.seekp(16+40*scnum);
    if (!os)
        throw Fatal(N_("could not seek on output file"));

    Output out(os, *m_object);

    // Get file offset of start of string table
    unsigned long strtab_offset = 16+40*scnum+16*symtab_count;

    // Output symbol table
    for (Object::const_symbol_iterator sym = m_object->symbols_begin(),
         end = m_object->symbols_end(); sym != end; ++sym)
    {
        out.output_sym(*sym, errwarns, all_syms, &strtab_offset);
    }

    // Output string table
    for (Object::const_symbol_iterator sym = m_object->symbols_begin(),
         end = m_object->symbols_end(); sym != end; ++sym)
    {
        if (all_syms || sym->get_visibility() != Symbol::LOCAL)
        {
            const std::string& name = sym->get_name();
            os << name << '\0';
        }
    }

    // Output section data/relocs
    for (Object::section_iterator i=m_object->sections_begin(),
         end=m_object->sections_end(); i != end; ++i)
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
    write_32(scratch, strtab_offset-16);
    assert(scratch.size() == 16);
    os << scratch;

    // Output section headers
    for (Object::const_section_iterator i=m_object->sections_begin(),
         end=m_object->sections_end(); i != end; ++i)
    {
        const XdfSectionData* xsd = get_xdf(*i);
        assert(xsd != 0);

        Bytes& scratch2 = out.get_scratch();
        xsd->write(scratch2, *i);
        assert(scratch2.size() == 40);
        os << scratch2;
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
    m_object->append_section(std::auto_ptr<Section>(section));

    // Define a label for the start of the section
    Location start = {&section->bcs_first(), 0};
    SymbolRef sym = m_object->get_sym(name);
    sym->define_label(start, line);

    // Add XDF data to the section
    section->add_assoc_data(XdfSectionData::key,
                            std::auto_ptr<AssocData>(new XdfSectionData(sym)));

    return section;
}

void
XdfObject::dir_section(Object& object,
                       NameValues& nvs,
                       NameValues& objext_nvs,
                       unsigned long line)
{
    assert(&object == m_object);

    if (!nvs.front().is_string())
        throw Error(N_("section name must be a string"));
    std::string sectname = nvs.front().get_string();

    Section* sect = m_object->find_section(sectname);
    bool first = true;
    if (sect)
        first = sect->is_default();
    else
        sect = append_section(sectname, line);

    XdfSectionData* xsd = get_xdf(*sect);
    assert(xsd != 0);

    m_object->set_cur_section(sect);
    sect->set_default(false);
    m_object->get_arch()->set_var("mode_bits", xsd->bits);  // reapply

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
    unsigned long flat = xsd->flat;


    DirHelpers helpers;
    helpers.add("use16", false,
                BIND::bind(&dir_flag_reset, _1, &xsd->bits, 16));
    helpers.add("use32", false,
                BIND::bind(&dir_flag_reset, _1, &xsd->bits, 32));
    helpers.add("use64", false,
                BIND::bind(&dir_flag_reset, _1, &xsd->bits, 64));
    helpers.add("bss", false, BIND::bind(&dir_flag_set, _1, &bss, 1));
    helpers.add("nobss", false, BIND::bind(&dir_flag_clear, _1, &bss, 1));
    helpers.add("code", false, BIND::bind(&dir_flag_set, _1, &code, 1));
    helpers.add("data", false, BIND::bind(&dir_flag_clear, _1, &code, 1));
    helpers.add("flat", false, BIND::bind(&dir_flag_set, _1, &flat, 1));
    helpers.add("noflat", false, BIND::bind(&dir_flag_clear, _1, &flat, 1));
    helpers.add("absolute", true,
                BIND::bind(&dir_intn, _1, m_object, line, &lma,
                           &xsd->has_addr));
    helpers.add("virtual", true,
                BIND::bind(&dir_intn, _1, m_object, line, &vma,
                           &xsd->has_vaddr));
    helpers.add("align", true,
                BIND::bind(&dir_intn, _1, m_object, line, &align, &has_align));

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
    xsd->flat = flat;
    m_object->get_arch()->set_var("mode_bits", xsd->bits);
}

void
XdfObject::add_directives(Directives& dirs, const std::string& parser)
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
    register_module<ObjectFormat, XdfObject>("xdf");
}

}}} // namespace yasm::objfmt::xdf

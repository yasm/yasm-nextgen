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
#include "util.h"

#include "yasmx/Support/bitcount.h"
#include "yasmx/Support/Compose.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/nocase.h"
#include "yasmx/Support/registry.h"
#include "yasmx/Arch.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/Directive.h"
#include "yasmx/DirHelpers.h"
#include "yasmx/Errwarns.h"
#include "yasmx/InputBuffer.h"
#include "yasmx/IntNum.h"
#include "yasmx/Location_util.h"
#include "yasmx/NameValue.h"
#include "yasmx/Object.h"
#include "yasmx/ObjectFormat.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"

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

    void AddDirectives(Directives& dirs, const llvm::StringRef& parser);

    void Read(const llvm::MemoryBuffer& in);
    void Output(llvm::raw_fd_ostream& os, bool all_syms, Errwarns& errwarns);

    Section* AddDefaultSection();
    Section* AppendSection(const llvm::StringRef& name, unsigned long line);

    static const char* getName() { return "Extended Dynamic Object"; }
    static const char* getKeyword() { return "xdf"; }
    static const char* getExtension() { return ".xdf"; }
    static unsigned int getDefaultX86ModeBits() { return 32; }
    static const char* getDefaultDebugFormatKeyword() { return "null"; }
    static std::vector<const char*> getDebugFormatKeywords();
    static bool isOkObject(Object& object);
    static bool Taste(const llvm::MemoryBuffer& in,
                      /*@out@*/ std::string* arch_keyword,
                      /*@out@*/ std::string* machine);

private:
    void DirSection(DirectiveInfo& info);
};

bool
XdfObject::isOkObject(Object& object)
{
    // Only support x86 arch
    if (!String::NocaseEqual(object.getArch()->getModule().getKeyword(), "x86"))
        return false;

    // Support x86 and amd64 machines of x86 arch
    if (!String::NocaseEqual(object.getArch()->getMachine(), "x86") &&
        !String::NocaseEqual(object.getArch()->getMachine(), "amd64"))
    {
        return false;
    }

    return true;
}

std::vector<const char*>
XdfObject::getDebugFormatKeywords()
{
    static const char* keywords[] = {"null"};
    return std::vector<const char*>(keywords, keywords+NELEMS(keywords));
}


class XdfOutput : public BytecodeStreamOutput
{
public:
    XdfOutput(llvm::raw_ostream& os, Object& object);
    ~XdfOutput();

    void OutputSection(Section& sect, Errwarns& errwarns);
    void OutputSymbol(const Symbol& sym,
                      Errwarns& errwarns,
                      bool all_syms,
                      unsigned long* strtab_offset);

    // OutputBytecode overrides
    void ConvertValueToBytes(Value& value,
                             Bytes& bytes,
                             Location loc,
                             int warn);

private:
    Object& m_object;
    BytecodeNoOutput m_no_output;
};

XdfOutput::XdfOutput(llvm::raw_ostream& os, Object& object)
    : BytecodeStreamOutput(os)
    , m_object(object)
{
}

XdfOutput::~XdfOutput()
{
}

void
XdfOutput::ConvertValueToBytes(Value& value,
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

    if (value.isSectionRelative())
        throw TooComplexError(N_("xdf: relocation too complex"));

    IntNum intn(0);
    if (value.isRelative())
    {
        bool pc_rel = false;
        IntNum intn2;
        if (value.CalcPCRelSub(&intn2, loc))
        {
            // Create PC-relative relocation type and fix up absolute portion.
            pc_rel = true;
            intn += intn2;
        }
        else if (value.hasSubRelative())
            throw TooComplexError(N_("xdf: relocation too complex"));

        std::auto_ptr<XdfReloc>
            reloc(new XdfReloc(loc.getOffset(), value, pc_rel));
        if (pc_rel)
            intn -= loc.getOffset();    // Adjust to start of section
        Section* sect = loc.bc->getContainer()->AsSection();
        sect->AddReloc(std::auto_ptr<Reloc>(reloc.release()));
    }

    if (Expr* abs = value.getAbs())
    {
        if (!abs->isIntNum())
            throw TooComplexError(N_("xdf: relocation too complex"));
        intn += abs->getIntNum();
    }

    m_object.getArch()->ToBytes(intn, bytes, value.getSize(), 0, warn);
}

void
XdfOutput::OutputSection(Section& sect, Errwarns& errwarns)
{
    BytecodeOutput* outputter = this;

    XdfSection* xsect = sect.getAssocData<XdfSection>();
    assert(xsect != NULL);

    uint64_t pos;
    if (sect.isBSS())
    {
        // Don't output BSS sections.
        outputter = &m_no_output;
        pos = 0;    // position = 0 because it's not in the file
        xsect->size = sect.bytecodes_last().getNextOffset();
    }
    else
    {
        pos = m_os.tell();
        if (m_os.has_error())
            throw Fatal(N_("could not get file position on output file"));
    }

    // Output bytecodes
    for (Section::bc_iterator i=sect.bytecodes_begin(),
         end=sect.bytecodes_end(); i != end; ++i)
    {
        try
        {
            i->Output(*outputter);
            xsect->size += i->getTotalLen();
        }
        catch (Error& err)
        {
            errwarns.Propagate(i->getLine(), err);
        }
        errwarns.Propagate(i->getLine());   // propagate warnings
    }

    // Sanity check final section size
    assert(xsect->size == sect.bytecodes_last().getNextOffset());

    // Empty?  Go on to next section
    if (xsect->size == 0)
        return;

    sect.setFilePos(static_cast<unsigned long>(pos));

    // No relocations to output?  Go on to next section
    if (sect.getRelocs().size() == 0)
        return;

    pos = m_os.tell();
    if (m_os.has_error())
        throw Fatal(N_("could not get file position on output file"));
    xsect->relptr = static_cast<unsigned long>(pos);

    for (Section::const_reloc_iterator i=sect.relocs_begin(),
         end=sect.relocs_end(); i != end; ++i)
    {
        const XdfReloc& reloc = static_cast<const XdfReloc&>(*i);
        Bytes& scratch = getScratch();
        reloc.Write(scratch);
        assert(scratch.size() == RELOC_SIZE);
        m_os << scratch;
    }
}

void
XdfOutput::OutputSymbol(const Symbol& sym,
                        Errwarns& errwarns,
                        bool all_syms,
                        unsigned long* strtab_offset)
{
    int vis = sym.getVisibility();

    if (!all_syms && vis == Symbol::LOCAL)
        return;

    unsigned long flags = 0;

    if (vis & Symbol::GLOBAL)
        flags = XdfSymbol::XDF_GLOBAL;

    unsigned long value = 0;
    long scnum = -3;        // -3 = debugging symbol

    // Look at symrec for value/scnum/etc.
    Location loc;
    if (sym.getLabel(&loc))
    {
        const Section* sect = 0;
        if (loc.bc)
            sect = loc.bc->getContainer()->AsSection();
        // it's a label: get value and offset.
        // If there is not a section, leave as debugging symbol.
        if (sect)
        {
            const XdfSection* xsect = sect->getAssocData<XdfSection>();
            assert(xsect != 0);
            scnum = xsect->scnum;
            value += loc.getOffset();
        }
    }
    else if (const Expr* equ_val = sym.getEqu())
    {
        Expr::Ptr equ_val_copy(equ_val->clone());
        equ_val_copy->Simplify();
        if (!equ_val_copy->isIntNum())
        {
            if (vis & Symbol::GLOBAL)
            {
                errwarns.Propagate(sym.getDefLine(),
                    NotConstantError(
                        N_("global EQU value not an integer expression")));
            }
        }
        else
            value = equ_val_copy->getIntNum().getUInt();

        flags |= XdfSymbol::XDF_EQU;
        scnum = -2;     // -2 = absolute symbol
    }
    else if (vis & Symbol::EXTERN)
    {
        flags = XdfSymbol::XDF_EXTERN;
        scnum = -1;
    }

    Bytes& scratch = getScratch();
    scratch.setLittleEndian();

    Write32(scratch, scnum);        // section number
    Write32(scratch, value);        // value
    Write32(scratch, *strtab_offset);
    Write32(scratch, flags);        // flags
    assert(scratch.size() == SYMBOL_SIZE);
    m_os << scratch;

    *strtab_offset += static_cast<unsigned long>(sym.getName().size()+1);
}

void
XdfObject::Output(llvm::raw_fd_ostream& os, bool all_syms, Errwarns& errwarns)
{
    all_syms = true;   // force all syms into symbol table

    // Get number of symbols and set symbol index in symbol data.
    unsigned long symtab_count = 0;
    for (Object::symbol_iterator sym = m_object.symbols_begin(),
         end = m_object.symbols_end(); sym != end; ++sym)
    {
        int vis = sym->getVisibility();
        if (vis & Symbol::COMMON)
        {
            errwarns.Propagate(sym->getDeclLine(),
                Error(N_("XDF object format does not support common variables")));
            continue;
        }
        if (all_syms || (vis != Symbol::LOCAL && !(vis & Symbol::DLOCAL)))
        {
            // Save index in symrec data
            sym->AddAssocData(
                std::auto_ptr<XdfSymbol>(new XdfSymbol(symtab_count)));

            ++symtab_count;
        }
    }

    // Number sections
    long scnum = 0;
    for (Object::section_iterator i = m_object.sections_begin(),
         end = m_object.sections_end(); i != end; ++i)
    {
        XdfSection* xsect = i->getAssocData<XdfSection>();
        assert(xsect != 0);
        xsect->scnum = scnum++;
    }

    // Allocate space for headers by seeking forward
    os.seek(FILEHEAD_SIZE+SECTHEAD_SIZE*scnum);
    if (os.has_error())
        throw Fatal(N_("could not seek on output file"));

    XdfOutput out(os, m_object);

    // Get file offset of start of string table
    unsigned long strtab_offset =
        FILEHEAD_SIZE + SECTHEAD_SIZE*scnum + SYMBOL_SIZE*symtab_count;

    // Output symbol table
    for (Object::const_symbol_iterator sym = m_object.symbols_begin(),
         end = m_object.symbols_end(); sym != end; ++sym)
    {
        out.OutputSymbol(*sym, errwarns, all_syms, &strtab_offset);
    }

    // Output string table
    for (Object::const_symbol_iterator sym = m_object.symbols_begin(),
         end = m_object.symbols_end(); sym != end; ++sym)
    {
        if (all_syms || sym->getVisibility() != Symbol::LOCAL)
            os << sym->getName() << '\0';
    }

    // Output section data/relocs
    for (Object::section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        out.OutputSection(*i, errwarns);
    }

    // Write headers
    os.seek(0);
    if (os.has_error())
        throw Fatal(N_("could not seek on output file"));

    // Output object header
    Bytes& scratch = out.getScratch();
    scratch.setLittleEndian();
    Write32(scratch, XDF_MAGIC);        // magic number
    Write32(scratch, scnum);            // number of sects
    Write32(scratch, symtab_count);     // number of symtabs
    // size of sect headers + symbol table + strings
    Write32(scratch, strtab_offset-FILEHEAD_SIZE);
    assert(scratch.size() == FILEHEAD_SIZE);
    os << scratch;

    // Output section headers
    for (Object::const_section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        const XdfSection* xsect = i->getAssocData<XdfSection>();
        assert(xsect != 0);

        Bytes& scratch2 = out.getScratch();
        xsect->Write(scratch2, *i);
        assert(scratch2.size() == SECTHEAD_SIZE);
        os << scratch2;
    }
}

bool
XdfObject::Taste(const llvm::MemoryBuffer& in,
                 /*@out@*/ std::string* arch_keyword,
                 /*@out@*/ std::string* machine)
{
    InputBuffer inbuf(in);

    // Check for XDF magic number in header
    if (inbuf.getReadableSize() < FILEHEAD_SIZE)
        return false;
    inbuf.setLittleEndian();
    unsigned long magic = ReadU32(inbuf);
    if (magic != XDF_MAGIC)
        return false;

    // all XDF files are x86/x86 or amd64 (can't tell which)
    arch_keyword->assign("x86");
    machine->assign("x86");
    return true;
}

class ReadString
{
public:
    ReadString(const llvm::MemoryBuffer& in,
               unsigned long strtab_offset,
               unsigned long strtab_len)
        : m_in(in)
        , m_offset(strtab_offset)
        , m_len(strtab_len)
    {}

    llvm::StringRef
    operator() (unsigned long str_index)
    {
        if (str_index < m_offset || str_index >= m_offset+m_len)
            throw Error(N_("invalid string table offset"));
        return m_in.getBufferStart() + str_index;
    }

private:
    const llvm::MemoryBuffer& m_in;
    unsigned long m_offset;
    unsigned long m_len;
};

void
XdfObject::Read(const llvm::MemoryBuffer& in)
{
    InputBuffer inbuf(in);
    inbuf.setLittleEndian();

    // Read object header
    if (inbuf.getReadableSize() < FILEHEAD_SIZE)
        throw Error(N_("could not read object header"));
    unsigned long magic = ReadU32(inbuf);
    if (magic != XDF_MAGIC)
        throw Error(N_("not an XDF file"));
    unsigned long scnum = ReadU32(inbuf);
    unsigned long symnum = ReadU32(inbuf);
    unsigned long headers_len = ReadU32(inbuf);
    if (inbuf.getReadableSize() < headers_len)
        throw Error(N_("could not read XDF header tables"));

    unsigned long section_offset = FILEHEAD_SIZE;
    unsigned long symtab_offset = section_offset + SECTHEAD_SIZE*scnum;
    unsigned long strtab_offset = symtab_offset + SYMBOL_SIZE*symnum;
    ReadString read_string(in, strtab_offset,
                           FILEHEAD_SIZE+headers_len-strtab_offset);

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
        xsect->Read(in, section_offset+SECTHEAD_SIZE*i, &name_sym_index, &lma,
                    &vma, &align, &bss, &filepos, &nrelocs);
        xsect->scnum = i;

        // get section name from section symbol entry
        inbuf.setPosition(symtab_offset+name_sym_index*SYMBOL_SIZE+8);
        llvm::StringRef sectname = read_string(ReadU32(inbuf));

        std::auto_ptr<Section> section(
            new Section(sectname, xsect->bits != 0, bss, 0));

        section->setFilePos(filepos);
        section->setVMA(vma);
        section->setLMA(lma);

        if (bss)
        {
            Bytecode& gap = section->AppendGap(xsect->size, 0);
            gap.CalcLen(0);     // force length calculation of gap
        }
        else
        {
            // Read section data
            inbuf.setPosition(filepos);
            if (inbuf.getReadableSize() < xsect->size)
                throw Error(String::Compose(
                    N_("could not read section `%1' data"), sectname));

            section->bytecodes_first().getFixed().Write(inbuf.Read(xsect->size),
                                                        xsect->size);
        }

        // Associate section data with section
        section->AddAssocData(xsect);

        // Add section to object
        m_object.AppendSection(section);

        sects_nrelocs.push_back(nrelocs);
    }

    // Create symbols
    inbuf.setPosition(symtab_offset);
    for (unsigned long i=0; i<symnum; ++i)
    {
        unsigned long sym_scnum = ReadU32(inbuf);           // section number
        unsigned long value = ReadU32(inbuf);               // value
        llvm::StringRef symname = read_string(ReadU32(inbuf));// name
        unsigned long flags = ReadU32(inbuf);               // flags

        SymbolRef sym = m_object.getSymbol(symname);
        if ((flags & XdfSymbol::XDF_GLOBAL) != 0)
            sym->Declare(Symbol::GLOBAL, 0);
        else if ((flags & XdfSymbol::XDF_EXTERN) != 0)
            sym->Declare(Symbol::EXTERN, 0);

        if ((flags & XdfSymbol::XDF_EQU) != 0)
            sym->DefineEqu(Expr(value), 0);
        else if (sym_scnum < scnum)
        {
            Section& sect = m_object.getSection(sym_scnum);
            Location loc = {&sect.bytecodes_first(), value};
            sym->DefineLabel(loc, 0);
        }

        // Save index in symrec data
        sym->AddAssocData(std::auto_ptr<XdfSymbol>(new XdfSymbol(i)));
    }

    // Update section symbol info, and create section relocations
    std::vector<unsigned long>::iterator nrelocsi = sects_nrelocs.begin();
    for (Object::section_iterator sect=m_object.sections_begin(),
         end=m_object.sections_end(); sect != end; ++sect, ++nrelocsi)
    {
        XdfSection* xsect = sect->getAssocData<XdfSection>();
        assert(xsect != 0);

        // Read relocations
        inbuf.setPosition(xsect->relptr);
        if (inbuf.getReadableSize() < (*nrelocsi) * RELOC_SIZE)
            throw Error(String::Compose(
                N_("could not read section `%1' relocs"), sect->getName()));

        for (unsigned long i=0; i<(*nrelocsi); ++i)
        {
            unsigned long addr = ReadU32(inbuf);
            unsigned long sym_index = ReadU32(inbuf);
            unsigned long basesym_index = ReadU32(inbuf);
            XdfReloc::Type type = static_cast<XdfReloc::Type>(ReadU8(inbuf));
            XdfReloc::Size size = static_cast<XdfReloc::Size>(ReadU8(inbuf));
            unsigned char shift = ReadU8(inbuf);
            ReadU8(inbuf);      // flags; ignored
            SymbolRef sym = m_object.getSymbol(sym_index);
            SymbolRef basesym(0);
            if (type == XdfReloc::XDF_WRT)
                basesym = m_object.getSymbol(basesym_index);
            sect->AddReloc(std::auto_ptr<Reloc>(
                new XdfReloc(addr, sym, basesym, type, size, shift)));
        }
    }
}

Section*
XdfObject::AddDefaultSection()
{
    Section* section = AppendSection(".text", 0);
    section->setDefault(true);
    return section;
}

Section*
XdfObject::AppendSection(const llvm::StringRef& name, unsigned long line)
{
    bool code = (name == ".text");
    Section* section = new Section(name, code, false, line);
    m_object.AppendSection(std::auto_ptr<Section>(section));

    // Define a label for the start of the section
    Location start = {&section->bytecodes_first(), 0};
    SymbolRef sym = m_object.getSymbol(name);
    sym->DefineLabel(start, line);

    // Add XDF data to the section
    section->AddAssocData(std::auto_ptr<XdfSection>(new XdfSection(sym)));

    return section;
}

void
XdfObject::DirSection(DirectiveInfo& info)
{
    assert(info.isObject(m_object));
    NameValues& nvs = info.getNameValues();
    unsigned long line = info.getLine();

    if (!nvs.front().isString())
        throw Error(N_("section name must be a string"));
    llvm::StringRef sectname = nvs.front().getString();

    Section* sect = m_object.FindSection(sectname);
    bool first = true;
    if (sect)
        first = sect->isDefault();
    else
        sect = AppendSection(sectname, line);

    XdfSection* xsect = sect->getAssocData<XdfSection>();
    assert(xsect != 0);

    m_object.setCurSection(sect);
    sect->setDefault(false);
    m_object.getArch()->setVar("mode_bits", xsect->bits);       // reapply

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

    unsigned long bss = sect->isBSS();
    unsigned long code = sect->isCode();
    IntNum vma = sect->getVMA();
    IntNum lma = sect->getLMA();
    unsigned long flat = xsect->flat;


    DirHelpers helpers;
    helpers.Add("use16", false,
                BIND::bind(&DirResetFlag, _1, &xsect->bits, 16));
    helpers.Add("use32", false,
                BIND::bind(&DirResetFlag, _1, &xsect->bits, 32));
    helpers.Add("use64", false,
                BIND::bind(&DirResetFlag, _1, &xsect->bits, 64));
    helpers.Add("bss", false, BIND::bind(&DirSetFlag, _1, &bss, 1));
    helpers.Add("nobss", false, BIND::bind(&DirClearFlag, _1, &bss, 1));
    helpers.Add("code", false, BIND::bind(&DirSetFlag, _1, &code, 1));
    helpers.Add("data", false, BIND::bind(&DirClearFlag, _1, &code, 1));
    helpers.Add("flat", false, BIND::bind(&DirSetFlag, _1, &flat, 1));
    helpers.Add("noflat", false, BIND::bind(&DirClearFlag, _1, &flat, 1));
    helpers.Add("absolute", true,
                BIND::bind(&DirIntNum, _1, &m_object, line, &lma,
                           &xsect->has_addr));
    helpers.Add("virtual", true,
                BIND::bind(&DirIntNum, _1, &m_object, line, &vma,
                           &xsect->has_vaddr));
    helpers.Add("align", true,
                BIND::bind(&DirIntNum, _1, &m_object, line, &align,
                           &has_align));

    helpers(++nvs.begin(), nvs.end(), DirNameValueWarn);

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
        if (aligni > 4096)
            throw ValueError(N_("XDF does not support alignments > 4096"));

        sect->setAlign(aligni);
    }

    sect->setBSS(bss);
    sect->setCode(code);
    sect->setVMA(vma);
    sect->setLMA(lma);
    xsect->flat = flat;
    m_object.getArch()->setVar("mode_bits", xsect->bits);
}

void
XdfObject::AddDirectives(Directives& dirs, const llvm::StringRef& parser)
{
    static const Directives::Init<XdfObject> nasm_dirs[] =
    {
        {"section", &XdfObject::DirSection, Directives::ARG_REQUIRED},
        {"segment", &XdfObject::DirSection, Directives::ARG_REQUIRED},
    };

    if (String::NocaseEqual(parser, "nasm"))
        dirs.AddArray(this, nasm_dirs, NELEMS(nasm_dirs));
}

void
DoRegister()
{
    RegisterModule<ObjectFormatModule,
                   ObjectFormatModuleImpl<XdfObject> >("xdf");
}

}}} // namespace yasm::objfmt::xdf

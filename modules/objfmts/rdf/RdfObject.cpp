//
// Relocatable Dynamic Object File Format (RDOFF) version 2 format
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
#include <util.h>

#include "llvm/ADT/IndexedMap.h"
#include "yasmx/Support/bitcount.h"
#include "yasmx/Support/Compose.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/nocase.h"
#include "yasmx/Support/registry.h"
#include "yasmx/Arch.h"
#include "yasmx/Bytecode.h"
#include "yasmx/BytecodeOutput.h"
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
#include "yasmx/Object_util.h"
#include "yasmx/ObjectFormat.h"
#include "yasmx/Reloc.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"
#include "yasmx/Symbol_util.h"
#include "yasmx/Value.h"

#include "RdfRecords.h"
#include "RdfReloc.h"
#include "RdfSection.h"
#include "RdfSymbol.h"


namespace yasm
{
namespace objfmt
{
namespace rdf
{

static const unsigned char RDF_MAGIC[] = {'R', 'D', 'O', 'F', 'F', '2'};

// Maximum size of an import/export label (including trailing zero)
static const unsigned int EXIM_LABEL_MAX = 64;

// Maximum size of library or module name (including trailing zero)
static const unsigned int MODLIB_NAME_MAX = 128;

// Maximum number of segments that we can handle in one file
static const unsigned int RDF_MAXSEGS = 64;

class RdfObject : public ObjectFormat
{
public:
    /// Constructor.
    /// To make object format truly usable, set_object()
    /// needs to be called.
    RdfObject(const ObjectFormatModule& module, Object& object)
        : ObjectFormat(module, object)
    {}

    /// Destructor.
    ~RdfObject() {}

    void AddDirectives(Directives& dirs, const llvm::StringRef& parser);

    void Read(const llvm::MemoryBuffer& in);
    void Output(llvm::raw_fd_ostream& os, bool all_syms, Errwarns& errwarns);

    Section* AddDefaultSection();
    Section* AppendSection(const llvm::StringRef& name, unsigned long line);

    static const char* getName()
    { return "Relocatable Dynamic Object File Format (RDOFF) v2.0"; }
    static const char* getKeyword() { return "rdf"; }
    static const char* getExtension() { return ".rdf"; }
    static unsigned int getDefaultX86ModeBits() { return 32; }
    static const char* getDefaultDebugFormatKeyword() { return "null"; }
    static std::vector<const char*> getDebugFormatKeywords();
    static bool isOkObject(Object& object) { return true; }
    static bool Taste(const llvm::MemoryBuffer& in,
                      /*@out@*/ std::string* arch_keyword,
                      /*@out@*/ std::string* machine);

private:
    void DirSection(Object& object,
                    NameValues& namevals,
                    NameValues& objext_namevals,
                    unsigned long line);
    void DirLibrary(Object& object,
                    NameValues& namevals,
                    NameValues& objext_namevals,
                    unsigned long line);
    void DirModule(Object& object,
                   NameValues& namevals,
                   NameValues& objext_namevals,
                   unsigned long line);

    void AddLibOrModule(const llvm::StringRef& name, bool lib);

    std::vector<std::string> m_module_names;
    std::vector<std::string> m_library_names;
};

std::vector<const char*>
RdfObject::getDebugFormatKeywords()
{
    static const char* keywords[] = {"null"};
    return std::vector<const char*>(keywords, keywords+NELEMS(keywords));
}

class RdfOutput : public BytecodeOutput
{
public:
    RdfOutput(llvm::raw_ostream& os, Object& object);
    ~RdfOutput();

    void OutputSectionToMemory(Section& sect, Errwarns& errwarns);
    void OutputSectionRelocs(const Section& sect);
    void OutputSectionToFile(const Section& sect);

    void OutputSymbol(Symbol& sym,
                      Errwarns& errwarns,
                      bool all_syms,
                      unsigned int* indx);

    void OutputBSS();

    // BytecodeOutput overrides
    void ConvertValueToBytes(Value& value,
                             Bytes& bytes,
                             Location loc,
                             int warn);
    void DoOutputGap(unsigned int size);
    void DoOutputBytes(const Bytes& bytes);

private:
    llvm::raw_ostream& m_os;

    RdfSection* m_rdfsect;
    Object& m_object;
    BytecodeNoOutput m_no_output;

    unsigned long m_bss_size;       // total BSS size
};

RdfOutput::RdfOutput(llvm::raw_ostream& os, Object& object)
    : m_os(os)
    , m_object(object)
    , m_bss_size(0)
{
}

RdfOutput::~RdfOutput()
{
}

typedef struct rdf_objfmt_output_info {
    unsigned long indx;             // symbol "segment" (extern/common only)
} rdf_objfmt_output_info;


void
RdfOutput::ConvertValueToBytes(Value& value,
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
        throw TooComplexError(N_("rdf: relocation too complex"));

    IntNum intn(0);
    if (value.isRelative())
    {
        if (value.isWRT())
            throw TooComplexError(N_("rdf: WRT not supported"));

        Section* sect = loc.bc->getContainer()->AsSection();
        IntNum addr = loc.getOffset();
        SymbolRef sym = value.getRelative();
        RdfReloc::Type type = RdfReloc::RDF_NORM;

        bool pc_rel = false;
        IntNum intn2;
        if (value.CalcPCRelSub(&intn2, loc))
        {
            // Create PC-relative relocation type and fix up absolute portion.
            pc_rel = true;
            intn += intn2;
        }
        else if (value.hasSubRelative())
            throw TooComplexError(N_("rdf: relocation too complex"));

        if (pc_rel)
        {
            type = RdfReloc::RDF_REL;
            intn -= loc.getOffset();    // Adjust to start of section
        }
        else if (value.isSegOf())
            type = RdfReloc::RDF_SEG;

        unsigned int refseg;
        Location symloc;
        if (sym->getLabel(&symloc))
        {
            // local, set the value to be the offset, and the refseg to the
            // segment number.
            Section* sym_sect = symloc.bc->getContainer()->AsSection();
            RdfSection* rdfsect = sym_sect->getAssocData<RdfSection>();
            assert(rdfsect != 0);
            refseg = rdfsect->scnum;
            intn += symloc.getOffset();
        }
        else
        {
            // must be common/external
            const RdfSymbol* rdfsym = sym->getAssocData<RdfSymbol>();
            assert(rdfsym != 0 && "rdf: no symbol data for relocated symbol");
            refseg = rdfsym->segment;
        }

        sect->AddReloc(std::auto_ptr<Reloc>(
            new RdfReloc(addr, sym, type, value.getSize()/8, refseg)));
    }

    if (Expr* abs = value.getAbs())
    {
        if (!abs->isIntNum())
            throw TooComplexError(N_("rdf: relocation too complex"));
        intn += abs->getIntNum();
    }

    m_object.getArch()->ToBytes(intn, bytes, value.getSize(), 0, warn);
}

void
RdfOutput::DoOutputGap(unsigned int size)
{
    setWarn(WARN_UNINIT_CONTENTS,
            N_("uninitialized space declared in code/data section: zeroing"));
    m_rdfsect->raw_data.insert(m_rdfsect->raw_data.end(), size, 0);
}

void
RdfOutput::DoOutputBytes(const Bytes& bytes)
{
    m_rdfsect->raw_data.insert(m_rdfsect->raw_data.end(), bytes.begin(),
                               bytes.end());
}

void
RdfOutput::OutputSectionToMemory(Section& sect, Errwarns& errwarns)
{
    BytecodeOutput* outputter = this;

    m_rdfsect = sect.getAssocData<RdfSection>();
    assert(m_rdfsect != 0);

    if (sect.isBSS())
    {
        // Don't output BSS sections.
        outputter = &m_no_output;
    }

    // See UGH comment in output() for why we're doing this
    m_rdfsect->raw_data.clear();
    unsigned long size = 0;

    // Output bytecodes
    for (Section::bc_iterator i=sect.bytecodes_begin(),
         end=sect.bytecodes_end(); i != end; ++i)
    {
        try
        {
            i->Output(*outputter);
            size += i->getTotalLen();
        }
        catch (Error& err)
        {
            errwarns.Propagate(i->getLine(), err);
        }
        errwarns.Propagate(i->getLine());   // propagate warnings
    }

    // Sanity check final section size
    assert(size == sect.bytecodes_last().getNextOffset());

    if (sect.isBSS())
    {
        m_bss_size += size;
        return;
    }

    // Sanity check raw data size
    assert(m_rdfsect->raw_data.size() == size);
}

void
RdfOutput::OutputSectionRelocs(const Section& sect)
{
    const RdfSection* rdfsect = sect.getAssocData<RdfSection>();
    assert(rdfsect != 0);

    if (sect.getRelocs().size() == 0)
        return;

    for (Section::const_reloc_iterator i=sect.relocs_begin(),
         end=sect.relocs_end(); i != end; ++i)
    {
        const RdfReloc& reloc = static_cast<const RdfReloc&>(*i);
        Bytes& scratch = getScratch();
        reloc.Write(scratch, rdfsect->scnum);
        assert(scratch.size() == 10);
        m_os << scratch;
    }
}

void
RdfOutput::OutputSectionToFile(const Section& sect)
{
    const RdfSection* rdfsect = sect.getAssocData<RdfSection>();
    assert(rdfsect != 0);

    if (sect.isBSS())
    {
        // Don't output BSS sections.
        return;
    }

    // Empty?  Go on to next section
    if (rdfsect->raw_data.empty())
        return;

    // Section header
    Bytes& bytes = getScratch();
    rdfsect->Write(bytes, sect);
    assert(bytes.size() == 10);
    m_os << bytes;

    // Section data
    m_os << rdfsect->raw_data;
}

enum RdfSymbolFlags
{
    // Flags for ExportRec/ImportRec
    SYM_DATA =      0x0001,
    SYM_FUNCTION =  0x0002,

    // Flags for ExportRec
    SYM_GLOBAL =    0x0004,

    // Flags for ImportRec
    SYM_IMPORT =    0x0008,
    SYM_FAR =       0x0010
};

static unsigned int
ParseFlags(Symbol& sym)
{
    unsigned long flags = 0;
    int vis = sym.getVisibility();

    NameValues* objext_nvs = getObjextNameValues(sym);
    if (!objext_nvs)
        return 0;

    DirHelpers helpers;
    helpers.Add("data", false, BIND::bind(&DirSetFlag, _1, &flags, SYM_DATA));
    helpers.Add("object", false, BIND::bind(&DirSetFlag, _1, &flags, SYM_DATA));
    helpers.Add("proc", false,
                BIND::bind(&DirSetFlag, _1, &flags, SYM_FUNCTION));
    helpers.Add("function", false,
                BIND::bind(&DirSetFlag, _1, &flags, SYM_FUNCTION));

    if (vis & Symbol::GLOBAL)
    {
        helpers.Add("export", false,
                    BIND::bind(&DirSetFlag, _1, &flags, SYM_GLOBAL));
    }
    if (vis & Symbol::EXTERN)
    {
        helpers.Add("import", false,
                    BIND::bind(&DirSetFlag, _1, &flags, SYM_IMPORT));
        helpers.Add("far", false, BIND::bind(&DirSetFlag, _1, &flags, SYM_FAR));
        helpers.Add("near", false,
                    BIND::bind(&DirClearFlag, _1, &flags, SYM_FAR));
    }

    helpers(objext_nvs->begin(), objext_nvs->end(), DirNameValueWarn);

    return static_cast<unsigned int>(flags);
}

void
RdfOutput::OutputSymbol(Symbol& sym,
                        Errwarns& errwarns,
                        bool all_syms,
                        unsigned int* indx)
{
    int vis = sym.getVisibility();

    unsigned long value = 0;
    unsigned int scnum = 0;

    if (!all_syms && (vis == Symbol::LOCAL || vis == Symbol::DLOCAL))
        return;     // skip local syms

    // Look at symrec for value/scnum/etc.
    Location loc;
    if (sym.getLabel(&loc))
    {
        // it's a label: get value and offset.
        Section* sect = loc.bc->getContainer()->AsSection();
        RdfSection* rdfsect = sect->getAssocData<RdfSection>();
        assert (rdfsect != 0);
        scnum = rdfsect->scnum;
        value = loc.getOffset();
    }
    else if (sym.getEqu())
    {
        setWarn(WARN_GENERAL,
                N_("rdf does not support exporting EQU/absolute values"));
        errwarns.Propagate(sym.getDeclLine());
        return;
    }

    llvm::StringRef name = sym.getName();
    size_t len = name.size();

    if (len > EXIM_LABEL_MAX-1)
    {
        setWarn(WARN_GENERAL, String::Compose(
                N_("label name too long, truncating to %1 bytes"),
                EXIM_LABEL_MAX));
        errwarns.Propagate(sym.getDeclLine());

        len = EXIM_LABEL_MAX-1;
    }

    Bytes& bytes = getScratch();
    bytes.setLittleEndian();
    if (vis & Symbol::GLOBAL)
    {
        Write8(bytes, RDFREC_GLOBAL);
        Write8(bytes, 6+len+1);                 // record length
        Write8(bytes, ParseFlags(sym));         // flags
        Write8(bytes, scnum);                   // segment referred to
        Write32(bytes, value);                  // offset
    }
    else
    {
        // Save symbol segment in symrec data (for later reloc gen)
        scnum = (*indx)++;
        sym.AddAssocData(std::auto_ptr<RdfSymbol>(new RdfSymbol(scnum)));

        if (vis & Symbol::COMMON)
        {
            unsigned long addralign = 0;

            Write8(bytes, RDFREC_COMMON);
            Write8(bytes, 8+len+1);         // record length
            Write16(bytes, scnum);          // segment allocated

            // size
            assert(getCommonSize(sym) != 0);
            Expr csize_expr(*getCommonSize(sym));
            SimplifyCalcDist(csize_expr);
            if (!csize_expr.isIntNum())
            {
                errwarns.Propagate(sym.getDeclLine(), NotConstantError(
                    N_("COMMON data size not an integer expression")));
            }
            else
                value = csize_expr.getIntNum().getUInt();
            Write32(bytes, value);

            // alignment
            if (NameValues* objext_nvs = getObjextNameValues(sym))
            {
                for (NameValues::iterator nv=objext_nvs->begin(),
                     end=objext_nvs->end(); nv != end; ++nv)
                {
                    if (nv->getName().empty())
                    {
                        if (!nv->isExpr())
                        {
                            errwarns.Propagate(sym.getDeclLine(),
                                ValueError(N_("alignment is not an integer")));
                            continue;
                        }
                        Expr aligne = nv->getExpr(m_object, sym.getDeclLine());
                        SimplifyCalcDist(aligne);
                        if (!aligne.isIntNum())
                        {
                            errwarns.Propagate(sym.getDeclLine(),
                                ValueError(N_("alignment is not an integer")));
                            continue;
                        }
                        addralign = aligne.getIntNum().getUInt();

                        // Alignments must be a power of two.
                        if (!isExp2(addralign))
                        {
                            errwarns.Propagate(sym.getDeclLine(), ValueError(
                                N_("alignment constraint is not a power of two")));
                            continue;
                        }
                    }
                    else
                    {
                        setWarn(WARN_GENERAL, String::Compose(
                            N_("Unrecognized qualifier `%1'"), nv->getName()));
                        errwarns.Propagate(sym.getDeclLine());
                    }
                }
            }
            Write16(bytes, addralign);
        }
        else if (vis & Symbol::EXTERN)
        {
            unsigned int flags = ParseFlags(sym);
            if (flags & SYM_FAR)
            {
                Write8(bytes, RDFREC_FARIMPORT);
                flags &= ~SYM_FAR;
            }
            else
                Write8(bytes, RDFREC_IMPORT);
            Write8(bytes, 3+len+1);     // record length
            Write8(bytes, flags);       // flags
            Write16(bytes, scnum);      // segment allocated
        }
    }

    // Symbol name
    bytes.insert(bytes.end(), name.begin(), name.begin()+len);
    Write8(bytes, 0);           // 0-terminated name

    m_os << bytes;
}

void
RdfOutput::OutputBSS()
{
    if (m_bss_size == 0)
        return;

    Bytes& bytes = getScratch();
    bytes.setLittleEndian();
    Write8(bytes, RDFREC_BSS);      // record type
    Write8(bytes, 4);               // record length
    Write32(bytes, m_bss_size);     // total BSS size
    m_os << bytes;
}

void
RdfObject::Output(llvm::raw_fd_ostream& os, bool all_syms, Errwarns& errwarns)
{
    // Number sections
    unsigned int scnum = 0;     // section numbering starts at 0
    for (Object::section_iterator i = m_object.sections_begin(),
         end = m_object.sections_end(); i != end; ++i)
    {
        RdfSection* rdfsect = i->getAssocData<RdfSection>();
        assert(rdfsect != 0);
        rdfsect->scnum = scnum++;
    }

    // Allocate space for file header by seeking forward
    os.seek(sizeof(RDF_MAGIC)+8);
    if (os.has_error())
        throw Fatal(N_("could not seek on output file"));

    RdfOutput out(os, m_object);

    // Output custom header records (library and module, etc)
    for (std::vector<std::string>::const_iterator i=m_module_names.begin(),
         end=m_module_names.end();  i != end; ++i)
    {
        Bytes& bytes = out.getScratch();
        Write8(bytes, RDFREC_MODNAME);          // record type
        Write8(bytes, i->length()+1);           // record length
        os << bytes;
        os << *i << '\0';                       // 0-terminated name
    }

    for (std::vector<std::string>::const_iterator i=m_library_names.begin(),
         end=m_library_names.end();  i != end; ++i)
    {
        Bytes& bytes = out.getScratch();
        Write8(bytes, RDFREC_DLL);              // record type
        Write8(bytes, i->length()+1);           // record length
        os << bytes;
        os << *i << '\0';                       // 0-terminated name
    }

    // Output symbol table
    for (Object::symbol_iterator sym = m_object.symbols_begin(),
         end = m_object.symbols_end(); sym != end; ++sym)
    {
        out.OutputSymbol(*sym, errwarns, all_syms, &scnum);
    }

    // UGH! Due to the fact the relocs go at the beginning of the file, and
    // we only know if we have relocs when we output the sections, we have
    // to output the section data before we have output the relocs.  But
    // we also don't know how much space to preallocate for relocs, so....
    // we output into memory buffers first (thus the UGH).
    //
    // Stupid object format design, if you ask me (basically all other
    // object formats put the relocs *after* the section data to avoid this
    // exact problem).
    //
    // We also calculate the total size of all BSS sections here.
    //
    for (Object::section_iterator i = m_object.sections_begin(),
         end = m_object.sections_end(); i != end; ++i)
    {
        out.OutputSectionToMemory(*i, errwarns);
    }

    // Output all relocs
    for (Object::const_section_iterator i = m_object.sections_begin(),
         end = m_object.sections_end(); i != end; ++i)
    {
        out.OutputSectionRelocs(*i);
    }

    // Output BSS record
    out.OutputBSS();

    // Determine header length
    uint64_t pos = os.tell();
    if (os.has_error())
        throw IOError(N_("could not get file position on output file"));
    unsigned long headerlen = static_cast<unsigned long>(pos);

    // Section data (to file)
    for (Object::const_section_iterator i = m_object.sections_begin(),
         end = m_object.sections_end(); i != end; ++i)
    {
        out.OutputSectionToFile(*i);
    }

    // NULL section to end file
    {
        Bytes& bytes = out.getScratch();
        bytes.resize(10);
        os << bytes;
    }

    // Determine object length
    pos = os.tell();
    if (os.has_error())
        throw IOError(N_("could not get file position on output file"));
    unsigned long filelen = static_cast<unsigned long>(pos);

    // Write file header
    os.seek(0);
    if (os.has_error())
        throw IOError(N_("could not seek on output file"));

    {
        Bytes& bytes = out.getScratch();
        bytes.insert(bytes.end(), RDF_MAGIC, RDF_MAGIC+sizeof(RDF_MAGIC));
        bytes.setLittleEndian();
        Write32(bytes, filelen-sizeof(RDF_MAGIC)-4);    // object size
        Write32(bytes, headerlen-sizeof(RDF_MAGIC)-8);  // header size
        os << bytes;
    }
}

bool
RdfObject::Taste(const llvm::MemoryBuffer& in,
                 /*@out@*/ std::string* arch_keyword,
                 /*@out@*/ std::string* machine)
{
    InputBuffer inbuf(in);

    // Check for RDF magic number in header
    if (inbuf.getReadableSize() < sizeof(RDF_MAGIC))
        return false;

    const unsigned char* magic = inbuf.Read(sizeof(RDF_MAGIC));
    for (unsigned int i=0; i<sizeof(RDF_MAGIC); ++i)
    {
        if (magic[i] != RDF_MAGIC[i])
            return false;
    }

    // Assume all RDF files are x86/x86
    arch_keyword->assign("x86");
    machine->assign("x86");
    return true;
}

void
RdfObject::Read(const llvm::MemoryBuffer& in)
{
    InputBuffer inbuf(in);

    // Read file header
    if (inbuf.getReadableSize() < sizeof(RDF_MAGIC)+8)
        throw IOError(N_("could not read object header"));

    const unsigned char* magic = inbuf.Read(sizeof(RDF_MAGIC));
    for (unsigned int i=0; i<sizeof(RDF_MAGIC); ++i)
    {
        if (magic[i] != RDF_MAGIC[i])
            throw Error(N_("not a RDF file"));
    }

    inbuf.setLittleEndian();
    unsigned long object_end = ReadU32(inbuf) + sizeof(RDF_MAGIC) + 4;
    unsigned long headers_end = ReadU32(inbuf) + sizeof(RDF_MAGIC) + 8;

    // Symbol table by index (aka section number)
    llvm::IndexedMap<SymbolRef> symtab;

    // Read sections
    inbuf.setPosition(headers_end);
    while (inbuf.getPosition() < object_end)
    {
        std::auto_ptr<RdfSection> rsect(
            new RdfSection(RdfSection::RDF_UNKNOWN, SymbolRef(0)));

        // Read section header
        unsigned long size;
        std::string sectname;
        rsect->Read(inbuf, &size, &sectname);

        // Stop reading on NULL section
        if (rsect->scnum == 0 && rsect->type == RdfSection::RDF_BSS &&
            rsect->reserved == 0 && size == 0)
            break;

        // Create and initialize section
        std::auto_ptr<Section> section(
            new Section(sectname, rsect->type == RdfSection::RDF_CODE,
                        rsect->type == RdfSection::RDF_BSS, 0));

        section->setFilePos(inbuf.getPosition());

        if (rsect->type == RdfSection::RDF_BSS)
        {
            Bytecode& gap = section->AppendGap(size, 0);
            gap.CalcLen(0);     // force length calculation of gap
        }
        else
        {
            // Read section data
            if (inbuf.getReadableSize() < size)
                throw Error(String::Compose(
                    N_("could not read section %1 data"), rsect->scnum));
            section->bytecodes_first().getFixed().Write(inbuf.Read(size), size);
        }

        // Create symbol for section start (used for relocations)
        SymbolRef sym = m_object.AddNonTableSymbol(sectname);
        Location loc = {&section->bytecodes_first(), 0};
        sym->DefineLabel(loc, 0);
        // and keep in symtab map
        symtab.grow(rsect->scnum);
        symtab[rsect->scnum] = sym;

        // Associate section data with section
        section->AddAssocData(rsect);

        // Add section to object
        m_object.AppendSection(section);
    }

    // Seek back to read headers
    inbuf.setPosition(sizeof(RDF_MAGIC)+8);
    while (inbuf.getPosition() < headers_end)
    {
        // Read record type and length
        unsigned int type = ReadU8(inbuf);
        unsigned int len = ReadU8(inbuf);
        InputBuffer recbuf(inbuf.Read(len), len);
        switch (type)
        {
            case RDFREC_COMMON:
            {
                // Read record
                recbuf.setLittleEndian();
                unsigned int scnum = ReadU16(recbuf);
                unsigned long value = ReadU32(recbuf);
                /*unsigned int align = */ReadU16(recbuf);
                size_t namelen = recbuf.getReadableSize();
                llvm::StringRef symname(
                    reinterpret_cast<const char*>(recbuf.Read(namelen)),
                    namelen-1);

                // Create symbol
                SymbolRef sym = m_object.getSymbol(symname);
                sym->Declare(Symbol::COMMON, 0);
                setCommonSize(*sym, Expr(value));
                // TODO: align
                sym->AddAssocData(std::auto_ptr<RdfSymbol>
                                  (new RdfSymbol(scnum)));

                // Keep in symtab map (needed for relocation lookups)
                symtab.grow(scnum);
                symtab[scnum] = sym;
                break;
            }
            case RDFREC_IMPORT:
            case RDFREC_FARIMPORT:
            {
                // Read record
                recbuf.setLittleEndian();
                /*unsigned int flags = */ReadU8(recbuf);
                unsigned int scnum = ReadU16(recbuf);
                size_t namelen = recbuf.getReadableSize();
                llvm::StringRef symname(
                    reinterpret_cast<const char*>(recbuf.Read(namelen)),
                    namelen-1);

                // Create symbol
                SymbolRef sym = m_object.getSymbol(symname);
                sym->Declare(Symbol::EXTERN, 0);
                sym->AddAssocData(std::auto_ptr<RdfSymbol>
                                  (new RdfSymbol(scnum)));

                // Keep in symtab map (needed for relocation lookups)
                symtab.grow(scnum);
                symtab[scnum] = sym;
                break;
            }
            case RDFREC_GLOBAL:
            {
                // Read record
                recbuf.setLittleEndian();
                /*unsigned int flags = */ReadU8(recbuf);
                unsigned int scnum = ReadU8(recbuf);
                unsigned long value = ReadU32(recbuf);
                size_t namelen = recbuf.getReadableSize();
                llvm::StringRef symname(
                    reinterpret_cast<const char*>(recbuf.Read(namelen)),
                    namelen-1);

                // Create symbol
                SymbolRef sym = m_object.getSymbol(symname);
                Section& sect = m_object.getSection(scnum);
                Location loc = {&sect.bytecodes_first(), value};
                sym->DefineLabel(loc, 0);
                sym->Declare(Symbol::GLOBAL, 0);
                break;
            }
            case RDFREC_MODNAME:
                m_module_names.push_back(
                    std::string(reinterpret_cast<const char*>(recbuf.Read(len)),
                                len-1));
                break;
            case RDFREC_DLL:
                m_library_names.push_back(
                    std::string(reinterpret_cast<const char*>(recbuf.Read(len)),
                                len-1));
                break;
            case RDFREC_BSS:
            {
                if (len != 4)
                    throw Error(N_("BSS record is not 4 bytes long"));

                // Make .bss section, populate it, and add it to the object.
                unsigned long size = ReadU32(recbuf);
                std::auto_ptr<RdfSection> rsect(
                    new RdfSection(RdfSection::RDF_BSS, SymbolRef(0)));
                rsect->scnum = 0;
                std::auto_ptr<Section> section(
                    new Section(".bss", false, true, 0));
                Bytecode& gap = section->AppendGap(size, 0);
                gap.CalcLen(0);     // force length calculation of gap

                // Create symbol for section start (used for relocations)
                SymbolRef sym = m_object.AddNonTableSymbol(".bss");
                Location loc = {&section->bytecodes_first(), 0};
                sym->DefineLabel(loc, 0);
                // and keep in symtab map
                unsigned int scnum = m_object.getNumSections();
                symtab.grow(scnum);
                symtab[scnum] = sym;

                // Associate data, and add section to object
                section->AddAssocData(rsect);
                m_object.AppendSection(section);
                break;
            }
            default:
                break;  // ignore unrecognized records
        }
    }

    // Seek back again and read relocations
    inbuf.setPosition(sizeof(RDF_MAGIC)+8);
    while (inbuf.getPosition() < headers_end)
    {
        // Read record type and length
        unsigned int type = ReadU8(inbuf);
        unsigned int len = ReadU8(inbuf);
        InputBuffer recbuf(inbuf.Read(len), len);
        switch (type)
        {
            case RDFREC_RELOC:
            case RDFREC_SEGRELOC:
            {
                // Section number
                unsigned int scnum = ReadU8(recbuf);

                // Check for relative reloc case
                RdfReloc::Type rtype;
                if (type == RDFREC_SEGRELOC)
                    rtype = RdfReloc::RDF_SEG;
                else if (scnum >= 0x40)
                {
                    rtype = RdfReloc::RDF_REL;
                    scnum -= 0x40;
                }
                else
                    rtype = RdfReloc::RDF_NORM;

                recbuf.setLittleEndian();
                unsigned long addr = ReadU32(recbuf);
                unsigned int size = ReadU8(recbuf);
                unsigned int refseg = ReadU16(recbuf);

                // Create relocation
                Section& sect = m_object.getSection(scnum);
                if (refseg >= symtab.size())
                    throw Error(String::Compose(
                        N_("relocation refseg %1 out of range"), refseg));
                SymbolRef sym = symtab[refseg];
                if (!sym)
                    throw Error(String::Compose(
                        N_("invalid refseg %1"), refseg));
                sect.AddReloc(std::auto_ptr<Reloc>(
                    new RdfReloc(addr, sym, rtype, size, refseg)));
                break;
            }
            default:
                break;  // ignore unrecognized records
        }
    }
}

Section*
RdfObject::AppendSection(const llvm::StringRef& name, unsigned long line)
{
    RdfSection::Type type = RdfSection::RDF_UNKNOWN;
    if (name == ".text")
        type = RdfSection::RDF_CODE;
    else if (name == ".data")
        type = RdfSection::RDF_DATA;
    else if (name == ".bss")
        type = RdfSection::RDF_BSS;

    Section* section = new Section(name, type == RdfSection::RDF_CODE,
                                   type == RdfSection::RDF_BSS, line);
    m_object.AppendSection(std::auto_ptr<Section>(section));

    // Define a label for the start of the section
    Location start = {&section->bytecodes_first(), 0};
    SymbolRef sym = m_object.getSymbol(name);
    sym->DefineLabel(start, line);

    // Add RDF data to the section
    section->AddAssocData(std::auto_ptr<RdfSection>(new RdfSection(type, sym)));

    return section;
}

Section*
RdfObject::AddDefaultSection()
{
    Section* section = AppendSection(".text", 0);
    section->setDefault(true);
    return section;
}

inline bool
SetReserved(NameValue& nv,
            Object* obj,
            unsigned long line,
            IntNum* out,
            bool* out_set)
{
    if (!nv.getName().empty() || !nv.isExpr())
        return DirNameValueWarn(nv);

    std::auto_ptr<Expr> e(nv.ReleaseExpr(*obj, line));

    if ((e.get() == 0) || !e->isIntNum())
        throw NotConstantError(String::Compose(
            N_("implicit reserved size is not an integer")));

    *out = e->getIntNum();
    *out_set = true;
    return true;
}

void
RdfObject::DirSection(Object& object,
                      NameValues& nvs,
                      NameValues& objext_nvs,
                      unsigned long line)
{
    assert(&object == &m_object);

    if (!nvs.front().isString())
        throw Error(N_("section name must be a string"));
    llvm::StringRef sectname = nvs.front().getString();

    Section* sect = m_object.FindSection(sectname);
    bool first = true;
    if (sect)
        first = sect->isDefault();
    else
        sect = AppendSection(sectname, line);

    RdfSection* rsect = sect->getAssocData<RdfSection>();
    assert(rsect != 0);

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
    IntNum reserved;
    bool has_reserved = false;
    unsigned long type = rsect->type;

    DirHelpers helpers;
    // FIXME: We don't allow multiple bss sections (for now) because we'd have
    // to merge them before output into a single section.
    //helpers.Add("bss", false,
    //            BIND::bind(&DirResetFlag, _1, &type, RdfSection::RDF_BSS));
    helpers.Add("code", false,
                BIND::bind(&DirResetFlag, _1, &type, RdfSection::RDF_CODE));
    helpers.Add("text", false,
                BIND::bind(&DirResetFlag, _1, &type, RdfSection::RDF_CODE));
    helpers.Add("data", false,
                BIND::bind(&DirResetFlag, _1, &type, RdfSection::RDF_DATA));
    helpers.Add("comment", false,
                BIND::bind(&DirResetFlag, _1, &type, RdfSection::RDF_COMMENT));
    helpers.Add("lcomment", false,
                BIND::bind(&DirResetFlag, _1, &type, RdfSection::RDF_LCOMMENT));
    helpers.Add("pcomment", false,
                BIND::bind(&DirResetFlag, _1, &type, RdfSection::RDF_PCOMMENT));
    helpers.Add("symdebug", false,
                BIND::bind(&DirResetFlag, _1, &type, RdfSection::RDF_SYMDEBUG));
    helpers.Add("linedebug", false, BIND::bind(&DirResetFlag, _1, &type,
                                               RdfSection::RDF_LINEDEBUG));
    helpers.Add("reserved", true, BIND::bind(&DirIntNum, _1, &m_object, line,
                                             &reserved, &has_reserved));

    helpers(++nvs.begin(), nvs.end(),
            BIND::bind(&SetReserved, _1, &m_object, line, &reserved,
                       &has_reserved));

    rsect->type = static_cast<RdfSection::Type>(type);
    if (rsect->type == RdfSection::RDF_UNKNOWN)
    {
        rsect->type = RdfSection::RDF_DATA;
        throw ValueError(N_("new segment declared without type code"));
    }

    if (has_reserved)
        rsect->reserved = reserved.getUInt();

    sect->setBSS(rsect->type == RdfSection::RDF_BSS);
    sect->setCode(rsect->type == RdfSection::RDF_CODE);
}

void
RdfObject::AddLibOrModule(const llvm::StringRef& name, bool lib)
{
    llvm::StringRef name2 = name;
    if (name2.size() > MODLIB_NAME_MAX)
    {
        setWarn(WARN_GENERAL,
                String::Compose(N_("name too long, truncating to %1 bytes"),
                                MODLIB_NAME_MAX));
        name2 = name2.substr(0, MODLIB_NAME_MAX);
    }

    if (lib)
        m_library_names.push_back(name2);
    else
        m_module_names.push_back(name2);
}

void
RdfObject::DirLibrary(Object& object,
                      NameValues& namevals,
                      NameValues& objext_namevals,
                      unsigned long line)
{
    AddLibOrModule(namevals.front().getString(), true);
}

void
RdfObject::DirModule(Object& object,
                     NameValues& namevals,
                     NameValues& objext_namevals,
                     unsigned long line)
{
    AddLibOrModule(namevals.front().getString(), false);
}

void
RdfObject::AddDirectives(Directives& dirs, const llvm::StringRef& parser)
{
    static const Directives::Init<RdfObject> nasm_dirs[] =
    {
        {"section", &RdfObject::DirSection, Directives::ARG_REQUIRED},
        {"segment", &RdfObject::DirSection, Directives::ARG_REQUIRED},
        {"library", &RdfObject::DirLibrary, Directives::ARG_REQUIRED},
        {"module",  &RdfObject::DirModule,  Directives::ARG_REQUIRED},
    };

    if (String::NocaseEqual(parser, "nasm"))
        dirs.AddArray(this, nasm_dirs, NELEMS(nasm_dirs));
}

#if 0
static const char *rdf_nasm_stdmac[] = {
    "%imacro library 1+.nolist",
    "[library %1]",
    "%endmacro",
    "%imacro module 1+.nolist",
    "[module %1]",
    "%endmacro",
    NULL
};

static const yasm_stdmac rdf_objfmt_stdmacs[] = {
    { "nasm", "nasm", rdf_nasm_stdmac },
    { NULL, NULL, NULL }
};
#endif

void
DoRegister()
{
    RegisterModule<ObjectFormatModule,
                   ObjectFormatModuleImpl<RdfObject> >("rdf");
}

}}} // namespace yasm::objfmt::rdf

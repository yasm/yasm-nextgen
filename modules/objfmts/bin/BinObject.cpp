//
// Flat-format binary object format
//
//  Copyright (C) 2002-2007  Peter Johnson
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
#include <iostream>

#include <yasmx/Support/bitcount.h>
#include <yasmx/Support/Compose.h>
#include <yasmx/Support/errwarn.h>
#include <yasmx/Support/nocase.h>
#include <yasmx/Support/registry.h>
#include <yasmx/BytecodeOutput.h>
#include <yasmx/Bytecode.h>
#include <yasmx/Bytes.h>
#include <yasmx/Directive.h>
#include <yasmx/DirHelpers.h>
#include <yasmx/Errwarns.h>
#include <yasmx/Expr.h>
#include <yasmx/IntNum.h>
#include <yasmx/Object.h>
#include <yasmx/ObjectFormat.h>
#include <yasmx/NameValue.h>
#include <yasmx/Section.h>
#include <yasmx/Symbol.h>
#include <yasmx/Value.h>

#include "BinLink.h"
#include "BinMapOutput.h"
#include "BinSection.h"
#include "BinSymbol.h"


namespace yasm
{
namespace objfmt
{
namespace bin
{

class BinObject : public ObjectFormat
{
public:
    /// Constructor.
    BinObject(const ObjectFormatModule& module, Object& object);

    /// Destructor.
    ~BinObject();

    void AddDirectives(Directives& dirs, const char* parser);

    void Output(std::ostream& os, bool all_syms, Errwarns& errwarns);

    Section* AddDefaultSection();
    Section* AppendSection(const std::string& name, unsigned long line);

    static const char* getName() { return "Flat format binary"; }
    static const char* getKeyword() { return "bin"; }
    static const char* getExtension() { return ""; }
    static const unsigned int getDefaultX86ModeBits() { return 16; }
    static const char* getDefaultDebugFormatKeyword() { return "null"; }
    static std::vector<const char*> getDebugFormatKeywords();
    static bool isOkObject(Object& object) { return true; }
    static bool Taste(const llvm::MemoryBuffer& in,
                      /*@out@*/ std::string* arch_keyword,
                      /*@out@*/ std::string* machine)
    { return false; }

private:
    void DirSection(Object& object,
                    NameValues& namevals,
                    NameValues& objext_namevals,
                    unsigned long line);
    void DirOrg(Object& object,
                NameValues& namevals,
                NameValues& objext_namevals,
                unsigned long line);
    void DirMap(Object& object,
                NameValues& namevals,
                NameValues& objext_namevals,
                unsigned long line);
    bool setMapFilename(const NameValue& nv);

    void OutputMap(const IntNum& origin,
                   const BinGroups& groups,
                   Errwarns& errwarns) const;

    enum
    {
        NO_MAP = 0,
        MAP_NONE = 0x01,
        MAP_BRIEF = 0x02,
        MAP_SECTIONS = 0x04,
        MAP_SYMBOLS = 0x08
    };
    unsigned long m_map_flags;
    std::string m_map_filename;

    util::scoped_ptr<Expr> m_org;
    unsigned long m_org_line;
};

BinObject::BinObject(const ObjectFormatModule& module, Object& object)
    : ObjectFormat(module, object), m_map_flags(NO_MAP), m_org(0)
{
}

BinObject::~BinObject()
{
}

void
BinObject::OutputMap(const IntNum& origin,
                     const BinGroups& groups,
                     Errwarns& errwarns) const
{
    int map_flags = m_map_flags;

    if (map_flags == NO_MAP)
        return;

    if (map_flags == MAP_NONE)
        map_flags = MAP_BRIEF;          // default to brief

    std::ofstream os;
    if (m_map_filename.empty())
        os.std::basic_ios<char>::rdbuf(std::cout.rdbuf()); // stdout
    else
    {
        os.open(m_map_filename.c_str());
        if (os.fail())
        {
            setWarn(WARN_GENERAL, String::Compose(
                N_("unable to open map file `%1'"), m_map_filename));
            errwarns.Propagate(0);
            return;
        }
    }

    BinMapOutput out(os, m_object, origin, groups);
    out.OutputHeader();
    out.OutputOrigin();

    if (map_flags & MAP_BRIEF)
        out.OutputSectionsSummary();

    if (map_flags & MAP_SECTIONS)
        out.OutputSectionsDetail();

    if (map_flags & MAP_SYMBOLS)
        out.OutputSectionsSymbols();
}

class BinOutput : public BytecodeStreamOutput
{
public:
    BinOutput(std::ostream& os, Object& object);
    ~BinOutput();

    void OutputSection(Section& sect,
                       const IntNum& origin,
                       Errwarns& errwarns);

    // OutputBytecode overrides
    void ConvertValueToBytes(Value& value,
                             Bytes& bytes,
                             Location loc,
                             int warn);

private:
    Object& m_object;
    BytecodeNoOutput m_no_output;
};

BinOutput::BinOutput(std::ostream& os, Object& object)
    : BytecodeStreamOutput(os),
      m_object(object)
{
}

BinOutput::~BinOutput()
{
}

void
BinOutput::OutputSection(Section& sect,
                         const IntNum& origin,
                         Errwarns& errwarns)
{
    BytecodeOutput* outputter;

    if (sect.isBSS())
    {
        outputter = &m_no_output;
    }
    else
    {
        IntNum file_start = sect.getLMA();
        file_start -= origin;
        if (file_start.getSign() < 0)
        {
            errwarns.Propagate(0, ValueError(String::Compose(
                N_("section `%1' starts before origin (ORG)"),
                sect.getName())));
            return;
        }
        if (!file_start.isOkSize(sizeof(unsigned long)*8, 0, 0))
        {
            errwarns.Propagate(0, ValueError(String::Compose(
                N_("section `%1' start value too large"),
                sect.getName())));
            return;
        }
        m_os.seekp(file_start.getUInt());
        if (!m_os.good())
            throw Fatal(N_("could not seek on output file"));

        outputter = this;
    }

    for (Section::bc_iterator i=sect.bytecodes_begin(),
         end=sect.bytecodes_end(); i != end; ++i)
    {
        try
        {
            i->Output(*outputter);
        }
        catch (Error& err)
        {
            errwarns.Propagate(i->getLine(), err);
        }
        errwarns.Propagate(i->getLine());   // propagate warnings
    }
}

void
BinOutput::ConvertValueToBytes(Value& value,
                               Bytes& bytes,
                               Location loc,
                               int warn)
{
    // Binary objects we need to resolve against object, not against section.
    if (value.isRelative())
    {
        Location label_loc;
        IntNum ssymval;
        Expr syme;
        SymbolRef rel = value.getRelative();

        if (rel->isAbsoluteSymbol())
            syme = Expr(0);
        else if (rel->getLabel(&label_loc) && label_loc.bc->getContainer())
            syme = Expr(rel);
        else if (getBinSSymValue(*rel, &ssymval))
            syme = Expr(ssymval);
        else
            goto done;

        // Handle PC-relative
        if (value.getSubLocation(&label_loc) && label_loc.bc->getContainer())
            syme -= label_loc;

        if (value.getRShift() > 0)
            syme >>= IntNum(value.getRShift());

        // Add into absolute portion
        value.AddAbs(syme);
        value.ClearRelative();
    }
done:
    // Simplify absolute portion of value, transforming symrecs
    if (Expr* abs = value.getAbs())
    {
        BinSimplify(*abs);
        abs->Simplify();
    }

    // Output
    Arch* arch = m_object.getArch();
    if (value.OutputBasic(bytes, warn, *arch))
        return;

    // Couldn't output, assume it contains an external reference.
    throw Error(
        N_("binary object format does not support external references"));
}

static void
CheckSymbol(const Symbol& sym, Errwarns& errwarns)
{
    int vis = sym.getVisibility();

    // Don't check internally-generated symbols.  Only internally generated
    // symbols have symrec data, so simply check for its presence.
    if (sym.getAssocData<BinSymbol>())
        return;

    if (vis & Symbol::EXTERN)
    {
        setWarn(WARN_GENERAL,
            N_("binary object format does not support extern variables"));
        errwarns.Propagate(sym.getDeclLine());
    }
    else if (vis & Symbol::GLOBAL)
    {
        setWarn(WARN_GENERAL,
            N_("binary object format does not support global variables"));
        errwarns.Propagate(sym.getDeclLine());
    }
    else if (vis & Symbol::COMMON)
    {
        errwarns.Propagate(sym.getDeclLine(), TypeError(
            N_("binary object format does not support common variables")));
    }
}

void
BinObject::Output(std::ostream& os, bool all_syms, Errwarns& errwarns)
{
    // Set ORG to 0 unless otherwise specified
    IntNum origin(0);
    if (m_org.get() != 0)
    {
        m_org->Simplify();
        if (!m_org->isIntNum())
        {
            errwarns.Propagate(m_org_line,
                TooComplexError(N_("ORG expression is too complex")));
            return;
        }
        IntNum orgi = m_org->getIntNum();
        if (orgi.getSign() < 0)
        {
            errwarns.Propagate(m_org_line,
                ValueError(N_("ORG expression is negative")));
            return;
        }
        origin = orgi;
    }

    // Check symbol table
    for (Object::const_symbol_iterator i=m_object.symbols_begin(),
         end=m_object.symbols_end(); i != end; ++i)
        CheckSymbol(*i, errwarns);

    BinLink link(m_object, errwarns);

    if (!link.DoLink(origin))
        return;

    // Output map file
    OutputMap(origin, link.getLMAGroups(), errwarns);

    // Ensure we don't have overlapping progbits LMAs.
    if (!link.CheckLMAOverlap())
        return;

    // Output sections
    BinOutput out(os, m_object);
    for (Object::section_iterator i=m_object.sections_begin(),
         end=m_object.sections_end(); i != end; ++i)
    {
        out.OutputSection(*i, origin, errwarns);
    }
}

Section*
BinObject::AddDefaultSection()
{
    Section* section = AppendSection(".text", 0);
    section->setDefault(true);
    return section;
}

Section*
BinObject::AppendSection(const std::string& name, unsigned long line)
{
    bool bss = (name == ".bss");
    bool code = (name == ".text");
    Section* section = new Section(name, code, bss, line);
    m_object.AppendSection(std::auto_ptr<Section>(section));

    // Initialize section data and symbols.
    std::auto_ptr<BinSection> bsd(new BinSection());

    SymbolRef start = m_object.getSymbol("section."+name+".start");
    start->Declare(Symbol::EXTERN, line);
    start->AddAssocData(std::auto_ptr<BinSymbol>
        (new BinSymbol(*section, *bsd, BinSymbol::START)));

    SymbolRef vstart = m_object.getSymbol("section."+name+".vstart");
    vstart->Declare(Symbol::EXTERN, line);
    vstart->AddAssocData(std::auto_ptr<BinSymbol>
        (new BinSymbol(*section, *bsd, BinSymbol::VSTART)));

    SymbolRef length = m_object.getSymbol("section."+name+".length");
    length->Declare(Symbol::EXTERN, line);
    length->AddAssocData(std::auto_ptr<BinSymbol>
        (new BinSymbol(*section, *bsd, BinSymbol::LENGTH)));

    section->AddAssocData(std::auto_ptr<BinSection>(bsd.release()));

    return section;
}

void
BinObject::DirSection(Object& object,
                      NameValues& nvs,
                      NameValues& objext_nvs,
                      unsigned long line)
{
    assert(&object == &m_object);

    if (!nvs.front().isString())
        throw Error(N_("section name must be a string"));
    std::string sectname = nvs.front().getString();

    Section* sect = m_object.FindSection(sectname);
    bool first = true;
    if (sect)
        first = sect->isDefault();
    else
        sect = AppendSection(sectname, line);

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
    bool has_follows = false, has_vfollows = false;
    bool has_start = false, has_vstart = false;
    std::auto_ptr<Expr> start(0);
    std::auto_ptr<Expr> vstart(0);

    BinSection* bsd = sect->getAssocData<BinSection>();
    assert(bsd);
    unsigned long bss = sect->isBSS();
    unsigned long code = sect->isCode();

    DirHelpers helpers;
    helpers.Add("follows", true,
                BIND::bind(&DirString, _1, &bsd->follows, &has_follows));
    helpers.Add("vfollows", true,
                BIND::bind(&DirString, _1, &bsd->vfollows, &has_vfollows));
    helpers.Add("start", true,
                BIND::bind(&DirExpr, _1, &m_object, line, &start, &has_start));
    helpers.Add("vstart", true,
                BIND::bind(&DirExpr, _1, &m_object, line, &vstart,
                           &has_vstart));
    helpers.Add("align", true,
                BIND::bind(&DirIntNum, _1, &m_object, line, &bsd->align,
                           &bsd->has_align));
    helpers.Add("valign", true,
                BIND::bind(&DirIntNum, _1, &m_object, line, &bsd->valign,
                           &bsd->has_valign));
    helpers.Add("nobits", false, BIND::bind(&DirSetFlag, _1, &bss, 1));
    helpers.Add("progbits", false, BIND::bind(&DirClearFlag, _1, &bss, 1));
    helpers.Add("code", false, BIND::bind(&DirSetFlag, _1, &code, 1));
    helpers.Add("data", false, BIND::bind(&DirClearFlag, _1, &code, 1));
    helpers.Add("execute", false, BIND::bind(&DirSetFlag, _1, &code, 1));
    helpers.Add("noexecute", false, BIND::bind(&DirClearFlag, _1, &code, 1));

    helpers(++nvs.begin(), nvs.end(), DirNameValueWarn);

    if (start.get() != 0)
    {
        bsd->start.reset(start.release());
        bsd->start_line = line;
    }
    if (vstart.get() != 0)
    {
        bsd->vstart.reset(vstart.release());
        bsd->vstart_line = line;
    }

    if (bsd->start.get() != 0 && !bsd->follows.empty())
    {
        throw Error(
            N_("cannot combine `start' and `follows' section attributes"));
    }

    if (bsd->vstart.get() != 0 && !bsd->vfollows.empty())
    {
        throw Error(
            N_("cannot combine `vstart' and `vfollows' section attributes"));
    }

    if (bsd->has_align)
    {
        unsigned long align = bsd->align.getUInt();

        // Alignments must be a power of two.
        if (!isExp2(align))
        {
            throw ValueError(String::Compose(
                N_("argument to `%1' is not a power of two"), "align"));
        }
    }

    if (bsd->has_valign)
    {
        unsigned long valign = bsd->valign.getUInt();

        // Alignments must be a power of two.
        if (!isExp2(valign))
        {
            throw ValueError(String::Compose(
                N_("argument to `%1' is not a power of two"), "valign"));
        }
    }

    sect->setBSS(bss);
    sect->setCode(code);
}

void
BinObject::DirOrg(Object& object,
                  NameValues& namevals,
                  NameValues& objext_namevals,
                  unsigned long line)
{
    // We only allow a single ORG in a program.
    if (m_org.get() != 0)
        throw Error(N_("program origin redefined"));

    // ORG takes just a simple expression as param
    const NameValue& nv = namevals.front();
    if (!nv.isExpr())
        throw SyntaxError(N_("argument to ORG must be expression"));
    m_org.reset(new Expr(nv.getExpr(object, line)));
    m_org_line = line;
}

bool
BinObject::setMapFilename(const NameValue& nv)
{
    if (!m_map_filename.empty())
        throw Error(N_("map file already specified"));

    if (!nv.isString())
        throw SyntaxError(N_("unexpected expression in [map]"));
    m_map_filename = nv.getString();
    return true;
}

void
BinObject::DirMap(Object& object,
                  NameValues& namevals,
                  NameValues& objext_namevals,
                  unsigned long line)
{
    DirHelpers helpers;
    helpers.Add("all", false,
                BIND::bind(&DirSetFlag, _1, &m_map_flags,
                           MAP_BRIEF|MAP_SECTIONS|MAP_SYMBOLS));
    helpers.Add("brief", false,
                BIND::bind(&DirSetFlag, _1, &m_map_flags,
                           static_cast<unsigned long>(MAP_BRIEF)));
    helpers.Add("sections", false,
                BIND::bind(&DirSetFlag, _1, &m_map_flags,
                           static_cast<unsigned long>(MAP_SECTIONS)));
    helpers.Add("segments", false,
                BIND::bind(&DirSetFlag, _1, &m_map_flags,
                           static_cast<unsigned long>(MAP_SECTIONS)));
    helpers.Add("symbols", false,
                BIND::bind(&DirSetFlag, _1, &m_map_flags,
                           static_cast<unsigned long>(MAP_SYMBOLS)));

    m_map_flags |= MAP_NONE;

    helpers(namevals.begin(), namevals.end(),
            BIND::bind(&BinObject::setMapFilename, this, _1));
}

std::vector<const char*>
BinObject::getDebugFormatKeywords()
{
    static const char* keywords[] = {"null"};
    return std::vector<const char*>(keywords, keywords+NELEMS(keywords));
}

void
BinObject::AddDirectives(Directives& dirs, const char* parser)
{
    static const Directives::Init<BinObject> nasm_dirs[] =
    {
        {"section", &BinObject::DirSection, Directives::ARG_REQUIRED},
        {"segment", &BinObject::DirSection, Directives::ARG_REQUIRED},
        {"org",     &BinObject::DirOrg, Directives::ARG_REQUIRED},
        {"map",     &BinObject::DirMap, Directives::ANY},
    };
    static const Directives::Init<BinObject> gas_dirs[] =
    {
        {".section", &BinObject::DirSection, Directives::ARG_REQUIRED},
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
                   ObjectFormatModuleImpl<BinObject> >("bin");
}

}}} // namespace yasm::objfmt::bin

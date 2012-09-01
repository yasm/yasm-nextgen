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

#include <vector>

#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Parse/Directive.h"
#include "yasmx/Parse/DirHelpers.h"
#include "yasmx/Parse/NameValue.h"
#include "yasmx/Support/bitcount.h"
#include "yasmx/Support/registry.h"
#include "yasmx/Arch.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Object.h"
#include "yasmx/Object_util.h"
#include "yasmx/Value.h"

#include "CoffSection.h"
#include "CoffSymbol.h"


using namespace yasm;
using namespace yasm::objfmt;

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
    , m_def_sym(0)
{
    // Support x86 and amd64 machines of x86 arch
    if (m_object.getArch()->getMachine().equals_lower("x86"))
        m_machine = MACHINE_I386;
    else if (m_object.getArch()->getMachine().equals_lower("amd64"))
        m_machine = MACHINE_AMD64;
}

std::vector<StringRef>
CoffObject::getDebugFormatKeywords()
{
    static const char* keywords[] =
        {"null", "dwarf", "dwarfpass", "dwarf2", "dwarf2pass"};
    size_t keywords_size = sizeof(keywords)/sizeof(keywords[0]);
    return std::vector<StringRef>(keywords, keywords+keywords_size);
}

bool
CoffObject::isOkObject(Object& object)
{
    // Support x86 and amd64 machines of x86 arch
    Arch* arch = object.getArch();
    if (!arch->getModule().getKeyword().equals_lower("x86"))
        return false;

    if (arch->getMachine().equals_lower("x86"))
        return true;
    if (arch->getMachine().equals_lower("amd64"))
        return true;
    return false;
}

void
CoffObject::InitSymbols(StringRef parser)
{
    // Add .file symbol
    SymbolRef filesym = m_object.AppendSymbol(".file");
    filesym->DefineSpecial(Symbol::GLOBAL);

    std::auto_ptr<CoffSymbol> coffsym(
        new CoffSymbol(CoffSymbol::SCL_FILE, CoffSymbol::AUX_FILE));
    m_file_coffsym = coffsym.get();

    filesym->AddAssocData(coffsym);
}

CoffObject::~CoffObject()
{
}

Section*
CoffObject::AddDefaultSection()
{
    IntrusiveRefCntPtr<DiagnosticIDs> diagids(new DiagnosticIDs);
    DiagnosticsEngine diags(diagids);
    Section* section = AppendSection(".text", SourceLocation(), diags);
    section->setDefault(true);
    return section;
}

bool
CoffObject::InitSection(StringRef name,
                        Section& section,
                        CoffSection* coffsect,
                        SourceLocation source,
                        DiagnosticsEngine& diags)
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
             || name.startswith(".rodata") || name.startswith(".rdata$"))
    {
        flags = CoffSection::DATA;
        diags.Report(source, diag::warn_coff_no_readonly_sections);
    }
    else if (name == ".drectve")
        flags = CoffSection::INFO;
    else if (name == ".comment")
        flags = CoffSection::INFO;
    else if (name.substr(0, 6).equals_lower(".debug"))
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
CoffObject::AppendSection(StringRef name,
                          SourceLocation source,
                          DiagnosticsEngine& diags)
{
    Section* section = new Section(name, false, false, source);
    m_object.AppendSection(std::auto_ptr<Section>(section));

    // Define a label for the start of the section
    Location start = {&section->bytecodes_front(), 0};
    SymbolRef sym = m_object.getSymbol(name);
    if (!sym->isDefined())
    {
        sym->DefineLabel(start);
        sym->setDefSource(source);
    }
    sym->Declare(Symbol::GLOBAL);
    sym->setDeclSource(source);
    sym->AddAssocData(
        std::auto_ptr<CoffSymbol>(new CoffSymbol(CoffSymbol::SCL_STAT,
                                                CoffSymbol::AUX_SECT)));
    section->setSymbol(sym);

    // Add COFF data to the section
    CoffSection* coffsect = new CoffSection(sym);
    section->AddAssocData(std::auto_ptr<CoffSection>(coffsect));
    InitSection(name, *section, coffsect, source, diags);

    return section;
}

void
CoffObject::DirGasSection(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    assert(info.isObject(m_object));
    NameValues& nvs = info.getNameValues();

    NameValue& sectname_nv = nvs.front();
    if (!sectname_nv.isString())
    {
        diags.Report(sectname_nv.getValueRange().getBegin(),
                     diag::err_value_string_or_id);
        return;
    }
    StringRef sectname = sectname_nv.getString();

    if (sectname.size() > 8 && !m_win32)
    {
        // win32 format supports >8 character section names in object
        // files via "/nnnn" (where nnnn is decimal offset into string table),
        // so only warn for regular COFF.
        diags.Report(sectname_nv.getValueRange().getBegin(),
                     diag::warn_coff_section_name_length);
        sectname = sectname.substr(0, 8);
    }

    Section* sect = m_object.FindSection(sectname);
    bool first = true;
    if (sect)
        first = sect->isDefault();
    else
        sect = AppendSection(sectname, info.getSource(), diags);

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
    NameValue& flags_nv = nvs[1];
    if (!flags_nv.isString())
    {
        diags.Report(flags_nv.getValueRange().getBegin(),
                     diag::err_expected_flag_string);
        return;
    }

    // Parse section flags
    bool alloc = false, load = false, readonly = false, code = false;
    bool datasect = false, shared = false;
    StringRef flagstr = flags_nv.getString();

    for (size_t i=0; i<flagstr.size(); ++i)
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
            {
                char print_flag[2] = {flagstr[i], 0};
                diags.Report(flags_nv.getValueRange().getBegin()
                             .getLocWithOffset(i),
                             diag::warn_unrecognized_section_attribute)
                    << print_flag;
            }
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
                                  bool* has_align)
{
    helpers.Add("code", false,
                TR1::bind(&DirResetFlag, _1, _2, &coffsect->m_flags,
                          CoffSection::TEXT |
                          CoffSection::EXECUTE |
                          CoffSection::READ));
    helpers.Add("text", false,
                TR1::bind(&DirResetFlag, _1, _2, &coffsect->m_flags,
                          CoffSection::TEXT |
                          CoffSection::EXECUTE |
                          CoffSection::READ));
    helpers.Add("data", false,
                TR1::bind(&DirResetFlag, _1, _2, &coffsect->m_flags,
                          CoffSection::DATA |
                          CoffSection::READ |
                          CoffSection::WRITE));
    helpers.Add("rdata", false,
                TR1::bind(&DirResetFlag, _1, _2, &coffsect->m_flags,
                          CoffSection::DATA | CoffSection::READ));
    helpers.Add("bss", false,
                TR1::bind(&DirResetFlag, _1, _2, &coffsect->m_flags,
                          CoffSection::BSS |
                          CoffSection::READ |
                          CoffSection::WRITE));
    helpers.Add("info", false,
                TR1::bind(&DirResetFlag, _1, _2, &coffsect->m_flags,
                          CoffSection::INFO |
                          CoffSection::DISCARD |
                          CoffSection::READ));
    helpers.Add("align", true,
                TR1::bind(&DirIntNumPower2, _1, _2, &m_object, align,
                          has_align));
}

void
CoffObject::DirSection(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    assert(info.isObject(m_object));
    NameValues& nvs = info.getNameValues();

    NameValue& sectname_nv = nvs.front();
    if (!sectname_nv.isString())
    {
        diags.Report(sectname_nv.getValueRange().getBegin(),
                     diag::err_value_string_or_id);
        return;
    }
    StringRef sectname = sectname_nv.getString();

    if (sectname.size() > 8 && !m_win32)
    {
        // win32 format supports >8 character section names in object
        // files via "/nnnn" (where nnnn is decimal offset into string table),
        // so only warn for regular COFF.
        diags.Report(sectname_nv.getValueRange().getBegin(),
                     diag::warn_coff_section_name_length);
        sectname = sectname.substr(0, 8);
    }

    Section* sect = m_object.FindSection(sectname);
    bool first = true;
    if (sect)
        first = sect->isDefault();
    else
        sect = AppendSection(sectname, info.getSource(), diags);

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
        diags.Report(info.getSource(), diag::warn_section_redef_flags);
        return;
    }

    // Parse section flags
    IntNum align;
    bool has_align = false;

    DirHelpers helpers;
    DirSectionInitHelpers(helpers, coffsect, &align, &has_align);
    helpers(++nvs.begin(), nvs.end(), info.getSource(), diags,
            DirNameValueWarn);

    sect->setBSS((coffsect->m_flags & CoffSection::BSS) != 0);
    sect->setCode((coffsect->m_flags & CoffSection::EXECUTE) != 0);

    if (!m_win32)
        coffsect->m_flags &= ~CoffSection::WIN32_MASK;

    if (has_align)
    {
        unsigned long aligni = align.getUInt();

        // Check to see if alignment is supported size
        // FIXME: Use actual value source location
        if (aligni > 8192)
            diags.Report(info.getSource(), diag::err_win32_align_too_big);

        sect->setAlign(aligni);
    }
}

void
CoffObject::DirIdent(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    assert(info.isObject(m_object));
    DirIdentCommon(*this, ".comment", info, diags);
}

void
CoffObject::DirGasDef(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (m_def_sym)
    {
        diags.Report(info.getSource(), diag::warn_nested_def);
        return;
    }

    NameValue& symname_nv = info.getNameValues().front();
    if (!symname_nv.isId())
    {
        diags.Report(symname_nv.getValueRange().getBegin(),
                     diag::err_value_id);
        return;
    }
    StringRef symname = symname_nv.getId();
    SymbolRef sym = m_object.getSymbol(symname);

    std::auto_ptr<CoffSymbol> coffsym(new CoffSymbol(CoffSymbol::SCL_NULL));
    m_def_sym = coffsym.get();

    sym->AddAssocData(coffsym);
}

void
CoffObject::DirGasScl(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!m_def_sym)
    {
        diags.Report(info.getSource(), diag::warn_outside_def) << ".scl";
        return;
    }

    IntNum val;
    bool val_set = false;
    DirIntNum(info.getNameValues().front(), diags, &m_object, &val, &val_set);
    if (!val_set)
        return;
    m_def_sym->m_sclass = static_cast<CoffSymbol::StorageClass>(val.getUInt());
}

void
CoffObject::DirGasType(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!m_def_sym)
    {
        diags.Report(info.getSource(), diag::warn_outside_def) << ".type";
        return;
    }

    IntNum val;
    bool val_set = false;
    DirIntNum(info.getNameValues().front(), diags, &m_object, &val, &val_set);
    if (!val_set)
        return;
    m_def_sym->m_type = val.getUInt();
}

void
CoffObject::DirGasEndef(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!m_def_sym)
    {
        diags.Report(info.getSource(), diag::warn_endef_before_def);
        return;
    }
    m_def_sym = NULL;
}

void
CoffObject::DirSecRel32(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    NameValues& nvs = info.getNameValues();
    for (NameValues::iterator i=nvs.begin(), end=nvs.end(); i != end; ++i)
    {
        std::auto_ptr<Value> val(new Value(32, i->ReleaseExpr(m_object)));
        val->setSource(i->getValueRange().getBegin());
        val->setSectionRelative();
        Bytecode& bc = m_object.getCurSection()->FreshBytecode();
        bc.AppendFixed(val);
    }
}

void
CoffObject::AddDirectives(Directives& dirs, StringRef parser)
{
    static const Directives::Init<CoffObject> nasm_dirs[] =
    {
        {"section", &CoffObject::DirSection, Directives::ARG_REQUIRED},
        {"segment", &CoffObject::DirSection, Directives::ARG_REQUIRED},
        {"ident",   &CoffObject::DirIdent, Directives::ANY},
    };
    static const Directives::Init<CoffObject> gas_dirs[] =
    {
        {".section",&CoffObject::DirGasSection, Directives::ARG_REQUIRED},
        {".ident",  &CoffObject::DirIdent,      Directives::ANY},
        {".def",    &CoffObject::DirGasDef,     Directives::ID_REQUIRED},
        {".scl",    &CoffObject::DirGasScl,     Directives::ARG_REQUIRED},
        {".type",   &CoffObject::DirGasType,    Directives::ARG_REQUIRED},
        {".endef",  &CoffObject::DirGasEndef,   Directives::ANY},
        {".secrel32", &CoffObject::DirSecRel32, Directives::ARG_REQUIRED},
    };

    if (parser.equals_lower("nasm"))
        dirs.AddArray(this, nasm_dirs);
    else if (parser.equals_lower("gas") || parser.equals_lower("gnu"))
        dirs.AddArray(this, gas_dirs);
}

void
yasm_objfmt_coff_DoRegister()
{
    RegisterModule<ObjectFormatModule,
                   ObjectFormatModuleImpl<CoffObject> >("coff");
}

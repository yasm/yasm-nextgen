//
// Win32 object format
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
#include "Win32Object.h"

#include <vector>

#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Parse/Directive.h"
#include "yasmx/Parse/DirHelpers.h"
#include "yasmx/Parse/NameValue.h"
#include "yasmx/Support/registry.h"
#include "yasmx/BytecodeContainer.h"
#include "yasmx/Object.h"
#include "yasmx/Symbol.h"

#include "modules/objfmts/coff/CoffSection.h"
#include "modules/objfmts/coff/CoffSymbol.h"
#include "SxData.h"


using namespace yasm;
using namespace yasm::objfmt;

Win32Object::Win32Object(const ObjectFormatModule& module, Object& object)
    : CoffObject(module, object, false, true)
{
}

Win32Object::~Win32Object()
{
}

std::vector<StringRef>
Win32Object::getDebugFormatKeywords()
{
    static const char* keywords[] =
        {"null", "dwarf", "dwarfpass", "dwarf2", "dwarf2pass", "cv8"};
    size_t keywords_size = sizeof(keywords)/sizeof(keywords[0]);
    return std::vector<StringRef>(keywords, keywords+keywords_size);
}

void
Win32Object::InitSymbols(StringRef parser)
{
    CoffObject::InitSymbols(parser);

    // Define a @feat.00 symbol to notify linker about safeseh handling.
    if (!isWin64())
    {
        SymbolRef feat00 = m_object.AppendSymbol("@feat.00");
        feat00->DefineEqu(Expr(IntNum(1)));

        std::auto_ptr<CoffSymbol>
            coffsym(new CoffSymbol(CoffSymbol::SCL_STAT));
        coffsym->m_forcevis = true;

        feat00->AddAssocData(coffsym);
    }
}

static inline void
setBool(NameValue& nv, DiagnosticsEngine& diags, bool* out, bool value)
{
    *out = value;
}

void
Win32Object::DirSectionInitHelpers(DirHelpers& helpers,
                                   CoffSection* csd,
                                   IntNum* align,
                                   bool* has_align)
{
    CoffObject::DirSectionInitHelpers(helpers, csd, align, has_align);

    helpers.Add("discard", false,
                TR1::bind(&DirSetFlag, _1, _2, &csd->m_flags,
                          CoffSection::DISCARD));
    helpers.Add("nodiscard", false,
                TR1::bind(&DirClearFlag, _1, _2, &csd->m_flags,
                          CoffSection::DISCARD));
    helpers.Add("cache", false,
                TR1::bind(&DirClearFlag, _1, _2, &csd->m_flags,
                          CoffSection::NOCACHE));
    helpers.Add("nocache", false,
                TR1::bind(&DirSetFlag, _1, _2, &csd->m_flags,
                          CoffSection::NOCACHE));
    helpers.Add("page", false,
                TR1::bind(&DirClearFlag, _1, _2, &csd->m_flags,
                          CoffSection::NOPAGE));
    helpers.Add("nopage", false,
                TR1::bind(&DirSetFlag, _1, _2, &csd->m_flags,
                          CoffSection::NOPAGE));
    helpers.Add("share", false,
                TR1::bind(&DirSetFlag, _1, _2, &csd->m_flags,
                          CoffSection::SHARED));
    helpers.Add("noshare", false,
                TR1::bind(&DirClearFlag, _1, _2, &csd->m_flags,
                          CoffSection::SHARED));
    helpers.Add("execute", false,
                TR1::bind(&DirSetFlag, _1, _2, &csd->m_flags,
                          CoffSection::EXECUTE));
    helpers.Add("noexecute", false,
                TR1::bind(&DirClearFlag, _1, _2, &csd->m_flags,
                          CoffSection::EXECUTE));
    helpers.Add("read", false,
                TR1::bind(&DirSetFlag, _1, _2, &csd->m_flags,
                          CoffSection::READ));
    helpers.Add("noread", false,
                TR1::bind(&DirClearFlag, _1, _2, &csd->m_flags,
                          CoffSection::READ));
    helpers.Add("write", false,
                TR1::bind(&DirSetFlag, _1, _2, &csd->m_flags,
                          CoffSection::WRITE));
    helpers.Add("nowrite", false,
                TR1::bind(&DirClearFlag, _1, _2, &csd->m_flags,
                          CoffSection::WRITE));
    helpers.Add("base", false,
                TR1::bind(&setBool, _1, _2, &csd->m_nobase, false));
    helpers.Add("nobase", false,
                TR1::bind(&setBool, _1, _2, &csd->m_nobase, true));
}

void
Win32Object::DirExport(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    assert(info.isObject(m_object));
    NameValues& namevals = info.getNameValues();

    NameValue& name_nv = namevals.front();
    if (!name_nv.isId())
    {
        diags.Report(info.getSource(), diag::err_value_id)
            << name_nv.getValueRange();
        return;
    }
    StringRef symname = name_nv.getId();

    // Reference exported symbol (to generate error if not declared)
    m_object.getSymbol(symname)->Use(info.getSource());

    // Add to end of linker directives, creating directive section if needed.
    Section* sect = m_object.FindSection(".drectve");
    if (!sect)
        sect = AppendSection(".drectve", info.getSource(), diags);

    // Add text to end of section
    AppendData(*sect, "-export:", 1, false);
    AppendData(*sect, symname, 1, false);
    AppendByte(*sect, ' ');
}

void
Win32Object::DirSafeSEH(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    assert(info.isObject(m_object));
    NameValues& namevals = info.getNameValues();
    SourceLocation source = info.getSource();

    NameValue& name_nv = namevals.front();
    if (!name_nv.isId())
    {
        diags.Report(info.getSource(), diag::err_value_id)
            << name_nv.getValueRange();
        return;
    }
    StringRef symname = name_nv.getId();

    // Reference symbol (to generate error if not declared)
    SymbolRef sym = m_object.getSymbol(symname);
    sym->Use(source);

    // Symbol must be externally visible and have a type of 0x20 (function)
    CoffSymbol* coffsym = sym->getAssocData<CoffSymbol>();
    if (!coffsym)
    {
        coffsym = new CoffSymbol(CoffSymbol::SCL_NULL);
        sym->AddAssocData(std::auto_ptr<CoffSymbol>(coffsym));
    }
    coffsym->m_forcevis = true;
    coffsym->m_type = 0x20;

    // Add symbol number to end of .sxdata section (creating if necessary)
    Section* sect = m_object.FindSection(".sxdata");
    if (!sect)
        sect = AppendSection(".sxdata", source, diags);

    AppendSxData(*sect, sym, source);
}

void
Win32Object::AddDirectives(Directives& dirs, StringRef parser)
{
    static const Directives::Init<Win32Object> gas_dirs[] =
    {
        { ".export",  &Win32Object::DirExport,  Directives::ID_REQUIRED },
        { ".safeseh", &Win32Object::DirSafeSEH, Directives::ID_REQUIRED },
    };
    static const Directives::Init<Win32Object> nasm_dirs[] =
    {
        { "export",  &Win32Object::DirExport,  Directives::ID_REQUIRED },
        { "safeseh", &Win32Object::DirSafeSEH, Directives::ID_REQUIRED },
    };

    if (parser.equals_lower("nasm"))
        dirs.AddArray(this, nasm_dirs);
    else if (parser.equals_lower("gas") || parser.equals_lower("gnu"))
        dirs.AddArray(this, gas_dirs);

    // Pull in coff directives
    CoffObject::AddDirectives(dirs, parser);
}

bool
Win32Object::InitSection(StringRef name,
                         Section& section,
                         CoffSection* coffsect,
                         SourceLocation source,
                         DiagnosticsEngine& diags)
{
    if (name == ".data")
    {
        coffsect->m_flags =
            CoffSection::DATA | CoffSection::READ | CoffSection::WRITE;
        if (getMachine() == MACHINE_AMD64)
            section.setAlign(16);
        else
            section.setAlign(4);
    }
    else if (name == ".bss")
    {
        coffsect->m_flags =
            CoffSection::BSS | CoffSection::READ | CoffSection::WRITE;
        if (getMachine() == MACHINE_AMD64)
            section.setAlign(16);
        else
            section.setAlign(4);
        section.setBSS(true);
    }
    else if (name == ".text")
    {
        coffsect->m_flags =
            CoffSection::TEXT | CoffSection::EXECUTE | CoffSection::READ;
        section.setAlign(16);
        section.setCode(true);
    }
    else if (name == ".rdata"
             || name.startswith(".rodata") || name.startswith(".rdata$"))
    {
        coffsect->m_flags = CoffSection::DATA | CoffSection::READ;
        section.setAlign(8);
    }
    else if (name == ".drectve")
    {
        coffsect->m_flags =
            CoffSection::INFO | CoffSection::DISCARD | CoffSection::READ;
    }
    else if (name == ".sxdata")
        coffsect->m_flags = CoffSection::INFO;
    else if (name == ".comment")
    {
        coffsect->m_flags =
            CoffSection::INFO | CoffSection::DISCARD | CoffSection::READ;
    }
    else if (name.substr(0, 6).equals_lower(".debug"))
    {
        coffsect->m_flags =
            CoffSection::DATA | CoffSection::DISCARD | CoffSection::READ;
        section.setAlign(1);
    }
    else
    {
        // Default to code (NASM default; note GAS has different default.
        coffsect->m_flags =
            CoffSection::TEXT | CoffSection::EXECUTE | CoffSection::READ;
        section.setAlign(16);
        section.setCode(true);
        return false;
    }
    return true;
}

#if 0
static const char *win32_nasm_stdmac[] =
{
    "%imacro export 1+.nolist",
    "[export %1]",
    "%endmacro",
    "%imacro safeseh 1+.nolist",
    "[safeseh %1]",
    "%endmacro",
    NULL
};

static const yasm_stdmac win32_objfmt_stdmacs[] =
{
    { "nasm", "nasm", win32_nasm_stdmac },
    { NULL, NULL, NULL }
};
#endif

void
yasm_objfmt_win32_DoRegister()
{
    RegisterModule<ObjectFormatModule,
                   ObjectFormatModuleImpl<Win32Object> >("win32");
}

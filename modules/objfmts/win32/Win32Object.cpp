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

#include <util.h>

#include <vector>

#include <yasmx/Support/nocase.h>
#include <yasmx/Support/registry.h>
#include <yasmx/BytecodeContainer_util.h>
#include <yasmx/Directive.h>
#include <yasmx/DirHelpers.h>
#include <yasmx/errwarn.h>
#include <yasmx/Object.h>
#include <yasmx/NameValue.h>
#include <yasmx/Symbol.h>

#include "modules/objfmts/coff/CoffSection.h"
#include "SxData.h"


namespace yasm
{
namespace objfmt
{
namespace win32
{

Win32Object::Win32Object()
    : CoffObject(false, true)
{
}

Win32Object::~Win32Object()
{
}

std::string
Win32Object::get_name() const
{
    return "Win32";
}

std::string
Win32Object::get_keyword() const
{
    return "win32";
}

std::string
Win32Object::get_extension() const
{
    return ".obj";
}

unsigned int
Win32Object::get_default_x86_mode_bits() const
{
    return 32;
}

std::vector<std::string>
Win32Object::get_dbgfmt_keywords() const
{
    static const char* keywords[] = {"null", "dwarf2", "cv8"};
    return std::vector<std::string>(keywords, keywords+NELEMS(keywords));
}

static inline void
set_bool(NameValue& nv, bool* out, bool value)
{
    *out = value;
}

void
Win32Object::dir_section_init_helpers(DirHelpers& helpers,
                                      CoffSection* csd,
                                      IntNum* align,
                                      bool* has_align,
                                      unsigned long line)
{
    CoffObject::dir_section_init_helpers(helpers, csd, align, has_align, line);

    helpers.add("discard", false,
                BIND::bind(&dir_flag_set, _1, &csd->m_flags,
                           CoffSection::DISCARD));
    helpers.add("nodiscard", false,
                BIND::bind(&dir_flag_clear, _1, &csd->m_flags,
                           CoffSection::DISCARD));
    helpers.add("cache", false,
                BIND::bind(&dir_flag_clear, _1, &csd->m_flags,
                           CoffSection::NOCACHE));
    helpers.add("nocache", false,
                BIND::bind(&dir_flag_set, _1, &csd->m_flags,
                           CoffSection::NOCACHE));
    helpers.add("page", false,
                BIND::bind(&dir_flag_clear, _1, &csd->m_flags,
                           CoffSection::NOPAGE));
    helpers.add("nopage", false,
                BIND::bind(&dir_flag_set, _1, &csd->m_flags,
                           CoffSection::NOPAGE));
    helpers.add("share", false,
                BIND::bind(&dir_flag_set, _1, &csd->m_flags,
                           CoffSection::SHARED));
    helpers.add("noshare", false,
                BIND::bind(&dir_flag_clear, _1, &csd->m_flags,
                           CoffSection::SHARED));
    helpers.add("execute", false,
                BIND::bind(&dir_flag_set, _1, &csd->m_flags,
                           CoffSection::EXECUTE));
    helpers.add("noexecute", false,
                BIND::bind(&dir_flag_clear, _1, &csd->m_flags,
                           CoffSection::EXECUTE));
    helpers.add("read", false,
                BIND::bind(&dir_flag_set, _1, &csd->m_flags,
                           CoffSection::READ));
    helpers.add("noread", false,
                BIND::bind(&dir_flag_clear, _1, &csd->m_flags,
                           CoffSection::READ));
    helpers.add("write", false,
                BIND::bind(&dir_flag_set, _1, &csd->m_flags,
                           CoffSection::WRITE));
    helpers.add("nowrite", false,
                BIND::bind(&dir_flag_clear, _1, &csd->m_flags,
                           CoffSection::WRITE));
    helpers.add("base", false,
                BIND::bind(&set_bool, _1, &csd->m_nobase, false));
    helpers.add("nobase", false,
                BIND::bind(&set_bool, _1, &csd->m_nobase, true));
}

void
Win32Object::dir_export(Object& object,
                        NameValues& namevals,
                        NameValues& objext_namevals,
                        unsigned long line)
{
    if (!namevals.front().is_id())
        throw SyntaxError(N_("argument to EXPORT must be symbol name"));
    std::string symname = namevals.front().get_id();

    // Reference exported symbol (to generate error if not declared)
    m_object->get_symbol(symname)->use(line);

    // Add to end of linker directives, creating directive section if needed.
    Section* sect = m_object->find_section(".drectve");
    if (!sect)
        sect = append_section(".drectve", line);

    // Add text to end of section
    append_data(*sect, "-export:", 1, false);
    append_data(*sect, symname, 1, false);
    append_byte(*sect, ' ');
}

void
Win32Object::dir_safeseh(Object& object,
                         NameValues& namevals,
                         NameValues& objext_namevals,
                         unsigned long line)
{
    if (!namevals.front().is_id())
        throw SyntaxError(N_("argument to SAFESEH must be symbol name"));
    std::string symname = namevals.front().get_id();

    // Reference symbol (to generate error if not declared)
    SymbolRef sym = m_object->get_symbol(symname);
    sym->use(line);

    // Symbol must be externally visible, so force global.
    sym->declare(Symbol::GLOBAL, line);

    // Add symbol number to end of .sxdata section (creating if necessary)
    Section* sect = m_object->find_section(".sxdata");
    if (!sect)
        sect = append_section(".sxdata", line);

    append_sxdata(*sect, sym, line);
}

void
Win32Object::add_directives(Directives& dirs, const std::string& parser)
{
    static const Directives::Init<Win32Object> gas_dirs[] =
    {
        { ".export",  &Win32Object::dir_export,  Directives::ID_REQUIRED },
        { ".safeseh", &Win32Object::dir_safeseh, Directives::ID_REQUIRED },
    };
    static const Directives::Init<Win32Object> nasm_dirs[] =
    {
        { "export",  &Win32Object::dir_export,  Directives::ID_REQUIRED },
        { "safeseh", &Win32Object::dir_safeseh, Directives::ID_REQUIRED },
    };

    if (String::nocase_equal(parser, "nasm"))
        dirs.add_array(this, nasm_dirs, NELEMS(nasm_dirs));
    else if (String::nocase_equal(parser, "gas"))
        dirs.add_array(this, gas_dirs, NELEMS(gas_dirs));

    // Pull in coff directives
    CoffObject::add_directives(dirs, parser);
}

bool
Win32Object::init_section(const std::string& name,
                          Section& section,
                          CoffSection* coffsect)
{
    if (name == ".data")
    {
        coffsect->m_flags =
            CoffSection::DATA | CoffSection::READ | CoffSection::WRITE;
        if (get_machine() == MACHINE_AMD64)
            section.set_align(16);
        else
            section.set_align(4);
    }
    else if (name == ".bss")
    {
        coffsect->m_flags =
            CoffSection::BSS | CoffSection::READ | CoffSection::WRITE;
        if (get_machine() == MACHINE_AMD64)
            section.set_align(16);
        else
            section.set_align(4);
        section.set_bss(true);
    }
    else if (name == ".text")
    {
        coffsect->m_flags =
            CoffSection::TEXT | CoffSection::EXECUTE | CoffSection::READ;
        section.set_align(16);
        section.set_code(true);
    }
    else if (name == ".rdata"
             || (name.length() >= 7 && name.compare(0, 7, ".rodata") == 0)
             || (name.length() >= 7 && name.compare(0, 7, ".rdata$") == 0))
    {
        coffsect->m_flags = CoffSection::DATA | CoffSection::READ;
        section.set_align(8);
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
    else if (String::nocase_equal(name, ".debug", 6))
    {
        coffsect->m_flags =
            CoffSection::DATA | CoffSection::DISCARD | CoffSection::READ;
        section.set_align(1);
    }
    else
    {
        // Default to code (NASM default; note GAS has different default.
        coffsect->m_flags =
            CoffSection::TEXT | CoffSection::EXECUTE | CoffSection::READ;
        section.set_align(16);
        section.set_code(true);
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
do_register()
{
    register_module<ObjectFormat, Win32Object>("win32");
}

}}} // namespace yasm::objfmt::win32

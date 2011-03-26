//
// Mac OS X ABI Mach-O File Format
//
//  Copyright (C) 2007 Henryk Richter, built upon xdf objfmt (C) Peter Johnson
//  Copyright (C) 2010 Peter Johnson
//
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
// notes: This implementation is rather basic. There are several implementation
//        issues to be sorted out for full compliance and error resilience.
//        Some examples are given below (nasm syntax).
//
// 1) addressing issues
//
// 1.1) symbol relative relocation (i.e. mov eax,[foo wrt bar])
//      Not implemented yet.
//
// 1.2) data referencing in 64 bit mode
//      While ELF allows 32 bit absolute relocations in 64 bit mode, Mach-O
//      does not. Therefore code like
//       lea rbx,[_foo]  ;48 8d 1c 25 00 00 00 00
//       mov rcx,[_bar]  ;48 8b 0c 25 00 00 00 00
//      with a 32 bit address field cannot be relocated into an address >= 0x100000000 (OSX actually
//      uses that). 
//
//      Actually, the only register where a 64 bit displacement is allowed in x86-64, is rax
//      as in the example 1).
//
//      A plausible workaround is either classic PIC (like in C), which is in turn
//      not implemented in this object format. The recommended was is PC relative 
//      code (called RIP-relative in x86-64). So instead of the lines above, just write:
//       lea rbx,[_foo wrt rip]
//       mov rcx,[_bar wrt rip]
//
#include "MachObject.h"

#include "llvm/ADT/STLExtras.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Parse/Directive.h"
#include "yasmx/Parse/DirHelpers.h"
#include "yasmx/Parse/NameValue.h"
#include "yasmx/Support/registry.h"
#include "yasmx/Arch.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Object.h"
#include "yasmx/Symbol.h"

#include "MachSection.h"
#include "MachSymbol.h"


using namespace yasm;
using namespace yasm::objfmt;

struct MachObject::StaticSectionConfig
{
    const char *name;       // ".name"
    const char *segname;    // segment name (e.g. __TEXT)
    const char *sectname;   // section name (e.g. __text)
    unsigned long flags;    // section flags
    unsigned int align;     // default alignment
};

struct MachObject::SectionConfig
{
    explicit SectionConfig(llvm::StringRef name_)
        : name(name_), flags(MachSection::S_REGULAR), align(0)
    {}
    SectionConfig(llvm::StringRef segname_, llvm::StringRef sectname_)
        : segname(segname_), sectname(sectname_)
        , flags(MachSection::S_REGULAR), align(0)
    {}
    SectionConfig(const StaticSectionConfig& config)
        : name(config.name)
        , segname(config.segname)
        , sectname(config.sectname)
        , flags(config.flags)
        , align(config.align)
    {}

    ~SectionConfig();

    std::string name;       // ".name"
    std::string segname;    // segment name (e.g. __TEXT)
    std::string sectname;   // section name (e.g. __text)
    unsigned long flags;    // section flags
    unsigned int align;     // default alignment
};

MachObject::SectionConfig::~SectionConfig()
{
}

static const MachObject::StaticSectionConfig mach_std_sections[] =
{
    {".text",           "__TEXT", "__text",
        MachSection::S_ATTR_PURE_INSTRUCTIONS, 0},
    {".const",          "__TEXT", "__const",        MachSection::S_REGULAR, 0},
    {".static_const",   "__TEXT", "__static_const", MachSection::S_REGULAR, 0},
    {".cstring",        "__TEXT", "__cstring",
        MachSection::S_CSTRING_LITERALS, 0},
    {".literal4",       "__TEXT", "__literal4",
        MachSection::S_4BYTE_LITERALS, 4},
    {".literal8",       "__TEXT", "__literal8",
        MachSection::S_8BYTE_LITERALS, 8},
    {".literal16",      "__TEXT", "__literal16",
        MachSection::S_16BYTE_LITERALS, 16},
    {".constructor",    "__TEXT", "__constructor",  MachSection::S_REGULAR, 0},
    {".destructor",     "__TEXT", "__destructor",   MachSection::S_REGULAR, 0},
    {".eh_frame",       "__TEXT", "__eh_frame",
        MachSection::S_COALESCED|MachSection::S_ATTR_LIVE_SUPPORT|
        MachSection::S_ATTR_STRIP_STATIC_SYMS|MachSection::S_ATTR_NO_TOC,
        4},
    {".data",           "__DATA", "__data",         MachSection::S_REGULAR, 0},
    {".bss",            "__DATA", "__bss",          MachSection::S_ZEROFILL, 0},
    {".const_data",     "__DATA", "__const",        MachSection::S_REGULAR, 0},
    {".rodata",         "__DATA", "__const",        MachSection::S_REGULAR, 0},
    {".static_data",    "__DATA", "__static_data",  MachSection::S_REGULAR, 0},
    {".mod_init_func",  "__DATA", "__mod_init_func",
        MachSection::S_MOD_INIT_FUNC_POINTERS, 4},
    {".mod_term_func",  "__DATA", "__mod_term_func",
        MachSection::S_MOD_TERM_FUNC_POINTERS, 4},
    {".dyld",           "__DATA", "__dyld",         MachSection::S_REGULAR, 0},
    {".cfstring",       "__DATA", "__cfstring",     MachSection::S_REGULAR, 0},
    {".debug_frame",    "__DWARF", "__debug_frame",     MachSection::S_ATTR_DEBUG, 0},
    {".debug_info",     "__DWARF", "__debug_info",      MachSection::S_ATTR_DEBUG, 0},
    {".debug_abbrev",   "__DWARF", "__debug_abbrev",    MachSection::S_ATTR_DEBUG, 0},
    {".debug_aranges",  "__DWARF", "__debug_aranges",   MachSection::S_ATTR_DEBUG, 0},
    {".debug_macinfo",  "__DWARF", "__debug_macinfo",   MachSection::S_ATTR_DEBUG, 0},
    {".debug_line",     "__DWARF", "__debug_line",      MachSection::S_ATTR_DEBUG, 0},
    {".debug_loc",      "__DWARF", "__debug_loc",       MachSection::S_ATTR_DEBUG, 0},
    {".debug_pubnames", "__DWARF", "__debug_pubnames",  MachSection::S_ATTR_DEBUG, 0},
    {".debug_pubtypes", "__DWARF", "__debug_pubtypes",  MachSection::S_ATTR_DEBUG, 0},
    {".debug_str",      "__DWARF", "__debug_str",       MachSection::S_ATTR_DEBUG, 0},
    {".debug_ranges",   "__DWARF", "__debug_ranges",    MachSection::S_ATTR_DEBUG, 0},
    {".debug_macro",    "__DWARF", "__debug_macro",     MachSection::S_ATTR_DEBUG, 0},
    {".objc_class_names",   "__TEXT", "__cstring",  MachSection::S_CSTRING_LITERALS, 0},
    {".objc_meth_var_types","__TEXT", "__cstring",  MachSection::S_CSTRING_LITERALS, 0},
    {".objc_meth_var_names","__TEXT", "__cstring",  MachSection::S_CSTRING_LITERALS, 0},
    {".objc_class",         "__OBJC", "__class",
        MachSection::S_ATTR_NO_DEAD_STRIP, 0},
    {".objc_meta_class",    "__OBJC", "__meta_class",
        MachSection::S_ATTR_NO_DEAD_STRIP, 0},
    {".objc_cat_cls_meth",  "__OBJC", "__cat_cls_meth",
        MachSection::S_ATTR_NO_DEAD_STRIP, 0},
    {".objc_cat_inst_meth", "__OBJC", "__cat_inst_meth",
        MachSection::S_ATTR_NO_DEAD_STRIP, 0},
    {".objc_protocol",      "__OBJC", "__protocol",
        MachSection::S_ATTR_NO_DEAD_STRIP, 0},
    {".objc_string_object", "__OBJC", "__string_object",
        MachSection::S_ATTR_NO_DEAD_STRIP, 0},
    {".objc_cls_meth",      "__OBJC", "__cls_meth",
        MachSection::S_ATTR_NO_DEAD_STRIP, 0},
    {".objc_inst_meth",     "__OBJC", "__inst_meth",
        MachSection::S_ATTR_NO_DEAD_STRIP, 0},
    {".objc_cls_refs",      "__OBJC", "__cls_refs",
        MachSection::S_LITERAL_POINTERS|MachSection::S_ATTR_NO_DEAD_STRIP, 4},
    {".objc_message_refs",  "__OBJC", "__message_refs",
        MachSection::S_LITERAL_POINTERS|MachSection::S_ATTR_NO_DEAD_STRIP, 4},
    {".objc_symbols",       "__OBJC", "__symbols",
        MachSection::S_ATTR_NO_DEAD_STRIP, 0},
    {".objc_category",      "__OBJC", "__category",
        MachSection::S_ATTR_NO_DEAD_STRIP, 0},
    {".objc_class_vars",    "__OBJC", "__class_vars",
        MachSection::S_ATTR_NO_DEAD_STRIP, 0},
    {".objc_instance_vars", "__OBJC", "__instance_vars",
        MachSection::S_ATTR_NO_DEAD_STRIP, 0},
    {".objc_module_info",   "__OBJC", "__module_info",
        MachSection::S_ATTR_NO_DEAD_STRIP, 0},
    {".objc_selector_strs", "__OBJC", "__selector_strs",
        MachSection::S_CSTRING_LITERALS, 0},
    {".objc_image_info", "__OBJC", "__image_info",
        MachSection::S_ATTR_NO_DEAD_STRIP, 0},
    {".objc_selector_fixup", "__OBJC", "__sel_fixup",
        MachSection::S_ATTR_NO_DEAD_STRIP, 0},
    {".objc1_class_ext", "__OBJC", "__class_ext",
        MachSection::S_ATTR_NO_DEAD_STRIP, 0},
    {".objc1_property_list", "__OBJC", "__property",
        MachSection::S_ATTR_NO_DEAD_STRIP, 0},
    {".objc1_protocol_ext", "__OBJC", "__protocol_ext",
        MachSection::S_ATTR_NO_DEAD_STRIP, 0},
    {0, 0, 0, 0, 0}
};

static const MachObject::StaticSectionConfig mach_x86_sections[] =
{
    {".symbol_stub",                "__TEXT", "__symbol_stub",
        MachSection::S_SYMBOL_STUBS|MachSection::S_ATTR_PURE_INSTRUCTIONS, 0},
    {".picsymbol_stub",             "__TEXT", "__picsymbol_stub",
        MachSection::S_SYMBOL_STUBS|MachSection::S_ATTR_PURE_INSTRUCTIONS, 0},
    {".non_lazy_symbol_pointer",    "__DATA", "__nl_symbol_ptr",
        MachSection::S_NON_LAZY_SYMBOL_POINTERS, 0},
    {".lazy_symbol_pointer",        "__DATA", "__la_symbol_ptr",
        MachSection::S_LAZY_SYMBOL_POINTERS, 0},
    {".lazy_symbol_pointer2",       "__DATA", "__la_sym_ptr2",
        MachSection::S_LAZY_SYMBOL_POINTERS, 0},
    {".lazy_symbol_pointer3",       "__DATA", "__la_sym_ptr3",
        MachSection::S_LAZY_SYMBOL_POINTERS, 0},
    {".picsymbol_stub3",            "__IMPORT", "__jump_table",
        MachSection::S_SYMBOL_STUBS|MachSection::S_ATTR_PURE_INSTRUCTIONS|
        MachSection::S_ATTR_SELF_MODIFYING_CODE, 64},
    {".non_lazy_symbol_ptr_x86",    "__IMPORT", "__pointers",
        MachSection::S_NON_LAZY_SYMBOL_POINTERS, 4},
    {0, 0, 0, 0, 0}
};

static const MachObject::StaticSectionConfig mach_x86_64_sections[] =
{
    {".eh_frame",       "__TEXT", "__eh_frame",
        MachSection::S_COALESCED|MachSection::S_ATTR_LIVE_SUPPORT|
        MachSection::S_ATTR_STRIP_STATIC_SYMS|MachSection::S_ATTR_NO_TOC,
        8},
    {0, 0, 0, 0, 0}
};

static const struct MachSectionTypeName {
    const char* name;
    unsigned long flags;
} mach_section_types[] = {
    {"regular",                     MachSection::S_REGULAR},
    {"coalesced",                   MachSection::S_COALESCED},
    {"zerofill",                    MachSection::S_ZEROFILL},
    {"cstring_literals",            MachSection::S_CSTRING_LITERALS},
    {"4byte_literals",              MachSection::S_4BYTE_LITERALS},
    {"8byte_literals",              MachSection::S_8BYTE_LITERALS},
    {"16byte_literals",             MachSection::S_16BYTE_LITERALS},
    {"literal_pointers",            MachSection::S_LITERAL_POINTERS},
    {"mod_init_func_pointers",      MachSection::S_MOD_INIT_FUNC_POINTERS},
    {"mod_term_func_pointers",      MachSection::S_MOD_TERM_FUNC_POINTERS},
    {"gb_zerofill",                 MachSection::S_GB_ZEROFILL},
    {"symbol_stubs",                MachSection::S_SYMBOL_STUBS},
    {"interposing",                 MachSection::S_INTERPOSING},
    {"dtrace_dof",                  MachSection::S_DTRACE_DOF},
    {"non_lazy_symbol_pointers",    MachSection::S_NON_LAZY_SYMBOL_POINTERS},
    {"lazy_symbol_pointers",        MachSection::S_LAZY_SYMBOL_POINTERS},
    {"symbol_stubs",                MachSection::S_SYMBOL_STUBS},
    {"lazy_dylib_symbol_pointers",  MachSection::S_LAZY_DYLIB_SYMBOL_POINTERS},
    {0, 0}
};

static unsigned long
MachLookupSectionType(llvm::StringRef name)
{
    for (const MachSectionTypeName* tn=mach_section_types; tn->name; ++tn)
    {
        if (name == tn->name)
            return tn->flags;
    }
    return MachSection::SECTION_TYPE;
}

static const struct MachSectionAttrName {
    const char* name;
    unsigned long flags;
} mach_section_attrs[] = {
    {"none",                    0},
    {"pure_instructions",       MachSection::S_ATTR_PURE_INSTRUCTIONS},
    {"some_instructions",       MachSection::S_ATTR_SOME_INSTRUCTIONS},
    {"loc_reloc",               MachSection::S_ATTR_LOC_RELOC},
    {"ext_reloc",               MachSection::S_ATTR_EXT_RELOC},
    {"debug",                   MachSection::S_ATTR_DEBUG},
    {"live_support",            MachSection::S_ATTR_LIVE_SUPPORT},
    {"no_dead_strip",           MachSection::S_ATTR_NO_DEAD_STRIP},
    {"strip_static_syms",       MachSection::S_ATTR_STRIP_STATIC_SYMS},
    {"no_toc",                  MachSection::S_ATTR_NO_TOC},
    {"self_modifying_code",     MachSection::S_ATTR_SELF_MODIFYING_CODE},
    {"modifying_code",          MachSection::S_ATTR_SELF_MODIFYING_CODE},
    {0, 0}
};

static unsigned long
MachLookupSectionAttr(llvm::StringRef name)
{
    for (const MachSectionAttrName* an=mach_section_attrs; an->name; ++an)
    {
        if (name == an->name)
            return an->flags;
    }
    return MachSection::SECTION_ATTRIBUTES;
}

MachObject::MachObject(const ObjectFormatModule& module,
                       Object& object,
                       unsigned int bits)
    : ObjectFormat(module, object)
    , m_bits(bits)
    , m_subsections_via_symbols(false)
{
    if (m_bits == 64)
        m_arch_sections = mach_x86_64_sections;
    else
        m_arch_sections = mach_x86_sections;
}

MachObject::~MachObject()
{
}

Mach32Object::~Mach32Object()
{
}

bool
Mach32Object::isOkObject(Object& object)
{
    // Only support x86 arch
    if (!object.getArch()->getModule().getKeyword().equals_lower("x86"))
        return false;

    // Support x86 machine of x86 arch
    return object.getArch()->getMachine().equals_lower("x86");
}

Mach64Object::~Mach64Object()
{
}

bool
Mach64Object::isOkObject(Object& object)
{
    // Only support x86 arch
    if (!object.getArch()->getModule().getKeyword().equals_lower("x86"))
        return false;

    // Support amd64 machine of x86 arch
    return object.getArch()->getMachine().equals_lower("amd64");
}

void
MachObject::InitSymbols(llvm::StringRef parser)
{
    // Set object options
    m_object.getOptions().PowerOfTwoAlignment = true;

    if (m_bits == 64)
    {
        m_gotpcrel_sym = m_object.AddSpecialSymbol("gotpcrel");
        m_gotpcrel_sym->DefineSpecial(Symbol::EXTERN);
    }

    // create special symbols for section types and attributes so the
    // parser doesn't create symbol table references.
    for (const MachSectionTypeName* tn=mach_section_types; tn->name; ++tn)
        m_object.AddSpecialSymbol(tn->name)->DefineSpecial(Symbol::LOCAL);
    for (const MachSectionAttrName* an=mach_section_attrs; an->name; ++an)
        m_object.AddSpecialSymbol(an->name)->DefineSpecial(Symbol::LOCAL);
}

std::vector<llvm::StringRef>
MachObject::getDebugFormatKeywords()
{
    static const char* keywords[] = {"null", "cfi", "dwarf2", "dwarf2pass"};
    size_t keywords_size = sizeof(keywords)/sizeof(keywords[0]);
    return std::vector<llvm::StringRef>(keywords, keywords+keywords_size);
}

Section*
MachObject::AddDefaultSection()
{
    Diagnostic diags(NULL);
    Section* section = AppendSection(".text", SourceLocation(), diags);
    section->setDefault(true);
    return section;
}

void
MachObject::InitSection(const SectionConfig& config, Section& section)
{
    // Add Mach data to the section
    MachSection* msect = section.getAssocData<MachSection>();
    if (!msect)
    {
        msect = new MachSection(config.segname, config.sectname);
        section.AddAssocData(std::auto_ptr<MachSection>(msect));
    }
    msect->flags = config.flags;
    // if pure instructions, also set some instructions
    if (config.flags & MachSection::S_ATTR_PURE_INSTRUCTIONS)
        msect->flags |= MachSection::S_ATTR_SOME_INSTRUCTIONS;

    section.setCode((config.flags & MachSection::S_ATTR_PURE_INSTRUCTIONS) != 0);
    section.setBSS((config.flags & MachSection::SECTION_TYPE) ==
                   MachSection::S_ZEROFILL);
    section.setAlign(config.align);
}

MachObject::SectionConfig
MachObject::LookupSection(llvm::StringRef name)
{
    // lookup arch-specific
    for (const StaticSectionConfig* conf=m_arch_sections; conf->name; ++conf)
    {
        if (name == conf->name)
            return *conf;
    }

    // lookup standard
    for (const StaticSectionConfig* conf=mach_std_sections; conf->name; ++conf)
    {
        if (name == conf->name)
            return *conf;
    }

    // not found, try to guess smartly, and ultimately default to text
    SectionConfig config(name);
    if (name.startswith(".debug"))
    {
        config.segname = "__DWARF";
        config.sectname = "__";
        config.sectname += name.substr(1);
        config.flags = MachSection::S_ATTR_DEBUG;
    }
    else if (name.startswith(".objc"))
    {
        config.segname = "__OBJC";
        config.sectname = "_";
        config.sectname += name.substr(5);
        config.flags = MachSection::S_ATTR_NO_DEAD_STRIP;
    }
    else
    {
        config.segname = "__TEXT";
        if (name[0] != '.')
            config.sectname = name;
        else
        {
            config.sectname = "__";
            config.sectname += name.substr(1);
        }
    }
    return config;
}

MachObject::SectionConfig
MachObject::LookupSection(llvm::StringRef segname, llvm::StringRef sectname)
{
    // lookup arch-specific
    for (const StaticSectionConfig* conf=m_arch_sections; conf->name; ++conf)
    {
        if (segname == conf->segname && sectname == conf->sectname)
            return *conf;
    }

    // lookup standard
    for (const StaticSectionConfig* conf=mach_std_sections; conf->name; ++conf)
    {
        if (segname == conf->segname && sectname == conf->sectname)
            return *conf;
    }

    // not found, build custom
    SectionConfig config(segname, sectname);
    config.name = "LC_SEGMENT.";
    config.name += segname;
    config.name += '.';
    config.name += sectname;
    return config;
}

Section*
MachObject::AppendSection(const SectionConfig& config,
                          SourceLocation source,
                          Diagnostic& diags)
{
    // Create section
    Section* section = new Section(config.name, false, false, source);
    m_object.AppendSection(std::auto_ptr<Section>(section));

    // Define a label for the start of the section
    Location start = {&section->bytecodes_front(), 0};
    SymbolRef sym = m_object.getSymbol(config.name);
    if (!sym->isDefined())
    {
        sym->DefineLabel(start);
        sym->setDefSource(source);
    }
    section->setSymbol(sym);

    // Initialize Mach data
    InitSection(config, *section);

    return section;
}

Section*
MachObject::AppendSection(llvm::StringRef name,
                          SourceLocation source,
                          Diagnostic& diags)
{
    if (name.startswith("LC_SEGMENT."))
    {
        // special name, extract segment and section
        llvm::StringRef segname, sectname;
        llvm::tie(segname, sectname) = name.substr(11).split('.');
        return AppendSection(LookupSection(segname, sectname), source, diags);
    }

    return AppendSection(LookupSection(name), source, diags);
}

Section*
MachObject::AppendSection(llvm::StringRef segname,
                          llvm::StringRef sectname,
                          SourceLocation source,
                          Diagnostic& diags)
{
    return AppendSection(LookupSection(segname, sectname), source, diags);
}

void
MachObject::DirGasSection(DirectiveInfo& info, Diagnostic& diags)
{
    assert(info.isObject(m_object));
    NameValues& nvs = info.getNameValues();
    // segname , sectname [[[ , type ] , attribute [+ attr...] ] , sizeof_stub ]

    if (nvs.size() < 2)
    {
        diags.Report(info.getSource(),
                     diag::err_macho_segment_section_required);
        return;
    }

    // segname
    NameValue& segname_nv = nvs.front();
    if (!segname_nv.isString())
    {
        diags.Report(segname_nv.getValueRange().getBegin(),
                     diag::err_value_string_or_id);
        return;
    }
    llvm::StringRef segname = segname_nv.getString();
    if (segname.size() > 16)
    {
        diags.Report(segname_nv.getValueRange().getBegin(),
                     diag::warn_macho_segment_name_length);
        segname = segname.substr(0, 16);
    }

    // sectname
    NameValue& sectname_nv = nvs[1];
    if (!sectname_nv.isString())
    {
        diags.Report(sectname_nv.getValueRange().getBegin(),
                     diag::err_value_string_or_id);
        return;
    }
    llvm::StringRef sectname = sectname_nv.getString();
    if (sectname.size() > 16)
    {
        diags.Report(sectname_nv.getValueRange().getBegin(),
                     diag::warn_macho_section_name_length);
        sectname = sectname.substr(0, 16);
    }

    SectionConfig config = LookupSection(segname, sectname);
    bool flags_set = nvs.size() > 2;

    if (nvs.size() > 2)
    {
        // type
        unsigned long type = MachSection::S_REGULAR;
        NameValue& type_nv = nvs[2];
        if (type_nv.isId())
            type = MachLookupSectionType(type_nv.getId());
        else if (type_nv.isExpr())
        {
            Expr e = type_nv.getExpr(m_object);
            e.Simplify(diags);
            if (e.isIntNum())
                type = e.getIntNum().getUInt();
            else
                diags.Report(type_nv.getValueRange().getBegin(),
                             diag::err_value_expression);
        }
        else
        {
            diags.Report(type_nv.getValueRange().getBegin(),
                         diag::err_value_expression);
        }
        if (type == MachSection::SECTION_TYPE)
        {
            diags.Report(type_nv.getValueRange().getBegin(),
                         diag::err_macho_unknown_section_type);
            type = MachSection::S_REGULAR;
        }
        config.flags = type;
    }

    if (nvs.size() > 3)
    {
        // attribute can be a single one or a ADD/OR of attributes
        unsigned long attr = 0;
        NameValue& attr_nv = nvs[3];
        if (attr_nv.isId())
        {
            attr = MachLookupSectionAttr(attr_nv.getId());
        }
        else if (attr_nv.isExpr())
        {
            Expr e = attr_nv.getExpr(m_object);
            e.Simplify(diags);
            if (e.isIntNum())
                attr = e.getIntNum().getUInt();
            else if (e.isOp(Op::ADD) || e.isOp(Op::OR))
            {
                // loop through terms and OR in each one.
                bool error = false;
                ExprTerms& terms = e.getTerms();
                for (int n = terms.size()-1; n >= 0; --n)
                {
                    ExprTerm& child = terms[n];
                    // look only at the top level
                    if (child.isEmpty())
                        continue;
                    if (child.m_depth != 1)
                        continue;
                    // handle integers and identifiers, otherwise an error
                    if (IntNum* intn = child.getIntNum())
                        attr |= intn->getUInt();
                    else if (SymbolRef sym = child.getSymbol())
                        attr |= MachLookupSectionAttr(sym->getName());
                    else
                    {
                        error = true;
                        break;
                    }
                }
                if (error)
                    diags.Report(attr_nv.getValueRange().getBegin(),
                                 diag::err_value_expression);
            }
            else
                diags.Report(attr_nv.getValueRange().getBegin(),
                             diag::err_value_expression);
        }
        else
        {
            diags.Report(attr_nv.getValueRange().getBegin(),
                         diag::err_value_expression);
        }
        if (attr == MachSection::SECTION_ATTRIBUTES)
        {
            diags.Report(attr_nv.getValueRange().getBegin(),
                         diag::err_macho_unknown_section_attr);
            attr = 0;
        }
        config.flags |= attr;
    }

    // Finish up
    Section* sect = m_object.FindSection(config.name);
    if (sect)
    {
        // Section already exists, initialize it if default
        MachSection* msect = sect->getAssocData<MachSection>();
        if (sect->isDefault() || !msect)
            InitSection(config, *sect);
        else
        {
            // otherwise check for flags conflict
            if (flags_set && config.flags != msect->flags)
                diags.Report(info.getSource(), diag::warn_section_redef_flags);
        }
    }
    else
        sect = AppendSection(config, info.getSource(), diags);
    // set it as current assembly section
    m_object.setCurSection(sect);
    sect->setDefault(false);
}

static void
MachDirSegname(NameValue& nv,
               Diagnostic& diags,
               std::string* out,
               bool* out_set)
{
    if (!nv.isString())
    {
        diags.Report(nv.getNameSource(), diag::err_value_string_or_id)
            << nv.getValueRange();
        return;
    }
    llvm::StringRef str = nv.getString();
    if (str.size() > 16)
    {
        diags.Report(nv.getValueRange().getBegin(),
                     diag::warn_macho_segment_name_length);
        str = str.substr(0, 16);
    }
    *out = str;
    *out_set = true;
}

void
MachObject::DirSection(DirectiveInfo& info, Diagnostic& diags)
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
    llvm::StringRef sectname = sectname_nv.getString();
    if (sectname.size() > 16)
    {
        diags.Report(sectname_nv.getValueRange().getBegin(),
                     diag::warn_macho_section_name_length);
        sectname = sectname.substr(0, 16);
    }

    std::string segname;
    bool segname_set = false;
    IntNum align;
    bool align_set = false;

    DirHelpers helpers;
    helpers.Add("segname", true,
                TR1::bind(&MachDirSegname, _1, _2, &segname, &segname_set));
    helpers.Add("align", true,
                TR1::bind(&DirIntNumPower2, _1, _2, &m_object, &align,
                          &align_set));
    helpers(++nvs.begin(), nvs.end(), info.getSource(), diags,
            DirNameValueWarn);

    SectionConfig config(sectname);
    if (segname_set)
        config = LookupSection(segname, sectname);
    else
        config = LookupSection(sectname);

    if (align_set)
    {
        config.align = align.getUInt();
        if (config.align > 16384)
        {
            diags.Report(info.getSource(), diag::err_macho_align_too_big);
            config.align = 16384;
        }
    }

    // Finish up
    Section* sect = m_object.FindSection(config.name);
    if (sect)
    {
        // Section already exists, initialize it if default
        MachSection* msect = sect->getAssocData<MachSection>();
        if (sect->isDefault() || !msect)
            InitSection(config, *sect);
        else
        {
            // otherwise check for flags conflict
            if (align_set && config.align != sect->getAlign())
                diags.Report(info.getSource(), diag::warn_section_redef_flags);
        }
    }
    else
        sect = AppendSection(config, info.getSource(), diags);
    // set it as current assembly section
    m_object.setCurSection(sect);
    sect->setDefault(false);
}

void
MachObject::DirGasStandardSection(const StaticSectionConfig* config,
                                  DirectiveInfo& info,
                                  Diagnostic& diags)
{
    Section* sect = m_object.FindSection(config->name);
    if (!sect)
        sect = AppendSection(*config, info.getSource(), diags);

    m_object.setCurSection(sect);
    sect->setDefault(false);
}

void
MachObject::DirZerofill(DirectiveInfo& info, Diagnostic& diags)
{
}

void
MachObject::DirIndirectSymbol(DirectiveInfo& info, Diagnostic& diags)
{
}

void
MachObject::DirReference(DirectiveInfo& info, Diagnostic& diags)
{
    assert(info.isObject(m_object));
    NameValues& namevals = info.getNameValues();
    for (NameValues::iterator nv = namevals.begin(), end = namevals.end();
         nv != end; ++nv)
    {
        SymbolRef sym = info.getObject().getSymbol(nv->getId());
        MachSymbol& msym = MachSymbol::Build(*sym);
        msym.m_ref_flag = MachSymbol::REFERENCE_FLAG_UNDEFINED_NON_LAZY;
        msym.m_no_dead_strip = true;
        msym.m_required = true;
    }
}

void
MachObject::DirLazyReference(DirectiveInfo& info, Diagnostic& diags)
{
    assert(info.isObject(m_object));
    NameValues& namevals = info.getNameValues();
    for (NameValues::iterator nv = namevals.begin(), end = namevals.end();
         nv != end; ++nv)
    {
        SymbolRef sym = info.getObject().getSymbol(nv->getId());
        MachSymbol& msym = MachSymbol::Build(*sym);
        msym.m_ref_flag = MachSymbol::REFERENCE_FLAG_UNDEFINED_LAZY;
        msym.m_no_dead_strip = true;
        msym.m_required = true;
    }
}

void
MachObject::DirWeakReference(DirectiveInfo& info, Diagnostic& diags)
{
    assert(info.isObject(m_object));
    NameValues& namevals = info.getNameValues();
    for (NameValues::iterator nv = namevals.begin(), end = namevals.end();
         nv != end; ++nv)
    {
        SymbolRef sym = info.getObject().getSymbol(nv->getId());
        MachSymbol& msym = MachSymbol::Build(*sym);
        msym.m_weak_ref = true;
        msym.m_required = true;
    }
}

void
MachObject::DirWeakDefinition(DirectiveInfo& info, Diagnostic& diags)
{
    assert(info.isObject(m_object));
    NameValues& namevals = info.getNameValues();
    for (NameValues::iterator nv = namevals.begin(), end = namevals.end();
         nv != end; ++nv)
    {
        SymbolRef sym = info.getObject().getSymbol(nv->getId());
        sym->CheckedDeclare(Symbol::GLOBAL, nv->getValueRange().getBegin(),
                            diags);
        MachSymbol& msym = MachSymbol::Build(*sym);
        msym.m_weak_def = true;
    }
}

void
MachObject::DirPrivateExtern(DirectiveInfo& info, Diagnostic& diags)
{
    assert(info.isObject(m_object));
    NameValues& namevals = info.getNameValues();
    for (NameValues::iterator nv = namevals.begin(), end = namevals.end();
         nv != end; ++nv)
    {
        SymbolRef sym = info.getObject().getSymbol(nv->getId());
        sym->CheckedDeclare(Symbol::GLOBAL, nv->getValueRange().getBegin(),
                            diags);
        MachSymbol& msym = MachSymbol::Build(*sym);
        msym.m_private_extern = true;
    }
}

void
MachObject::DirDesc(DirectiveInfo& info, Diagnostic& diags)
{
    assert(info.isObject(m_object));

    NameValues& namevals = info.getNameValues();
    if (namevals.size() < 2)
    {
        diags.Report(info.getSource(), diag::err_macho_desc_requires_expr);
        return;
    }

    IntNum val;
    bool val_set;
    DirIntNum(namevals[1], diags, &m_object, &val, &val_set);
    if (!val_set)
        return;

    SymbolRef sym = info.getObject().getSymbol(namevals[0].getId());
    MachSymbol& msym = MachSymbol::Build(*sym);
    msym.setDesc(val.getUInt());
    msym.m_required = true;
}

void
MachObject::DirNoDeadStrip(DirectiveInfo& info, Diagnostic& diags)
{
    assert(info.isObject(m_object));
    NameValues& namevals = info.getNameValues();
    for (NameValues::iterator nv = namevals.begin(), end = namevals.end();
         nv != end; ++nv)
    {
        SymbolRef sym = info.getObject().getSymbol(nv->getId());
        MachSymbol& msym = MachSymbol::Build(*sym);
        msym.m_no_dead_strip = true;
        msym.m_required = true;
    }
}

void
MachObject::DirSubsectionsViaSymbols(DirectiveInfo& info, Diagnostic& diags)
{
    m_subsections_via_symbols = true;
}

void
MachObject::AddDirectives(Directives& dirs, llvm::StringRef parser)
{
    static const Directives::Init<MachObject> nasm_dirs[] =
    {
        {"section", &MachObject::DirSection, Directives::ARG_REQUIRED},
        {"segment", &MachObject::DirSection, Directives::ARG_REQUIRED},
    };
    static const Directives::Init<MachObject> gas_dirs[] =
    {
        {".section",    &MachObject::DirGasSection, Directives::ARG_REQUIRED},
        {".zerofill",   &MachObject::DirZerofill,   Directives::ID_REQUIRED},
        {".indirect_symbol",            &MachObject::DirIndirectSymbol,
            Directives::ID_REQUIRED},
        {".reference",                  &MachObject::DirReference,
            Directives::ID_REQUIRED},
        {".lazy_reference",             &MachObject::DirLazyReference,
            Directives::ID_REQUIRED},
        {".weak_reference",             &MachObject::DirWeakReference,
            Directives::ID_REQUIRED},
        {".weak_definition",            &MachObject::DirWeakDefinition,
            Directives::ID_REQUIRED},
        {".private_extern",             &MachObject::DirPrivateExtern,
            Directives::ID_REQUIRED},
        {".desc",                       &MachObject::DirDesc,
            Directives::ID_REQUIRED},
        {".no_dead_strip",              &MachObject::DirNoDeadStrip,
            Directives::ID_REQUIRED},
        {".subsections_via_symbols",    &MachObject::DirSubsectionsViaSymbols,
            Directives::ANY},
    };

    if (parser.equals_lower("nasm"))
        dirs.AddArray(this, nasm_dirs);
    else if (parser.equals_lower("gas") || parser.equals_lower("gnu"))
    {
        dirs.AddArray(this, gas_dirs);

        // section directives
        for (const StaticSectionConfig* conf=m_arch_sections; conf->name;
             ++conf)
        {
            dirs.Add(conf->name,
                     TR1::bind(&MachObject::DirGasStandardSection, this, conf,
                               _1, _2));
        }
        for (const StaticSectionConfig* conf=mach_std_sections; conf->name;
             ++conf)
        {
            dirs.Add(conf->name,
                     TR1::bind(&MachObject::DirGasStandardSection, this, conf,
                               _1, _2));
        }
    }
}

void
yasm_objfmt_mach_DoRegister()
{
    RegisterModule<ObjectFormatModule,
                   ObjectFormatModuleImpl<MachObject> >("macho");
    RegisterModule<ObjectFormatModule,
                   ObjectFormatModuleImpl<Mach32Object> >("macho32");
    RegisterModule<ObjectFormatModule,
                   ObjectFormatModuleImpl<Mach64Object> >("macho64");
}

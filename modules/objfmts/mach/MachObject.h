#ifndef YASM_MACHOBJECT_H
#define YASM_MACHOBJECT_H
//
// Mac OS X ABI Mach-O File Format
//
//  Copyright (C) 2007 Henryk Richter, built upon xdf objfmt (C) Peter Johnson
//  Copyright (C) 2010 Peter Johnson
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
#include "yasmx/ObjectFormat.h"
#include "yasmx/SymbolRef.h"


namespace yasm
{
class DiagnosticsEngine;
class DirectiveInfo;

namespace objfmt
{

class YASM_STD_EXPORT MachObject : public ObjectFormat
{
public:
    struct SectionConfig;
    struct StaticSectionConfig;

    MachObject(const ObjectFormatModule& module,
               Object& object,
               unsigned int bits=0);

    /// Destructor.
    ~MachObject();

    void AddDirectives(Directives& dirs, StringRef parser);

    void InitSymbols(StringRef parser);

    void Output(raw_fd_ostream& os,
                bool all_syms,
                DebugFormat& dbgfmt,
                DiagnosticsEngine& diags);

    Section* AddDefaultSection();
    Section* AppendSection(const SectionConfig& config,
                           SourceLocation source,
                           DiagnosticsEngine& diags);
    Section* AppendSection(StringRef name,
                           SourceLocation source,
                           DiagnosticsEngine& diags);
    Section* AppendSection(StringRef segname,
                           StringRef sectname,
                           SourceLocation source,
                           DiagnosticsEngine& diags);

    static StringRef getName() { return "Mac OS X ABI Mach-O"; }
    static StringRef getKeyword() { return "macho"; }
    static StringRef getExtension() { return ".o"; }
    static unsigned int getDefaultX86ModeBits() { return 0; }
    static StringRef getDefaultDebugFormatKeyword() { return "cfi"; }
    static std::vector<StringRef> getDebugFormatKeywords();
    static bool isOkObject(Object& object) { return true; }
    static bool Taste(const MemoryBuffer& in,
                      /*@out@*/ std::string* arch_keyword,
                      /*@out@*/ std::string* machine)
    { return false; }

private:
    void InitSection(const SectionConfig& config, Section& section);
    SectionConfig LookupSection(StringRef name);
    SectionConfig LookupSection(StringRef segname, StringRef sectname);
    void DirGasSection(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirSection(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirGasStandardSection(const StaticSectionConfig* config,
                               DirectiveInfo& info,
                               DiagnosticsEngine& diags);
    void DirZerofill(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirIndirectSymbol(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirReference(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirLazyReference(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirWeakReference(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirWeakDefinition(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirPrivateExtern(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirDesc(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirNoDeadStrip(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirSubsectionsViaSymbols(DirectiveInfo& info,
                                  DiagnosticsEngine& diags);

    unsigned int m_bits;
    bool m_subsections_via_symbols;
    SymbolRef m_gotpcrel_sym;   // ..gotpcrel
    const StaticSectionConfig* m_arch_sections;
};

class YASM_STD_EXPORT Mach32Object : public MachObject
{
public:
    Mach32Object(const ObjectFormatModule& module, Object& object)
        : MachObject(module, object, 32)
    {}
    ~Mach32Object();

    static StringRef getName() { return "Mac OS X ABI Mach-O (32-bit)"; }
    static StringRef getKeyword() { return "macho32"; }
    static StringRef getExtension() { return MachObject::getExtension(); }
    static unsigned int getDefaultX86ModeBits() { return 32; }

    static StringRef getDefaultDebugFormatKeyword()
    { return MachObject::getDefaultDebugFormatKeyword(); }
    static std::vector<StringRef> getDebugFormatKeywords()
    { return MachObject::getDebugFormatKeywords(); }

    static bool isOkObject(Object& object);
    static bool Taste(const MemoryBuffer& in,
                      /*@out@*/ std::string* arch_keyword,
                      /*@out@*/ std::string* machine)
    { return false; }
};

class YASM_STD_EXPORT Mach64Object : public MachObject
{
public:
    Mach64Object(const ObjectFormatModule& module, Object& object)
        : MachObject(module, object, 64)
    {}
    ~Mach64Object();

    static StringRef getName() { return "Mac OS X ABI Mach-O (64-bit)"; }
    static StringRef getKeyword() { return "macho64"; }
    static StringRef getExtension() { return MachObject::getExtension(); }
    static unsigned int getDefaultX86ModeBits() { return 64; }

    static StringRef getDefaultDebugFormatKeyword()
    { return MachObject::getDefaultDebugFormatKeyword(); }
    static std::vector<StringRef> getDebugFormatKeywords()
    { return MachObject::getDebugFormatKeywords(); }

    static bool isOkObject(Object& object);
    static bool Taste(const MemoryBuffer& in,
                      /*@out@*/ std::string* arch_keyword,
                      /*@out@*/ std::string* machine)
    { return false; }
};

}} // namespace yasm::objfmt

#endif

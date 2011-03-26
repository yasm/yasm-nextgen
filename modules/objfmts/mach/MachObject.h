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
class Diagnostic;
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

    void AddDirectives(Directives& dirs, llvm::StringRef parser);

    void InitSymbols(llvm::StringRef parser);

    void Output(llvm::raw_fd_ostream& os,
                bool all_syms,
                DebugFormat& dbgfmt,
                Diagnostic& diags);

    Section* AddDefaultSection();
    Section* AppendSection(const SectionConfig& config,
                           SourceLocation source,
                           Diagnostic& diags);
    Section* AppendSection(llvm::StringRef name,
                           SourceLocation source,
                           Diagnostic& diags);
    Section* AppendSection(llvm::StringRef segname,
                           llvm::StringRef sectname,
                           SourceLocation source,
                           Diagnostic& diags);

    static llvm::StringRef getName() { return "Mac OS X ABI Mach-O"; }
    static llvm::StringRef getKeyword() { return "macho"; }
    static llvm::StringRef getExtension() { return ".o"; }
    static unsigned int getDefaultX86ModeBits() { return 0; }
    static llvm::StringRef getDefaultDebugFormatKeyword() { return "cfi"; }
    static std::vector<llvm::StringRef> getDebugFormatKeywords();
    static bool isOkObject(Object& object) { return true; }
    static bool Taste(const llvm::MemoryBuffer& in,
                      /*@out@*/ std::string* arch_keyword,
                      /*@out@*/ std::string* machine)
    { return false; }

private:
    void InitSection(const SectionConfig& config, Section& section);
    SectionConfig LookupSection(llvm::StringRef name);
    SectionConfig LookupSection(llvm::StringRef segname, llvm::StringRef sectname);
    void DirGasSection(DirectiveInfo& info, Diagnostic& diags);
    void DirSection(DirectiveInfo& info, Diagnostic& diags);
    void DirGasStandardSection(const StaticSectionConfig* config,
                               DirectiveInfo& info,
                               Diagnostic& diags);
    void DirZerofill(DirectiveInfo& info, Diagnostic& diags);
    void DirIndirectSymbol(DirectiveInfo& info, Diagnostic& diags);
    void DirReference(DirectiveInfo& info, Diagnostic& diags);
    void DirLazyReference(DirectiveInfo& info, Diagnostic& diags);
    void DirWeakReference(DirectiveInfo& info, Diagnostic& diags);
    void DirWeakDefinition(DirectiveInfo& info, Diagnostic& diags);
    void DirPrivateExtern(DirectiveInfo& info, Diagnostic& diags);
    void DirDesc(DirectiveInfo& info, Diagnostic& diags);
    void DirNoDeadStrip(DirectiveInfo& info, Diagnostic& diags);
    void DirSubsectionsViaSymbols(DirectiveInfo& info, Diagnostic& diags);

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

    static llvm::StringRef getName() { return "Mac OS X ABI Mach-O (32-bit)"; }
    static llvm::StringRef getKeyword() { return "macho32"; }
    static llvm::StringRef getExtension() { return MachObject::getExtension(); }
    static unsigned int getDefaultX86ModeBits() { return 32; }

    static llvm::StringRef getDefaultDebugFormatKeyword()
    { return MachObject::getDefaultDebugFormatKeyword(); }
    static std::vector<llvm::StringRef> getDebugFormatKeywords()
    { return MachObject::getDebugFormatKeywords(); }

    static bool isOkObject(Object& object);
    static bool Taste(const llvm::MemoryBuffer& in,
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

    static llvm::StringRef getName() { return "Mac OS X ABI Mach-O (64-bit)"; }
    static llvm::StringRef getKeyword() { return "macho64"; }
    static llvm::StringRef getExtension() { return MachObject::getExtension(); }
    static unsigned int getDefaultX86ModeBits() { return 64; }

    static llvm::StringRef getDefaultDebugFormatKeyword()
    { return MachObject::getDefaultDebugFormatKeyword(); }
    static std::vector<llvm::StringRef> getDebugFormatKeywords()
    { return MachObject::getDebugFormatKeywords(); }

    static bool isOkObject(Object& object);
    static bool Taste(const llvm::MemoryBuffer& in,
                      /*@out@*/ std::string* arch_keyword,
                      /*@out@*/ std::string* machine)
    { return false; }
};

}} // namespace yasm::objfmt

#endif

#ifndef YASM_WIN32OBJECT_H
#define YASM_WIN32OBJECT_H
//
// Win32 object format
//
//  Copyright (C) 2007-2008  Peter Johnson
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
#include "yasmx/Config/export.h"

#include "modules/objfmts/coff/CoffObject.h"


namespace yasm
{
namespace objfmt
{

class YASM_STD_EXPORT Win32Object : public CoffObject
{
public:
    Win32Object(const ObjectFormatModule& module, Object& object);
    virtual ~Win32Object();

    virtual void AddDirectives(Directives& dirs, StringRef parser);
    virtual void InitSymbols(StringRef parser);

    //virtual void InitSymbols()
    //virtual void Read()
    //virtual void Output()

    static StringRef getName() { return "Win32"; }
    static StringRef getKeyword() { return "win32"; }
    static StringRef getExtension() { return ".obj"; }
    static unsigned int getDefaultX86ModeBits() { return 32; }

    static StringRef getDefaultDebugFormatKeyword()
    { return CoffObject::getDefaultDebugFormatKeyword(); }
    static std::vector<StringRef> getDebugFormatKeywords();

    static bool isOkObject(Object& object)
    { return CoffObject::isOkObject(object); }
    static bool Taste(const MemoryBuffer& in,
                      /*@out@*/ std::string* arch_keyword,
                      /*@out@*/ std::string* machine)
    { return false; }

protected:
    virtual bool InitSection(StringRef name,
                             Section& section,
                             CoffSection* coffsect,
                             SourceLocation source,
                             DiagnosticsEngine& diags);
    virtual void DirSectionInitHelpers(DirHelpers& helpers,
                                       CoffSection* csd,
                                       IntNum* align,
                                       bool* has_align);

protected:
    void DirExport(DirectiveInfo& info, DiagnosticsEngine& diags);

private:
    void DirSafeSEH(DirectiveInfo& info, DiagnosticsEngine& diags);
};

}} // namespace yasm::objfmt

#endif

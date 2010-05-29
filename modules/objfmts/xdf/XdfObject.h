#ifndef YASM_XDFOBJECT_H
#define YASM_XDFOBJECT_H
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
#include "yasmx/ObjectFormat.h"


namespace yasm
{
class Diagnostic;
class DirectiveInfo;

namespace objfmt
{

class YASM_STD_EXPORT XdfObject : public ObjectFormat
{
public:
    /// Constructor.
    /// To make object format truly usable, set_object()
    /// needs to be called.
    XdfObject(const ObjectFormatModule& module, Object& object)
        : ObjectFormat(module, object)
    {}

    /// Destructor.
    ~XdfObject();

    void AddDirectives(Directives& dirs, llvm::StringRef parser);

    void Read(const llvm::MemoryBuffer& in);
    void Output(llvm::raw_fd_ostream& os, bool all_syms, Diagnostic& diags);

    Section* AddDefaultSection();
    Section* AppendSection(llvm::StringRef name, clang::SourceLocation source);

    static llvm::StringRef getName() { return "Extended Dynamic Object"; }
    static llvm::StringRef getKeyword() { return "xdf"; }
    static llvm::StringRef getExtension() { return ".xdf"; }
    static unsigned int getDefaultX86ModeBits() { return 32; }
    static llvm::StringRef getDefaultDebugFormatKeyword() { return "null"; }
    static std::vector<llvm::StringRef> getDebugFormatKeywords();
    static bool isOkObject(Object& object);
    static bool Taste(const llvm::MemoryBuffer& in,
                      /*@out@*/ std::string* arch_keyword,
                      /*@out@*/ std::string* machine);

private:
    void DirSection(DirectiveInfo& info, Diagnostic& diags);
};

}} // namespace yasm::objfmt

#endif

#ifndef YASM_WIN64OBJECT_H
#define YASM_WIN64OBJECT_H
//
// Win64 object format
//
//  Copyright (C) 2002-2009  Peter Johnson
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
#include "modules/objfmts/win32/Win32Object.h"

#include "UnwindCode.h"
#include "UnwindInfo.h"


namespace yasm
{
namespace objfmt
{

class YASM_STD_EXPORT Win64Object : public Win32Object
{
public:
    Win64Object(const ObjectFormatModule& module, Object& object);
    virtual ~Win64Object();

    virtual void AddDirectives(Directives& dirs, llvm::StringRef parser);

    //virtual void InitSymbols(llvm::StringRef parser);
    //virtual void Read(const llvm::MemoryBuffer& in);
    virtual void Output(llvm::raw_fd_ostream& os,
                        bool all_syms,
                        Errwarns& errwarns,
                        Diagnostic& diags);

    static llvm::StringRef getName() { return "Win64"; }
    static llvm::StringRef getKeyword() { return "win64"; }
    static llvm::StringRef getExtension() { return ".obj"; }
    static unsigned int getDefaultX86ModeBits() { return 64; }

    static llvm::StringRef getDefaultDebugFormatKeyword()
    { return Win32Object::getDefaultDebugFormatKeyword(); }
    static std::vector<llvm::StringRef> getDebugFormatKeywords()
    { return Win32Object::getDebugFormatKeywords(); }

    static bool isOkObject(Object& object)
    { return Win32Object::isOkObject(object); }
    static bool Taste(const llvm::MemoryBuffer& in,
                      /*@out@*/ std::string* arch_keyword,
                      /*@out@*/ std::string* machine)
    { return false; }

private:
    virtual bool InitSection(llvm::StringRef name,
                             Section& section,
                             CoffSection* coffsect);

    bool CheckProcFrameState(clang::SourceLocation dir_source,
                             Diagnostic& diags);

    void DirProcFrame(DirectiveInfo& info, Diagnostic& diags);
    void DirPushReg(DirectiveInfo& info, Diagnostic& diags);
    void DirSetFrame(DirectiveInfo& info, Diagnostic& diags);
    void DirAllocStack(DirectiveInfo& info, Diagnostic& diags);

    void SaveCommon(DirectiveInfo& info,
                    UnwindCode::Opcode op,
                    Diagnostic& diags);
    void DirSaveReg(DirectiveInfo& info, Diagnostic& diags);
    void DirSaveXMM128(DirectiveInfo& info, Diagnostic& diags);
    void DirPushFrame(DirectiveInfo& info, Diagnostic& diags);
    void DirEndProlog(DirectiveInfo& info, Diagnostic& diags);
    void DirEndProcFrame(DirectiveInfo& info, Diagnostic& diags);

    // data for proc_frame and related directives
    clang::SourceLocation m_proc_frame;     // start of proc source location
    clang::SourceLocation m_done_prolog;    // end of prologue source location
    std::auto_ptr<UnwindInfo> m_unwind; // Unwind info
};

}} // namespace yasm::objfmt

#endif
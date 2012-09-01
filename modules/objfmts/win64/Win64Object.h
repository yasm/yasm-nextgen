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

    virtual void AddDirectives(Directives& dirs, StringRef parser);

    //virtual void InitSymbols()
    //virtual void Read()
    virtual void Output(raw_fd_ostream& os,
                        bool all_syms,
                        DebugFormat& dbgfmt,
                        DiagnosticsEngine& diags);

    static StringRef getName() { return "Win64"; }
    static StringRef getKeyword() { return "win64"; }
    static StringRef getExtension() { return ".obj"; }
    static unsigned int getDefaultX86ModeBits() { return 64; }

    static StringRef getDefaultDebugFormatKeyword()
    { return Win32Object::getDefaultDebugFormatKeyword(); }
    static std::vector<StringRef> getDebugFormatKeywords()
    { return Win32Object::getDebugFormatKeywords(); }

    static bool isOkObject(Object& object)
    { return Win32Object::isOkObject(object); }
    static bool Taste(const MemoryBuffer& in,
                      /*@out@*/ std::string* arch_keyword,
                      /*@out@*/ std::string* machine)
    { return false; }

private:
    virtual bool InitSection(StringRef name,
                             Section& section,
                             CoffSection* coffsect,
                             SourceLocation source,
                             DiagnosticsEngine& diags);

    bool CheckProcFrameState(SourceLocation dir_source,
                             DiagnosticsEngine& diags);

    void DirProcFrame(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirPushReg(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirSetFrame(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirAllocStack(DirectiveInfo& info, DiagnosticsEngine& diags);

    void SaveCommon(DirectiveInfo& info,
                    UnwindCode::Opcode op,
                    DiagnosticsEngine& diags);
    void DirSaveReg(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirSaveXMM128(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirPushFrame(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirEndProlog(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirEndProcFrame(DirectiveInfo& info, DiagnosticsEngine& diags);

    // data for proc_frame and related directives
    SourceLocation m_proc_frame;        // start of proc source location
    SourceLocation m_done_prolog;       // end of prologue source location
    std::auto_ptr<UnwindInfo> m_unwind; // Unwind info
};

}} // namespace yasm::objfmt

#endif

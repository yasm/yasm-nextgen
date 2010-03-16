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
#include "util.h"

#include "yasmx/Support/Compose.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/nocase.h"
#include "yasmx/Support/registry.h"
#include "yasmx/Arch.h"
#include "yasmx/BytecodeContainer_util.h"
#include "yasmx/Directive.h"
#include "yasmx/Errwarns.h"
#include "yasmx/Object.h"
#include "yasmx/NameValue.h"
#include "yasmx/Symbol.h"

#include "modules/objfmts/win32/Win32Object.h"
#include "modules/objfmts/coff/CoffSection.h"
#include "UnwindCode.h"
#include "UnwindInfo.h"

namespace yasm
{
namespace objfmt
{
namespace win64
{

using yasm::objfmt::coff::CoffSection;
using yasm::objfmt::win32::Win32Object;

class Win64Object : public Win32Object
{
public:
    Win64Object(const ObjectFormatModule& module, Object& object);
    virtual ~Win64Object();

    virtual void AddDirectives(Directives& dirs, llvm::StringRef parser);

    //virtual void InitSymbols(llvm::StringRef parser);
    //virtual void Read(const llvm::MemoryBuffer& in);
    virtual void Output(llvm::raw_fd_ostream& os,
                        bool all_syms,
                        Errwarns& errwarns);

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

    void CheckProcFrameState(llvm::StringRef dirname);

    void DirProcFrame(DirectiveInfo& info);
    void DirPushReg(DirectiveInfo& info);
    void DirSetFrame(DirectiveInfo& info);
    void DirAllocStack(DirectiveInfo& info);

    void SaveCommon(DirectiveInfo& info,
                    llvm::StringRef dirname,
                    UnwindCode::Opcode op);
    void DirSaveReg(DirectiveInfo& info);
    void DirSaveXMM128(DirectiveInfo& info);
    void DirPushFrame(DirectiveInfo& info);
    void DirEndProlog(DirectiveInfo& info);
    void DirEndProcFrame(DirectiveInfo& info);

    // data for proc_frame and related directives
    clang::SourceLocation m_proc_frame;     // start of proc source location
    clang::SourceLocation m_done_prolog;    // end of prologue source location
    std::auto_ptr<UnwindInfo> m_unwind; // Unwind info
};

Win64Object::Win64Object(const ObjectFormatModule& module, Object& object)
    : Win32Object(module, object)
    , m_unwind(0)
{
}

Win64Object::~Win64Object()
{
}

void
Win64Object::Output(llvm::raw_fd_ostream& os, bool all_syms, Errwarns& errwarns)
{
    if (m_proc_frame.isValid())
    {
        Error err(N_("end of file in procedure frame"));
        err.setXRef(m_proc_frame, N_("procedure started here"));
        errwarns.Propagate(clang::SourceRange(), err);
        return;
    }

    // Force all syms for win64 because they're needed for relocations.
    // FIXME: Not *all* syms need to be output, only the ones needed for
    // relocation.  Find a way to do that someday.
    Win32Object::Output(os, true, errwarns);
}

void
Win64Object::DirProcFrame(DirectiveInfo& info)
{
    assert(info.isObject(m_object));
    NameValues& namevals = info.getNameValues();
    clang::SourceLocation source = info.getSource();

    if (!namevals.front().isId())
        throw SyntaxError(N_("argument to [PROC_FRAME] must be symbol name"));
    llvm::StringRef name = namevals.front().getId();

    if (m_proc_frame.isValid())
    {
        SyntaxError err(
            N_("nested procedures not supported (didn't use [ENDPROC_FRAME]?)"));
        err.setXRef(m_proc_frame, N_("previous procedure started here"));
        throw err;
    }
    m_proc_frame = source;
    m_done_prolog = clang::SourceLocation();
    m_unwind.reset(new UnwindInfo);

    SymbolRef proc = m_object.getSymbol(name);
    proc->Use(source);
    m_unwind->setProc(proc);

    // Optional error handler
    if (namevals.size() > 1)
    {
        if (!namevals[1].isId())
        {
            throw SyntaxError(
                N_("[PROC_FRAME] error handler must be symbol name"));
        }
        SymbolRef ehandler = m_object.getSymbol(namevals[1].getId());
        ehandler->Use(source);
        m_unwind->setEHandler(ehandler);
    }
}

void
Win64Object::CheckProcFrameState(llvm::StringRef dirname)
{
    if (!m_proc_frame.isValid())
    {
        throw SyntaxError(String::Compose(
            N_("[%1] without preceding [PROC_FRAME]"), dirname));
    }

    if (m_done_prolog.isValid())
    {
        SyntaxError err(String::Compose(
            N_("[%1] after end of prologue"), dirname));
        err.setXRef(m_done_prolog, N_("prologue ended here"));
        throw err;
    }
}

// Get current assembly position.
static SymbolRef
getCurPos(Object& object, llvm::StringRef dirname, clang::SourceLocation source)
{
    Section* sect = object.getCurSection();
    if (!sect)
    {
        throw SyntaxError(String::Compose(
            N_("[%1] can only be used inside of a section"), dirname));
    }
    SymbolRef sym = object.AddNonTableSymbol("$");
    Bytecode& bc = sect->FreshBytecode();
    Location loc = {&bc, bc.getFixedLen()};
    sym->DefineLabel(loc, source);
    return sym;
}

static const Register*
getRegisterFromNameValue(Object& object, NameValue& nv,
                         clang::SourceLocation source)
{
    if (!nv.isExpr())
        return 0;
    Expr e = nv.getExpr(object, source);
    if (!e.isRegister())
        return 0;
    return e.getRegister();
}

void
Win64Object::DirPushReg(DirectiveInfo& info)
{
    assert(info.isObject(m_object));
    NameValues& namevals = info.getNameValues();

    assert(!namevals.empty());
    CheckProcFrameState("PUSHREG");

    const Register* reg =
        getRegisterFromNameValue(m_object, namevals.front(), info.getSource());
    if (!reg)
    {
        throw SyntaxError(String::Compose(
            N_("[%1] requires a register as the first parameter"), "PUSHREG"));
    }

    // Generate a PUSH_NONVOL unwind code.
    m_unwind->AddCode(std::auto_ptr<UnwindCode>(new UnwindCode(
        m_unwind->getProc(),
        getCurPos(m_object, "PUSHREG", info.getSource()),
        UnwindCode::PUSH_NONVOL,
        reg->getNum() & 0xF)));
}

void
Win64Object::DirSetFrame(DirectiveInfo& info)
{
    assert(info.isObject(m_object));
    NameValues& namevals = info.getNameValues();
    clang::SourceLocation source = info.getSource();

    assert(!namevals.empty());
    CheckProcFrameState("SETFRAME");

    const Register* reg =
        getRegisterFromNameValue(m_object, namevals.front(), source);
    if (!reg)
    {
        throw SyntaxError(String::Compose(
            N_("[%1] requires a register as the first parameter"), "SETFRAME"));
    }

    std::auto_ptr<Expr> off(0);
    if (namevals.size() > 1)
        off = namevals[1].ReleaseExpr(m_object, source);
    else
        off.reset(new Expr(0));

    // Set the frame fields in the unwind info
    m_unwind->setFrameReg(reg->getNum());
    m_unwind->setFrameOff(Value(8, std::auto_ptr<Expr>(off->clone())));

    // Generate a SET_FPREG unwind code
    m_unwind->AddCode(std::auto_ptr<UnwindCode>(new UnwindCode(
        m_unwind->getProc(),
        getCurPos(m_object, "SETFRAME", source),
        UnwindCode::SET_FPREG,
        reg->getNum() & 0xF,
        8,
        off)));
}

void
Win64Object::DirAllocStack(DirectiveInfo& info)
{
    assert(info.isObject(m_object));
    NameValues& namevals = info.getNameValues();

    assert(!namevals.empty());
    CheckProcFrameState("ALLOCSTACK");

    NameValue& nv = namevals.front();
    if (!nv.isExpr())
    {
        throw SyntaxError(String::Compose(
            N_("[%1] requires a size"), "ALLOCSTACK"));
    }

    // Generate an ALLOC_SMALL unwind code; this will get enlarged to an
    // ALLOC_LARGE if necessary.
    m_unwind->AddCode(std::auto_ptr<UnwindCode>(new UnwindCode(
        m_unwind->getProc(),
        getCurPos(m_object, "ALLOCSTACK", info.getSource()),
        UnwindCode::ALLOC_SMALL,
        0,
        7,
        nv.ReleaseExpr(m_object, info.getSource()))));
}

void
Win64Object::SaveCommon(DirectiveInfo& info,
                        llvm::StringRef dirname,
                        UnwindCode::Opcode op)
{
    assert(info.isObject(m_object));
    NameValues& namevals = info.getNameValues();
    clang::SourceLocation source = info.getSource();

    assert(!namevals.empty());
    CheckProcFrameState(dirname);

    const Register* reg =
        getRegisterFromNameValue(m_object, namevals.front(), source);
    if (!reg)
    {
        throw SyntaxError(String::Compose(
            N_("[%1] requires a register as the first parameter"), dirname));
    }

    if (namevals.size() < 2 || !namevals[1].isExpr())
    {
        throw SyntaxError(String::Compose(
            N_("[%1] requires an offset as the second parameter"), dirname));
    }

    // Generate a SAVE_XXX unwind code; this will get enlarged to a
    // SAVE_XXX_FAR if necessary.
    m_unwind->AddCode(std::auto_ptr<UnwindCode>(new UnwindCode(
        m_unwind->getProc(),
        getCurPos(m_object, dirname, source),
        op,
        reg->getNum() & 0xF,
        16,
        namevals[1].ReleaseExpr(m_object, source))));
}

void
Win64Object::DirSaveReg(DirectiveInfo& info)
{
    SaveCommon(info, "SAVEREG", UnwindCode::SAVE_NONVOL);
}

void
Win64Object::DirSaveXMM128(DirectiveInfo& info)
{
    SaveCommon(info, "SAVEXMM128", UnwindCode::SAVE_XMM128);
}

void
Win64Object::DirPushFrame(DirectiveInfo& info)
{
    assert(info.isObject(m_object));
    CheckProcFrameState("PUSHFRAME");

    // Generate a PUSH_MACHFRAME unwind code.  If there's any parameter,
    // we set info to 1.  Otherwise we set info to 0.
    m_unwind->AddCode(std::auto_ptr<UnwindCode>(new UnwindCode(
        m_unwind->getProc(),
        getCurPos(m_object, "PUSHFRAME", info.getSource()),
        UnwindCode::PUSH_MACHFRAME,
        info.getNameValues().empty() ? 0 : 1)));
}

void
Win64Object::DirEndProlog(DirectiveInfo& info)
{
    assert(info.isObject(m_object));
    CheckProcFrameState("ENDPROLOG");
    m_done_prolog = info.getSource();

    m_unwind->setProlog(getCurPos(m_object, "ENDPROLOG", info.getSource()));
}

void
Win64Object::DirEndProcFrame(DirectiveInfo& info)
{
    assert(info.isObject(m_object));
    clang::SourceLocation source = info.getSource();

    if (!m_proc_frame.isValid())
    {
        throw SyntaxError(String::Compose(
            N_("[%1] without preceding [PROC_FRAME]"), "ENDPROC_FRAME"));
    }
    if (!m_done_prolog.isValid())
    {
        SyntaxError err(N_("ended procedure without ending prologue"));
        err.setXRef(m_proc_frame, N_("procedure started here"));

        m_unwind.reset(0);
        m_proc_frame = clang::SourceLocation();
        throw err;
    }
    assert(m_unwind.get() != 0 && "unwind info not present");

    SymbolRef proc_sym = m_unwind->getProc();

    SymbolRef curpos = getCurPos(m_object, "ENDPROC_FRAME", source);

    //
    // Add unwind info to end of .xdata section.
    //

    Section* xdata = m_object.FindSection(".xdata");

    // Create xdata section if needed.
    if (!xdata)
        xdata = AppendSection(".xdata", source);

    // Get current position in .xdata section.
    SymbolRef unwindpos = m_object.AddNonTableSymbol("$");
    Location unwindpos_loc =
        {&xdata->bytecodes_last(), xdata->bytecodes_last().getFixedLen()};
    unwindpos->DefineLabel(unwindpos_loc, source);
    // Get symbol for .xdata as we'll want to reference it with WRT.
    SymbolRef xdata_sym = xdata->getAssocData<CoffSection>()->m_sym;

    // Add unwind info.  Use line number of start of procedure.
    Arch& arch = *m_object.getArch();
    Generate(m_unwind, *xdata, m_proc_frame, arch);

    //
    // Add function lookup to end of .pdata section.
    //

    Section* pdata = m_object.FindSection(".pdata");

    // Initialize pdata section if needed.
    if (!pdata)
        pdata = AppendSection(".pdata", source);

    // Add function structure to end of .pdata
    AppendData(*pdata, std::auto_ptr<Expr>(new Expr(proc_sym)), 4, arch,
               source);
    AppendData(*pdata, std::auto_ptr<Expr>(new Expr(WRT(curpos, proc_sym))),
               4, arch, source);
    AppendData(*pdata,
               std::auto_ptr<Expr>(new Expr(WRT(unwindpos, xdata_sym))),
               4, arch, source);

    m_proc_frame = clang::SourceLocation();
    m_done_prolog = clang::SourceLocation();
}

void
Win64Object::AddDirectives(Directives& dirs, llvm::StringRef parser)
{
    static const Directives::Init<Win64Object> gas_dirs[] =
    {
        {".export",     &Win64Object::DirExport,     Directives::ID_REQUIRED},
        {".proc_frame", &Win64Object::DirProcFrame,  Directives::ID_REQUIRED},
        {".pushreg",    &Win64Object::DirPushReg,    Directives::ARG_REQUIRED},
        {".setframe",   &Win64Object::DirSetFrame,   Directives::ARG_REQUIRED},
        {".allocstack", &Win64Object::DirAllocStack, Directives::ARG_REQUIRED},
        {".savereg",    &Win64Object::DirSaveReg,    Directives::ARG_REQUIRED},
        {".savexmm128", &Win64Object::DirSaveXMM128, Directives::ARG_REQUIRED},
        {".pushframe",  &Win64Object::DirPushFrame,  Directives::ANY},
        {".endprolog",  &Win64Object::DirEndProlog,  Directives::ANY},
        {".endproc_frame", &Win64Object::DirEndProcFrame, Directives::ANY},
    };
    static const Directives::Init<Win64Object> nasm_dirs[] =
    {
        {"export",     &Win64Object::DirExport,     Directives::ID_REQUIRED},
        {"proc_frame", &Win64Object::DirProcFrame,  Directives::ID_REQUIRED},
        {"pushreg",    &Win64Object::DirPushReg,    Directives::ARG_REQUIRED},
        {"setframe",   &Win64Object::DirSetFrame,   Directives::ARG_REQUIRED},
        {"allocstack", &Win64Object::DirAllocStack, Directives::ARG_REQUIRED},
        {"savereg",    &Win64Object::DirSaveReg,    Directives::ARG_REQUIRED},
        {"savexmm128", &Win64Object::DirSaveXMM128, Directives::ARG_REQUIRED},
        {"pushframe",  &Win64Object::DirPushFrame,  Directives::ANY},
        {"endprolog",  &Win64Object::DirEndProlog,  Directives::ANY},
        {"endproc_frame", &Win64Object::DirEndProcFrame, Directives::ANY},
    };

    if (String::NocaseEqual(parser, "nasm"))
        dirs.AddArray(this, nasm_dirs, NELEMS(nasm_dirs));
    else if (String::NocaseEqual(parser, "gas"))
        dirs.AddArray(this, gas_dirs, NELEMS(gas_dirs));

    // Pull in coff directives (but not win32 directives)
    CoffObject::AddDirectives(dirs, parser);
}

bool
Win64Object::InitSection(llvm::StringRef name,
                         Section& section,
                         CoffSection* coffsect)
{
    if (Win32Object::InitSection(name, section, coffsect))
        return true;
    else if (name == ".pdata")
    {
        coffsect->m_flags = CoffSection::DATA | CoffSection::READ;
        section.setAlign(4);
        coffsect->m_nobase = true;
    }
    else if (name == ".xdata")
    {
        coffsect->m_flags = CoffSection::DATA | CoffSection::READ;
        section.setAlign(8);
    }
    else
    {
        // Default to code (NASM default; note GAS has different default.
        coffsect->m_flags =
            CoffSection::TEXT | CoffSection::EXECUTE | CoffSection::READ;
        section.setCode(true);
        return false;
    }
    return true;
}

#if 0
#include "win64-nasm.c"
#include "win64-gas.c"

static const yasm_stdmac win64_objfmt_stdmacs[] =
{
    { "nasm", "nasm", win64_nasm_stdmac },
    { "gas", "nasm", win64_gas_stdmac },
    { NULL, NULL, NULL }
};
#endif

void
DoRegister()
{
    RegisterModule<ObjectFormatModule,
                   ObjectFormatModuleImpl<Win64Object> >("win64");
    RegisterModule<ObjectFormatModule,
                   ObjectFormatModuleImpl<Win64Object> >("x64");
}

}}} // namespace yasm::objfmt::win64

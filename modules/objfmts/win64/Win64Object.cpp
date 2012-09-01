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
#include "Win64Object.h"

#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Parse/Directive.h"
#include "yasmx/Parse/NameValue.h"
#include "yasmx/Support/registry.h"
#include "yasmx/Arch.h"
#include "yasmx/BytecodeContainer.h"
#include "yasmx/Object.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"

#include "modules/objfmts/coff/CoffSection.h"


using namespace yasm;
using namespace yasm::objfmt;

Win64Object::Win64Object(const ObjectFormatModule& module, Object& object)
    : Win32Object(module, object)
    , m_unwind(0)
{
}

Win64Object::~Win64Object()
{
}

void
Win64Object::Output(raw_fd_ostream& os,
                    bool all_syms,
                    DebugFormat& dbgfmt,
                    DiagnosticsEngine& diags)
{
    if (m_proc_frame.isValid())
    {
        diags.Report(SourceLocation(), diag::err_eof_proc_frame);
        diags.Report(m_proc_frame, diag::note_proc_started_here);
        return;
    }

    // Force all syms for win64 because they're needed for relocations.
    // FIXME: Not *all* syms need to be output, only the ones needed for
    // relocation.  Find a way to do that someday.
    Win32Object::Output(os, true, dbgfmt, diags);
}

void
Win64Object::DirProcFrame(DirectiveInfo& info, DiagnosticsEngine& diags)
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
    StringRef name = name_nv.getId();

    if (m_proc_frame.isValid())
    {
        diags.Report(info.getSource(),
                     diags.getCustomDiagID(DiagnosticsEngine::Error,
            "nested procedures not supported (didn't use [ENDPROC_FRAME]?)"));
        diags.Report(m_proc_frame,
                     diags.getCustomDiagID(DiagnosticsEngine::Note,
                                           "previous procedure started here"));
        return;
    }
    m_proc_frame = source;
    m_done_prolog = SourceLocation();
    m_unwind.reset(new UnwindInfo);

    SymbolRef proc = m_object.getSymbol(name);
    proc->Use(source);
    m_unwind->setProc(proc);

    // Optional error handler
    if (namevals.size() > 1)
    {
        NameValue& ehandler_nv = namevals[1];
        if (!ehandler_nv.isId())
        {
            diags.Report(info.getSource(), diag::err_value_id)
                << ehandler_nv.getValueRange();
            return;
        }
        SymbolRef ehandler = m_object.getSymbol(ehandler_nv.getId());
        ehandler->Use(ehandler_nv.getValueRange().getBegin());
        m_unwind->setEHandler(ehandler);
    }
}

bool
Win64Object::CheckProcFrameState(SourceLocation dir_source,
                                 DiagnosticsEngine& diags)
{
    if (!m_proc_frame.isValid())
    {
        diags.Report(dir_source,
                     diags.getCustomDiagID(DiagnosticsEngine::Error,
                                           "no preceding [PROC_FRAME]"));
        return false;
    }

    if (m_done_prolog.isValid())
    {
        diags.Report(dir_source,
                     diags.getCustomDiagID(DiagnosticsEngine::Error,
                                           "must come before [END_PROLOGUE]"));
        diags.Report(m_done_prolog,
            diags.getCustomDiagID(DiagnosticsEngine::Note,
                                  "prologue ended here"));
        return false;
    }
    return true;
}

void
Win64Object::DirPushReg(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    assert(info.isObject(m_object));
    NameValues& namevals = info.getNameValues();

    assert(!namevals.empty());
    if (!CheckProcFrameState(info.getSource(), diags))
        return;

    if (!namevals.front().isRegister())
    {
        diags.Report(info.getSource(), diag::err_value_register)
            << namevals.front().getValueRange();
        return;
    }
    const Register* reg = namevals.front().getRegister();

    // Generate a PUSH_NONVOL unwind code.
    m_unwind->AddCode(std::auto_ptr<UnwindCode>(new UnwindCode(
        m_unwind->getProc(),
        m_object.getSymbol(info.getLocation()),
        UnwindCode::PUSH_NONVOL,
        reg->getNum() & 0xF)));
}

void
Win64Object::DirSetFrame(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    assert(info.isObject(m_object));
    NameValues& namevals = info.getNameValues();

    assert(!namevals.empty());
    if (!CheckProcFrameState(info.getSource(), diags))
        return;

    if (!namevals.front().isRegister())
    {
        diags.Report(info.getSource(), diag::err_value_register)
            << namevals.front().getValueRange();
        return;
    }
    const Register* reg = namevals.front().getRegister();

    std::auto_ptr<Expr> off(0);
    if (namevals.size() > 1)
        off = namevals[1].ReleaseExpr(m_object);
    else
        off.reset(new Expr(0));

    // Set the frame fields in the unwind info
    m_unwind->setFrameReg(reg->getNum());
    m_unwind->setFrameOff(Value(8, std::auto_ptr<Expr>(off->clone())));

    // Generate a SET_FPREG unwind code
    m_unwind->AddCode(std::auto_ptr<UnwindCode>(new UnwindCode(
        m_unwind->getProc(),
        m_object.getSymbol(info.getLocation()),
        UnwindCode::SET_FPREG,
        reg->getNum() & 0xF,
        8,
        off)));
}

void
Win64Object::DirAllocStack(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    assert(info.isObject(m_object));
    NameValues& namevals = info.getNameValues();

    assert(!namevals.empty());
    if (!CheckProcFrameState(info.getSource(), diags))
        return;

    NameValue& nv = namevals.front();
    if (!nv.isExpr())
    {
        diags.Report(info.getSource(), diag::err_value_expression)
            << nv.getValueRange();
        return;
    }

    // Generate an ALLOC_SMALL unwind code; this will get enlarged to an
    // ALLOC_LARGE if necessary.
    m_unwind->AddCode(std::auto_ptr<UnwindCode>(new UnwindCode(
        m_unwind->getProc(),
        m_object.getSymbol(info.getLocation()),
        UnwindCode::ALLOC_SMALL,
        0,
        7,
        nv.ReleaseExpr(m_object))));
}

void
Win64Object::SaveCommon(DirectiveInfo& info,
                        UnwindCode::Opcode op,
                        DiagnosticsEngine& diags)
{
    assert(info.isObject(m_object));
    NameValues& namevals = info.getNameValues();
    SourceLocation source = info.getSource();

    assert(!namevals.empty());
    if (!CheckProcFrameState(info.getSource(), diags))
        return;

    if (!namevals.front().isRegister())
    {
        diags.Report(source, diag::err_value_register)
            << namevals.front().getValueRange();
        return;
    }
    const Register* reg = namevals.front().getRegister();

    if (namevals.size() < 2)
    {
        diags.Report(source, diag::err_no_offset);
        return;
    }

    if (!namevals[1].isExpr())
    {
        diags.Report(source, diag::err_offset_expression)
            << namevals[1].getValueRange();
        return;
    }

    // Generate a SAVE_XXX unwind code; this will get enlarged to a
    // SAVE_XXX_FAR if necessary.
    m_unwind->AddCode(std::auto_ptr<UnwindCode>(new UnwindCode(
        m_unwind->getProc(),
        m_object.getSymbol(info.getLocation()),
        op,
        reg->getNum() & 0xF,
        16,
        namevals[1].ReleaseExpr(m_object))));
}

void
Win64Object::DirSaveReg(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    SaveCommon(info, UnwindCode::SAVE_NONVOL, diags);
}

void
Win64Object::DirSaveXMM128(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    SaveCommon(info, UnwindCode::SAVE_XMM128, diags);
}

void
Win64Object::DirPushFrame(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    assert(info.isObject(m_object));
    if (!CheckProcFrameState(info.getSource(), diags))
        return;

    // Generate a PUSH_MACHFRAME unwind code.  If there's any parameter,
    // we set info to 1.  Otherwise we set info to 0.
    m_unwind->AddCode(std::auto_ptr<UnwindCode>(new UnwindCode(
        m_unwind->getProc(),
        m_object.getSymbol(info.getLocation()),
        UnwindCode::PUSH_MACHFRAME,
        info.getNameValues().empty() ? 0 : 1)));
}

void
Win64Object::DirEndProlog(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    assert(info.isObject(m_object));
    if (!CheckProcFrameState(info.getSource(), diags))
        return;
    m_done_prolog = info.getSource();

    m_unwind->setProlog(m_object.getSymbol(info.getLocation()));
}

void
Win64Object::DirEndProcFrame(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    assert(info.isObject(m_object));
    SourceLocation source = info.getSource();

    if (!m_proc_frame.isValid())
    {
        diags.Report(source,
                     diags.getCustomDiagID(DiagnosticsEngine::Error,
                                           "no preceding [PROC_FRAME]"));
        return;
    }
    if (!m_done_prolog.isValid())
    {
        diags.Report(info.getSource(),
                     diags.getCustomDiagID(DiagnosticsEngine::Error,
            "ended procedure without ending prologue"));
        diags.Report(m_proc_frame,
                     diags.getCustomDiagID(DiagnosticsEngine::Note,
                                           "procedure started here"));
        m_unwind.reset(0);
        m_proc_frame = SourceLocation();
        return;
    }
    assert(m_unwind.get() != 0 && "unwind info not present");

    SymbolRef proc_sym = m_unwind->getProc();

    SymbolRef curpos = m_object.getSymbol(info.getLocation());

    //
    // Add unwind info to end of .xdata section.
    //

    Section* xdata = m_object.FindSection(".xdata");

    // Create xdata section if needed.
    if (!xdata)
        xdata = AppendSection(".xdata", source, diags);

    // Get current position in .xdata section.
    SymbolRef unwindpos = m_object.AddNonTableSymbol("$");
    Location unwindpos_loc =
        {&xdata->bytecodes_back(), xdata->bytecodes_back().getFixedLen()};
    unwindpos->DefineLabel(unwindpos_loc);
    // Get symbol for .xdata as we'll want to reference it with WRT.
    SymbolRef xdata_sym = xdata->getAssocData<CoffSection>()->m_sym;

    // Add unwind info.  Use line number of start of procedure.
    Arch& arch = *m_object.getArch();
    Generate(m_unwind, *xdata, m_proc_frame, arch, diags);

    //
    // Add function lookup to end of .pdata section.
    //

    Section* pdata = m_object.FindSection(".pdata");

    // Initialize pdata section if needed.
    if (!pdata)
        pdata = AppendSection(".pdata", source, diags);

    // Add function structure to end of .pdata
    AppendData(*pdata, std::auto_ptr<Expr>(new Expr(proc_sym)), 4, arch,
               source, diags);
    AppendData(*pdata, std::auto_ptr<Expr>(new Expr(WRT(curpos, proc_sym))),
               4, arch, source, diags);
    AppendData(*pdata,
               std::auto_ptr<Expr>(new Expr(WRT(unwindpos, xdata_sym))),
               4, arch, source, diags);

    m_proc_frame = SourceLocation();
    m_done_prolog = SourceLocation();
}

void
Win64Object::AddDirectives(Directives& dirs, StringRef parser)
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

    if (parser.equals_lower("nasm"))
        dirs.AddArray(this, nasm_dirs);
    else if (parser.equals_lower("gas") || parser.equals_lower("gnu"))
        dirs.AddArray(this, gas_dirs);

    // Pull in coff directives (but not win32 directives)
    CoffObject::AddDirectives(dirs, parser);
}

bool
Win64Object::InitSection(StringRef name,
                         Section& section,
                         CoffSection* coffsect,
                         SourceLocation source,
                         DiagnosticsEngine& diags)
{
    if (Win32Object::InitSection(name, section, coffsect, source, diags))
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
yasm_objfmt_win64_DoRegister()
{
    RegisterModule<ObjectFormatModule,
                   ObjectFormatModuleImpl<Win64Object> >("win64");
    RegisterModule<ObjectFormatModule,
                   ObjectFormatModuleImpl<Win64Object> >("x64");
}

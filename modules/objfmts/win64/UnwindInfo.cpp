//
// Win64 structured exception handling support
//
//  Copyright (C) 2007  Peter Johnson
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
#include "UnwindInfo.h"

#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/BytecodeContainer.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/Expr.h"
#include "yasmx/Symbol.h"


using namespace yasm;
using namespace yasm::objfmt;

#define UNW_FLAG_EHANDLER   0x01
#define UNW_FLAG_UHANDLER   0x02
#define UNW_FLAG_CHAININFO  0x04

UnwindInfo::UnwindInfo()
    : m_proc(0)
    , m_prolog(0)
    , m_ehandler(0)
    , m_framereg(0)
    , m_frameoff(8)     // Frameoff is really a 4-bit value, scaled by 16
    , m_prolog_size(8)
    , m_codes_count(8)
{
}

UnwindInfo::~UnwindInfo()
{
    for (std::vector<UnwindCode*>::iterator i=m_codes.begin(),
         end=m_codes.end(); i != end; ++i)
    {
        delete *i;
    }
}

bool
UnwindInfo::Finalize(Bytecode& bc, DiagnosticsEngine& diags)
{
    if (!m_prolog_size.Finalize(diags))
        return false;
    if (!m_codes_count.Finalize(diags))
        return false;
    if (!m_frameoff.Finalize(diags))
        return false;
    return true;
}

bool
UnwindInfo::CalcLen(Bytecode& bc,
                    /*@out@*/ unsigned long* len,
                    const Bytecode::AddSpanFunc& add_span,
                    DiagnosticsEngine& diags)
{
    // Want to make sure prolog size and codes count doesn't exceed
    // byte-size, and scaled frame offset doesn't exceed 4 bits.
    add_span(bc, 1, m_prolog_size, 0, 255);
    add_span(bc, 2, m_codes_count, 0, 255);

    IntNum intn;
    if (m_frameoff.getIntNum(&intn, false, diags))
    {
        if (!intn.isInRange(0, 240))
        {
            diags.Report(m_frameoff.getSource().getBegin(),
                         diag::err_offset_out_of_range)
                << intn.getStr() << "0" << 240;
            return false;
        }
        else if ((intn.getUInt() & 0xF) != 0)
        {
            diags.Report(m_frameoff.getSource().getBegin(),
                         diag::err_offset_not_multiple)
                << intn.getStr() << 16;
            return false;
        }
    }
    else
        add_span(bc, 3, m_frameoff, 0, 240);

    *len = 4;
    return true;
}

bool
UnwindInfo::Expand(Bytecode& bc,
                   unsigned long* len,
                   int span,
                   long old_val,
                   long new_val,
                   bool* keep,
                   /*@out@*/ long* neg_thres,
                   /*@out@*/ long* pos_thres,
                   DiagnosticsEngine& diags)
{
    switch (span)
    {
        case 1:
        {
            diags.Report(m_prolog_size.getSource().getBegin(),
                         diag::err_prologue_too_large)
                << static_cast<int>(new_val);
            diags.Report(m_prolog->getDefSource(), diag::note_prologue_end);
            return false;
        }
        case 2:
            diags.Report(m_frameoff.getSource().getBegin(),
                         diag::err_too_many_unwind_codes)
                << static_cast<int>(new_val);
            return false;
        case 3:
            diags.Report(m_frameoff.getSource().getBegin(),
                         diag::err_offset_out_of_range)
                << static_cast<int>(new_val) << "0" << 240;
            return false;
        default:
            assert(false && "unrecognized span id");
    }
    *keep = false;
    return true;
}

bool
UnwindInfo::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    Bytes& bytes = bc_out.getScratch();
    Location loc = {&bc, 0};

    // Version and flags
    if (m_ehandler)
        Write8(bytes, 1 | (UNW_FLAG_EHANDLER << 3));
    else
        Write8(bytes, 1);
    bc_out.OutputBytes(bytes, bc.getSource());

    // Size of prolog
    {
        bytes.resize(0);
        Write8(bytes, 0);
        NumericOutput num_out(bytes);
        m_prolog_size.ConfigureOutput(&num_out);
        bc_out.OutputValue(m_prolog_size, loc, num_out);
    }

    // Count of codes
    {
        bytes.resize(0);
        Write8(bytes, 0);
        NumericOutput num_out(bytes);
        m_codes_count.ConfigureOutput(&num_out);
        bc_out.OutputValue(m_codes_count, loc, num_out);
    }

    // Frame register and offset
    IntNum intn;
    if (!m_frameoff.getIntNum(&intn, true, bc_out.getDiagnostics()))
    {
        bc_out.Diag(m_frameoff.getSource().getBegin(),
                    diag::err_too_complex_expression);
        return false;
    }

    if (!intn.isInRange(0, 240))
    {
        bc_out.Diag(m_frameoff.getSource().getBegin(),
                    diag::err_offset_out_of_range)
            << intn.getStr() << "0" << 240;
        return false;
    }
    else if ((intn.getUInt() & 0xF) != 0)
    {
        bc_out.Diag(m_frameoff.getSource().getBegin(),
                    diag::err_offset_not_multiple)
            << intn.getStr() << 16;
        return false;
    }

    bytes.resize(0);
    Write8(bytes, (intn.getUInt() & 0xF0) | (m_framereg & 0x0F));
    bc_out.OutputBytes(bytes, m_frameoff.getSource().getBegin());
    return true;
}

StringRef
UnwindInfo::getType() const
{
    return "yasm::objfmt::UnwindInfo";
}

UnwindInfo*
UnwindInfo::clone() const
{
    std::auto_ptr<UnwindInfo> info(new UnwindInfo);
    info->m_proc = m_proc;
    info->m_prolog = m_prolog;
    info->m_ehandler = m_ehandler;
    info->m_framereg = m_framereg;
    info->m_frameoff = m_frameoff;
    for (std::vector<UnwindCode*>::const_iterator i=m_codes.begin(),
         end=m_codes.end(); i != end; ++i)
    {
        if (*i)
            info->m_codes.push_back((*i)->clone());
        else
            info->m_codes.push_back(0);
    }
    info->m_prolog_size = m_prolog_size;
    info->m_codes_count = m_codes_count;
    return info.release();
}

#ifdef WITH_XML
pugi::xml_node
UnwindInfo::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("UnwindInfo");

    if (m_proc)
        append_child(root, "Proc", m_proc);

    if (m_prolog)
        append_child(root, "Prolog", m_prolog);

    if (m_ehandler)
        append_child(root, "EHandler", m_ehandler);

    append_child(root, "FrameReg", m_framereg);
    append_child(root, "FrameOff", m_frameoff);

    for (std::vector<UnwindCode*>::const_iterator i=m_codes.begin(),
        end=m_codes.end(); i != end; ++i)
        append_data(root, **i);

    append_child(root, "PrologSize", m_prolog_size);
    append_child(root, "CodesCount", m_codes_count);
    return root;
}
#endif // WITH_XML

void
objfmt::Generate(std::auto_ptr<UnwindInfo> uwinfo,
                 BytecodeContainer& xdata,
                 SourceLocation source,
                 const Arch& arch,
                 DiagnosticsEngine& diags)
{
    UnwindInfo* info = uwinfo.get();

    // 4-byte align the start of unwind info
    AppendAlign(xdata, Expr(4), Expr(), Expr(), 0, source);

    // Prolog size = end of prolog - start of procedure
    info->m_prolog_size.AddAbs(SUB(info->m_prolog, info->m_proc));

    // Unwind info
    Bytecode& infobc = xdata.FreshBytecode();
    infobc.Transform(Bytecode::Contents::Ptr(uwinfo));
    infobc.setSource(source);

    Bytecode& startbc = xdata.FreshBytecode();
    Location startloc = {&startbc, startbc.getFixedLen()};

    // Code array
    for (std::vector<UnwindCode*>::reverse_iterator i=info->m_codes.rbegin(),
         end=info->m_codes.rend(); i != end; ++i)
    {
        AppendUnwindCode(xdata, std::auto_ptr<UnwindCode>(*i));
        *i = 0; // don't double-free
    }

    // Number of codes = (Last code - end of info) >> 1
    if (info->m_codes.size() > 0)
    {
        Bytecode& bc = xdata.FreshBytecode();
        Location endloc = {&bc, bc.getFixedLen()};
        info->m_codes_count.AddAbs(SHR(SUB(endloc, startloc), 1));
    }

    // 4-byte align
    AppendAlign(xdata, Expr(4), Expr(), Expr(), 0, source);

    // Exception handler, if present.
    if (info->m_ehandler)
        AppendData(xdata, Expr::Ptr(new Expr(info->m_ehandler)), 4, arch,
                   source, diags);
}

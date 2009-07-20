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

#include <util.h>

#include <YAML/emitter.h>
#include <yasmx/Support/Compose.h>
#include <yasmx/Support/errwarn.h>
#include <yasmx/BytecodeContainer.h>
#include <yasmx/BytecodeContainer_util.h>
#include <yasmx/BytecodeOutput.h>
#include <yasmx/Bytes.h>
#include <yasmx/Bytes_util.h>
#include <yasmx/Expr.h>
#include <yasmx/Symbol.h>


namespace yasm
{
namespace objfmt
{
namespace win64
{

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

void
UnwindInfo::Finalize(Bytecode& bc)
{
    if (!m_prolog_size.Finalize())
        throw ValueError(N_("prolog size expression too complex"));

    if (!m_codes_count.Finalize())
        throw ValueError(N_("codes count expression too complex"));

    if (!m_frameoff.Finalize())
        throw ValueError(N_("frame offset expression too complex"));
}

unsigned long
UnwindInfo::CalcLen(Bytecode& bc, const Bytecode::AddSpanFunc& add_span)
{
    // Want to make sure prolog size and codes count doesn't exceed
    // byte-size, and scaled frame offset doesn't exceed 4 bits.
    add_span(bc, 1, m_prolog_size, 0, 255);
    add_span(bc, 2, m_codes_count, 0, 255);

    IntNum intn;
    if (m_frameoff.getIntNum(&intn, false))
    {
        if (!intn.isInRange(0, 240))
            throw ValueError(String::Compose(
                N_("frame offset of %1 bytes, must be between 0 and 240"),
                intn));
        else if ((intn.getUInt() & 0xF) != 0)
            throw ValueError(String::Compose(
                N_("frame offset of %1 is not a multiple of 16"), intn));
    }
    else
        add_span(bc, 3, m_frameoff, 0, 240);

    return 4;
}

bool
UnwindInfo::Expand(Bytecode& bc,
                   unsigned long& len,
                   int span,
                   long old_val,
                   long new_val,
                   /*@out@*/ long* neg_thres,
                   /*@out@*/ long* pos_thres)
{
    switch (span)
    {
        case 1:
        {
            ValueError err(String::Compose(
                N_("prologue %1 bytes, must be <256"), new_val));
            err.setXRef(m_prolog->getDefLine(), N_("prologue ended here"));
            throw err;
        }
        case 2:
            throw ValueError(String::Compose(
                N_("%1 unwind codes, maximum of 255"), new_val));
        case 3:
            throw ValueError(String::Compose(
                N_("frame offset of %1 bytes, must be between 0 and 240"),
                new_val));
        default:
            assert(false && "unrecognized span id");
    }
    return 0;
}

void
UnwindInfo::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    Bytes& bytes = bc_out.getScratch();
    Location loc = {&bc, 0};

    // Version and flags
    if (m_ehandler)
        Write8(bytes, 1 | (UNW_FLAG_EHANDLER << 3));
    else
        Write8(bytes, 1);
    bc_out.Output(bytes);

    // Size of prolog
    bytes.resize(0);
    Write8(bytes, 0);
    bc_out.Output(m_prolog_size, bytes, loc, 1);

    // Count of codes
    bytes.resize(0);
    Write8(bytes, 0);
    bc_out.Output(m_codes_count, bytes, loc, 1);

    // Frame register and offset
    IntNum intn;
    if (!m_frameoff.getIntNum(&intn, true))
        throw ValueError(N_("frame offset expression too complex"));
    if (!intn.isInRange(0, 240))
        throw ValueError(String::Compose(
            N_("frame offset of %1 bytes, must be between 0 and 240"), intn));
    else if ((intn.getUInt() & 0xF) != 0)
        throw ValueError(String::Compose(
            N_("frame offset of %1 is not a multiple of 16"), intn));

    bytes.resize(0);
    Write8(bytes, (intn.getUInt() & 0xF0) | (m_framereg & 0x0F));
    bc_out.Output(bytes);
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

void
UnwindInfo::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << "UnwindInfo";

    out << YAML::Key << "proc" << YAML::Value;
    if (m_proc)
        out << m_proc->getName();
    else
        out << YAML::Null;

    out << YAML::Key << "prolog" << YAML::Value;
    if (m_prolog)
        out << m_prolog->getName();
    else
        out << YAML::Null;

    out << YAML::Key << "ehandler" << YAML::Value;
    if (m_ehandler)
        out << m_ehandler->getName();
    else
        out << YAML::Null;

    out << YAML::Key << "frame reg" << YAML::Value << m_framereg;
    out << YAML::Key << "frame off" << YAML::Value << m_frameoff;

    out << YAML::Key << "codes" << YAML::Value << YAML::BeginSeq;
    for (std::vector<UnwindCode*>::const_iterator i=m_codes.begin(),
        end=m_codes.end(); i != end; ++i)
        (*i)->Write(out);
    out << YAML::EndSeq;

    out << YAML::Key << "prolog size" << YAML::Value << m_prolog_size;
    out << YAML::Key << "codes count" << YAML::Value << m_codes_count;

    out << YAML::EndMap;
}

void
Generate(std::auto_ptr<UnwindInfo> uwinfo,
         BytecodeContainer& xdata,
         unsigned long line,
         const Arch& arch)
{
    UnwindInfo* info = uwinfo.get();

    // 4-byte align the start of unwind info
    AppendAlign(xdata, Expr(4), Expr(), Expr(), 0, line);

    // Prolog size = end of prolog - start of procedure
    info->m_prolog_size.AddAbs(SUB(info->m_prolog, info->m_proc));

    // Unwind info
    Bytecode& infobc = xdata.FreshBytecode();
    infobc.Transform(Bytecode::Contents::Ptr(uwinfo));
    infobc.setLine(line);

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
    AppendAlign(xdata, Expr(4), Expr(), Expr(), 0, line);

    // Exception handler, if present.
    if (info->m_ehandler)
        AppendData(xdata, Expr::Ptr(new Expr(info->m_ehandler)), 4, arch, line);
}

}}} // namespace yasm::objfmt::win64

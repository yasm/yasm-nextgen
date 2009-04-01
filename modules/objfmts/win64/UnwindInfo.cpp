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
UnwindInfo::put(marg_ostream& os) const
{
    // TODO
}

void
UnwindInfo::finalize(Bytecode& bc)
{
    if (!m_prolog_size.finalize())
        throw ValueError(N_("prolog size expression too complex"));

    if (!m_codes_count.finalize())
        throw ValueError(N_("codes count expression too complex"));

    if (!m_frameoff.finalize())
        throw ValueError(N_("frame offset expression too complex"));
}

unsigned long
UnwindInfo::calc_len(Bytecode& bc, const Bytecode::AddSpanFunc& add_span)
{
    // Want to make sure prolog size and codes count doesn't exceed
    // byte-size, and scaled frame offset doesn't exceed 4 bits.
    add_span(bc, 1, m_prolog_size, 0, 255);
    add_span(bc, 2, m_codes_count, 0, 255);

    IntNum intn;
    if (m_frameoff.get_intnum(&intn, false))
    {
        if (!intn.in_range(0, 240))
            throw ValueError(String::compose(
                N_("frame offset of %1 bytes, must be between 0 and 240"),
                intn));
        else if ((intn.get_uint() & 0xF) != 0)
            throw ValueError(String::compose(
                N_("frame offset of %1 is not a multiple of 16"), intn));
    }
    else
        add_span(bc, 3, m_frameoff, 0, 240);

    return 4;
}

bool
UnwindInfo::expand(Bytecode& bc,
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
            ValueError err(String::compose(
                N_("prologue %1 bytes, must be <256"), new_val));
            err.set_xref(m_prolog->get_def_line(), N_("prologue ended here"));
            throw err;
        }
        case 2:
            throw ValueError(String::compose(
                N_("%1 unwind codes, maximum of 255"), new_val));
        case 3:
            throw ValueError(String::compose(
                N_("frame offset of %1 bytes, must be between 0 and 240"),
                new_val));
        default:
            assert(false && "unrecognized span id");
    }
    return 0;
}

void
UnwindInfo::output(Bytecode& bc, BytecodeOutput& bc_out)
{
    Bytes& bytes = bc_out.get_scratch();
    Location loc = {&bc, 0};

    // Version and flags
    if (m_ehandler)
        write_8(bytes, 1 | (UNW_FLAG_EHANDLER << 3));
    else
        write_8(bytes, 1);
    bc_out.output(bytes);

    // Size of prolog
    bytes.resize(0);
    write_8(bytes, 0);
    bc_out.output(m_prolog_size, bytes, loc, 1);

    // Count of codes
    bytes.resize(0);
    write_8(bytes, 0);
    bc_out.output(m_codes_count, bytes, loc, 1);

    // Frame register and offset
    IntNum intn;
    if (!m_frameoff.get_intnum(&intn, true))
        throw ValueError(N_("frame offset expression too complex"));
    if (!intn.in_range(0, 240))
        throw ValueError(String::compose(
            N_("frame offset of %1 bytes, must be between 0 and 240"), intn));
    else if ((intn.get_uint() & 0xF) != 0)
        throw ValueError(String::compose(
            N_("frame offset of %1 is not a multiple of 16"), intn));

    bytes.resize(0);
    write_8(bytes, (intn.get_uint() & 0xF0) | (m_framereg & 0x0F));
    bc_out.output(bytes);
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
generate(std::auto_ptr<UnwindInfo> uwinfo,
         BytecodeContainer& xdata,
         unsigned long line,
         const Arch& arch)
{
    UnwindInfo* info = uwinfo.get();

    // 4-byte align the start of unwind info
    append_align(xdata, Expr::Ptr(new Expr(IntNum(4))), Expr::Ptr(0),
                 Expr::Ptr(0), 0, line);

    // Prolog size = end of prolog - start of procedure
    info->m_prolog_size.add_abs(SUB(info->m_prolog, info->m_proc));

    // Unwind info
    Bytecode& infobc = xdata.fresh_bytecode();
    infobc.transform(Bytecode::Contents::Ptr(uwinfo));
    infobc.set_line(line);

    Bytecode& startbc = xdata.fresh_bytecode();
    Location startloc = {&startbc, startbc.get_fixed_len()};

    // Code array
    for (std::vector<UnwindCode*>::reverse_iterator i=info->m_codes.rbegin(),
         end=info->m_codes.rend(); i != end; ++i)
    {
        append_unwind_code(xdata, std::auto_ptr<UnwindCode>(*i));
        *i = 0; // don't double-free
    }

    // Number of codes = (Last code - end of info) >> 1
    if (info->m_codes.size() > 0)
    {
        Bytecode& bc = xdata.fresh_bytecode();
        Location endloc = {&bc, bc.get_fixed_len()};
        info->m_codes_count.add_abs(SHR(SUB(endloc, startloc), 1));
    }

    // 4-byte align
    append_align(xdata, Expr::Ptr(new Expr(IntNum(4))), Expr::Ptr(0),
                 Expr::Ptr(0), 0, line);

    // Exception handler, if present.
    if (info->m_ehandler)
        append_data(xdata, Expr::Ptr(new Expr(info->m_ehandler)), 4, arch,
                    line);
}

}}} // namespace yasm::objfmt::win64

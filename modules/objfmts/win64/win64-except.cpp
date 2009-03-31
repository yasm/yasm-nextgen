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
#include "win64-except.h"

#include <util.h>

#include <libyasmx/BytecodeContainer.h>
#include <libyasmx/BytecodeContainer_util.h>
#include <libyasmx/BytecodeOutput.h>
#include <libyasmx/Bytes.h>
#include <libyasmx/Bytes_util.h>
#include <libyasmx/Compose.h>
#include <libyasmx/errwarn.h>
#include <libyasmx/Expr.h>
#include <libyasmx/Symbol.h>


namespace yasm
{
namespace objfmt
{
namespace win64
{

#define UNW_FLAG_EHANDLER   0x01
#define UNW_FLAG_UHANDLER   0x02
#define UNW_FLAG_CHAININFO  0x04

UnwindCode::~UnwindCode()
{
}

void
UnwindCode::put(marg_ostream& os) const
{
    // TODO
}

void
UnwindCode::finalize(Bytecode& bc)
{
    if (!m_off.finalize())
        throw ValueError(N_("offset expression too complex"));
}

unsigned long
UnwindCode::calc_len(Bytecode& bc, const Bytecode::AddSpanFunc& add_span)
{
    unsigned long len = 0;
    int span = 0;
    long low, high, mask;

    len += 1;   // Code and info

    switch (m_opcode)
    {
        case PUSH_NONVOL:
        case SET_FPREG:
        case PUSH_MACHFRAME:
            // always 1 node
            return len;
        case ALLOC_SMALL:
        case ALLOC_LARGE:
            // Start with smallest, then work our way up as necessary
            m_opcode = ALLOC_SMALL;
            m_info = 0;
            span = 1;
            low = 8;
            high = 128;
            mask = 0x7;
            break;
        case SAVE_NONVOL:
        case SAVE_NONVOL_FAR:
            // Start with smallest, then work our way up as necessary
            m_opcode = SAVE_NONVOL;
            len += 2;                   // Scaled offset
            span = 2;
            low = 0;
            high = 8*64*1024-8;         // 16-bit field, *8 scaling
            mask = 0x7;
            break;
        case SAVE_XMM128:
        case SAVE_XMM128_FAR:
            // Start with smallest, then work our way up as necessary
            m_opcode = SAVE_XMM128;
            len += 2;                   // Scaled offset
            span = 3;
            low = 0;
            high = 16*64*1024-16;       // 16-bit field, *16 scaling
            mask = 0xF;
            break;
        default:
            assert(false && "unrecognied unwind opcode");
            return 0;
    }

    IntNum intn;
    if (m_off.get_intnum(&intn, false))
    {
        long intv = intn.get_int();
        if (intv > high)
        {
            // Expand it ourselves here if we can and we're already larger
            if (expand(bc, len, span, intv, intv, &low, &high))
                add_span(bc, span, m_off, low, high);
        }
        if (intv < low)
            throw ValueError(N_("negative offset not allowed"));
        if ((intv & mask) != 0)
            throw ValueError(String::compose(
                N_("offset of %1 is not a multiple of %2"), intv, mask+1));
    }
    else
        add_span(bc, span, m_off, low, high);
    return len;
}

bool
UnwindCode::expand(Bytecode& bc,
                   unsigned long& len,
                   int span,
                   long old_val,
                   long new_val,
                   /*@out@*/ long* neg_thres,
                   /*@out@*/ long* pos_thres)
{
    if (new_val < 0)
        throw ValueError(N_("negative offset not allowed"));

    if (span == 1)
    {
        // 3 stages: SMALL, LARGE and info=0, LARGE and info=1
        assert((m_opcode != ALLOC_LARGE || m_info != 1) &&
               "expansion on already largest alloc");

        if (m_opcode == ALLOC_SMALL && new_val > 128)
        {
            // Overflowed small size
            m_opcode = ALLOC_LARGE;
            len += 2;
        }
        if (new_val <= 8*64*1024-8)
        {
            // Still can grow one more size
            *pos_thres = 8*64*1024-8;
            return true;
        }
        // We're into the largest size
        m_info = 1;
        len += 2;
    }
    else if (m_opcode == SAVE_NONVOL && span == 2)
    {
        m_opcode = SAVE_NONVOL_FAR;
        len += 2;
    }
    else if (m_opcode == SAVE_XMM128 && span == 3)
    {
        m_opcode = SAVE_XMM128_FAR;
        len += 2;
    }
    return false;
}

void
UnwindCode::output(Bytecode& bc, BytecodeOutput& bc_out)
{
    Bytes& bytes = bc_out.get_scratch();

    // Offset value
    unsigned int size;
    int shift;
    long low, high;
    unsigned long mask;
    switch (m_opcode)
    {
        case PUSH_NONVOL:
        case SET_FPREG:
        case PUSH_MACHFRAME:
            // just 1 node, no offset; write opcode and info and we're done
            write_8(bytes, (m_info << 4) | (m_opcode & 0xF));
            return;
        case ALLOC_SMALL:
            // 1 node, but offset stored in info
            size = 0; low = 8; high = 128; shift = 3; mask = 0x7;
            break;
        case ALLOC_LARGE:
            if (m_info == 0)
            {
                size = 2; low = 136; high = 8*64*1024-8; shift = 3;
            }
            else
            {
                size = 4; low = high = 0; shift = 0;
            }
            mask = 0x7;
            break;
        case SAVE_NONVOL:
            size = 2; low = 0; high = 8*64*1024-8; shift = 3; mask = 0x7;
            break;
        case SAVE_XMM128:
            size = 2; low = 0; high = 16*64*1024-16; shift = 4; mask = 0xF;
            break;
        case SAVE_NONVOL_FAR:
            size = 4; low = high = 0; shift = 0; mask = 0x7;
            break;
        case SAVE_XMM128_FAR:
            size = 4; low = high = 0; shift = 0; mask = 0xF;
            break;
        default:
            assert(false && "unrecognied unwind opcode");
            return;
    }

    // Check for overflow
    IntNum intn;
    if (!m_off.get_intnum(&intn, true))
        throw ValueError(N_("offset expression too complex"));
    if (size != 4 && !intn.in_range(low, high))
    {
        throw ValueError(String::compose(
            N_("offset of %1 bytes, must be between %2 and %3"),
            intn, low, high));
    }

    if ((intn.get_uint() & mask) != 0)
    {
        throw ValueError(String::compose(
            N_("offset of %1 is not a multiple of %2"), intn, mask+1));
    }
    intn >>= shift;

    // Stored value in info instead of extra code space
    if (size == 0)
        m_info = intn.get_uint()-1;

    // Opcode and info
    write_8(bytes, (m_info << 4) | (m_opcode & 0xF));

    bytes << little_endian;
    if (size == 2)
        write_16(bytes, intn);
    else if (size == 4)
        write_32(bytes, intn);
    bc_out.output(bytes);
}

UnwindCode*
UnwindCode::clone() const
{
    UnwindCode* uwcode = new UnwindCode(m_proc, m_loc, m_opcode, m_info);
    uwcode->m_off = m_off;
    return uwcode;
}

void
append_unwind_code(BytecodeContainer& container,
                   std::auto_ptr<UnwindCode> uwcode)
{
    // Offset in prolog
    Bytecode& bc = container.fresh_bytecode();
    bc.append_fixed(1,
                    Expr::Ptr(new Expr(SUB(uwcode->m_loc, uwcode->m_proc))),
                    0);

    switch (uwcode->m_opcode)
    {
        case UnwindCode::PUSH_NONVOL:
        case UnwindCode::SET_FPREG:
        case UnwindCode::PUSH_MACHFRAME:
            // just 1 node, no offset; write opcode and info and we're done
            append_byte(container,
                        (uwcode->m_info << 4) | (uwcode->m_opcode & 0xF));
            return;
        default:
            break;
    }

    bc.set_line(uwcode->m_loc->get_def_line());
    bc.transform(Bytecode::Contents::Ptr(uwcode));
}

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

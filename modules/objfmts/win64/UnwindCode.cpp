//
// Win64 structured exception handling unwind code
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
#include "UnwindCode.h"

#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/BytecodeContainer.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/Expr.h"
#include "yasmx/Symbol.h"


using namespace yasm;
using namespace yasm::objfmt;

UnwindCode::~UnwindCode()
{
}

bool
UnwindCode::Finalize(Bytecode& bc, DiagnosticsEngine& diags)
{
    if (!m_off.Finalize(diags))
        return false;
    return true;
}

bool
UnwindCode::CalcLen(Bytecode& bc,
                    /*@out@*/ unsigned long* len,
                    const Bytecode::AddSpanFunc& add_span,
                    DiagnosticsEngine& diags)
{
    *len = 0;
    int span = 0;
    long low, high, mask;

    *len += 1;  // Code and info

    switch (m_opcode)
    {
        case PUSH_NONVOL:
        case SET_FPREG:
        case PUSH_MACHFRAME:
            // always 1 node
            return true;
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
            *len += 2;                  // Scaled offset
            span = 2;
            low = 0;
            high = 8*64*1024-8;         // 16-bit field, *8 scaling
            mask = 0x7;
            break;
        case SAVE_XMM128:
        case SAVE_XMM128_FAR:
            // Start with smallest, then work our way up as necessary
            m_opcode = SAVE_XMM128;
            *len += 2;                  // Scaled offset
            span = 3;
            low = 0;
            high = 16*64*1024-16;       // 16-bit field, *16 scaling
            mask = 0xF;
            break;
        default:
            assert(false && "unrecognied unwind opcode");
            return false;
    }

    IntNum intn;
    if (m_off.getIntNum(&intn, false, diags))
    {
        long intv = intn.getInt();
        if (intv > high)
        {
            // Expand it ourselves here if we can and we're already larger
            bool keep;
            if (!Expand(bc, len, span, intv, intv, &keep, &low, &high, diags))
                return false;
            if (keep)
                add_span(bc, span, m_off, low, high);
        }
        if (intv < low)
        {
            diags.Report(m_off.getSource().getBegin(),
                         diag::err_negative_offset);
            return false;
        }
        if ((intv & mask) != 0)
        {
            diags.Report(m_off.getSource().getBegin(),
                         diag::err_offset_not_multiple)
                << static_cast<int>(intv) << static_cast<int>(mask+1);
            return false;
        }
    }
    else
        add_span(bc, span, m_off, low, high);
    return true;
}

bool
UnwindCode::Expand(Bytecode& bc,
                   unsigned long* len,
                   int span,
                   long old_val,
                   long new_val,
                   bool* keep,
                   /*@out@*/ long* neg_thres,
                   /*@out@*/ long* pos_thres,
                   DiagnosticsEngine& diags)
{
    if (new_val < 0)
    {
        diags.Report(m_off.getSource().getBegin(), diag::err_negative_offset);
        return false;
    }

    if (span == 1)
    {
        // 3 stages: SMALL, LARGE and info=0, LARGE and info=1
        assert((m_opcode != ALLOC_LARGE || m_info != 1) &&
               "expansion on already largest alloc");

        if (m_opcode == ALLOC_SMALL && new_val > 128)
        {
            // Overflowed small size
            m_opcode = ALLOC_LARGE;
            *len += 2;
        }
        if (new_val <= 8*64*1024-8)
        {
            // Still can grow one more size
            *pos_thres = 8*64*1024-8;
            *keep = true;
            return true;
        }
        // We're into the largest size
        m_info = 1;
        *len += 2;
    }
    else if (m_opcode == SAVE_NONVOL && span == 2)
    {
        m_opcode = SAVE_NONVOL_FAR;
        *len += 2;
    }
    else if (m_opcode == SAVE_XMM128 && span == 3)
    {
        m_opcode = SAVE_XMM128_FAR;
        *len += 2;
    }
    *keep = false;
    return true;
}

bool
UnwindCode::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    Bytes& bytes = bc_out.getScratch();

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
            Write8(bytes, (m_info << 4) | (m_opcode & 0xF));
            return true;
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
            return false;
    }

    // Check for overflow
    IntNum intn;
    if (!m_off.getIntNum(&intn, true, bc_out.getDiagnostics()))
    {
        bc_out.Diag(m_off.getSource().getBegin(),
                    diag::err_too_complex_expression);
        return false;
    }

    if (size != 4 && !intn.isInRange(low, high))
    {
        bc_out.Diag(m_off.getSource().getBegin(), diag::err_offset_out_of_range)
            << intn.getStr() << static_cast<int>(low) << static_cast<int>(high);
        return false;
    }

    if ((intn.getUInt() & mask) != 0)
    {
        bc_out.Diag(m_off.getSource().getBegin(), diag::err_offset_not_multiple)
            << intn.getStr() << static_cast<int>(mask+1);
        return false;
    }
    intn >>= shift;

    // Stored value in info instead of extra code space
    if (size == 0)
        m_info = intn.getUInt()-1;

    // Opcode and info
    Write8(bytes, (m_info << 4) | (m_opcode & 0xF));

    bytes.setLittleEndian();
    if (size == 2)
        Write16(bytes, intn);
    else if (size == 4)
        Write32(bytes, intn);
    bc_out.OutputBytes(bytes, bc.getSource());
    return true;
}

StringRef
UnwindCode::getType() const
{
    return "yasm::objfmt::UnwindCode";
}

UnwindCode*
UnwindCode::clone() const
{
    UnwindCode* uwcode = new UnwindCode(m_proc, m_loc, m_opcode, m_info);
    uwcode->m_off = m_off;
    return uwcode;
}

#ifdef WITH_XML
pugi::xml_node
UnwindCode::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("UnwindCode");
    append_child(root, "Proc", m_proc);
    append_child(root, "Loc", m_loc);
    pugi::xml_attribute opcode = root.append_attribute("opcode");
    switch (m_opcode)
    {
        case PUSH_NONVOL:       opcode = "PUSH_NONVOL"; break;
        case ALLOC_LARGE:       opcode = "ALLOC_LARGE"; break;
        case ALLOC_SMALL:       opcode = "ALLOC_SMALL"; break;
        case SET_FPREG:         opcode = "SET_FPREG"; break;
        case SAVE_NONVOL:       opcode = "SAVE_NONVOL"; break;
        case SAVE_NONVOL_FAR:   opcode = "SAVE_NONVOL_FAR"; break;
        case SAVE_XMM128:       opcode = "SAVE_XMM128"; break;
        case SAVE_XMM128_FAR:   opcode = "SAVE_XMM128_FAR"; break;
        case PUSH_MACHFRAME:    opcode = "PUSH_MACHFRAME"; break;
    }
    append_child(root, "Info", m_info);
    append_child(root, "Off", m_off);
    return root;
}
#endif // WITH_XML

void
objfmt::AppendUnwindCode(BytecodeContainer& container,
                         std::auto_ptr<UnwindCode> uwcode)
{
    // Offset in prolog
    Bytecode& bc = container.FreshBytecode();
    bc.AppendFixed(1, Expr::Ptr(new Expr(SUB(uwcode->m_loc, uwcode->m_proc))),
                   SourceLocation());

    switch (uwcode->m_opcode)
    {
        case UnwindCode::PUSH_NONVOL:
        case UnwindCode::SET_FPREG:
        case UnwindCode::PUSH_MACHFRAME:
            // just 1 node, no offset; write opcode and info and we're done
            AppendByte(container,
                       (uwcode->m_info << 4) | (uwcode->m_opcode & 0xF));
            return;
        default:
            break;
    }

    bc.setSource(uwcode->m_loc->getDefSource());
    bc.Transform(Bytecode::Contents::Ptr(uwcode));
}

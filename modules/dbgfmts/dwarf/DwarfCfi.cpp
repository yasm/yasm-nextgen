//
// DWARF Call Frame Information implementation
//
//  Copyright (C) 2010-2011  Peter Johnson
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
#include "DwarfCfi.h"

#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Parse/Directive.h"
#include "yasmx/Parse/DirHelpers.h"
#include "yasmx/Parse/NameValue.h"
#include "yasmx/Arch.h"
#include "yasmx/BytecodeContainer.h"
#include "yasmx/Object.h"
#include "yasmx/ObjectFormat.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"

#include "DwarfDebug.h"


using namespace yasm;
using namespace yasm::dbgfmt;

#define CIE_id          0xffffffffUL
#define CIE_version     1

#define DW_OP_addr      0x03
#define DW_OP_GNU_encoded_addr 0xf1

namespace
{

enum DwarfCfiPersonality
{
    DW_EH_PE_absptr = 0x00,
    DW_EH_PE_omit = 0xff,

    DW_EH_PE_uleb128 = 0x01,
    DW_EH_PE_udata2 = 0x02,
    DW_EH_PE_udata4 = 0x03,
    DW_EH_PE_udata8 = 0x04,
    DW_EH_PE_sleb128 = 0x09,
    DW_EH_PE_sdata2 = 0x0A,
    DW_EH_PE_sdata4 = 0x0B,
    DW_EH_PE_sdata8 = 0x0C,
    DW_EH_PE_signed = 0x08,

    DW_EH_PE_pcrel = 0x10,
    DW_EH_PE_textrel = 0x20,
    DW_EH_PE_datarel = 0x30,
    DW_EH_PE_funcrel = 0x40,
    DW_EH_PE_aligned = 0x50,

    DW_EH_PE_indirect = 0x80
};

class IsFdeMatch
{
public:
    IsFdeMatch(const DwarfCfiFde& fde) : m_fde(fde) {}
    bool operator() (const DwarfCfiCie& fde);
private:
    const DwarfCfiFde& m_fde;
};

} // anonymous namespace

bool
IsFdeMatch::operator() (const DwarfCfiCie& cie)
{
    if (cie.m_fde->m_personality_encoding != m_fde.m_personality_encoding ||
        cie.m_fde->m_lsda_encoding != m_fde.m_lsda_encoding ||
        cie.m_fde->m_return_column != m_fde.m_return_column ||
        cie.m_fde->m_signal_frame != m_fde.m_signal_frame)
        return false;

    if (cie.m_fde->m_personality_encoding != DW_EH_PE_omit)
    {
        // check for equal personality
    }

    // check for commonality in instructions
    stdx::ptr_vector<DwarfCfiInsn>::const_iterator
         lhs = cie.m_fde->m_insns.begin(),
         lhsend = cie.m_fde->m_insns.begin() + cie.m_num_insns,
         rhs = m_fde.m_insns.begin(),
         rhsend = m_fde.m_insns.end();
    for (; lhs != lhsend && rhs != rhsend; ++lhs, ++rhs)
    {
        switch (lhs->getOp())
        {
            case DwarfCfiInsn::DW_CFA_advance_loc:
            case DwarfCfiInsn::DW_CFA_remember_state:
            case DwarfCfiInsn::CFI_escape:
            case DwarfCfiInsn::CFI_val_encoded_addr:
                // should have reached end of CIE list first
                return false;
            default:
                if (*lhs != *rhs)
                    return false;
                break;
        }
    }
    if (lhs != lhsend)
        return false;
    if (rhs == rhsend)
        return true;
    switch (rhs->getOp())
    {
        case DwarfCfiInsn::DW_CFA_advance_loc:
        case DwarfCfiInsn::DW_CFA_remember_state:
        case DwarfCfiInsn::CFI_escape:
        case DwarfCfiInsn::CFI_val_encoded_addr:
            return true;
        default:
            return false;
    }
}

static unsigned int
getEncodingSize(unsigned int encoding, Arch& arch)
{
    if (encoding == DW_EH_PE_omit)
        return 0;
    switch (encoding & 0x7)
    {
        case DW_EH_PE_absptr: return arch.getAddressSize()/8;
        case DW_EH_PE_udata2: return 2;
        case DW_EH_PE_udata4: return 4;
        case DW_EH_PE_udata8: return 8;
        default: assert(false && "invalid encoding"); return 0;
    }
}

bool
DwarfCfiInsn::operator== (const DwarfCfiInsn& oth) const
{
    if (m_op != oth.m_op)
        return false;
    switch (m_op)
    {
        case DW_CFA_advance_loc:
        case DW_CFA_set_loc:
        case DW_CFA_advance_loc1:
        case DW_CFA_advance_loc2:
        case DW_CFA_advance_loc4:
            return m_from == oth.m_from && m_to == oth.m_to;
        case DW_CFA_offset:
        case DW_CFA_offset_extended:
        case DW_CFA_offset_extended_sf:
        case DW_CFA_def_cfa:
        case DW_CFA_def_cfa_sf:
            return m_regs[0] == oth.m_regs[0] && m_off == oth.m_off;
        case DW_CFA_nop:
        case DW_CFA_remember_state:
        case DW_CFA_restore_state:
        case DW_CFA_GNU_window_save:
            return true;
        case DW_CFA_restore:
        case DW_CFA_restore_extended:
        case DW_CFA_undefined:
        case DW_CFA_same_value:
        case DW_CFA_def_cfa_register:
            return m_regs[0] == oth.m_regs[0];
        case DW_CFA_register:
            return m_regs[0] == oth.m_regs[0] && m_regs[1] == oth.m_regs[1];
        case DW_CFA_def_cfa_offset:
        case DW_CFA_def_cfa_offset_sf:
        case DW_CFA_GNU_args_size:
            return m_off == oth.m_off;
        case DW_CFA_def_cfa_expression:
        case DW_CFA_expression:
        case DW_CFA_val_offset:
        case DW_CFA_val_offset_sf:
        case DW_CFA_val_expression:
        case CFI_escape:
        case CFI_val_encoded_addr:
            return false;   // TODO: these are hard
        default:
            assert(false && "bad op value");
            return false;
    }
}

DwarfCfiInsn*
DwarfCfiInsn::MakeOffset(unsigned int reg, const IntNum& off)
{
    DwarfCfiInsn* insn = new DwarfCfiInsn(DW_CFA_offset, off);
    insn->m_regs[0] = reg;
    return insn;
}

DwarfCfiInsn*
DwarfCfiInsn::MakeRestore(unsigned int reg)
{
    DwarfCfiInsn* insn = new DwarfCfiInsn(DW_CFA_restore);
    insn->m_regs[0] = reg;
    return insn;
}

DwarfCfiInsn*
DwarfCfiInsn::MakeUndefined(unsigned int reg)
{
    DwarfCfiInsn* insn = new DwarfCfiInsn(DW_CFA_undefined);
    insn->m_regs[0] = reg;
    return insn;
}

DwarfCfiInsn*
DwarfCfiInsn::MakeSameValue(unsigned int reg)
{
    DwarfCfiInsn* insn = new DwarfCfiInsn(DW_CFA_same_value);
    insn->m_regs[0] = reg;
    return insn;
}

DwarfCfiInsn*
DwarfCfiInsn::MakeRegister(unsigned int reg1, unsigned int reg2)
{
    DwarfCfiInsn* insn = new DwarfCfiInsn(DW_CFA_register);
    insn->m_regs[0] = reg1;
    insn->m_regs[1] = reg2;
    return insn;
}

DwarfCfiInsn*
DwarfCfiInsn::MakeDefCfa(unsigned int reg, const IntNum& off)
{
    DwarfCfiInsn* insn = new DwarfCfiInsn(DW_CFA_def_cfa, off);
    insn->m_regs[0] = reg;
    return insn;
}

DwarfCfiInsn*
DwarfCfiInsn::MakeDefCfaRegister(unsigned int reg)
{
    DwarfCfiInsn* insn = new DwarfCfiInsn(DW_CFA_def_cfa_register);
    insn->m_regs[0] = reg;
    return insn;
}

DwarfCfiInsn*
DwarfCfiInsn::MakeAdvanceLoc(Location from, Location to)
{
    DwarfCfiInsn* insn = new DwarfCfiInsn(DW_CFA_advance_loc);
    insn->m_from = from;
    insn->m_to = to;
    return insn;
}

DwarfCfiInsn*
DwarfCfiInsn::MakeEscape(std::vector<Expr>* esc)
{
    DwarfCfiInsn* insn = new DwarfCfiInsn(CFI_escape);
    esc->swap(insn->m_esc);
    return insn;
}

DwarfCfiInsn*
DwarfCfiInsn::MakeValEncodedAddr(unsigned int reg,
                                 unsigned int encoding,
                                 Expr e)
{
    DwarfCfiInsn* insn = new DwarfCfiInsn(CFI_val_encoded_addr);
    // somewhat of a hack to store it this way
    insn->m_regs[0] = reg;
    insn->m_regs[1] = encoding;
    insn->m_esc.push_back(e);
    return insn;
}

DwarfCfiInsn::DwarfCfiInsn(Op op)
    : m_op(op)
{
}

DwarfCfiInsn::DwarfCfiInsn(Op op, const IntNum& off)
    : m_op(op), m_off(off)
{
}

DwarfCfiInsn::~DwarfCfiInsn()
{
}

void
DwarfCfiInsn::Output(DwarfCfiOutput& out)
{
    BytecodeContainer& container = out.container;
    Arch& arch = *out.debug.m_object.getArch();
    DiagnosticsEngine& diags = out.diags;

    switch (m_op)
    {
        case DW_CFA_advance_loc:
        {
            // If locations are fixed distance apart emit more compactly.
            // It's safe to use CalcDist because this is run after
            // optimization.
            IntNum dist;
            if (CalcDist(m_from, m_to, &dist))
            {
                dist /= out.debug.m_min_insn_len;
                if (dist.isInRange(0, 0x3F))
                    AppendByte(container, DW_CFA_advance_loc | dist.getUInt());
                else if (dist.isInRange(0, 0xFF))
                {
                    AppendByte(container, DW_CFA_advance_loc1);
                    AppendByte(container, dist.getUInt());
                }
                else if (dist.isInRange(0, 0xFFFF))
                {
                    AppendByte(container, DW_CFA_advance_loc2);
                    AppendData(container, dist, 2, arch);
                }
                else
                {
                    AppendByte(container, DW_CFA_advance_loc4);
                    AppendData(container, dist, 4, arch);
                }
            }
            else
            {
                AppendByte(container, DW_CFA_advance_loc4);
                Expr::Ptr e(new Expr(m_to));
                *e -= m_from;
                AppendData(container, e, 4, arch, m_source, diags);
            }
            break;
        }

        case DW_CFA_offset:
        {
            unsigned int reg = m_regs[0];
            IntNum off = m_off / out.debug.m_cie_data_alignment;
            if (off.getSign() < 0)
            {
                AppendByte(container, DW_CFA_offset_extended_sf);
                AppendLEB128(container, reg, false, m_source, diags);
                AppendLEB128(container, off, true, m_source, diags);
            }
            else if (reg <= 0x3F)
            {
                AppendByte(container, DW_CFA_offset | reg);
                AppendLEB128(container, off, false, m_source, diags);
            }
            else
            {
                AppendByte(container, DW_CFA_offset_extended);
                AppendLEB128(container, reg, false, m_source, diags);
                AppendLEB128(container, off, false, m_source, diags);
            }
            break;
        }

        case DW_CFA_restore:
        {
            unsigned int reg = m_regs[0];
            if (reg <= 0x3F)
                AppendByte(container, DW_CFA_restore | reg);
            else
            {
                AppendByte(container, DW_CFA_restore_extended);
                AppendLEB128(container, reg, false, m_source, diags);
            }
            break;
        }

        case DW_CFA_register:
            AppendByte(container, m_op);
            AppendLEB128(container, m_regs[0], false, m_source, diags);
            AppendLEB128(container, m_regs[1], false, m_source, diags);
            break;

        case DW_CFA_remember_state:
        case DW_CFA_restore_state:
        case DW_CFA_GNU_window_save:
            AppendByte(container, m_op);
            break;

        case DW_CFA_def_cfa:
            if (m_off.getSign() < 0)
            {
                AppendByte(container, DW_CFA_def_cfa_sf);
                AppendLEB128(container, m_regs[0], false, m_source, diags);
                AppendLEB128(container, m_off / out.debug.m_cie_data_alignment,
                             true, m_source, diags);
            }
            else
            {
                AppendByte(container, DW_CFA_def_cfa);
                AppendLEB128(container, m_regs[0], false, m_source, diags);
                AppendLEB128(container, m_off, false, m_source, diags);
            }
            break;

        case DW_CFA_undefined:
        case DW_CFA_same_value:
        case DW_CFA_def_cfa_register:
            AppendByte(container, m_op);
            AppendLEB128(container, m_regs[0], false, m_source, diags);
            break;

        case DW_CFA_def_cfa_offset:
            if (m_off.getSign() < 0)
            {
                AppendByte(container, DW_CFA_def_cfa_offset_sf);
                AppendLEB128(container, m_off / out.debug.m_cie_data_alignment,
                             true, m_source, diags);
            }
            else
            {
                AppendByte(container, DW_CFA_def_cfa_offset);
                AppendLEB128(container, m_off, false, m_source, diags);
            }
            break;

        case CFI_escape:
            for (std::vector<Expr>::const_iterator i=m_esc.begin(),
                 end=m_esc.end(); i != end; ++i)
                AppendByte(container, Expr::Ptr(new Expr(*i)), m_source, diags);
            break;

        case CFI_val_encoded_addr:
        {
            unsigned int reg = m_regs[0];
            unsigned int encoding = m_regs[1];
            Expr::Ptr e(new Expr(m_esc.back()));

            unsigned int size = getEncodingSize(encoding, arch);
            if (size == 0)
                break;

            AppendByte(container, DW_CFA_val_expression);
            AppendLEB128(container, reg, false, m_source, diags);

            if (encoding == DW_EH_PE_absptr)
            {
                AppendLEB128(container, size+1, false, m_source, diags);
                AppendByte(container, DW_OP_addr);
            }
            else
            {
                AppendLEB128(container, size+2, false, m_source, diags);
                AppendByte(container, DW_OP_GNU_encoded_addr);
                AppendByte(container, encoding);
                if ((encoding & 0x70) == DW_EH_PE_pcrel)
                    *e -= out.object.getSymbol(container.getEndLoc());
            }
            AppendData(container, e, size, arch, m_source, diags);

            break;
        }

        default:
            assert(false && "invalid opcode");
            break;
    }
}

DwarfCfiCie::DwarfCfiCie(DwarfCfiFde* fde)
    : m_fde(fde), m_num_insns(0)
{
    for (stdx::ptr_vector<DwarfCfiInsn>::const_iterator
         i = fde->m_insns.begin(), end = fde->m_insns.end(); i != end; ++i)
    {
        switch (i->getOp())
        {
            case DwarfCfiInsn::DW_CFA_advance_loc:
            case DwarfCfiInsn::DW_CFA_remember_state:
            case DwarfCfiInsn::CFI_escape:
            case DwarfCfiInsn::CFI_val_encoded_addr:
                return;
            default:
                break;
        }
        ++m_num_insns;
    }
}

void
DwarfCfiCie::Output(DwarfCfiOutput& out, unsigned int align)
{
    BytecodeContainer& container = out.container;
    Arch& arch = *out.debug.m_object.getArch();
    unsigned int sizeof_offset = out.eh_frame ? 4 : out.debug.m_sizeof_offset;

    m_start = out.object.getSymbol(container.getEndLoc());

    // Length
    SymbolRef cie_start = out.object.AddNonTableSymbol("$");
    SymbolRef cie_end = out.object.AddNonTableSymbol("$");
    if (!out.eh_frame && out.debug.m_format == DwarfDebug::FORMAT_64BIT)
    {
        AppendByte(container, 0xff);
        AppendByte(container, 0xff);
        AppendByte(container, 0xff);
        AppendByte(container, 0xff);
    }
    AppendData(container, Expr::Ptr(new Expr(SUB(cie_end, cie_start))),
               sizeof_offset, arch, m_fde->m_source, out.diags);
    cie_start->DefineLabel(container.getEndLoc());

    // CIE id: always 0 in .eh_frame
    if (!out.eh_frame && out.debug.m_format == DwarfDebug::FORMAT_64BIT)
    {
        AppendByte(container, 0xff);
        AppendByte(container, 0xff);
        AppendByte(container, 0xff);
        AppendByte(container, 0xff);
    }
    AppendData(container, out.eh_frame ? 0 : CIE_id, 4, arch);

    // CIE version
    AppendByte(container, CIE_version);

    if (out.eh_frame)
    {
        // Augmentation flags
        AppendByte(container, 'z');
        if (m_fde->m_personality_encoding != DW_EH_PE_omit)
            AppendByte(container, 'P');
        if (m_fde->m_lsda_encoding != DW_EH_PE_omit)
            AppendByte(container, 'L');
        AppendByte(container, 'R');
    }
    if (m_fde->m_signal_frame)
        AppendByte(container, 'S');
    AppendByte(container, 0);

    // Code alignment
    AppendLEB128(container, out.debug.m_min_insn_len, false, m_fde->m_source,
                 out.diags);

    // Data alignment
    AppendLEB128(container, out.debug.m_cie_data_alignment, true,
                 m_fde->m_source, out.diags);

    // Return column
    AppendByte(container, m_fde->m_return_column);

    if (out.eh_frame)
    {
        // Augmentation data
        unsigned int per_size =
            getEncodingSize(m_fde->m_personality_encoding, arch);

        unsigned int size = 1;
        if (per_size != 0)
            size += 1 + per_size;
        if (m_fde->m_lsda_encoding != DW_EH_PE_omit)
            ++size;
        AppendLEB128(container, size, false, m_fde->m_source, out.diags);

        if (m_fde->m_personality_encoding != DW_EH_PE_omit)
        {
            AppendByte(container, m_fde->m_personality_encoding);
            Expr::Ptr e(new Expr(m_fde->m_personality));
            if ((m_fde->m_personality_encoding & 0x70) == DW_EH_PE_pcrel)
                *e -= out.object.getSymbol(container.getEndLoc());
            AppendData(container, e, per_size, arch,
                       m_fde->m_personality_source, out.diags);
        }

        if (m_fde->m_lsda_encoding != DW_EH_PE_omit)
            AppendByte(container, m_fde->m_lsda_encoding);

        // relocation setting
        unsigned int pe = DW_EH_PE_pcrel;
        switch (out.debug.m_fde_reloc_size)
        {
            case 2: pe |= DW_EH_PE_sdata2; break;
            case 4: pe |= DW_EH_PE_sdata4; break;
            case 8: pe |= DW_EH_PE_sdata8; break;
            default: assert(false && "invalid FDE reloc size"); break;
        }
        AppendByte(container, pe);
    }

    // Instructions
    for (size_t i=0; i<m_num_insns; ++i)
        m_fde->m_insns[i].Output(out);

    // Align
    AppendAlign(container, Expr(align), Expr(DwarfCfiInsn::DW_CFA_nop),
                Expr(), 0, m_fde->m_source);

    cie_end->DefineLabel(container.getEndLoc());
}

DwarfCfiFde::DwarfCfiFde(const DwarfDebug& debug,
                         Location start,
                         SourceLocation source)
    : m_source(source)
    , m_start(start)
    , m_insns_owner(m_insns)
    , m_personality_encoding(DW_EH_PE_omit)
    , m_lsda_encoding(DW_EH_PE_omit)
    , m_return_column(debug.m_default_return_column)
    , m_signal_frame(false)
{
}

DwarfCfiFde::~DwarfCfiFde()
{
}

void
DwarfCfiFde::Output(DwarfCfiOutput& out, DwarfCfiCie& cie, unsigned int align)
{
    BytecodeContainer& container = out.container;
    Arch& arch = *out.object.getArch();
    unsigned int sizeof_offset = out.eh_frame ? 4 : out.debug.m_sizeof_offset;
    unsigned int sizeof_address = out.eh_frame ? 4 : out.debug.m_sizeof_address;

    // Length
    SymbolRef fde_start = out.object.AddNonTableSymbol("$");
    SymbolRef fde_end = out.object.AddNonTableSymbol("$");
    if (!out.eh_frame && out.debug.m_format == DwarfDebug::FORMAT_64BIT)
    {
        AppendByte(container, 0xff);
        AppendByte(container, 0xff);
        AppendByte(container, 0xff);
        AppendByte(container, 0xff);
    }
    AppendData(container, Expr::Ptr(new Expr(SUB(fde_end, fde_start))),
               sizeof_offset, arch, m_source, out.diags);
    fde_start->DefineLabel(container.getEndLoc());

    // CIE offset
    if (out.eh_frame)
        AppendData(container, Expr::Ptr(new Expr(SUB(fde_start, cie.m_start))),
                   sizeof_offset, arch, m_source, out.diags);
    else
        AppendData(container, Expr::Ptr(new Expr(cie.m_start)),
                   sizeof_offset, arch, m_source, out.diags);

    // Code offset
    SymbolRef start = out.object.getSymbol(m_start);
    if (out.eh_frame)
        AppendData(container,
                   Expr::Ptr(new Expr(SUB(start,
                        out.object.getSymbol(container.getEndLoc())))),
                   4, arch, m_source, out.diags);
    else
        AppendData(container, Expr::Ptr(new Expr(start)), 4, arch, m_source,
                   out.diags);

    // Code length
    AppendData(container,
               Expr::Ptr(new Expr(SUB(out.object.getSymbol(m_end), start))),
               sizeof_address, arch, m_source, out.diags);

    // lsda
    unsigned int lsda_size = getEncodingSize(m_lsda_encoding, arch);
    if (out.eh_frame)
        AppendLEB128(container, lsda_size, false, m_source, out.diags);

    if (m_lsda_encoding != DW_EH_PE_omit)
    {
        Expr::Ptr e(new Expr(m_lsda));
        if ((m_lsda_encoding & 0x70) == DW_EH_PE_pcrel)
            *e -= out.object.getSymbol(container.getEndLoc());
        AppendData(container, e, lsda_size, arch, m_lsda_source, out.diags);
    }

    // Instructions
    for (size_t i = cie.m_num_insns, end = m_insns.size(); i < end; ++i)
        m_insns[i].Output(out);

    // Align
    AppendAlign(container, Expr(align), Expr(DwarfCfiInsn::DW_CFA_nop),
                Expr(), 0, m_source);

    fde_end->DefineLabel(container.getEndLoc());
}

void
DwarfDebug::DirCfiStartproc(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (m_cur_fde)
    {
        diags.Report(info.getSource(), diag::err_nested_cfi);
        return;
    }
    m_cur_fde = new DwarfCfiFde(*this, info.getLocation(), info.getSource());
    m_fdes.push_back(m_cur_fde);
    m_last_address = info.getLocation();

    NameValues& nvs = info.getNameValues();
    if (!nvs.empty() && nvs.front().isId() && nvs.front().getId() == "simple")
        ;
    else if (m_frame_initial_instructions != 0)
        m_frame_initial_instructions(*m_cur_fde);
}

void
DwarfDebug::DirCfiEndproc(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!m_cur_fde)
    {
        diags.Report(info.getSource(), diag::warn_cfi_endproc_before_startproc);
        return;
    }
    m_cur_fde->Close(info.getLocation());
    m_cur_fde = 0;
    m_last_address = Location();
}

void
DwarfDebug::DirCfiSections(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    NameValues& nvs = info.getNameValues();
    bool eh_frame = false;
    bool debug_frame = false;

    for (NameValues::const_iterator i=nvs.begin(), end=nvs.end();
         i != end; ++i)
    {
        if (!i->isString())
        {
            diags.Report(i->getValueRange().getBegin(),
                         diag::err_value_string_or_id);
            return;
        }
        StringRef name = i->getString();
        if (name == ".eh_frame")
            eh_frame = true;
        else if (name == ".debug_frame")
            debug_frame = true;
    }
    m_eh_frame = eh_frame;
    m_debug_frame = debug_frame;
}

bool
DwarfDebug::DirCheck(DirectiveInfo& info,
                     DiagnosticsEngine& diags,
                     unsigned int nargs)
{
    if (!m_cur_fde)
    {
        diags.Report(info.getSource(), diag::warn_outside_cfiproc);
        return false;
    }

    if (nargs == 0)
        return true;

    if (nargs == 1 && info.getNameValues().size() > 1)
    {
        diags.Report(info.getSource(), diag::warn_directive_one_arg);
        return true;
    }

    if (info.getNameValues().size() < nargs)
    {
        diags.Report(info.getSource(), diag::err_directive_too_few_args)
            << nargs;
        return false;
    }
    return true;
}

void
DwarfDebug::DirRegNum(NameValue& nv,
                      DiagnosticsEngine& diags,
                      Object* obj,
                      unsigned int* out,
                      bool* out_set)
{
#if 0
    // TODO: dwarf has its own register numbering
    if (nv.isRegister())
    {
        *out = nv.getRegister()->getNum();
        *out_set = true;
        return;
    }
#endif
    if (nv.isExpr())
    {
        Expr e = nv.getExpr(*obj);
        if (e.isIntNum())
        {
            *out = e.getIntNum().getUInt();
            *out_set = true;
            return;
        }
    }
    diags.Report(nv.getNameSource(), diag::err_value_register)
        << nv.getValueRange();
}

void
DwarfDebug::AdvanceCfiAddress(Location loc, SourceLocation source)
{
    if (loc == m_last_address)
        return;
    DwarfCfiInsn* insn = DwarfCfiInsn::MakeAdvanceLoc(m_last_address, loc);
    insn->setSource(source);
    m_cur_fde->m_insns.push_back(insn);
    m_last_address = loc;
}

static bool
DirCfiEncLabel(DirectiveInfo& info,
               DiagnosticsEngine& diags,
               unsigned char* dest_encoding,
               Expr* dest_expr,
               SourceLocation* dest_source,
               unsigned int start=0)
{
    bool have_encoding = false;
    IntNum encoding;

    NameValues& nvs = info.getNameValues();
    DirIntNum(nvs[start], diags, &info.getObject(), &encoding, &have_encoding);
    if (!have_encoding)
        return false;

    unsigned int enc_int = encoding.getUInt();
    // exit early if encoding is 0xff
    if (enc_int == DW_EH_PE_omit)
    {
        *dest_encoding = enc_int;
        if (nvs.size() > (start+1))
            diags.Report(nvs[1].getValueRange().getBegin(),
                         diag::warn_cfi_routine_ignored);
        return false;
    }

    // check for valid encoding values
    if (enc_int > 0xff ||
        ((encoding & 0x70) != 0 && (encoding & 0x70) != DW_EH_PE_pcrel) ||
        (enc_int & 7) == DW_EH_PE_uleb128 || (enc_int & 7) > 4)
    {
        diags.Report(nvs.front().getValueRange().getBegin(),
                     diag::err_cfi_invalid_encoding);
        return false;
    }

    // non-0xff encoding requires a routine
    if (nvs.size() < (start+2))
    {
        diags.Report(info.getSource(), diag::err_cfi_routine_required);
        return false;
    }

    bool have_expr = false;
    std::auto_ptr<Expr> expr;
    DirExpr(nvs[start+1], diags, &info.getObject(), &expr, &have_expr);
    if (!have_expr)
        return false;

    if (dest_source)
        *dest_source = nvs[start+1].getValueRange().getBegin();

    *dest_encoding = enc_int;
    dest_expr->swap(*expr);
    return true;
}

void
DwarfDebug::DirCfiPersonality(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!m_cur_fde)
    {
        diags.Report(info.getSource(), diag::warn_outside_cfiproc);
        return;
    }

    DirCfiEncLabel(info, diags, &m_cur_fde->m_personality_encoding,
                   &m_cur_fde->m_personality, &m_cur_fde->m_personality_source);
}

void
DwarfDebug::DirCfiLsda(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!m_cur_fde)
    {
        diags.Report(info.getSource(), diag::warn_outside_cfiproc);
        return;
    }

    DirCfiEncLabel(info, diags, &m_cur_fde->m_lsda_encoding,
                   &m_cur_fde->m_lsda, &m_cur_fde->m_lsda_source);
}

void
DwarfDebug::DirCfiDefCfa(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!DirCheck(info, diags, 2))
        return;

    bool have_reg = false, have_off = false;
    unsigned int reg;
    IntNum off;

    NameValues& nvs = info.getNameValues();
    DirRegNum(nvs.front(), diags, &info.getObject(), &reg, &have_reg);
    DirIntNum(nvs[1], diags, &info.getObject(), &off, &have_off);
    if (!have_reg || !have_off)
        return;

    AdvanceCfiAddress(info.getLocation(), info.getSource());
    DwarfCfiInsn* insn = DwarfCfiInsn::MakeDefCfa(reg, off);
    insn->setSource(info.getSource());
    m_cur_fde->m_insns.push_back(insn);
}

void
DwarfDebug::DirCfiDefCfaRegister(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!DirCheck(info, diags, 1))
        return;

    bool have_reg = false;
    unsigned int reg;

    NameValues& nvs = info.getNameValues();
    DirRegNum(nvs.front(), diags, &info.getObject(), &reg, &have_reg);
    if (!have_reg)
        return;

    AdvanceCfiAddress(info.getLocation(), info.getSource());
    DwarfCfiInsn* insn = DwarfCfiInsn::MakeDefCfaRegister(reg);
    insn->setSource(info.getSource());
    m_cur_fde->m_insns.push_back(insn);
}

void
DwarfDebug::DirCfiDefCfaOffset(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!DirCheck(info, diags, 1))
        return;

    bool have_off = false;
    IntNum off;

    NameValues& nvs = info.getNameValues();
    DirIntNum(nvs.front(), diags, &info.getObject(), &off, &have_off);
    if (!have_off)
        return;

    AdvanceCfiAddress(info.getLocation(), info.getSource());
    DwarfCfiInsn* insn = DwarfCfiInsn::MakeDefCfaOffset(off);
    insn->setSource(info.getSource());
    m_cur_fde->m_insns.push_back(insn);
}

void
DwarfDebug::DirCfiAdjustCfaOffset(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!DirCheck(info, diags, 1))
        return;

    bool have_off = false;
    IntNum off;

    NameValues& nvs = info.getNameValues();
    DirIntNum(nvs.front(), diags, &info.getObject(), &off, &have_off);
    if (!have_off)
        return;

    AdvanceCfiAddress(info.getLocation(), info.getSource());
    DwarfCfiInsn* insn = DwarfCfiInsn::MakeDefCfaOffset(m_cfa_cur_offset+off);
    insn->setSource(info.getSource());
    m_cur_fde->m_insns.push_back(insn);
}

void
DwarfDebug::DirCfiOffset(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!DirCheck(info, diags, 2))
        return;

    bool have_reg = false, have_off = false;
    unsigned int reg;
    IntNum off;

    NameValues& nvs = info.getNameValues();
    DirRegNum(nvs.front(), diags, &info.getObject(), &reg, &have_reg);
    DirIntNum(nvs[1], diags, &info.getObject(), &off, &have_off);
    if (!have_reg || !have_off)
        return;

    AdvanceCfiAddress(info.getLocation(), info.getSource());
    DwarfCfiInsn* insn = DwarfCfiInsn::MakeOffset(reg, off);
    insn->setSource(info.getSource());
    m_cur_fde->m_insns.push_back(insn);
}

void
DwarfDebug::DirCfiRelOffset(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!DirCheck(info, diags, 2))
        return;

    bool have_reg = false, have_off = false;
    unsigned int reg;
    IntNum off;

    NameValues& nvs = info.getNameValues();
    DirRegNum(nvs.front(), diags, &info.getObject(), &reg, &have_reg);
    DirIntNum(nvs[1], diags, &info.getObject(), &off, &have_off);
    if (!have_reg || !have_off)
        return;

    AdvanceCfiAddress(info.getLocation(), info.getSource());
    DwarfCfiInsn* insn = DwarfCfiInsn::MakeOffset(reg, off-m_cfa_cur_offset);
    insn->setSource(info.getSource());
    m_cur_fde->m_insns.push_back(insn);
}

void
DwarfDebug::DirCfiRegister(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!DirCheck(info, diags, 2))
        return;

    bool have_reg1 = false, have_reg2 = false;
    unsigned int reg1;
    unsigned int reg2;

    NameValues& nvs = info.getNameValues();
    DirRegNum(nvs.front(), diags, &info.getObject(), &reg1, &have_reg1);
    DirRegNum(nvs[1], diags, &info.getObject(), &reg2, &have_reg2);
    if (!have_reg1 || !have_reg2)
        return;

    AdvanceCfiAddress(info.getLocation(), info.getSource());
    DwarfCfiInsn* insn = DwarfCfiInsn::MakeRegister(reg1, reg2);
    insn->setSource(info.getSource());
    m_cur_fde->m_insns.push_back(insn);
}

void
DwarfDebug::DirCfiRestore(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!DirCheck(info, diags, 1))
        return;

    bool have_reg = false;
    unsigned int reg;

    NameValues& nvs = info.getNameValues();
    DirRegNum(nvs.front(), diags, &info.getObject(), &reg, &have_reg);
    if (!have_reg)
        return;

    AdvanceCfiAddress(info.getLocation(), info.getSource());
    DwarfCfiInsn* insn = DwarfCfiInsn::MakeRestore(reg);
    insn->setSource(info.getSource());
    m_cur_fde->m_insns.push_back(insn);
}

void
DwarfDebug::DirCfiUndefined(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!DirCheck(info, diags, 1))
        return;

    bool have_reg = false;
    unsigned int reg;

    NameValues& nvs = info.getNameValues();
    DirRegNum(nvs.front(), diags, &info.getObject(), &reg, &have_reg);
    if (!have_reg)
        return;

    AdvanceCfiAddress(info.getLocation(), info.getSource());
    DwarfCfiInsn* insn = DwarfCfiInsn::MakeUndefined(reg);
    insn->setSource(info.getSource());
    m_cur_fde->m_insns.push_back(insn);
}

void
DwarfDebug::DirCfiSameValue(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!DirCheck(info, diags, 1))
        return;

    bool have_reg = false;
    unsigned int reg;

    NameValues& nvs = info.getNameValues();
    DirRegNum(nvs.front(), diags, &info.getObject(), &reg, &have_reg);
    if (!have_reg)
        return;

    AdvanceCfiAddress(info.getLocation(), info.getSource());
    DwarfCfiInsn* insn = DwarfCfiInsn::MakeSameValue(reg);
    insn->setSource(info.getSource());
    m_cur_fde->m_insns.push_back(insn);
}

void
DwarfDebug::DirCfiRememberState(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!DirCheck(info, diags, 0))
        return;

    m_cfa_stack.push_back(m_cfa_cur_offset);

    AdvanceCfiAddress(info.getLocation(), info.getSource());
    DwarfCfiInsn* insn = DwarfCfiInsn::MakeRememberState();
    insn->setSource(info.getSource());
    m_cur_fde->m_insns.push_back(insn);
}

void
DwarfDebug::DirCfiRestoreState(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!DirCheck(info, diags, 0))
        return;

    if (m_cfa_stack.empty())
    {
        diags.Report(info.getSource(), diag::err_cfi_state_stack_empty);
        return;
    }
    m_cfa_cur_offset = m_cfa_stack.back();
    m_cfa_stack.pop_back();

    AdvanceCfiAddress(info.getLocation(), info.getSource());
    DwarfCfiInsn* insn = DwarfCfiInsn::MakeRememberState();
    insn->setSource(info.getSource());
    m_cur_fde->m_insns.push_back(insn);
}

void
DwarfDebug::DirCfiReturnColumn(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!DirCheck(info, diags, 1))
        return;

    bool have_reg = false;
    unsigned int reg;

    NameValues& nvs = info.getNameValues();
    DirRegNum(nvs.front(), diags, &info.getObject(), &reg, &have_reg);
    if (!have_reg)
        return;

    m_cur_fde->m_return_column = reg;
}

void
DwarfDebug::DirCfiSignalFrame(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!DirCheck(info, diags, 0))
        return;
    m_cur_fde->m_signal_frame = true;
}

void
DwarfDebug::DirCfiWindowSave(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!DirCheck(info, diags, 0))
        return;
    AdvanceCfiAddress(info.getLocation(), info.getSource());
    DwarfCfiInsn* insn = DwarfCfiInsn::MakeGNUWindowSave();
    insn->setSource(info.getSource());
    m_cur_fde->m_insns.push_back(insn);
}

void
DwarfDebug::DirCfiEscape(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    NameValues& nvs = info.getNameValues();
    std::vector<Expr> esc;

    if (!m_cur_fde)
    {
        diags.Report(info.getSource(), diag::warn_outside_cfiproc);
        return;
    }

    for (NameValues::const_iterator i=nvs.begin(), end=nvs.end();
         i != end; ++i)
    {
        if (!i->isExpr())
        {
            diags.Report(i->getValueRange().getBegin(),
                         diag::err_value_expression);
            return;
        }
        esc.push_back(i->getExpr(info.getObject()));
    }

    AdvanceCfiAddress(info.getLocation(), info.getSource());
    DwarfCfiInsn* insn = DwarfCfiInsn::MakeEscape(&esc);
    insn->setSource(info.getSource());
    m_cur_fde->m_insns.push_back(insn);
}

void
DwarfDebug::DirCfiValEncodedAddr(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    if (!m_cur_fde)
    {
        diags.Report(info.getSource(), diag::warn_outside_cfiproc);
        return;
    }

    bool have_reg = false;
    unsigned int reg;

    NameValues& nvs = info.getNameValues();
    DirRegNum(nvs.front(), diags, &info.getObject(), &reg, &have_reg);
    if (!have_reg)
        return;

    unsigned char encoding;
    Expr func;
    if (!DirCfiEncLabel(info, diags, &encoding, &func, 0, 1))
        return;

    AdvanceCfiAddress(info.getLocation(), info.getSource());
    DwarfCfiInsn* insn = DwarfCfiInsn::MakeValEncodedAddr(reg, encoding, func);
    insn->setSource(info.getSource());
    m_cur_fde->m_insns.push_back(insn);
}

static void
x86_x86_frame_initial_insns(DwarfCfiFde& fde)
{
    fde.m_insns.push_back(DwarfCfiInsn::MakeDefCfa(4, 4));
    fde.m_insns.push_back(DwarfCfiInsn::MakeOffset(8, -4));
}

static void
x86_amd64_frame_initial_insns(DwarfCfiFde& fde)
{
    fde.m_insns.push_back(DwarfCfiInsn::MakeDefCfa(7, 8));
    fde.m_insns.push_back(DwarfCfiInsn::MakeOffset(16, -8));
}

void
DwarfDebug::InitCfi(Arch& arch)
{
    m_eh_frame = true;
    m_debug_frame = false;

    m_fde_reloc_size = 4;

    if (arch.getModule().getKeyword() == "x86" && arch.getMachine() == "x86")
    {
        m_cie_data_alignment = -4;
        m_default_return_column = 8;
        m_frame_initial_instructions = x86_x86_frame_initial_insns;
    }
    else if (arch.getModule().getKeyword() == "x86" &&
             arch.getMachine() == "amd64")
    {
        m_cie_data_alignment = -8;
        m_default_return_column = 16;
        m_frame_initial_instructions = x86_amd64_frame_initial_insns;
    }
    else
    {
        m_cie_data_alignment = 0;
        m_default_return_column = 0;
        m_frame_initial_instructions = 0;
    }
}

void
DwarfDebug::AddCfiDirectives(Directives& dirs, StringRef parser)
{
    static const Directives::Init<DwarfDebug> gas_dirs[] =
    {
        {".cfi_startproc", &DwarfDebug::DirCfiStartproc, Directives::ANY},
        {".cfi_endproc", &DwarfDebug::DirCfiEndproc, Directives::ANY},
        {".cfi_sections", &DwarfDebug::DirCfiSections, Directives::ID_REQUIRED},
        {".cfi_personality", &DwarfDebug::DirCfiPersonality,
         Directives::ARG_REQUIRED},
        {".cfi_lsda", &DwarfDebug::DirCfiLsda, Directives::ARG_REQUIRED},
        {".cfi_def_cfa", &DwarfDebug::DirCfiDefCfa, Directives::ARG_REQUIRED},
        {".cfi_def_cfa_register", &DwarfDebug::DirCfiDefCfaRegister,
         Directives::ARG_REQUIRED},
        {".cfi_def_cfa_offset", &DwarfDebug::DirCfiDefCfaOffset,
         Directives::ARG_REQUIRED},
        {".cfi_adjust_cfa_offset", &DwarfDebug::DirCfiAdjustCfaOffset,
         Directives::ARG_REQUIRED},
        {".cfi_offset", &DwarfDebug::DirCfiOffset, Directives::ARG_REQUIRED},
        {".cfi_rel_offset", &DwarfDebug::DirCfiRelOffset,
         Directives::ARG_REQUIRED},
        {".cfi_register", &DwarfDebug::DirCfiRegister,
         Directives::ARG_REQUIRED},
        {".cfi_restore", &DwarfDebug::DirCfiRestore, Directives::ARG_REQUIRED},
        {".cfi_undefined", &DwarfDebug::DirCfiUndefined,
         Directives::ARG_REQUIRED},
        {".cfi_same_value", &DwarfDebug::DirCfiSameValue,
         Directives::ARG_REQUIRED},
        {".cfi_remember_state", &DwarfDebug::DirCfiRememberState,
         Directives::ANY},
        {".cfi_restore_state", &DwarfDebug::DirCfiRestoreState,
         Directives::ANY},
        {".cfi_return_column", &DwarfDebug::DirCfiReturnColumn,
         Directives::ARG_REQUIRED},
        {".cfi_signal_frame", &DwarfDebug::DirCfiSignalFrame, Directives::ANY},
        {".cfi_window_save", &DwarfDebug::DirCfiWindowSave, Directives::ANY},
        {".cfi_escape", &DwarfDebug::DirCfiEscape, Directives::ARG_REQUIRED},
        {".cfi_val_encoded_addr", &DwarfDebug::DirCfiValEncodedAddr,
         Directives::ARG_REQUIRED},
    };

    if (parser.equals_lower("gas") || parser.equals_lower("gnu"))
        dirs.AddArray(this, gas_dirs);
}

void
DwarfDebug::GenerateCfiSection(ObjectFormat& ofmt,
                               DiagnosticsEngine& diags,
                               StringRef sectname,
                               bool eh_frame)
{
    unsigned int align;
    if (eh_frame)
        align = ofmt.getModule().getDefaultX86ModeBits() / 8;
    else
        align = m_sizeof_address;

    Section* sect = m_object.FindSection(sectname);
    if (!sect)
        sect = ofmt.AppendSection(sectname, SourceLocation(), diags);
    sect->setAlign(align);

    DwarfCfiOutput out(*sect, diags, *this, m_object, eh_frame);
    std::vector<DwarfCfiCie> cies;

    for (FDEs::iterator i=m_fdes.begin(), end=m_fdes.end(); i != end; ++i)
    {
        if (!eh_frame)
        {
            // Modify directly; note this means eh_frame version must be
            // called first if generating both!
            i->m_personality_encoding = DW_EH_PE_omit;
            i->m_lsda_encoding = DW_EH_PE_omit;
        }

        // Try to find an existing CIE that matches this FDE
        IsFdeMatch matcher(*i);
        std::vector<DwarfCfiCie>::iterator ciep =
            std::find_if(cies.begin(), cies.end(), matcher);
        DwarfCfiCie* cie;
        if (ciep != cies.end())
            cie = &(*ciep);
        else
        {
            cies.push_back(DwarfCfiCie(&(*i)));
            cie = &cies.back();
            cie->Output(out, eh_frame ? 4 : align);
        }
        i->Output(out, *cie, (eh_frame && (i+1) != m_fdes.end()) ? 4 : align);
    }

    sect->Finalize(diags);
    sect->Optimize(diags);
}

void
DwarfDebug::GenerateCfi(ObjectFormat& ofmt,
                        SourceManager& smgr,
                        DiagnosticsEngine& diags)
{
    if (m_cur_fde)
    {
        diags.Report(SourceLocation(), diag::err_eof_inside_cfiproc);
        return;
    }
    if (m_fdes.empty())
        return;

    if (m_eh_frame)
        GenerateCfiSection(ofmt, diags, ".eh_frame", true);

    if (m_debug_frame)
        GenerateCfiSection(ofmt, diags, ".debug_frame", false);
}

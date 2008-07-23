//
// x86 identifier recognition and instruction handling
//
//  Copyright (C) 2002-2007  Peter Johnson
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
#include "x86insn.h"

#include <util.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

#include <libyasmx/compose.h>
#include <libyasmx/effaddr.h>
#include <libyasmx/errwarn.h>
#include <libyasmx/expr.h>
#include <libyasmx/intnum.h>
#include <libyasmx/marg_ostream.h>
#include <libyasmx/phash.h>

#include "x86arch.h"
#include "x86common.h"
#include "x86effaddr.h"
#include "x86general.h"
#include "x86jmp.h"
#include "x86jmpfar.h"
#include "x86opcode.h"
#include "x86prefix.h"


namespace yasm
{
namespace arch
{
namespace x86
{

static std::string cpu_find_reverse(unsigned int cpu0, unsigned int cpu1,
                                    unsigned int cpu2);

// Opcode modifiers.
enum X86OpcodeModifier
{
    MOD_Gap = 0,        // Eats a parameter / does nothing
    MOD_PreAdd = 1,     // Parameter adds to "special" prefix
    MOD_Op0Add = 2,     // Parameter adds to opcode byte 0
    MOD_Op1Add = 3,     // Parameter adds to opcode byte 1
    MOD_Op2Add = 4,     // Parameter adds to opcode byte 2
    MOD_SpAdd = 5,      // Parameter adds to "spare" value
    MOD_OpSizeR = 6,    // Parameter replaces opersize
    MOD_Imm8 = 7,       // Parameter is included as immediate byte
    MOD_AdSizeR = 8,    // Parameter replaces addrsize (jmp only)
    MOD_DOpS64R = 9,    // Parameter replaces default 64-bit opersize
    MOD_Op1AddSp = 10,  // Parameter is added as "spare" to opcode byte 2
    MOD_SetVEX = 11     // Parameter replaces internal VEX prefix value
};

// GAS suffix flags for instructions
enum X86GasSuffixFlags
{
    NONE = 0,
    SUF_B = 1<<0,
    SUF_W = 1<<1,
    SUF_L = 1<<2,
    SUF_Q = 1<<3,
    SUF_S = 1<<4,
    SUF_MASK = SUF_B|SUF_W|SUF_L|SUF_Q|SUF_S,

    // Flags only used in x86_insn_info
    GAS_ONLY = 1<<5,        // Only available in GAS mode
    GAS_ILLEGAL = 1<<6,     // Illegal in GAS mode
    GAS_NO_REV = 1<<7,      // Don't reverse operands in GAS mode

    // Flags only used in InsnPrefixParseData
    WEAK = 1<<5             // Relaxed operand mode for GAS
};

// Miscellaneous flag tests for instructions
enum X86MiscFlags {
    // These are tested against BITS==64.
    ONLY_64 = 1<<0,         // Only available in 64-bit mode
    NOT_64 = 1<<1,          // Not available (invalid) in 64-bit mode
    // These are tested against whether the base instruction is an AVX one.
    ONLY_AVX = 1<<2,        // Only available in AVX instruction
    NOT_AVX = 1<<3          // Not available (invalid) in AVX instruction
};

enum X86OperandType
{
    OPT_Imm = 0,        // immediate
    OPT_Reg = 1,        // any general purpose or FPU register
    OPT_Mem = 2,        // memory
    OPT_RM = 3,         // any general purpose or FPU register OR memory
    OPT_SIMDReg = 4,    // any MMX or XMM register
    OPT_SIMDRM = 5,     // any MMX or XMM register OR memory
    OPT_SegReg = 6,     // any segment register
    OPT_CRReg = 7,      // any CR register
    OPT_DRReg = 8,      // any DR register
    OPT_TRReg = 9,      // any TR register
    OPT_ST0 = 10,       // ST0
    OPT_Areg = 11,      // AL/AX/EAX/RAX (depending on size)
    OPT_Creg = 12,      // CL/CX/ECX/RCX (depending on size)
    OPT_Dreg = 13,      // DL/DX/EDX/RDX (depending on size)
    OPT_CS = 14,        // CS
    OPT_DS = 15,        // DS
    OPT_ES = 16,        // ES
    OPT_FS = 17,        // FS
    OPT_GS = 18,        // GS
    OPT_SS = 19,        // SS
    OPT_CR4 = 20,       // CR4
    // memory offset (an EA, but with no registers allowed)
    // [special case for MOV opcode]
    OPT_MemOffs = 21,
    OPT_Imm1 = 22,      // immediate, value=1 (for special-case shift)
    // immediate, does not contain SEG:OFF (for jmp/call)
    OPT_ImmNotSegOff = 23,
    OPT_XMM0 = 24,      /* XMM0 */
    // AX/EAX/RAX memory operand only (EA) [special case for SVM opcodes]
    OPT_MemrAX = 25,
    // EAX memory operand only (EA) [special case for SVM skinit opcode]
    OPT_MemEAX = 26,
    // SIMDReg with value equal to operand 0 SIMDReg
    OPT_SIMDRegMatch0 = 27
};

enum X86OperandSize
{
    // any size acceptable/no size spec acceptable (dep. on strict)
    OPS_Any = 0,
    // 8/16/32/64/80/128/256 bits (from user or reg size)
    OPS_8 = 1,
    OPS_16 = 2,
    OPS_32 = 3,
    OPS_64 = 4,
    OPS_80 = 5,
    OPS_128 = 6,
    OPS_256 = 7,
    // current BITS setting; when this is used the size matched
    // gets stored into the opersize as well.
    OPS_BITS = 8
};

enum X86OperandTargetmod
{
    OPTM_None = 0,  // no target mod acceptable
    OPTM_Near = 1,  // NEAR
    OPTM_Short = 2, // SHORT
    OPTM_Far = 3,   // FAR (or SEG:OFF immediate)
    OPTM_To = 4     // TO
};

enum X86OperandAction
{
    OPA_None = 0,   // does nothing (operand data is discarded)
    OPA_EA = 1,     // operand data goes into ea field
    OPA_Imm = 2,    // operand data goes into imm field
    OPA_SImm = 3,   // operand data goes into sign-extended imm field
    OPA_Spare = 4,  // operand data goes into "spare" field
    OPA_Op0Add = 5, // operand data is added to opcode byte 0
    OPA_Op1Add = 6, // operand data is added to opcode byte 1
    // operand data goes into BOTH ea and spare
    // (special case for imul opcode)
    OPA_SpareEA = 7,
    // relative jump (outputs a jmp instead of normal insn)
    OPA_JmpRel = 8,
    // operand size goes into address size (jmp only)
    OPA_AdSizeR = 9,
    // far jump (outputs a farjmp instead of normal insn)
    OPA_JmpFar = 10,
    // ea operand only sets address size (no actual ea field)
    OPA_AdSizeEA = 11,
    OPA_DREX = 12,  // operand data goes into DREX "dest" field
    OPA_VEX = 13,   // operand data goes into VEX "vvvv" field
    // operand data goes into BOTH VEX "vvvv" field and ea field
    OPA_EAVEX = 14,
    // operand data goes into BOTH VEX "vvvv" field and spare field
    OPA_SpareVEX = 15,
    // operand data goes into upper 4 bits of immediate byte (VEX is4 field)
    OPA_VEXImmSrc = 16,
    // operand data goes into bottom 4 bits of immediate byte
    // (currently only VEX imz2 field)
    OPA_VEXImm = 17
};

enum X86OperandPostAction
{
    OPAP_None = 0,
    // sign-extended imm8 that could expand to a large imm16/32
    OPAP_SImm8 = 1,
    // could become a short opcode mov with bits=64 and a32 prefix
    OPAP_ShortMov = 2,
    // forced 16-bit address size (override ignored, no prefix)
    OPAP_A16 = 3,
    // large imm64 that can become a sign-extended imm32
    OPAP_SImm32Avail = 4
};

struct X86InfoOperand
{
    // Operand types.  These are more detailed than the "general" types for all
    // architectures, as they include the size, for instance.

    // general type (must be exact match, except for RM types):
    unsigned int type:5;

    // size (user-specified, or from register size)
    unsigned int size:4;

    // size implicit or explicit ("strictness" of size matching on
    // non-registers -- registers are always strictly matched):
    // 0 = user size must exactly match size above.
    // 1 = user size either unspecified or exactly match size above.
    unsigned int relaxed:1;

    // effective address size
    // 0 = any address size allowed except for 64-bit
    // 1 = only 64-bit address size allowed
    unsigned int eas64:1;

    // target modification
    unsigned int targetmod:3;

    // Actions: what to do with the operand if the instruction matches.
    // Essentially describes what part of the output bytecode gets the
    // operand.  This may require conversion (e.g. a register going into
    // an ea field).  Naturally, only one of each of these may be contained
    // in the operands of a single insn_info structure.
    unsigned int action:5;

    // Postponed actions: actions which can't be completed at
    // parse-time due to possibly dependent expressions.  For these, some
    // additional data (stored in the second byte of the opcode with a
    // one-byte opcode) is passed to later stages of the assembler with
    // flags set to indicate postponed actions.
    unsigned int post_action:3;
};

struct X86InsnInfo
{
    // GAS suffix flags
    unsigned int gas_flags:8;      // Enabled for these GAS suffixes

    // Tests against BITS==64 and AVX
    unsigned int misc_flags:6;

    // The CPU feature flags needed to execute this instruction.  This is OR'ed
    // with arch-specific data[2].  This combined value is compared with
    // cpu_enabled to see if all bits set here are set in cpu_enabled--if so,
    // the instruction is available on this CPU.
    unsigned int cpu0:6;
    unsigned int cpu1:6;
    unsigned int cpu2:6;

    // Opcode modifiers for variations of instruction.  As each modifier reads
    // its parameter in LSB->MSB order from the arch-specific data[1] from the
    // lexer data, and the LSB of the arch-specific data[1] is reserved for the
    // count of insn_info structures in the instruction grouping, there can
    // only be a maximum of 3 modifiers.
    unsigned char modifiers[3];

    // Operand Size
    unsigned char opersize;

    // Default operand size in 64-bit mode (0 = 32-bit for readability).
    unsigned char def_opersize_64;

    // A special instruction prefix, used for some of the Intel SSE and SSE2
    // instructions.  Intel calls these 3-byte opcodes, but in AMD64's 64-bit
    // mode, they're treated like normal prefixes (e.g. the REX prefix needs
    // to be *after* the F2/F3/66 "prefix").
    // (0=no special prefix)
    // 0xC0 - 0xCF indicate a VEX prefix, with the four LSBs holding "WLpp":
    //  W: VEX.W field (meaning depends on opcode)
    //  L: 0=128-bit, 1=256-bit
    //  pp: SIMD prefix designation:
    //      00: None
    //      01: 66
    //      10: F3
    //      11: F2
    unsigned char special_prefix;

    // The DREX base byte value (almost).  The only bit kept from this
    // value is the OC0 bit (0x08).  The MSB (0x80) of this value indicates
    // if the DREX byte needs to be present in the instruction.
#define NEED_DREX_MASK 0x80
#define DREX_OC0_MASK 0x08
    unsigned char drex_oc0;

    // The length of the basic opcode
    unsigned char opcode_len;

    // The basic 1-3 byte opcode (not including the special instruction
    // prefix).
    unsigned char opcode[3];

    // The 3-bit "spare" value (extended opcode) for the R/M byte field
    unsigned char spare;

    // The number of operands this form of the instruction takes
    unsigned int num_operands:4;

    // The index into the insn_operands array which contains the type of each
    // operand, see above
    unsigned int operands_index:12;
};

inline
X86Prefix::X86Prefix(Type type, unsigned char value)
    : m_type(type), m_value(value)
{
}

inline
X86Prefix::~X86Prefix()
{
}

void
X86Prefix::put(std::ostream& os) const
{
    // TODO
    os << "PREFIX";
}

#include "x86insns.cpp"

void
X86Insn::do_append_jmpfar(BytecodeContainer& container, const X86InsnInfo& info)
{
    Operand& op = m_operands.front();
    std::auto_ptr<Expr> imm = op.release_imm();
    assert(imm.get() != 0);

    std::auto_ptr<Expr> segment(op.release_seg());

    const X86TargetModifier* tmod =
        static_cast<const X86TargetModifier*>(op.get_targetmod());
    if (segment.get() == 0 && tmod && tmod->type() == X86TargetModifier::FAR)
    {
        // "FAR imm" target needs to become "seg imm:imm".
        segment.reset(new Expr(Op::SEG, imm->clone(), imm->get_line()));
    }
    else if (segment.get() == 0)
        throw InternalError(N_("didn't get FAR expression in jmpfar"));

    X86Common common;
    common.m_opersize = info.opersize;
    common.m_mode_bits = m_mode_bits;
    common.apply_prefixes(info.def_opersize_64, m_prefixes);
    common.finish();
    append_jmpfar(container, common, X86Opcode(info.opcode_len, info.opcode),
                  segment, imm);
}

bool
X86Insn::match_jmp_info(const X86InsnInfo& info, unsigned int opersize,
                        X86Opcode& shortop, X86Opcode& nearop) const
{
    // Match CPU
    if (m_mode_bits != 64 && (info.misc_flags & ONLY_64))
        return false;
    if (m_mode_bits == 64 && (info.misc_flags & NOT_64))
        return false;

    if (!m_active_cpu[info.cpu0] ||
        !m_active_cpu[info.cpu1] ||
        !m_active_cpu[info.cpu2])
        return false;

    if (info.num_operands == 0)
        return false;

    if (insn_operands[info.operands_index+0].action != OPA_JmpRel)
        return false;

    if (info.opersize != opersize)
        return false;

    switch (insn_operands[info.operands_index+0].targetmod)
    {
        case OPTM_Short:
            shortop = X86Opcode(info.opcode_len, info.opcode);
            for (unsigned int i=0; i<NELEMS(info.modifiers); i++)
            {
                if (info.modifiers[i] == MOD_Op0Add)
                    shortop.add(0, m_mod_data[i]);
            }
            if (!nearop.is_empty())
                return true;
            break;
        case OPTM_Near:
            nearop = X86Opcode(info.opcode_len, info.opcode);
            for (unsigned int i=0; i<NELEMS(info.modifiers); i++)
            {
                if (info.modifiers[i] == MOD_Op1Add)
                    nearop.add(1, m_mod_data[i]);
            }
            if (!shortop.is_empty())
                return true;
            break;
    }
    return false;
}

void
X86Insn::do_append_jmp(BytecodeContainer& container, const X86InsnInfo& jinfo)
{
    static const unsigned char size_lookup[] =
        {0, 8, 16, 32, 64, 80, 128, 0, 0};  // 256 not needed

    // We know the target is in operand 0, but sanity check for Imm.
    Operand& op = m_operands.front();
    std::auto_ptr<Expr> imm = op.release_imm();
    assert(imm.get() != 0);

    JmpOpcodeSel op_sel;

    // See if the user explicitly specified short/near/far.
    switch (insn_operands[jinfo.operands_index+0].targetmod)
    {
        case OPTM_Short:
            op_sel = JMP_SHORT;
            break;
        case OPTM_Near:
            op_sel = JMP_NEAR;
            break;
        default:
            op_sel = JMP_NONE;
    }

    // Scan through other infos for this insn looking for short/near versions.
    // Needs to match opersize and number of operands, also be within CPU.
    X86Opcode shortop, nearop;
    for (const X86InsnInfo* info = &m_group[0]; info != &m_group[m_num_info];
         ++info)
    {
        if (match_jmp_info(*info, jinfo.opersize, shortop, nearop))
            break;
    }

    if ((op_sel == JMP_SHORT) && shortop.is_empty())
        throw TypeError(N_("no SHORT form of that jump instruction exists"));
    if ((op_sel == JMP_NEAR) && nearop.is_empty())
        throw TypeError(N_("no NEAR form of that jump instruction exists"));

    if (op_sel == JMP_NONE)
    {
        if (nearop.is_empty())
            op_sel = JMP_SHORT;
        if (shortop.is_empty())
            op_sel = JMP_NEAR;
    }

    X86Common common;
    common.m_opersize = jinfo.opersize;
    common.m_mode_bits = m_mode_bits;

    // Check for address size setting in second operand, if present
    if (jinfo.num_operands > 1 &&
        insn_operands[jinfo.operands_index+1].action == OPA_AdSizeR)
        common.m_addrsize = (unsigned char)
            size_lookup[insn_operands[jinfo.operands_index+1].size];

    // Check for address size override
    for (unsigned int i=0; i<NELEMS(jinfo.modifiers); i++)
    {
        if (jinfo.modifiers[i] == MOD_AdSizeR)
            common.m_addrsize = m_mod_data[i];
    }

    common.apply_prefixes(jinfo.def_opersize_64, m_prefixes);
    common.finish();

    append_jmp(container, common, shortop, nearop, imm, op_sel);
}

bool
X86Insn::match_operand(const Operand& op, const X86InfoOperand& info_op,
                       const Operand& op0, const unsigned int* size_lookup,
                       int bypass) const
{
    const X86Register* reg = static_cast<const X86Register*>(op.get_reg());
    const X86SegmentRegister* segreg =
        static_cast<const X86SegmentRegister*>(op.get_segreg());
    EffAddr* ea = op.get_memory();

    // Check operand type
    switch (info_op.type)
    {
        case OPT_Imm:
            if (!op.is_type(Insn::Operand::IMM))
                return false;
            break;
        case OPT_RM:
            if (op.is_type(Insn::Operand::MEMORY))
                break;
            /*@fallthrough@*/
        case OPT_Reg:
            if (!reg)
                return false;
            switch (reg->type())
            {
                case X86Register::REG8:
                case X86Register::REG8X:
                case X86Register::REG16:
                case X86Register::REG32:
                case X86Register::REG64:
                case X86Register::FPUREG:
                    break;
                default:
                    return false;
            }
            break;
        case OPT_Mem:
            if (!op.is_type(Insn::Operand::MEMORY))
                return false;
            break;
        case OPT_SIMDRM:
            if (op.is_type(Insn::Operand::MEMORY))
                break;
            /*@fallthrough@*/
        case OPT_SIMDRegMatch0:
        case OPT_SIMDReg:
            if (!reg)
                return false;
            switch (reg->type())
            {
                case X86Register::MMXREG:
                case X86Register::XMMREG:
                case X86Register::YMMREG:
                    break;
                default:
                    return false;
            }
            if (info_op.type == OPT_SIMDRegMatch0 && bypass != 7 &&
                reg != op0.get_reg())
                return false;
            break;
        case OPT_SegReg:
            if (!op.is_type(Insn::Operand::SEGREG))
                return false;
            break;
        case OPT_CRReg:
            if (!reg || reg->type() != X86Register::CRREG)
                return false;
            break;
        case OPT_DRReg:
            if (!reg || reg->type() != X86Register::DRREG)
                return false;
            break;
        case OPT_TRReg:
            if (!reg || reg->type() != X86Register::TRREG)
                return false;
            break;
        case OPT_ST0:
            if (!reg || reg->type() != X86Register::FPUREG)
                return false;
            break;
        case OPT_Areg:
            if (!reg || reg->num() != 0 ||
                (info_op.size == OPS_8 &&
                 reg->type() != X86Register::REG8 &&
                 reg->type() != X86Register::REG8X) ||
                (info_op.size == OPS_16 && reg->type() != X86Register::REG16) ||
                (info_op.size == OPS_32 && reg->type() != X86Register::REG32) ||
                (info_op.size == OPS_64 && reg->type() != X86Register::REG64))
                return false;
            break;
        case OPT_Creg:
            if (!reg || reg->num() != 1 ||
                (info_op.size == OPS_8 &&
                 reg->type() != X86Register::REG8 &&
                 reg->type() != X86Register::REG8X) ||
                (info_op.size == OPS_16 && reg->type() != X86Register::REG16) ||
                (info_op.size == OPS_32 && reg->type() != X86Register::REG32) ||
                (info_op.size == OPS_64 && reg->type() != X86Register::REG64))
                return false;
            break;
        case OPT_Dreg:
            if (!reg || reg->num() != 2 ||
                (info_op.size == OPS_8 &&
                 reg->type() != X86Register::REG8 &&
                 reg->type() != X86Register::REG8X) ||
                (info_op.size == OPS_16 && reg->type() != X86Register::REG16) ||
                (info_op.size == OPS_32 && reg->type() != X86Register::REG32) ||
                (info_op.size == OPS_64 && reg->type() != X86Register::REG64))
                return false;
            break;
        case OPT_CS:
            if (!segreg || segreg->type() != X86SegmentRegister::CS)
                return false;
            break;
        case OPT_DS:
            if (!segreg || segreg->type() != X86SegmentRegister::DS)
                return false;
            break;
        case OPT_ES:
            if (!segreg || segreg->type() != X86SegmentRegister::ES)
                return false;
            break;
        case OPT_FS:
            if (!segreg || segreg->type() != X86SegmentRegister::FS)
                return false;
            break;
        case OPT_GS:
            if (!segreg || segreg->type() != X86SegmentRegister::GS)
                return false;
            break;
        case OPT_SS:
            if (!segreg || segreg->type() != X86SegmentRegister::SS)
                return false;
            break;
        case OPT_CR4:
            if (!reg || reg->type() != X86Register::CRREG || reg->num() != 4)
                return false;
            break;
        case OPT_MemOffs:
            if (!ea ||
                ea->m_disp.get_abs()->contains(ExprTerm::REG) ||
                ea->m_pc_rel ||
                (!ea->m_not_pc_rel && m_default_rel && ea->m_disp.m_size != 64))
                return false;
            break;
        case OPT_Imm1:
        {
            if (Expr* imm = op.get_imm())
            {
                const IntNum* num = imm->get_intnum();
                if (!num || !num->is_pos1())
                    return false;
            }
            else
                return false;
            break;
        }
        case OPT_ImmNotSegOff:
        {
            Expr* imm = op.get_imm();
            if (!imm || op.get_targetmod() != 0 || op.get_seg() != 0)
                return false;
            break;
        }
        case OPT_XMM0:
            if (!reg || reg->type() != X86Register::XMMREG || reg->num() != 0)
                return false;
            break;
        case OPT_MemrAX:
        {
            const Register* reg2;
            if (!ea ||
                !(reg2 = ea->m_disp.get_abs()->get_reg()) ||
                reg->num() != 0 ||
                (reg->type() != X86Register::REG16 &&
                 reg->type() != X86Register::REG32 &&
                 reg->type() != X86Register::REG64))
                return false;
            break;
        }
        case OPT_MemEAX:
        {
            const Register* reg2;
            if (!ea ||
                !(reg2 = ea->m_disp.get_abs()->get_reg()) ||
                reg->type() != X86Register::REG32 ||
                reg->num() != 0)
                return false;
            break;
        }
        default:
            throw InternalError(N_("invalid operand type"));
    }

    // Check operand size
    unsigned int size = size_lookup[info_op.size];
    if (m_suffix != 0)
    {
        // Require relaxed operands for GAS mode (don't allow
        // per-operand sizing).
        if (reg && op.get_size() == 0)
        {
            // Register size must exactly match
            if (reg->get_size() != size)
                return false;
        }
        else if ((info_op.type == OPT_Imm
                  || info_op.type == OPT_ImmNotSegOff
                  || info_op.type == OPT_Imm1)
                 && !info_op.relaxed
                 && info_op.action != OPA_JmpRel)
            return false;
    }
    else
    {
        if (reg && op.get_size() == 0)
        {
            // Register size must exactly match
            if ((bypass == 4 && (&op-&op0) == 0)
                || (bypass == 5 && (&op-&op0) == 1)
                || (bypass == 6 && (&op-&op0) == 2))
                ;
            else if (reg->get_size() != size)
                return false;
        }
        else
        {
            if ((bypass == 1 && (&op-&op0) == 0)
                || (bypass == 2 && (&op-&op0) == 1)
                || (bypass == 3 && (&op-&op0) == 2))
                ;
            else if (info_op.relaxed)
            {
                // Relaxed checking
                if (size != 0 && op.get_size() != size && op.get_size() != 0)
                    return false;
            }
            else
            {
                // Strict checking
                if (op.get_size() != size)
                    return false;
            }
        }
    }

    // Check for 64-bit effective address size in NASM mode
    if (m_suffix == 0 && ea)
    {
        if (info_op.eas64)
        {
            if (ea->m_disp.m_size != 64)
                return false;
        }
        else if (ea->m_disp.m_size == 64)
            return false;
    }

    // Check target modifier
    const X86TargetModifier* targetmod =
        static_cast<const X86TargetModifier*>(op.get_targetmod());
    switch (info_op.targetmod)
    {
        case OPTM_None:
            if (targetmod != 0)
                return false;
            break;
        case OPTM_Near:
            if (targetmod->type() != X86TargetModifier::NEAR)
                return false;
            break;
        case OPTM_Short:
            if (targetmod->type() != X86TargetModifier::SHORT)
                return false;
            break;
        case OPTM_Far:
            if (targetmod->type() != X86TargetModifier::FAR)
                return false;
            break;
        case OPTM_To:
            if (targetmod->type() != X86TargetModifier::TO)
                return false;
            break;
        default:
            throw InternalError(N_("invalid target modifier type"));
    }

    return true;
}

bool
X86Insn::match_info(const X86InsnInfo& info, const unsigned int* size_lookup,
                    int bypass) const
{
    // Match CPU
    if (m_mode_bits != 64 && (info.misc_flags & ONLY_64))
        return false;
    if (m_mode_bits == 64 && (info.misc_flags & NOT_64))
        return false;

    if (bypass != 8 && (!m_active_cpu[info.cpu0] ||
                        !m_active_cpu[info.cpu1] ||
                        !m_active_cpu[info.cpu2]))
        return false;

    // Match # of operands
    if (m_operands.size() != info.num_operands)
        return false;

    // Match AVX
    if (!(m_misc_flags & ONLY_AVX) && (info.misc_flags & ONLY_AVX))
        return false;
    if ((m_misc_flags & ONLY_AVX) && (info.misc_flags & NOT_AVX))
        return false;

    // Match parser mode
    unsigned int gas_flags = info.gas_flags;
    if ((gas_flags & GAS_ONLY) && m_parser != X86Arch::PARSER_GAS)
        return false;
    if ((gas_flags & GAS_ILLEGAL) && m_parser == X86Arch::PARSER_GAS)
        return false;

    // Match suffix (if required)
    if (m_suffix != 0 && m_suffix != WEAK
        && ((m_suffix & SUF_MASK) & (gas_flags & SUF_MASK)) == 0)
        return false;

    if (m_operands.size() == 0)
        return true;    // no operands -> must have a match here.

    // Match each operand type and size
    // Use reversed operands in GAS mode if not otherwise specified
#if 0
    if (m_parser == X86Arch::PARSER_GAS && !(gas_flags & GAS_NO_REV))
        return std::equal(m_operands.rbegin(), m_operands.rend(),
                          &insn_operands[info.operands_index],
                          BIND::bind(&X86Insn::match_operand, this, _1, _2,
                                     REF::ref(m_operands.back()),
                                     size_lookup, bypass));
    else
        return std::equal(m_operands.begin(), m_operands.end(),
                          &insn_operands[info.operands_index],
                          BIND::bind(&X86Insn::match_operand, this, _1, _2,
                                     REF::ref(m_operands.front()),
                                     size_lookup, bypass));
#else
    const X86InfoOperand* first2 = &insn_operands[info.operands_index];
    if (m_parser == X86Arch::PARSER_GAS && !(gas_flags & GAS_NO_REV))
    {
        const Operand& first = m_operands.back();
        Operands::const_reverse_iterator
            first1 = m_operands.rbegin(), last1 = m_operands.rend();
        while (first1 != last1)
        {
            if (!match_operand(*first1, *first2, first, size_lookup, bypass))
                return false;
            ++first1;
            ++first2;
        }
        return true;
    }
    else
    {
        const Operand& first = m_operands.front();
        Operands::const_iterator
            first1 = m_operands.begin(), last1 = m_operands.end();
        while (first1 != last1)
        {
            if (!match_operand(*first1, *first2, first, size_lookup, bypass))
                return false;
            ++first1;
            ++first2;
        }
        return true;
    }
#endif
}

const X86InsnInfo*
X86Insn::find_match(const unsigned int* size_lookup, int bypass) const
{
    // Just do a simple linear search through the info array for a match.
    // First match wins.
    const X86InsnInfo* info =
        std::find_if(&m_group[0], &m_group[m_num_info],
                     BIND::bind(&X86Insn::match_info, this, _1, size_lookup,
                                bypass));
    if (info == &m_group[m_num_info])
        return 0;
    return info;
}

void
X86Insn::match_error(const unsigned int* size_lookup) const
{
    const X86InsnInfo* i = m_group;

    // Check for matching # of operands
    bool found = false;
    for (int ni=m_num_info; ni>0; ni--, ++i)
    {
        if (m_operands.size() == i->num_operands)
        {
            found = true;
            break;
        }
    }
    if (!found)
        throw TypeError(N_("invalid number of operands"));

    int bypass;
    for (bypass=1; bypass<9; bypass++)
    {
        i = find_match(size_lookup, bypass);
        if (i)
            break;
    }

    switch (bypass)
    {
        case 1:
        case 4:
            throw TypeError(
                String::compose(N_("invalid size for operand %d"), 1));
        case 2:
        case 5:
            throw TypeError(
                String::compose(N_("invalid size for operand %d"), 2));
        case 3:
        case 6:
            throw TypeError(
                String::compose(N_("invalid size for operand %d"), 3));
        case 7:
            throw TypeError(
                N_("one of source operand 1 or 3 must match dest operand"));
        case 8:
        {
            unsigned int cpu0 = i->cpu0, cpu1 = i->cpu1, cpu2 = i->cpu2;
            throw TypeError(
                String::compose(N_("requires CPU%s"),
                                cpu_find_reverse(cpu0, cpu1, cpu2)));
        }
    }
}

void
X86Insn::do_append(BytecodeContainer& container)
{
    unsigned int size_lookup[] = {0, 8, 16, 32, 64, 80, 128, 256, 0};
    size_lookup[OPS_BITS] = m_mode_bits;

    if (m_operands.size() > 5)
        throw TypeError(N_("too many operands"));

    // If we're running in GAS mode, look at the first insn_info to see
    // if this is a relative jump (OPA_JmpRel).  If so, run through the
    // operands and adjust for dereferences / lack thereof.
    if (m_parser == X86Arch::PARSER_GAS
        && insn_operands[m_group->operands_index+0].action == OPA_JmpRel)
    {
        for (Insn::Operands::iterator op = m_operands.begin(),
             end = m_operands.end(); op != end; ++op)
        {
            const X86Register* reg =
                static_cast<const X86Register*>(op->get_reg());
            EffAddr* ea = op->get_memory();

            if (!op->is_deref() && (reg || (ea && ea->m_strong)))
                warn_set(WARN_GENERAL, N_("indirect call without `*'"));
            if (!op->is_deref() && ea && !ea->m_strong)
            {
                // Memory that is not dereferenced, and not strong, is
                // actually an immediate for the purposes of relative jumps.
                if (ea->m_segreg != 0)
                    warn_set(WARN_GENERAL,
                             N_("skipping prefixes on this instruction"));
                *op = std::auto_ptr<Expr>(ea->m_disp.get_abs()->clone());
                delete ea;
            }
        }
    }

    const X86InsnInfo* info = find_match(size_lookup, 0);

    if (!info)
    {
        // Didn't find a match
        match_error(size_lookup);
        throw TypeError(N_("invalid combination of opcode and operands"));
    }

    if (m_operands.size() > 0)
    {
        switch (insn_operands[info->operands_index+0].action)
        {
            case OPA_JmpRel:
                // Shortcut to JmpRel
                do_append_jmp(container, *info);
                return;
            case OPA_JmpFar:
                // Shortcut to JmpFar
                do_append_jmpfar(container, *info);
                return;
        }
    }

    do_append_general(container, *info, size_lookup);
}

class BuildGeneral
{
public:
    BuildGeneral(const X86InsnInfo& info, unsigned int mode_bits,
                 const unsigned int* size_lookup, bool force_strict,
                 bool default_rel);
    ~BuildGeneral();

    void apply_modifiers(unsigned char* mod_data);
    void update_rex();
    void apply_operands(X86Arch::ParserSelect parser,
                        Insn::Operands& operands);
    void apply_segregs(const Insn::SegRegs& segregs);
    void finish(BytecodeContainer& container, const Insn::Prefixes& prefixes);

private:
    void apply_operand(const X86InfoOperand& info_op, Insn::Operand& op);

    const X86InsnInfo& m_info;
    unsigned int m_mode_bits;
    const unsigned int* m_size_lookup;
    bool m_force_strict;
    bool m_default_rel;

    X86Opcode m_opcode;
    std::auto_ptr<X86EffAddr> m_x86_ea;
    std::auto_ptr<Expr> m_imm;
    unsigned int m_def_opersize_64;
    unsigned char m_special_prefix;
    unsigned char m_spare;
    unsigned char m_drex;
    unsigned char m_im_len;
    unsigned char m_im_sign;
    GeneralPostOp m_postop;
    unsigned char m_rex;
    unsigned char* m_pdrex;
    unsigned char m_vexdata;
    unsigned char m_vexreg;
    unsigned char m_opersize;
    unsigned char m_addrsize;
};

inline
BuildGeneral::BuildGeneral(const X86InsnInfo& info,
                           unsigned int mode_bits,
                           const unsigned int* size_lookup,
                           bool force_strict,
                           bool default_rel)
    : m_info(info),
      m_mode_bits(mode_bits),
      m_size_lookup(size_lookup),
      m_force_strict(force_strict),
      m_default_rel(default_rel),
      m_opcode(info.opcode_len, info.opcode),
      m_x86_ea(0),
      m_imm(0),
      m_def_opersize_64(info.def_opersize_64),
      m_special_prefix(info.special_prefix),
      m_spare(info.spare),
      m_drex(info.drex_oc0 & DREX_OC0_MASK),
      m_im_len(0),
      m_im_sign(0),
      m_postop(POSTOP_NONE),
      m_rex(0),
      m_pdrex((info.drex_oc0 & NEED_DREX_MASK) ? &m_drex : 0),
      m_vexdata(0),
      m_vexreg(0),
      m_opersize(info.opersize),
      m_addrsize(0)
{
    // Move VEX data (stored in special prefix) to separate location to
    // allow overriding of special prefix by modifiers.
    if ((m_special_prefix & 0xF0) == 0xC0)
    {
        m_vexdata = m_special_prefix;
        m_special_prefix = 0;
    }
}

inline
BuildGeneral::~BuildGeneral()
{
}

void
BuildGeneral::apply_modifiers(unsigned char* mod_data)
{
    // Apply modifiers
    for (unsigned int i=0; i<NELEMS(m_info.modifiers); i++)
    {
        switch (m_info.modifiers[i])
        {
            case MOD_Gap:
                break;
            case MOD_PreAdd:
                m_special_prefix += mod_data[i];
                break;
            case MOD_Op0Add:
                m_opcode.add(0, mod_data[i]);
                break;
            case MOD_Op1Add:
                m_opcode.add(1, mod_data[i]);
                break;
            case MOD_Op2Add:
                m_opcode.add(2, mod_data[i]);
                break;
            case MOD_SpAdd:
                m_spare += mod_data[i];
                break;
            case MOD_OpSizeR:
                m_opersize = mod_data[i];
                break;
            case MOD_Imm8:
                m_imm.reset(new Expr(new IntNum(mod_data[i]), 0));
                m_im_len = 8;
                break;
            case MOD_DOpS64R:
                m_def_opersize_64 = mod_data[i];
                break;
            case MOD_Op1AddSp:
                m_opcode.add(1, mod_data[i]<<3);
                break;
            case MOD_SetVEX:
                m_vexdata = mod_data[i];
                break;
            default:
                break;
        }
    }
}

void
BuildGeneral::update_rex()
{
    // In 64-bit mode, if opersize is 64 and default is not 64,
    // force REX byte.
    if (m_mode_bits == 64 && m_opersize == 64 && m_def_opersize_64 != 64)
        m_rex = 0x48;
}

void
BuildGeneral::apply_operands(X86Arch::ParserSelect parser,
                             Insn::Operands& operands)
{
    // Go through operands and assign
    if (operands.size() > 0)
    {
        const X86InfoOperand* info_ops = &insn_operands[m_info.operands_index];

        // Use reversed operands in GAS mode if not otherwise specified
        if (parser == X86Arch::PARSER_GAS &&
            !(m_info.gas_flags & GAS_NO_REV))
        {
            Insn::Operands::reverse_iterator
                first = operands.rbegin(), last = operands.rend();
            while (first != last)
                apply_operand(*info_ops++, *first++);
        }
        else
        {
            Insn::Operands::iterator
                first = operands.begin(), last = operands.end();
            while (first != last)
                apply_operand(*info_ops++, *first++);
        }
    }
}

void
BuildGeneral::apply_operand(const X86InfoOperand& info_op, Insn::Operand& op)
{
    switch (info_op.action)
    {
        case OPA_None:
            // Throw away the operand contents
            break;
        case OPA_EA:
            switch (op.get_type())
            {
                case Insn::Operand::NONE:
                    throw InternalError(
                        N_("invalid operand conversion"));
                case Insn::Operand::REG:
                    m_x86_ea.reset(new X86EffAddr(
                        static_cast<const X86Register*>(op.get_reg()),
                        &m_rex, m_pdrex, m_mode_bits));
                    break;
                case Insn::Operand::SEGREG:
                    throw InternalError(
                        N_("invalid operand conversion"));
                case Insn::Operand::MEMORY:
                {
                    if (op.get_seg() != 0)
                    {
                        throw ValueError(
                            N_("invalid segment in effective address"));
                    }
                    m_x86_ea.reset(static_cast<X86EffAddr*>
                                   (op.release_memory().release()));
                    const X86SegmentRegister* segreg =
                        static_cast<const X86SegmentRegister*>
                        (m_x86_ea->m_segreg);
                    if (info_op.type == OPT_MemOffs)
                        // Special-case for MOV MemOffs instruction
                        m_x86_ea->set_disponly();
                    else if (m_default_rel &&
                             !m_x86_ea->m_not_pc_rel &&
                             (!segreg ||
                              (segreg->type() != X86SegmentRegister::FS &&
                               segreg->type() != X86SegmentRegister::GS)) &&
                             !m_x86_ea->m_disp.get_abs()->contains(ExprTerm::REG))
                        // Enable default PC-rel if no regs and segreg
                        // is not FS or GS.
                        m_x86_ea->m_pc_rel = true;
                    break;
                }
                case Insn::Operand::IMM:
                    m_x86_ea.reset(new X86EffAddr(op.release_imm(),
                                                  m_size_lookup[info_op.size]));
                    break;
            }
            break;
        case OPA_EAVEX:
            if (const X86Register* reg =
                static_cast<const X86Register*>(op.get_reg()))
            {
                m_x86_ea.reset(new X86EffAddr(reg, &m_rex, m_pdrex,
                                              m_mode_bits));
                m_vexreg = reg->num() & 0xF;
            }
            else
                throw InternalError(N_("invalid operand conversion"));
            break;
        case OPA_Imm:
            if (op.get_seg() != 0)
                throw ValueError(N_("immediate does not support segment"));
            m_imm = op.release_imm();
            if (m_imm.get() == 0)
                throw InternalError(N_("invalid operand conversion"));

            m_im_len = m_size_lookup[info_op.size];
            break;
        case OPA_SImm:
            if (op.get_seg() != 0)
                throw ValueError(N_("immediate does not support segment"));
            m_imm = op.release_imm();
            if (m_imm.get() == 0)
                throw InternalError(N_("invalid operand conversion"));

            m_im_len = m_size_lookup[info_op.size];
            m_im_sign = 1;
            break;
        case OPA_Spare:
            if (const SegmentRegister* segreg = op.get_segreg())
            {
                m_spare =
                    (static_cast<const X86SegmentRegister*>(segreg))->num();
            }
            else if (const Register* reg = op.get_reg())
            {
                set_rex_from_reg(&m_rex, m_pdrex, &m_spare,
                                 static_cast<const X86Register*>(reg),
                                 m_mode_bits, X86_REX_R);
            }
            else
                throw InternalError(N_("invalid operand conversion"));
            break;
        case OPA_SpareVEX:
            if (const X86Register* reg =
                static_cast<const X86Register*>(op.get_reg()))
            {
                set_rex_from_reg(&m_rex, m_pdrex, &m_spare, reg, m_mode_bits,
                                 X86_REX_R);
                m_vexreg = reg->num() & 0xF;
            }
            else
                throw InternalError(N_("invalid operand conversion"));
            break;
        case OPA_Op0Add:
            if (const Register* reg = op.get_reg())
            {
                unsigned char opadd;
                set_rex_from_reg(&m_rex, m_pdrex, &opadd,
                                 static_cast<const X86Register*>(reg),
                                 m_mode_bits, X86_REX_B);
                m_opcode.add(0, opadd);
            }
            else
                throw InternalError(N_("invalid operand conversion"));
            break;
        case OPA_Op1Add:
            if (const Register* reg = op.get_reg())
            {
                unsigned char opadd;
                set_rex_from_reg(&m_rex, m_pdrex, &opadd,
                                 static_cast<const X86Register*>(reg),
                                 m_mode_bits, X86_REX_B);
                m_opcode.add(1, opadd);
            }
            else
                throw InternalError(N_("invalid operand conversion"));
            break;
        case OPA_SpareEA:
            if (const Register* reg = op.get_reg())
            {
                const X86Register* x86_reg =
                    static_cast<const X86Register*>(reg);
                m_x86_ea.reset(new X86EffAddr(x86_reg, &m_rex, m_pdrex,
                                              m_mode_bits));
                set_rex_from_reg(&m_rex, m_pdrex, &m_spare, x86_reg,
                                 m_mode_bits, X86_REX_R);
            }
            else
                throw InternalError(N_("invalid operand conversion"));
            break;
        case OPA_AdSizeEA:
        {
            // Only implement this for OPT_MemrAX and OPT_MemEAX
            // for now.
            EffAddr* ea = op.get_memory();
            const X86Register* reg = static_cast<const X86Register*>
                (ea->m_disp.get_abs()->get_reg());
            if (!ea || !reg)
                throw InternalError(N_("invalid operand conversion"));
            X86Register::Type regtype = reg->type();
            unsigned int regnum = reg->num();
            // 64-bit mode does not allow 16-bit addresses
            if (m_mode_bits == 64 && regtype == X86Register::REG16 &&
                regnum == 0)
                throw TypeError(
                    N_("16-bit addresses not supported in 64-bit mode"));
            else if (regtype == X86Register::REG16 && regnum == 0)
                m_addrsize = 16;
            else if (regtype == X86Register::REG32 && regnum == 0)
                m_addrsize = 32;
            else if (m_mode_bits == 64 && regtype == X86Register::REG64 &&
                     regnum == 0)
                m_addrsize = 64;
            else
                throw TypeError(N_("unsupported address size"));
            break;
        }
        case OPA_DREX:
            if (const Register* reg = op.get_reg())
            {
                m_drex &= 0x0F;
                m_drex |= (static_cast<const X86Register*>(reg)->num() << 4)
                    & 0xF0;
            }
            else
                throw InternalError(N_("invalid operand conversion"));
            break;
        case OPA_VEX:
            if (const Register* reg = op.get_reg())
                m_vexreg = static_cast<const X86Register*>(reg)->num() & 0xF;
            else
                throw InternalError(N_("invalid operand conversion"));
            break;
        case OPA_VEXImmSrc:
            if (const X86Register* reg =
                static_cast<const X86Register*>(op.get_reg()))
            {
                if (m_imm.get() == 0)
                    m_imm.reset(new Expr(IntNum((reg->num() << 4) & 0xF0)));
                else
                {
                    m_imm.reset(new Expr(
                        new Expr(m_imm, Op::AND, IntNum(0x0F)),
                        Op::OR,
                        IntNum((reg->num() << 4) & 0xF0)));
                }
                m_im_len = 8;
            }
            else
                throw InternalError(N_("invalid operand conversion"));
            break;
        case OPA_VEXImm:
            if (op.get_type() == Insn::Operand::IMM)
            {
                if (m_imm.get() == 0)
                    m_imm = op.release_imm();
                else
                {
                    m_imm.reset(new Expr(
                        new Expr(op.release_imm(), Op::AND, IntNum(0x0F)),
                        Op::OR,
                        new Expr(m_imm, Op::AND, IntNum(0xF0))));
                }
                m_im_len = 8;
            }
            else
                throw InternalError(N_("invalid operand conversion"));
            break;
        default:
            throw InternalError(N_("unknown operand action"));
    }

    if (info_op.size == OPS_BITS)
        m_opersize = (unsigned char)m_mode_bits;

    switch (info_op.post_action)
    {
        case OPAP_None:
            break;
        case OPAP_SImm8:
            // Check operand strictness; if strict and non-8-bit,
            // pre-emptively expand to full size.
            // For unspecified size case, still optimize.
            if (!(m_force_strict || op.is_strict())
                || op.get_size() == 0)
                m_postop = POSTOP_SIGNEXT_IMM8;
            else if (op.get_size() != 8)
                m_opcode.make_alt_1();
            break;
        case OPAP_ShortMov:
            m_postop = POSTOP_SHORT_MOV;
            break;
        case OPAP_A16:
            m_postop = POSTOP_ADDRESS16;
            break;
        case OPAP_SImm32Avail:
            m_postop = POSTOP_SIMM32_AVAIL;
            break;
        default:
            throw InternalError(N_("unknown operand postponed action"));
    }
}

void
BuildGeneral::apply_segregs(const Insn::SegRegs& segregs)
{
    X86EffAddr* x86_ea = m_x86_ea.get();
    if (x86_ea)
    {
        x86_ea->init(m_spare, m_drex,
                     (m_info.drex_oc0 & NEED_DREX_MASK) != 0);
        std::for_each(segregs.begin(), segregs.end(),
                      BIND::bind(&X86EffAddr::set_segreg, x86_ea, _1));
    }
    else if (segregs.size() > 0 && m_special_prefix == 0)
    {
        if (segregs.size() > 1)
            warn_set(WARN_GENERAL,
                     N_("multiple segment overrides, using leftmost"));
        m_special_prefix =
            static_cast<const X86SegmentRegister*>(segregs.back())->prefix();
    }
    else if (segregs.size() > 0)
        throw InternalError(N_("unhandled segment prefix"));
}

void
BuildGeneral::finish(BytecodeContainer& container,
                     const Insn::Prefixes& prefixes)
{
    std::auto_ptr<Value> imm_val(0);

    if (m_imm.get() != 0)
    {
        imm_val.reset(new Value(m_im_len, m_imm));
        imm_val->m_sign = m_im_sign;
    }

    X86Common common;
    common.m_addrsize = m_addrsize;
    common.m_opersize = m_opersize;
    common.m_mode_bits = m_mode_bits;
    common.apply_prefixes(m_def_opersize_64, prefixes, &m_rex);
    common.finish();

    // Convert to VEX prefixes if requested.
    // To save space in the insn structure, the VEX prefix is written into
    // special_prefix and the first 2 bytes of the instruction are set to
    // the second two VEX bytes.  During calc_len() it may be shortened to
    // one VEX byte (this can only be done after knowledge of REX value).
    if (m_vexdata)
    {
        // Look at the first bytes of the opcode to see what leading bytes
        // to encode in the VEX mmmmm field.  Leave R=X=B=1 for now.
        if (m_opcode.get(0) != 0x0F)
            throw InternalError(N_("first opcode byte of VEX must be 0x0F"));

        unsigned char opcode[3];    // VEX opcode; 0=VEX1, 1=VEX2, 2=Opcode
        opcode[0] = 0xE0;           // R=X=B=1, mmmmm=0
        if (m_opcode.get(1) == 0x38)
        {
            opcode[2] = m_opcode.get(2);
            opcode[0] |= 0x02;      // implied 0x0F 0x38
        }
        else if (m_opcode.get(1) == 0x3A)
        {
            opcode[2] = m_opcode.get(2);
            opcode[0] |= 0x03;      // implied 0x0F 0x3A
        }
        else
        {
            // A 0F-only opcode; thus opcode is in byte 1.
            opcode[2] = m_opcode.get(1);
            opcode[0] |= 0x01;      // implied 0x0F
        }

        // Check for update of special prefix by modifiers
        if (m_special_prefix != 0)
        {
            m_vexdata &= ~0x03;
            switch (m_special_prefix)
            {
                case 0x66:
                    m_vexdata |= 0x01;
                    break;
                case 0xF3:
                    m_vexdata |= 0x02;
                    break;
                case 0xF2:
                    m_vexdata |= 0x03;
                    break;
                default:
                    throw InternalError(N_("unrecognized special prefix"));
            }
        }

        // 2nd VEX byte is WvvvvLpp.
        // W, L, pp come from vexdata
        // vvvv comes from 1s complement of vexreg
        opcode[1] = (((m_vexdata & 0x8) << 4) |          // W
                     ((15 - (m_vexreg & 0xF)) << 3) |    // vvvv
                     (m_vexdata & 0x7));                 // Lpp

        // Save to special_prefix and opcode
        m_special_prefix = 0xC4;    // VEX prefix
        m_opcode = X86Opcode(3, opcode); // two prefix bytes and 1 opcode byte
    }

    append_general(container,
                   common,
                   m_opcode,
                   m_x86_ea,
                   imm_val,
                   m_special_prefix,
                   m_rex,
                   m_postop,
                   m_default_rel);
}

void
X86Insn::do_append_general(BytecodeContainer& container,
                           const X86InsnInfo& info,
                           const unsigned int* size_lookup)
{
    BuildGeneral buildgen(info, m_mode_bits, size_lookup, m_force_strict,
                          m_default_rel);

    buildgen.apply_modifiers(m_mod_data);
    buildgen.update_rex();
    buildgen.apply_operands((X86Arch::ParserSelect)m_parser, m_operands);
    buildgen.apply_segregs(m_segregs);
    buildgen.finish(container, m_prefixes);
}

// Static parse data structure for instructions
struct InsnPrefixParseData
{
    const char* name;

    // If num_info > 0, instruction parse group
    // If num_info == 0, prefix
    const void* struc;

    // For instruction, number of elements in group.
    // 0 if prefix
    unsigned int num_info:8;

    // Instruction GAS suffix flags.
    unsigned int flags:8;

    // Instruction modifier data.
    unsigned int mod_data0:8;
    unsigned int mod_data1:8;
    unsigned int mod_data2:8;

    // Tests against BITS==64 and AVX
    unsigned int misc_flags:6;

    // CPU flags
    unsigned int cpu0:6;
    unsigned int cpu1:6;
    unsigned int cpu2:6;
};

// Pull in all parse data
#include "x86insn_nasm.cpp"
#include "x86insn_gas.cpp"

static std::string
cpu_find_reverse(unsigned int cpu0, unsigned int cpu1, unsigned int cpu2)
{
    std::ostringstream cpuname;
    std::bitset<64> cpu;

    cpu.set(cpu0);
    cpu.set(cpu1);
    cpu.set(cpu2);

    if (cpu[CPU_Prot])
        cpuname << " Protected";
    if (cpu[CPU_Undoc])
        cpuname << " Undocumented";
    if (cpu[CPU_Obs])
        cpuname << " Obsolete";
    if (cpu[CPU_Priv])
        cpuname << " Privileged";

    if (cpu[CPU_FPU])
        cpuname << " FPU";
    if (cpu[CPU_MMX])
        cpuname << " MMX";
    if (cpu[CPU_SSE])
        cpuname << " SSE";
    if (cpu[CPU_SSE2])
        cpuname << " SSE2";
    if (cpu[CPU_SSE3])
        cpuname << " SSE3";
    if (cpu[CPU_3DNow])
        cpuname << " 3DNow";
    if (cpu[CPU_Cyrix])
        cpuname << " Cyrix";
    if (cpu[CPU_AMD])
        cpuname << " AMD";
    if (cpu[CPU_SMM])
        cpuname << " SMM";
    if (cpu[CPU_SVM])
        cpuname << " SVM";
    if (cpu[CPU_PadLock])
        cpuname << " PadLock";
    if (cpu[CPU_EM64T])
        cpuname << " EM64T";
    if (cpu[CPU_SSSE3])
        cpuname << " SSSE3";
    if (cpu[CPU_SSE41])
        cpuname << " SSE4.1";
    if (cpu[CPU_SSE42])
        cpuname << " SSE4.2";

    if (cpu[CPU_186])
        cpuname << " 186";
    if (cpu[CPU_286])
        cpuname << " 286";
    if (cpu[CPU_386])
        cpuname << " 386";
    if (cpu[CPU_486])
        cpuname << " 486";
    if (cpu[CPU_586])
        cpuname << " 586";
    if (cpu[CPU_686])
        cpuname << " 686";
    if (cpu[CPU_P3])
        cpuname << " P3";
    if (cpu[CPU_P4])
        cpuname << " P4";
    if (cpu[CPU_IA64])
        cpuname << " IA64";
    if (cpu[CPU_K6])
        cpuname << " K6";
    if (cpu[CPU_Athlon])
        cpuname << " Athlon";
    if (cpu[CPU_Hammer])
        cpuname << " Hammer";

    return cpuname.str();
}

Arch::InsnPrefix
X86Arch::parse_check_insnprefix(const char* id, size_t id_len,
                                unsigned long line) const
{
    if (id_len > 15)
        return InsnPrefix();

    static char lcaseid[16];
    for (size_t i=0; i<id_len; i++)
        lcaseid[i] = tolower(id[i]);
    lcaseid[id_len] = '\0';

    /*@null@*/ const InsnPrefixParseData* pdata;
    switch (m_parser)
    {
        case PARSER_NASM:
            pdata = InsnPrefixNasmHash::in_word_set(lcaseid, id_len);
            break;
        case PARSER_GAS:
            pdata = InsnPrefixGasHash::in_word_set(lcaseid, id_len);
            break;
        default:
            pdata = 0;
    }
    if (!pdata)
        return InsnPrefix();

    if (pdata->num_info > 0)
    {
        if (m_mode_bits != 64 && (pdata->misc_flags & ONLY_64))
        {
            warn_set(WARN_GENERAL, String::compose(
                N_("`%s' is an instruction in 64-bit mode"), id));
            return InsnPrefix();
        }
        if (m_mode_bits == 64 && (pdata->misc_flags & NOT_64))
        {
            throw Error(String::compose(N_("`%s' invalid in 64-bit mode"),
                                        id));
        }

        if (!m_active_cpu[pdata->cpu0] ||
            !m_active_cpu[pdata->cpu1] ||
            !m_active_cpu[pdata->cpu2])
        {
            warn_set(WARN_GENERAL, String::compose(
                N_("`%s' is an instruction in CPU%s"), id,
                cpu_find_reverse(pdata->cpu0, pdata->cpu1, pdata->cpu2)));
            return InsnPrefix();
        }

        return std::auto_ptr<Insn>(new X86Insn(
            *this,
            static_cast<const X86InsnInfo*>(pdata->struc),
            m_active_cpu,
            pdata->mod_data0,
            pdata->mod_data1,
            pdata->mod_data2,
            pdata->num_info,
            m_mode_bits,
            pdata->flags,
            pdata->misc_flags,
            m_parser,
            m_force_strict,
            m_default_rel));
    }
    else
    {
        const X86Prefix* prefix = static_cast<const X86Prefix*>(pdata->struc);

        if (m_mode_bits == 64)
        {
            X86Prefix::Type type = prefix->get_type();
            unsigned char value = prefix->get_value();

            if (type == X86Prefix::OPERSIZE && value == 32)
            {
                throw Error(
                    N_("Cannot override data size to 32 bits in 64-bit mode"));
            }

            if (type == X86Prefix::ADDRSIZE && value == 16)
            {
                throw Error(
                    N_("Cannot override address size to 16 bits in 64-bit mode"));
            }
        }

        if (m_mode_bits != 64 && (pdata->misc_flags & ONLY_64))
        {
            warn_set(WARN_GENERAL, String::compose(
                N_("`%s' is a prefix in 64-bit mode"), id));
            return InsnPrefix();
        }

        return prefix;
    }
}

inline
X86Insn::X86Insn(const X86Arch& arch,
                 const X86InsnInfo* group,
                 const X86Arch::CpuMask& active_cpu,
                 unsigned char mod_data0,
                 unsigned char mod_data1,
                 unsigned char mod_data2,
                 unsigned int num_info,
                 unsigned int mode_bits,
                 unsigned int suffix,
                 unsigned int misc_flags,
                 X86Arch::ParserSelect parser,
                 bool force_strict,
                 bool default_rel)
    : m_arch(arch),
      m_group(group),
      m_active_cpu(active_cpu),
      m_num_info(num_info),
      m_mode_bits(mode_bits),
      m_suffix(suffix),
      m_misc_flags(misc_flags),
      m_parser(parser),
      m_force_strict(force_strict),
      m_default_rel(default_rel)
{
    m_mod_data[0] = mod_data0;
    m_mod_data[1] = mod_data1;
    m_mod_data[2] = mod_data2;
}

X86Insn::~X86Insn()
{
}

void
X86Insn::put(marg_ostream& os) const
{
    Insn::put(os);
    // TODO
}

X86Insn*
X86Insn::clone() const
{
    return new X86Insn(*this);
}

std::auto_ptr<Insn>
X86Arch::create_empty_insn() const
{
    return std::auto_ptr<Insn>(new X86Insn(
        *this,
        empty_insn,
        m_active_cpu,
        0,
        0,
        0,
        NELEMS(empty_insn),
        m_mode_bits,
        0,
        0,
        m_parser,
        m_force_strict,
        m_default_rel));
}

}}} // namespace yasm::arch::x86

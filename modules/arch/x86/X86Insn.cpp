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
#define DEBUG_TYPE "x86"

#include "X86Insn.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/raw_ostream.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Config/functional.h"
#include "yasmx/Support/phash.h"
#include "yasmx/Bytes.h"
#include "yasmx/EffAddr.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"

#include "X86Arch.h"
#include "X86Common.h"
#include "X86EffAddr.h"
#include "X86General.h"
#include "X86Jmp.h"
#include "X86JmpFar.h"
#include "X86Opcode.h"
#include "X86Prefix.h"


#ifndef NELEMS
#define NELEMS(array)   (sizeof(array) / sizeof(array[0]))
#endif

STATISTIC(num_groups_scanned, "Total number of instruction groups scanned");
STATISTIC(num_jmp_groups_scanned, "Total number of jump groups scanned");
STATISTIC(num_empty_insn, "Number of empty instructions created");

using namespace yasm;
using namespace yasm::arch;

static std::string CpuFindReverse(unsigned int cpu0, unsigned int cpu1,
                                  unsigned int cpu2);

namespace {
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
    SUF_Z = 1<<0,           // no suffix
    SUF_B = 1<<1,
    SUF_W = 1<<2,
    SUF_L = 1<<3,
    SUF_Q = 1<<4,
    SUF_S = 1<<5,
    SUF_MASK = SUF_Z|SUF_B|SUF_W|SUF_L|SUF_Q|SUF_S,

    // Flags only used in x86_insn_info
    GAS_ONLY = 1<<6,        // Only available in GAS mode
    GAS_ILLEGAL = 1<<7,     // Illegal in GAS mode
    GAS_NO_REV = 1<<8       // Don't reverse operands in GAS mode
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
    // DX memory operand only (EA) [special case for in/out opcodes]
    OPT_MemDX = 27,
    // XMM VSIB memory operand
    OPT_MemXMMIndex = 28,
    // YMM VSIB memory operand
    OPT_MemYMMIndex = 29
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
    OPA_VEX = 12,   // operand data goes into VEX/XOP "vvvv" field
    // operand data goes into BOTH VEX/XOP "vvvv" field and ea field
    OPA_EAVEX = 13,
    // operand data goes into BOTH VEX/XOP "vvvv" field and spare field
    OPA_SpareVEX = 14,
    // operand data goes into upper 4 bits of immediate byte (VEX/XOP is4 field)
    OPA_VEXImmSrc = 15,
    // operand data goes into bottom 4 bits of immediate byte
    // (currently only VEX imz2 field)
    OPA_VEXImm = 16
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

class IsRegType
{
public:
    IsRegType(X86Register::Type type) : m_type(type) {}
    bool operator() (const ExprTerm& term) const
    {
        if (const Register* reg = term.getRegister())
        {
            const X86Register* x86reg = static_cast<const X86Register*>(reg);
            if (x86reg->is(m_type))
                return true;
        }
        return false;
    }
private:
    X86Register::Type m_type;
};

template <typename Func>
bool
ContainsMatch(const Expr& e, const Func& match)
{
    const ExprTerms& terms = e.getTerms();
    int pos = terms.size() - 1;

    const ExprTerm& parent = terms[pos];
    if (!parent.isOp())
    {
        if (match(parent))
            return true;
        return false;
    }
    for (int n=pos-1; n>=0; --n)
    {
        const ExprTerm& child = terms[n];
        if (child.isEmpty())
            continue;
        if (child.m_depth <= parent.m_depth)
            break;  // Stop when we're out of children
        if (match(child))
            return true;
    }
    return false;
}
} // anonymous namespace

namespace yasm { namespace arch {
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
    unsigned int gas_flags:9;      // Enabled for these GAS suffixes

    // Tests against BITS==64, AVX, and XOP
    unsigned int misc_flags:5;

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
    // 0x80 - 0x8F indicate a XOP prefix, with the four LSBs holding "WLpp":
    //  same meanings as VEX prefix.
    unsigned char special_prefix;

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
}} // namespace yasm::arch

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
X86Prefix::Put(raw_ostream& os) const
{
    // TODO
    os << "PREFIX";
}

#ifdef WITH_XML
pugi::xml_node
X86Prefix::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("X86Prefix");
    pugi::xml_attribute type = root.append_attribute("type");
    switch (m_type)
    {
        case LOCKREP:   type = "LOCKREP"; break;
        case ADDRSIZE:  type = "ADDRSIZE"; break;
        case OPERSIZE:  type = "OPERSIZE"; break;
        case SEGREG:    type = "SEGREG"; break;
        case REX:       type = "REX"; break;
        case ACQREL:    type = "ACQREL"; break;
    }
    append_data(root, Twine::utohexstr(m_value).str());
    return root;
}
#endif // WITH_XML

#include "X86Insns.cpp"

bool
X86Insn::DoAppendJmpFar(BytecodeContainer& container,
                        const X86InsnInfo& info,
                        SourceLocation source,
                        DiagnosticsEngine& diags)
{
    Operand& op = m_operands.front();
    std::auto_ptr<Expr> imm = op.ReleaseImm();
    assert(imm.get() != 0);

    unsigned char opersize = info.opersize;
    std::auto_ptr<Expr> segment(op.ReleaseSeg());

    const X86TargetModifier* tmod =
        static_cast<const X86TargetModifier*>(op.getTargetMod());
    if (segment.get() == 0 && tmod && tmod->is(X86TargetModifier::FAR))
    {
        // "FAR imm" target needs to become "seg imm:imm".
        segment.reset(new Expr(SEG(*imm)));
    }
    else if (m_operands.size() > 1)
    {
        // Two operand form (gas)
        Operand& op2 = m_operands[1];
        segment = imm;
        imm = op2.ReleaseImm();
        if (op2.getSize() == OPS_BITS)
            opersize = m_mode_bits;
    }
    else if (segment.get() == 0)
        assert(false && "didn't get FAR expression in jmpfar");

    X86Opcode opcode(info.opcode_len, info.opcode);

    // Apply modifiers
    for (unsigned int i=0; i<NELEMS(info.modifiers); i++)
    {
        switch (info.modifiers[i])
        {
            case MOD_Gap:
                break;
            case MOD_Op0Add:
                opcode.Add(0, m_mod_data[i]);
                break;
            case MOD_Op1Add:
                opcode.Add(1, m_mod_data[i]);
                break;
            case MOD_Op2Add:
                opcode.Add(2, m_mod_data[i]);
                break;
            case MOD_Op1AddSp:
                opcode.Add(1, m_mod_data[i]<<3);
                break;
            default:
                break;
        }
    }

    X86Common common;
    common.m_opersize = opersize;
    common.m_mode_bits = m_mode_bits;
    common.ApplyPrefixes(info.def_opersize_64, m_prefixes, diags);
    common.Finish();
    AppendJmpFar(container, common, opcode, segment, imm, source);
    return true;
}

bool
X86Insn::MatchJmpInfo(const X86InsnInfo& info, unsigned int opersize,
                      X86Opcode& shortop, X86Opcode& nearop) const
{
    ++num_jmp_groups_scanned;

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
                    shortop.Add(0, m_mod_data[i]);
            }
            if (!nearop.isEmpty())
                return true;
            break;
        case OPTM_Near:
            nearop = X86Opcode(info.opcode_len, info.opcode);
            for (unsigned int i=0; i<NELEMS(info.modifiers); i++)
            {
                if (info.modifiers[i] == MOD_Op1Add)
                    nearop.Add(1, m_mod_data[i]);
            }
            if (!shortop.isEmpty())
                return true;
            break;
    }
    return false;
}

bool
X86Insn::DoAppendJmp(BytecodeContainer& container,
                     const X86InsnInfo& jinfo,
                     SourceLocation source,
                     DiagnosticsEngine& diags)
{
    static const unsigned char size_lookup[] =
        {0, 8, 16, 32, 64, 80, 128, 0, 0};  // 256 not needed

    // We know the target is in operand 0, but sanity check for Imm.
    Operand& op = m_operands.front();
    std::auto_ptr<Expr> imm = op.ReleaseImm();
    assert(imm.get() != 0);
    SourceLocation imm_source = op.getSource();

    X86JmpOpcodeSel op_sel;

    // See if the user explicitly specified short/near/far.
    switch (insn_operands[jinfo.operands_index+0].targetmod)
    {
        case OPTM_Short:
            op_sel = X86_JMP_SHORT;
            break;
        case OPTM_Near:
            op_sel = X86_JMP_NEAR;
            break;
        default:
            op_sel = X86_JMP_NONE;
    }

    // Scan through other infos for this insn looking for short/near versions.
    // Needs to match opersize and number of operands, also be within CPU.
    X86Opcode shortop, nearop;
    for (const X86InsnInfo* info = &m_group[0]; info != &m_group[m_num_info];
         ++info)
    {
        if (MatchJmpInfo(*info, jinfo.opersize, shortop, nearop))
            break;
    }

    if ((op_sel == X86_JMP_SHORT) && shortop.isEmpty())
    {
        diags.Report(source, diag::err_missing_jump_form) << "SHORT";
        return false;
    }
    if ((op_sel == X86_JMP_NEAR) && nearop.isEmpty())
    {
        diags.Report(source, diag::err_missing_jump_form) << "NEAR";
        return false;
    }

    if (op_sel == X86_JMP_NONE)
    {
        if (nearop.isEmpty())
            op_sel = X86_JMP_SHORT;
        if (shortop.isEmpty())
            op_sel = X86_JMP_NEAR;
    }

    X86Common common;
    common.m_opersize = jinfo.opersize;
    common.m_mode_bits = m_mode_bits;

    // Check for address size setting in second operand, if present
    if (jinfo.num_operands > 1 &&
        insn_operands[jinfo.operands_index+1].action == OPA_AdSizeR)
        common.m_addrsize =
            size_lookup[insn_operands[jinfo.operands_index+1].size];

    // Check for address size override
    for (unsigned int i=0; i<NELEMS(jinfo.modifiers); i++)
    {
        if (jinfo.modifiers[i] == MOD_AdSizeR)
            common.m_addrsize = m_mod_data[i];
    }

    common.ApplyPrefixes(jinfo.def_opersize_64, m_prefixes, diags);
    common.Finish();

    AppendJmp(container, common, shortop, nearop, imm, imm_source, source,
              op_sel);
    return true;
}

bool
X86Insn::MatchOperand(const Operand& op, const X86InfoOperand& info_op,
                      const Operand& op0, const unsigned int* size_lookup,
                      int bypass) const
{
    const X86Register* reg = static_cast<const X86Register*>(op.getReg());
    const X86SegmentRegister* segreg =
        static_cast<const X86SegmentRegister*>(op.getSegReg());
    EffAddr* ea = op.getMemory();

    // Check operand type
    switch (info_op.type)
    {
        case OPT_Imm:
            if (!op.isType(Operand::IMM))
                return false;
            break;
        case OPT_RM:
            if (op.isType(Operand::MEMORY))
                break;
            /*@fallthrough@*/
        case OPT_Reg:
            if (!reg)
                return false;
            switch (reg->getType())
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
            if (!op.isType(Operand::MEMORY))
                return false;
            break;
        case OPT_SIMDRM:
            if (op.isType(Operand::MEMORY))
                break;
            /*@fallthrough@*/
        case OPT_SIMDReg:
            if (!reg)
                return false;
            switch (reg->getType())
            {
                case X86Register::MMXREG:
                case X86Register::XMMREG:
                case X86Register::YMMREG:
                    break;
                default:
                    return false;
            }
            break;
        case OPT_SegReg:
            if (!op.isType(Operand::SEGREG))
                return false;
            break;
        case OPT_CRReg:
            if (!reg || reg->isNot(X86Register::CRREG))
                return false;
            break;
        case OPT_DRReg:
            if (!reg || reg->isNot(X86Register::DRREG))
                return false;
            break;
        case OPT_TRReg:
            if (!reg || reg->isNot(X86Register::TRREG))
                return false;
            break;
        case OPT_ST0:
            if (!reg || reg->isNot(X86Register::FPUREG) || reg->getNum() != 0)
                return false;
            break;
        case OPT_Areg:
            if (!reg || reg->getNum() != 0 ||
                (info_op.size == OPS_8 &&
                 reg->isNot(X86Register::REG8) &&
                 reg->isNot(X86Register::REG8X)) ||
                (info_op.size == OPS_16 && reg->isNot(X86Register::REG16)) ||
                (info_op.size == OPS_32 && reg->isNot(X86Register::REG32)) ||
                (info_op.size == OPS_64 && reg->isNot(X86Register::REG64)))
                return false;
            break;
        case OPT_Creg:
            if (!reg || reg->getNum() != 1 ||
                (info_op.size == OPS_8 &&
                 reg->isNot(X86Register::REG8) &&
                 reg->isNot(X86Register::REG8X)) ||
                (info_op.size == OPS_16 && reg->isNot(X86Register::REG16)) ||
                (info_op.size == OPS_32 && reg->isNot(X86Register::REG32)) ||
                (info_op.size == OPS_64 && reg->isNot(X86Register::REG64)))
                return false;
            break;
        case OPT_Dreg:
            if (!reg || reg->getNum() != 2 ||
                (info_op.size == OPS_8 &&
                 reg->isNot(X86Register::REG8) &&
                 reg->isNot(X86Register::REG8X)) ||
                (info_op.size == OPS_16 && reg->isNot(X86Register::REG16)) ||
                (info_op.size == OPS_32 && reg->isNot(X86Register::REG32)) ||
                (info_op.size == OPS_64 && reg->isNot(X86Register::REG64)))
                return false;
            break;
        case OPT_CS:
            if (!segreg || segreg->isNot(X86SegmentRegister::kCS))
                return false;
            break;
        case OPT_DS:
            if (!segreg || segreg->isNot(X86SegmentRegister::kDS))
                return false;
            break;
        case OPT_ES:
            if (!segreg || segreg->isNot(X86SegmentRegister::kES))
                return false;
            break;
        case OPT_FS:
            if (!segreg || segreg->isNot(X86SegmentRegister::kFS))
                return false;
            break;
        case OPT_GS:
            if (!segreg || segreg->isNot(X86SegmentRegister::kGS))
                return false;
            break;
        case OPT_SS:
            if (!segreg || segreg->isNot(X86SegmentRegister::kSS))
                return false;
            break;
        case OPT_CR4:
            if (!reg || reg->isNot(X86Register::CRREG) || reg->getNum() != 4)
                return false;
            break;
        case OPT_MemOffs:
            if (!ea ||
                ea->m_disp.getAbs()->Contains(ExprTerm::REG) ||
                ea->m_pc_rel ||
                (!ea->m_not_pc_rel && m_default_rel &&
                 ea->m_disp.getSize() != 64))
                return false;
            break;
        case OPT_Imm1:
        {
            if (Expr* imm = op.getImm())
            {
                if (!imm->isIntNum() || !imm->getIntNum().isPos1())
                    return false;
            }
            else
                return false;
            break;
        }
        case OPT_ImmNotSegOff:
        {
            Expr* imm = op.getImm();
            if (!imm || op.getTargetMod() != 0 || op.getSeg() != 0)
                return false;
            break;
        }
        case OPT_XMM0:
            if (!reg || reg->isNot(X86Register::XMMREG) || reg->getNum() != 0)
                return false;
            break;
        case OPT_MemrAX:
        {
            if (!ea || !ea->m_disp.getAbs()->isRegister())
                return false;
            const X86Register* reg2 = static_cast<const X86Register*>
                (ea->m_disp.getAbs()->getRegister());
            if (reg2->getNum() != 0 ||
                (reg2->isNot(X86Register::REG16) &&
                 reg2->isNot(X86Register::REG32) &&
                 reg2->isNot(X86Register::REG64)))
                return false;
            break;
        }
        case OPT_MemEAX:
        {
            if (!ea || !ea->m_disp.getAbs()->isRegister())
                return false;
            const X86Register* reg2 = static_cast<const X86Register*>
                (ea->m_disp.getAbs()->getRegister());
            if (reg2->isNot(X86Register::REG32) || reg2->getNum() != 0)
                return false;
            break;
        }
        case OPT_MemDX:
        {
            if (!ea || !ea->m_disp.getAbs()->isRegister())
                return false;
            const X86Register* reg2 = static_cast<const X86Register*>
                (ea->m_disp.getAbs()->getRegister());
            if (reg2->isNot(X86Register::REG16) || reg2->getNum() != 2)
                return false;
            break;
        }
        case OPT_MemXMMIndex:
        {
            if (!ea || !ContainsMatch(*ea->m_disp.getAbs(),
                                      IsRegType(X86Register::XMMREG)))
                return false;
            break;
        }
        case OPT_MemYMMIndex:
        {
            if (!ea || !ContainsMatch(*ea->m_disp.getAbs(),
                                      IsRegType(X86Register::YMMREG)))
                return false;
            break;
        }
        default:
            assert(false && "invalid operand type");
    }

    // Check operand size
    unsigned int size = size_lookup[info_op.size];
    if (m_parser == X86Arch::PARSER_GAS)
    {
        // Require relaxed operands for GAS mode (don't allow
        // per-operand sizing).
        if (reg && op.getSize() == 0)
        {
            // Register size must exactly match
            if (reg->getSize() != size)
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
        if (reg && op.getSize() == 0)
        {
            // Register size must exactly match
            if ((bypass == 4 && (&op-&op0) == 0)
                || (bypass == 5 && (&op-&op0) == 1)
                || (bypass == 6 && (&op-&op0) == 2))
                ;
            else if (reg->getSize() != size)
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
                if (size != 0 && op.getSize() != size && op.getSize() != 0)
                    return false;
            }
            else
            {
                // Strict checking
                if (op.getSize() != size)
                    return false;
            }
        }
    }

    // Check for 64-bit effective address size in NASM mode
    if (m_parser != X86Arch::PARSER_GAS && ea)
    {
        if (info_op.eas64)
        {
            if (ea->m_disp.getSize() != 64)
                return false;
        }
        else if (ea->m_disp.getSize() == 64)
            return false;
    }

    // Check target modifier
    const X86TargetModifier* targetmod =
        static_cast<const X86TargetModifier*>(op.getTargetMod());
    switch (info_op.targetmod)
    {
        case OPTM_None:
            if (targetmod != 0)
                return false;
            break;
        case OPTM_Near:
            if (targetmod == 0 || targetmod->isNot(X86TargetModifier::NEAR))
                return false;
            break;
        case OPTM_Short:
            if (targetmod == 0 || targetmod->isNot(X86TargetModifier::SHORT))
                return false;
            break;
        case OPTM_Far:
            if (targetmod == 0 || targetmod->isNot(X86TargetModifier::FAR))
                return false;
            break;
        case OPTM_To:
            if (targetmod == 0 || targetmod->isNot(X86TargetModifier::TO))
                return false;
            break;
        default:
            assert(false && "invalid target modifier type");
            return false;
    }

    return true;
}

bool
X86Insn::MatchInfo(const X86InsnInfo& info, const unsigned int* size_lookup,
                   int bypass) const
{
    ++num_groups_scanned;

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
    if (m_parser == X86Arch::PARSER_GAS
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
                          TR1::bind(&X86Insn::MatchOperand, this, _1, _2,
                                    TR1::ref(m_operands.back()),
                                    size_lookup, bypass));
    else
        return std::equal(m_operands.begin(), m_operands.end(),
                          &insn_operands[info.operands_index],
                          TR1::bind(&X86Insn::MatchOperand, this, _1, _2,
                                    TR1::ref(m_operands.front()),
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
            if (!MatchOperand(*first1, *first2, first, size_lookup, bypass))
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
            if (!MatchOperand(*first1, *first2, first, size_lookup, bypass))
                return false;
            ++first1;
            ++first2;
        }
        return true;
    }
#endif
}

const X86InsnInfo*
X86Insn::FindMatch(const unsigned int* size_lookup, int bypass) const
{
    // Just do a simple linear search through the info array for a match.
    // First match wins.
    const X86InsnInfo* info =
        std::find_if(&m_group[0], &m_group[m_num_info],
                     TR1::bind(&X86Insn::MatchInfo, this, _1, size_lookup,
                               bypass));
    if (info == &m_group[m_num_info])
        return 0;
    return info;
}

void
X86Insn::MatchError(const unsigned int* size_lookup,
                    SourceLocation source,
                    DiagnosticsEngine& diags) const
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
    {
        diags.Report(source, diag::err_bad_num_operands);
        return;
    }

    int bypass;
    for (bypass=1; bypass<9; bypass++)
    {
        i = FindMatch(size_lookup, bypass);
        if (i)
            break;
    }

    switch (bypass)
    {
        case 1:
        case 4:
            assert(m_operands.size() >= 1 && "not enough operands for error");
            diags.Report(m_operands[0].getSource(), diag::err_bad_operand_size);
            break;
        case 2:
        case 5:
            assert(m_operands.size() >= 2 && "not enough operands for error");
            diags.Report(m_operands[1].getSource(), diag::err_bad_operand_size);
            break;
        case 3:
        case 6:
            assert(m_operands.size() >= 3 && "not enough operands for error");
            diags.Report(m_operands[2].getSource(), diag::err_bad_operand_size);
            break;
        case 7:
            assert(m_operands.size() >= 4 && "not enough operands for error");
            diags.Report(m_operands[0].getSource(),
                         diag::err_dest_not_src1_or_src3)
                << m_operands[1].getSource()
                << m_operands[3].getSource();
            break;
        case 8:
        {
            unsigned int cpu0 = i->cpu0, cpu1 = i->cpu1, cpu2 = i->cpu2;
            diags.Report(source, diag::err_requires_cpu)
                << CpuFindReverse(cpu0, cpu1, cpu2);
            break;
        }
        default:
            diags.Report(source, diag::err_bad_insn_operands);
            break;
    }
}

bool
X86Insn::DoAppend(BytecodeContainer& container,
                  SourceLocation source,
                  DiagnosticsEngine& diags)
{
    unsigned int size_lookup[] = {0, 8, 16, 32, 64, 80, 128, 256, 0};
    size_lookup[OPS_BITS] = m_mode_bits;

    if (m_operands.size() > 5)
    {
        diags.Report(m_operands[5].getSource(), diag::err_too_many_operands)
            << 5;
        return false;
    }

    // If we're running in GAS mode, look at the first insn_info to see
    // if this is a relative jump (OPA_JmpRel).  If so, run through the
    // operands and adjust for dereferences / lack thereof.
    if (m_parser == X86Arch::PARSER_GAS
        && insn_operands[m_group->operands_index+0].action == OPA_JmpRel)
    {
        for (Operands::iterator op = m_operands.begin(),
             end = m_operands.end(); op != end; ++op)
        {
            const X86Register* reg =
                static_cast<const X86Register*>(op->getReg());
            EffAddr* ea = op->getMemory();

            if (!op->isDeref() && (reg || (ea && ea->m_strong)))
                diags.Report(op->getSource(), diag::warn_indirect_call_no_deref);
            if (!op->isDeref() && ea && !ea->m_strong)
            {
                // Memory that is not dereferenced, and not strong, is
                // actually an immediate for the purposes of relative jumps.
                if (ea->m_segreg != 0)
                    diags.Report(source, diag::warn_prefixes_skipped);
                SourceLocation source = op->getSource();
                *op = Operand(std::auto_ptr<Expr>(
                    ea->m_disp.getAbs()->clone()));
                op->setSource(source);
                delete ea;
            }
        }
    }

    const X86InsnInfo* info = FindMatch(size_lookup, 0);

    if (!info)
    {
        // Didn't find a match
        MatchError(size_lookup, source, diags);
        return false;
    }

    if (m_operands.size() > 0)
    {
        switch (insn_operands[info->operands_index+0].action)
        {
            case OPA_JmpRel:
                // Shortcut to JmpRel
                return DoAppendJmp(container, *info, source, diags);
            case OPA_JmpFar:
                // Shortcut to JmpFar
                return DoAppendJmpFar(container, *info, source, diags);
        }
    }

    return DoAppendGeneral(container, *info, size_lookup, source, diags);
}

namespace {
class BuildGeneral
{
public:
    BuildGeneral(const X86InsnInfo& info,
                 unsigned int mode_bits,
                 const unsigned int* size_lookup,
                 bool force_strict,
                 bool default_rel,
                 DiagnosticsEngine& diags);
    ~BuildGeneral();

    void ApplyModifiers(unsigned char* mod_data);
    void UpdateRex();
    void ApplyOperands(X86Arch::ParserSelect parser,
                       Insn::Operands& operands);
    void CheckSegReg(const X86SegmentRegister* segreg, SourceLocation source);
    void ApplySegReg(const SegmentRegister* segreg, SourceLocation source);
    bool Finish(BytecodeContainer& container,
                const Insn::Prefixes& prefixes,
                SourceLocation source);

private:
    void ApplyOperand(const X86InfoOperand& info_op, Operand& op);

    const X86InsnInfo& m_info;
    unsigned int m_mode_bits;
    const unsigned int* m_size_lookup;
    bool m_force_strict;
    bool m_default_rel;
    DiagnosticsEngine& m_diags;

    X86Opcode m_opcode;
    std::auto_ptr<X86EffAddr> m_x86_ea;
    std::auto_ptr<Expr> m_imm;
    unsigned int m_def_opersize_64;
    unsigned char m_special_prefix;
    unsigned char m_spare;
    unsigned char m_im_len;
    unsigned char m_im_sign;
    SourceLocation m_im_source;
    X86GeneralPostOp m_postop;
    unsigned char m_rex;
    unsigned char m_vexdata;
    unsigned char m_vexreg;
    unsigned char m_opersize;
    unsigned char m_addrsize;
};
} // anonymous namespace

inline
BuildGeneral::BuildGeneral(const X86InsnInfo& info,
                           unsigned int mode_bits,
                           const unsigned int* size_lookup,
                           bool force_strict,
                           bool default_rel,
                           DiagnosticsEngine& diags)
    : m_info(info),
      m_mode_bits(mode_bits),
      m_size_lookup(size_lookup),
      m_force_strict(force_strict),
      m_default_rel(default_rel),
      m_diags(diags),
      m_opcode(info.opcode_len, info.opcode),
      m_x86_ea(0),
      m_imm(0),
      m_def_opersize_64(info.def_opersize_64),
      m_special_prefix(info.special_prefix),
      m_spare(info.spare),
      m_im_len(0),
      m_im_sign(0),
      m_postop(X86_POSTOP_NONE),
      m_rex(0),
      m_vexdata(0),
      m_vexreg(0),
      m_opersize(info.opersize),
      m_addrsize(0)
{
    // Move VEX/XOP data (stored in special prefix) to separate location to
    // allow overriding of special prefix by modifiers.
    if ((m_special_prefix & 0xF0) == 0xC0 || (m_special_prefix & 0xF0) == 0x80)
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
BuildGeneral::ApplyModifiers(unsigned char* mod_data)
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
                m_opcode.Add(0, mod_data[i]);
                break;
            case MOD_Op1Add:
                m_opcode.Add(1, mod_data[i]);
                break;
            case MOD_Op2Add:
                m_opcode.Add(2, mod_data[i]);
                break;
            case MOD_SpAdd:
                m_spare += mod_data[i];
                break;
            case MOD_OpSizeR:
                m_opersize = mod_data[i];
                break;
            case MOD_Imm8:
                m_imm.reset(new Expr(IntNum(mod_data[i])));
                m_im_len = 8;
                break;
            case MOD_DOpS64R:
                m_def_opersize_64 = mod_data[i];
                break;
            case MOD_Op1AddSp:
                m_opcode.Add(1, mod_data[i]<<3);
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
BuildGeneral::UpdateRex()
{
    // In 64-bit mode, if opersize is 64 and default is not 64,
    // force REX byte.
    if (m_mode_bits == 64 && m_opersize == 64 && m_def_opersize_64 != 64)
        m_rex = 0x48;
}

void
BuildGeneral::ApplyOperands(X86Arch::ParserSelect parser,
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
                ApplyOperand(*info_ops++, *first++);
        }
        else
        {
            Insn::Operands::iterator
                first = operands.begin(), last = operands.end();
            while (first != last)
                ApplyOperand(*info_ops++, *first++);
        }
    }
}

void
BuildGeneral::ApplyOperand(const X86InfoOperand& info_op, Operand& op)
{
    switch (info_op.action)
    {
        case OPA_None:
            // Throw away the operand contents
            break;
        case OPA_EA:
            switch (op.getType())
            {
                case Operand::NONE:
                    assert(false && "invalid operand conversion");
                    break;
                case Operand::REG:
                    if (m_x86_ea.get() == 0)
                        m_x86_ea.reset(new X86EffAddr());
                    if (!m_x86_ea->setReg(static_cast<const X86Register*>(op.getReg()),
                                          &m_rex, m_mode_bits))
                    {
                        m_diags.Report(op.getSource(),
                                       diag::err_high8_rex_conflict);
                        return;
                    }
                    m_x86_ea->m_disp.setSource(op.getSource());
                    break;
                case Operand::SEGREG:
                    assert(false && "invalid operand conversion");
                    break;
                case Operand::MEMORY:
                {
                    if (op.getSeg() != 0)
                    {
                        m_diags.Report(op.getSource(),
                                       diag::err_invalid_ea_segment);
                        return;
                    }
                    m_x86_ea.reset(static_cast<X86EffAddr*>
                                   (op.ReleaseMemory().release()));
                    m_x86_ea->m_disp.setSource(op.getSource());
                    const X86SegmentRegister* segreg =
                        static_cast<const X86SegmentRegister*>
                        (m_x86_ea->m_segreg);
                    if (info_op.type == OPT_MemOffs)
                        // Special-case for MOV MemOffs instruction
                        m_x86_ea->setDispOnly();
                    else if (info_op.type == OPT_MemXMMIndex)
                    {
                        // Remember VSIB mode
                        m_x86_ea->m_vsib_mode = 1;
                        m_x86_ea->m_need_sib = 1;
                    }
                    else if (info_op.type == OPT_MemYMMIndex)
                    {
                        // Remember VSIB mode
                        m_x86_ea->m_vsib_mode = 2;
                        m_x86_ea->m_need_sib = 1;
                    }
                    else if (m_default_rel &&
                             !m_x86_ea->m_not_pc_rel &&
                             (!segreg ||
                              (segreg->isNot(X86SegmentRegister::kFS) &&
                               segreg->isNot(X86SegmentRegister::kGS))) &&
                             !m_x86_ea->m_disp.getAbs()->Contains(ExprTerm::REG))
                        // Enable default PC-rel if no regs and segreg
                        // is not FS or GS.
                        m_x86_ea->m_pc_rel = true;
                    // Warn on 64-bit cs/es/ds/ss segment overrides
                    CheckSegReg(segreg, op.getSource());
                    break;
                }
                case Operand::IMM:
                    if (m_x86_ea.get() == 0)
                        m_x86_ea.reset(new X86EffAddr());
                    m_x86_ea->setImm(op.ReleaseImm(),
                                     m_size_lookup[info_op.size]);
                    m_x86_ea->m_disp.setSource(op.getSource());
                    break;
            }
            break;
        case OPA_EAVEX:
        {
            const X86Register* reg =
                static_cast<const X86Register*>(op.getReg());
            assert(reg != 0 && "invalid operand conversion");
            if (m_x86_ea.get() == 0)
                m_x86_ea.reset(new X86EffAddr());
            if (!m_x86_ea->setReg(reg, &m_rex, m_mode_bits))
            {
                m_diags.Report(op.getSource(), diag::err_high8_rex_conflict);
                return;
            }
            m_vexreg = reg->getNum() & 0xF;
            break;
        }
        case OPA_Imm:
            if (op.getSeg() != 0)
            {
                m_diags.Report(op.getSource(), diag::err_imm_segment_override);
                return;
            }
            m_imm = op.ReleaseImm();
            assert(m_imm.get() != 0 && "invalid operand conversion");

            m_im_len = m_size_lookup[info_op.size];
            m_im_source = op.getSource();
            break;
        case OPA_SImm:
            if (op.getSeg() != 0)
            {
                m_diags.Report(op.getSource(), diag::err_imm_segment_override);
                return;
            }
            m_imm = op.ReleaseImm();
            assert(m_imm.get() != 0 && "invalid operand conversion");

            m_im_len = m_size_lookup[info_op.size];
            m_im_source = op.getSource();
            m_im_sign = 1;
            break;
        case OPA_Spare:
            if (const SegmentRegister* segreg = op.getSegReg())
            {
                m_spare =
                    (static_cast<const X86SegmentRegister*>(segreg))->getNum();
            }
            else if (const Register* reg = op.getReg())
            {
                if (!setRexFromReg(&m_rex, &m_spare,
                                   static_cast<const X86Register*>(reg),
                                   m_mode_bits, X86_REX_R))
                {
                    m_diags.Report(op.getSource(),
                                   diag::err_high8_rex_conflict);
                    return;
                }
            }
            else
                assert(false && "invalid operand conversion");
            break;
        case OPA_SpareVEX:
        {
            const X86Register* reg =
                static_cast<const X86Register*>(op.getReg());
            assert(reg != 0 && "invalid operand conversion");
            if (!setRexFromReg(&m_rex, &m_spare, reg, m_mode_bits, X86_REX_R))
            {
                m_diags.Report(op.getSource(), diag::err_high8_rex_conflict);
                return;
            }
            m_vexreg = reg->getNum() & 0xF;
            break;
        }
        case OPA_Op0Add:
        {
            const Register* reg = op.getReg();
            assert(reg != 0 && "invalid operand conversion");
            unsigned char opadd;
            if (!setRexFromReg(&m_rex, &opadd,
                               static_cast<const X86Register*>(reg),
                               m_mode_bits, X86_REX_B))
            {
                m_diags.Report(op.getSource(), diag::err_high8_rex_conflict);
                return;
            }
            m_opcode.Add(0, opadd);
            break;
        }
        case OPA_Op1Add:
        {
            const Register* reg = op.getReg();
            assert(reg != 0 && "invalid operand conversion");
            unsigned char opadd;
            if (!setRexFromReg(&m_rex, &opadd,
                               static_cast<const X86Register*>(reg),
                               m_mode_bits, X86_REX_B))
            {
                m_diags.Report(op.getSource(), diag::err_high8_rex_conflict);
                return;
            }
            m_opcode.Add(1, opadd);
            break;
        }
        case OPA_SpareEA:
        {
            const Register* reg = op.getReg();
            assert(reg != 0 && "invalid operand conversion");
            const X86Register* x86_reg = static_cast<const X86Register*>(reg);
            if (m_x86_ea.get() == 0)
                m_x86_ea.reset(new X86EffAddr());
            if (!m_x86_ea->setReg(x86_reg, &m_rex, m_mode_bits) ||
                !setRexFromReg(&m_rex, &m_spare, x86_reg, m_mode_bits,
                               X86_REX_R))
            {
                m_diags.Report(op.getSource(), diag::err_high8_rex_conflict);
                return;
            }
            break;
        }
        case OPA_AdSizeEA:
        {
            // Only implement this for OPT_MemrAX and OPT_MemEAX
            // for now.
            EffAddr* ea = op.getMemory();
            assert(ea && ea->m_disp.getAbs()->isRegister() &&
                   "invalid operand conversion");
            const X86Register* reg = static_cast<const X86Register*>
                (ea->m_disp.getAbs()->getRegister());
            unsigned int regnum = reg->getNum();
            // 64-bit mode does not allow 16-bit addresses
            if (m_mode_bits == 64 && reg->is(X86Register::REG16) && regnum == 0)
            {
                m_diags.Report(op.getSource(), diag::err_16addr_64mode);
                return;
            }
            else if (reg->is(X86Register::REG16) && regnum == 0)
                m_addrsize = 16;
            else if (reg->is(X86Register::REG32) && regnum == 0)
                m_addrsize = 32;
            else if (m_mode_bits == 64 && reg->is(X86Register::REG64) &&
                     regnum == 0)
                m_addrsize = 64;
            else
            {
                m_diags.Report(op.getSource(), diag::err_bad_address_size);
                return;
            }
            break;
        }
        case OPA_VEX:
        {
            const Register* reg = op.getReg();
            assert(reg != 0 && "invalid operand conversion");
            m_vexreg = static_cast<const X86Register*>(reg)->getNum() & 0xF;
            break;
        }
        case OPA_VEXImmSrc:
        {
            const X86Register* reg =
                static_cast<const X86Register*>(op.getReg());
            assert(reg != 0 && "invalid operand conversion");
            if (m_imm.get() == 0)
                m_imm.reset(new Expr((reg->getNum() << 4) & 0xF0));
            else
            {
                *m_imm &= IntNum(0x0F);
                *m_imm |= IntNum((reg->getNum() << 4) & 0xF0);
            }
            m_im_len = 8;
            m_im_source = op.getSource();
            break;
        }
        case OPA_VEXImm:
        {
            assert(op.isType(Operand::IMM) && "invalid operand conversion");
            if (m_imm.get() == 0)
                m_imm = op.ReleaseImm();
            else
            {
                *m_imm &= IntNum(0xF0);
                *op.getImm() &= IntNum(0x0F);
                *m_imm |= *op.getImm();
                op.ReleaseImm();
            }
            m_im_len = 8;
            m_im_source = op.getSource();
            break;
        }
        default:
            assert(false && "unknown operand action");
    }

    if (info_op.size == OPS_BITS)
        m_opersize = static_cast<unsigned char>(m_mode_bits);

    switch (info_op.post_action)
    {
        case OPAP_None:
            break;
        case OPAP_SImm8:
            // Check operand strictness; if strict and non-8-bit,
            // pre-emptively expand to full size.
            // For unspecified size case, still optimize.
            if (!(m_force_strict || op.isStrict()) || op.getSize() == 0)
                m_postop = X86_POSTOP_SIGNEXT_IMM8;
            else if (op.getSize() != 8)
                m_opcode.MakeAlt1();
            break;
        case OPAP_ShortMov:
            m_postop = X86_POSTOP_SHORT_MOV;
            break;
        case OPAP_A16:
            m_postop = X86_POSTOP_ADDRESS16;
            break;
        case OPAP_SImm32Avail:
            m_postop = X86_POSTOP_SIMM32_AVAIL;
            break;
        default:
            assert(false && "unknown operand postponed action");
    }
}

void
BuildGeneral::CheckSegReg(const X86SegmentRegister* segreg,
                          SourceLocation source)
{
    if (!segreg || m_mode_bits != 64)
        return;

    const char* segname = NULL;
    if (segreg->is(X86SegmentRegister::kCS))
        segname = "cs";
    else if (segreg->is(X86SegmentRegister::kDS))
        segname = "ds";
    else if (segreg->is(X86SegmentRegister::kES))
        segname = "es";
    else if (segreg->is(X86SegmentRegister::kSS))
        segname = "ss";
    if (!segname)
        return;
    m_diags.Report(source, diag::warn_seg_ignored_in_xxmode) << segname << 64;
}

void
BuildGeneral::ApplySegReg(const SegmentRegister* segreg, SourceLocation source)
{
    if (X86EffAddr* x86_ea = m_x86_ea.get())
    {
        x86_ea->Init(m_spare);
        if (segreg == 0)
            return;
        if (x86_ea->m_segreg != 0)
            m_diags.Report(source, diag::warn_multiple_seg_override);
        x86_ea->m_segreg = segreg;
    }
    else if (segreg != 0 && m_special_prefix == 0)
    {
        m_special_prefix =
            static_cast<const X86SegmentRegister*>(segreg)->getPrefix();
    }
    else if (segreg != 0)
        assert(false && "unhandled segment prefix");
    CheckSegReg(static_cast<const X86SegmentRegister*>(segreg), source);
}

bool
BuildGeneral::Finish(BytecodeContainer& container,
                     const Insn::Prefixes& prefixes,
                     SourceLocation source)
{
    std::auto_ptr<Value> imm_val(0);

    if (m_imm.get() != 0)
    {
        imm_val.reset(new Value(m_im_len, m_imm));
        imm_val->setSigned(m_im_sign);
        imm_val->setSource(m_im_source);
    }

    X86Common common;
    common.m_addrsize = m_addrsize;
    common.m_opersize = m_opersize;
    common.m_mode_bits = m_mode_bits;
    common.ApplyPrefixes(m_def_opersize_64, prefixes, m_diags, &m_rex);
    common.Finish();

    // Convert to VEX/XOP prefixes if requested.
    // To save space in the insn structure, the VEX/XOP prefix is written into
    // special_prefix and the first 2 bytes of the instruction are set to
    // the second two VEX/XOP bytes.  During calc_len() it may be shortened to
    // one VEX byte (this can only be done after knowledge of REX value); this
    // further optimization is not possible for XOP.
    if (m_vexdata)
    {
        bool xop = ((m_vexdata & 0xF0) == 0x80);
        unsigned char opcode[3];    // VEX opcode; 0=VEX1, 1=VEX2, 2=Opcode
        opcode[0] = 0xE0;           // R=X=B=1, mmmmm=0

        if (xop)
        {
            // Look at the first byte of the opcode for the XOP mmmmm field.
            // Leave R=X=B=1 for now.
            unsigned char op0 = m_opcode.get(0);
            assert((op0 == 0x08 || op0 == 0x09 || op0 == 0x0A) &&
                   "first opcode byte of XOP must be 0x08, 0x09, or 0x0A");
            // Real opcode is in byte 1.
            opcode[2] = m_opcode.get(1);
            opcode[0] |= op0;
        }
        else
        {
            // Look at the first bytes of the opcode to see what leading bytes
            // to encode in the VEX mmmmm field.  Leave R=X=B=1 for now.
            assert(m_opcode.get(0) == 0x0F &&
                   "first opcode byte of VEX must be 0x0F");

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
                    assert(false && "unrecognized special prefix");
            }
        }

        // 2nd VEX byte is WvvvvLpp.
        // W, L, pp come from vexdata
        // vvvv comes from 1s complement of vexreg
        opcode[1] = (((m_vexdata & 0x8) << 4) |          // W
                     ((15 - (m_vexreg & 0xF)) << 3) |    // vvvv
                     (m_vexdata & 0x7));                 // Lpp

        // Save to special_prefix and opcode
        m_special_prefix = xop ? 0x8F : 0xC4;   // VEX/XOP prefix
        m_opcode = X86Opcode(3, opcode); // two prefix bytes and 1 opcode byte
    }

    AppendGeneral(container,
                  common,
                  m_opcode,
                  m_x86_ea,
                  imm_val,
                  m_special_prefix,
                  m_rex,
                  m_postop,
                  m_default_rel,
                  source);
    return true;
}

bool
X86Insn::DoAppendGeneral(BytecodeContainer& container,
                         const X86InsnInfo& info,
                         const unsigned int* size_lookup,
                         SourceLocation source,
                         DiagnosticsEngine& diags)
{
    BuildGeneral buildgen(info, m_mode_bits, size_lookup, m_force_strict,
                          m_default_rel, diags);

    buildgen.ApplyModifiers(m_mod_data);
    buildgen.UpdateRex();
    buildgen.ApplyOperands(static_cast<X86Arch::ParserSelect>(m_parser),
                           m_operands);
    buildgen.ApplySegReg(m_segreg, m_segreg_source);
    return buildgen.Finish(container, m_prefixes, source);
}

namespace {
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
#include "X86Insn_nasm.cpp"
#include "X86Insn_gas.cpp"
} // anonymous namespace

static std::string
CpuFindReverse(unsigned int cpu0, unsigned int cpu1, unsigned int cpu2)
{
    std::string s;
    llvm::raw_string_ostream cpuname(s);
    X86Arch::CpuMask cpu;

    cpu.set(cpu0);
    cpu.set(cpu1);
    cpu.set(cpu2);

    if (cpu[X86Arch::CPU_Prot])
        cpuname << " Protected";
    if (cpu[X86Arch::CPU_Undoc])
        cpuname << " Undocumented";
    if (cpu[X86Arch::CPU_Obs])
        cpuname << " Obsolete";
    if (cpu[X86Arch::CPU_Priv])
        cpuname << " Privileged";

    if (cpu[X86Arch::CPU_FPU])
        cpuname << " FPU";
    if (cpu[X86Arch::CPU_MMX])
        cpuname << " MMX";
    if (cpu[X86Arch::CPU_SSE])
        cpuname << " SSE";
    if (cpu[X86Arch::CPU_SSE2])
        cpuname << " SSE2";
    if (cpu[X86Arch::CPU_SSE3])
        cpuname << " SSE3";
    if (cpu[X86Arch::CPU_3DNow])
        cpuname << " 3DNow";
    if (cpu[X86Arch::CPU_Cyrix])
        cpuname << " Cyrix";
    if (cpu[X86Arch::CPU_AMD])
        cpuname << " AMD";
    if (cpu[X86Arch::CPU_SMM])
        cpuname << " SMM";
    if (cpu[X86Arch::CPU_SVM])
        cpuname << " SVM";
    if (cpu[X86Arch::CPU_PadLock])
        cpuname << " PadLock";
    if (cpu[X86Arch::CPU_EM64T])
        cpuname << " EM64T";
    if (cpu[X86Arch::CPU_SSSE3])
        cpuname << " SSSE3";
    if (cpu[X86Arch::CPU_SSE41])
        cpuname << " SSE4.1";
    if (cpu[X86Arch::CPU_SSE42])
        cpuname << " SSE4.2";

    if (cpu[X86Arch::CPU_186])
        cpuname << " 186";
    if (cpu[X86Arch::CPU_286])
        cpuname << " 286";
    if (cpu[X86Arch::CPU_386])
        cpuname << " 386";
    if (cpu[X86Arch::CPU_486])
        cpuname << " 486";
    if (cpu[X86Arch::CPU_586])
        cpuname << " 586";
    if (cpu[X86Arch::CPU_686])
        cpuname << " 686";
    if (cpu[X86Arch::CPU_P3])
        cpuname << " P3";
    if (cpu[X86Arch::CPU_P4])
        cpuname << " P4";
    if (cpu[X86Arch::CPU_IA64])
        cpuname << " IA64";
    if (cpu[X86Arch::CPU_K6])
        cpuname << " K6";
    if (cpu[X86Arch::CPU_Athlon])
        cpuname << " Athlon";
    if (cpu[X86Arch::CPU_Hammer])
        cpuname << " Hammer";

    cpuname.flush();
    return s;
}

Arch::InsnPrefix
X86Arch::ParseCheckInsnPrefix(StringRef id,
                              SourceLocation source,
                              DiagnosticsEngine& diags) const
{
    size_t id_len = id.size();
    if (id_len > 16)
        return InsnPrefix();

    static char lcaseid[17];
    for (size_t i=0; i<id_len; i++)
        lcaseid[i] = tolower(id[i]);
    lcaseid[id_len] = '\0';

    /*@null@*/ const InsnPrefixParseData* pdata;
    switch (m_parser)
    {
        case PARSER_NASM:
        case PARSER_GAS_INTEL:
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
            diags.Report(source, diag::warn_insn_in_64mode);
            return InsnPrefix();
        }
        if (m_mode_bits == 64 && (pdata->misc_flags & NOT_64))
        {
            diags.Report(source, diag::err_insn_invalid_64mode);
            return InsnPrefix();
        }

        if (!m_active_cpu[pdata->cpu0] ||
            !m_active_cpu[pdata->cpu1] ||
            !m_active_cpu[pdata->cpu2])
        {
            diags.Report(source, diag::warn_insn_with_cpu)
                << CpuFindReverse(pdata->cpu0, pdata->cpu1, pdata->cpu2);
            return InsnPrefix();
        }

        return InsnPrefix(reinterpret_cast<const InsnInfo*>(pdata));
    }
    else
    {
        const X86Prefix* prefix = static_cast<const X86Prefix*>(pdata->struc);

        if (m_mode_bits != 64 && (pdata->misc_flags & ONLY_64))
        {
            diags.Report(source, diag::warn_prefix_in_64mode);
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

X86Insn*
X86Insn::clone() const
{
    return new X86Insn(*this);
}

#ifdef WITH_XML
pugi::xml_node
X86Insn::DoWrite(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("X86Insn");

    append_child(root, "ActiveCpu",
                 m_active_cpu.to_string<char, std::char_traits<char>,
                                        std::allocator<char> >());

    append_child(root, "ModData", Bytes(m_mod_data, m_mod_data+2));
    
    append_child(root, "NumInfo", m_num_info);
    append_child(root, "ModeBits", m_mode_bits);

    append_child(root, "SuffixFlags", Twine::utohexstr(m_suffix).str());
    append_child(root, "MiscFlags", Twine::utohexstr(m_misc_flags).str());
    append_child(root, "Parser", m_parser);

    if (m_force_strict)
        root.append_attribute("force_strict") = true;
    if (m_default_rel)
        root.append_attribute("default_rel") = true;

    return root;
}
#endif // WITH_XML

std::auto_ptr<Insn>
X86Arch::CreateEmptyInsn() const
{
    ++num_empty_insn;
    return std::auto_ptr<Insn>(new X86Insn(
        *this,
        empty_insn,
        m_active_cpu,
        0,
        0,
        0,
        NELEMS(empty_insn),
        m_mode_bits,
        (m_parser == PARSER_GAS) ? SUF_Z : 0,
        0,
        m_parser,
        m_force_strict,
        m_default_rel));
}

std::auto_ptr<Insn>
X86Arch::CreateInsn(const Arch::InsnInfo* info) const
{
    const InsnPrefixParseData* pdata =
        reinterpret_cast<const InsnPrefixParseData*>(info);

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

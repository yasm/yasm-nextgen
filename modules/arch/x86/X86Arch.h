#ifndef YASM_X86ARCH_H
#define YASM_X86ARCH_H
//
// x86 Architecture header file
//
//  Copyright (C) 2001-2007  Peter Johnson
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
#include <bitset>

#include "yasmx/Config/export.h"
#include "yasmx/Arch.h"

#include "X86Register.h"
#include "X86TargetModifier.h"


namespace yasm
{

class DiagnosticsEngine;
class DirectiveInfo;
class Object;

namespace arch
{

class X86RegisterGroup;

class YASM_STD_EXPORT X86RegTmod
{
public:
    static const X86RegTmod& Instance();

    const X86Register* getReg(X86Register::Type type, unsigned int num) const
    { return m_reg[type][num]; }

    const X86RegisterGroup* getRegGroup(X86Register::Type type) const
    { return m_reg_group[type]; }

    const X86SegmentRegister* getSegReg(X86SegmentRegister::Type type) const
    { return m_segreg[type]; }

    const X86TargetModifier* getTargetMod(X86TargetModifier::Type type) const
    { return m_targetmod[type]; }

private:
    // Registers
    X86Register** m_reg[X86Register::TYPE_COUNT];

    // Register groups
    X86RegisterGroup* m_reg_group[X86Register::TYPE_COUNT];

    // Segment registers
    X86SegmentRegister* m_segreg[X86SegmentRegister::TYPE_COUNT];

    // Target modifiers
    X86TargetModifier* m_targetmod[X86TargetModifier::TYPE_COUNT];

private:
    X86RegTmod();
    X86RegTmod(const X86RegTmod &) {}
    X86RegTmod & operator= (const X86RegTmod &) { return *this; }
    ~X86RegTmod();
};

class YASM_STD_EXPORT X86Arch : public Arch
{
public:
    // Available CPU feature flags
    enum CpuFeature
    {
        CPU_Any = 0,        // Any old cpu will do
        CPU_086 = CPU_Any,
        CPU_186,            // i186 or better required
        CPU_286,            // i286 or better required
        CPU_386,            // i386 or better required
        CPU_486,            // i486 or better required
        CPU_586,            // i585 or better required
        CPU_686,            // i686 or better required
        CPU_P3,             // Pentium3 or better required
        CPU_P4,             // Pentium4 or better required
        CPU_IA64,           // IA-64 or better required
        CPU_K6,             // AMD K6 or better required
        CPU_Athlon,         // AMD Athlon or better required
        CPU_Hammer,         // AMD Sledgehammer or better required
        CPU_FPU,            // FPU support required
        CPU_MMX,            // MMX support required
        CPU_SSE,            // Streaming SIMD extensions required
        CPU_SSE2,           // Streaming SIMD extensions 2 required
        CPU_SSE3,           // Streaming SIMD extensions 3 required
        CPU_3DNow,          // 3DNow! support required
        CPU_Cyrix,          // Cyrix-specific instruction
        CPU_AMD,            // AMD-specific inst. (older than K6)
        CPU_SMM,            // System Management Mode instruction
        CPU_Prot,           // Protected mode only instruction
        CPU_Undoc,          // Undocumented instruction
        CPU_Obs,            // Obsolete instruction
        CPU_Priv,           // Priveleged instruction
        CPU_SVM,            // Secure Virtual Machine instruction
        CPU_PadLock,        // VIA PadLock instruction
        CPU_EM64T,          // Intel EM64T or better
        CPU_SSSE3,          // Streaming SIMD extensions 3 required
        CPU_SSE41,          // Streaming SIMD extensions 4.1 required
        CPU_SSE42,          // Streaming SIMD extensions 4.2 required
        CPU_SSE4a,          // AMD Streaming SIMD extensions 4a required
        CPU_XSAVE,          // Intel XSAVE instruction
        CPU_AVX,            // Intel Advanced Vector Extensions
        CPU_FMA,            // Intel Fused-Multiply-Add Extensions
        CPU_AES,            // AES instruction
        CPU_CLMUL,          // PCLMULQDQ instruction
        CPU_MOVBE,          // MOVBE instruction
        CPU_XOP,            // AMD XOP extensions
        CPU_FMA4,           // AMD Fused-Multiply-Add extensions
        CPU_F16C,           // Intel float-16 instructions
        CPU_FSGSBASE,       // Intel FSGSBASE instructions
        CPU_RDRAND,         // Intel RDRAND instruction
        CPU_XSAVEOPT,       // Intel XSAVEOPT instruction
        CPU_EPTVPID,        // Intel INVEPT, INVVPID instructions
        CPU_SMX,            // Intel SMX instruction (GETSEC)
        CPU_AVX2,           // Intel AVX2 instructions
        CPU_BMI1,           // Intel BMI1 instructions
        CPU_BMI2,           // Intel BMI2 instructions
        CPU_INVPCID,        // Intel INVPCID instruction
        CPU_LZCNT,          // Intel LZCNT instruction
        CPU_TBM,            // AMD TBM instruction
        CPU_TSX             // Intel TSX instructions
    };
    typedef std::bitset<64> CpuMask;

    enum ParserSelect
    {
        PARSER_NASM = 0,
        PARSER_GAS = 1,
        PARSER_GAS_INTEL = 2,
        PARSER_UNKNOWN
    };
    enum NopFormat
    {
        NOP_BASIC,
        NOP_INTEL,
        NOP_AMD
    };

    /// Constructor.
    X86Arch(const ArchModule& module);

    /// Destructor.
    ~X86Arch();

    void AddDirectives(Directives& dir, StringRef parser);

    bool setParser(StringRef parser);
    bool setMachine(StringRef machine);
    StringRef getMachine() const;
    unsigned int getAddressSize() const;

    bool setVar(StringRef var, unsigned long val);

    InsnPrefix ParseCheckInsnPrefix(StringRef id,
                                    SourceLocation source,
                                    DiagnosticsEngine& diags) const;
    RegTmod ParseCheckRegTmod(StringRef id,
                              SourceLocation source,
                              DiagnosticsEngine& diags) const;

    const unsigned char** getFill() const;

    void setEndian(Bytes& bytes) const;

    std::auto_ptr<EffAddr> CreateEffAddr(std::auto_ptr<Expr> e) const;

    std::auto_ptr<Insn> CreateEmptyInsn() const;
    std::auto_ptr<Insn> CreateInsn(const InsnInfo* info) const;

    ParserSelect getParser() const { return m_parser; }

    unsigned int getModeBits() const { return m_mode_bits; }

    static const char* getName()
    { return "x86 (IA-32 and derivatives), AMD64"; }
    static const char* getKeyword() { return "x86"; }
    static unsigned int getWordSize() { return 16; }
    static unsigned int getMinInsnLen() { return 1; }
    static ArchModule::MachineNames getMachines();

private:
    // Returns false if cpuid not recognized
    bool ParseCpu(StringRef cpuid);

    // Directives
    void DirCpu(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirBits(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCode16(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCode32(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCode64(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirDefault(DirectiveInfo& info, DiagnosticsEngine& diags);

    // What instructions/features are enabled?
    CpuMask m_active_cpu;

    bool m_amd64_machine;
    ParserSelect m_parser;
    unsigned int m_mode_bits;
    bool m_force_strict;
    bool m_default_rel;
    NopFormat m_nop;
};

}} // namespace yasm::arch

#endif

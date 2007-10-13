#ifndef YASM_X86REGTMOD_H
#define YASM_X86REGTMOD_H
//
// x86 register header file
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
#include <libyasm/arch.h>
#include <libyasm/insn.h>

namespace yasm { namespace arch { namespace x86 {

class X86Register : public Register {
public:
    // 0-15 (low 4 bits) used for register number, stored in same data area.
    enum Type {
        REG8 = 0x1,
        REG8X = 0x2,    // 64-bit mode only, REX prefix version of REG8
        REG16 = 0x3,
        REG32 = 0x4,
        REG64 = 0x5,    // 64-bit mode only
        FPUREG = 0x6,
        MMXREG = 0x7,
        XMMREG = 0x8,
        CRREG = 0x9,
        DRREG = 0xA,
        TRREG = 0xB,
        RIP = 0xC       // 64-bit mode only, always RIP (reg num ignored)
    };

    X86Register(Type type, unsigned int num);
    ~X86Register();

    /// Get the equivalent size of a register in bits.
    /// @param reg  register
    /// @return 0 if there is no suitable equivalent size, otherwise the
    ///         size.
    unsigned int get_size() const;

    /// Print a register.  For debugging purposes.
    /// @param os   output stream
    void put(std::ostream& os) const;

    Type type() const { return m_type; }
    unsigned int num() const { return m_num; }

private:
    // Register type.
    Type m_type;

    // Register number.
    // Note 8-15 are only valid for some registers, and only in 64-bit mode.
    unsigned int m_num;
};

extern const X86Register* X86_CRREG[16];
extern const X86Register* X86_DRREG[8];
extern const X86Register* X86_TRREG[8];
extern const X86Register* X86_FPUREG[8];
extern const X86Register* X86_MMXREG[8];
extern const X86Register* X86_XMMREG[16];
extern const X86Register* X86_REG64[16];
extern const X86Register* X86_REG32[16];
extern const X86Register* X86_REG16[16];
extern const X86Register* X86_REG8[16];
extern const X86Register* X86_REG8X[8];
extern const X86Register* X86_RIP;

class X86RegisterGroup : public RegisterGroup {
public:
    X86RegisterGroup(const X86Register** regs, unsigned long size);
    ~X86RegisterGroup();

    /// Get a specific register of a register group, based on the register
    /// group and the index within the group.
    /// @param regindex     register index
    /// @return 0 if regindex is not valid for that register group,
    ///         otherwise the specific register.
    const X86Register* get_reg(unsigned long regindex) const;

private:
    const X86Register** m_regs;
    unsigned long m_size;
};

extern const X86RegisterGroup* X86_FPUGroup;
extern const X86RegisterGroup* X86_MMXGroup;
extern const X86RegisterGroup* X86_XMMGroup;

class X86SegmentRegister : public SegmentRegister {
public:
    X86SegmentRegister(unsigned int num, unsigned char prefix);
    ~X86SegmentRegister();

    /// Print a segment register.  For debugging purposes.
    /// @param os   output stream
    void put(std::ostream& os) const;

    unsigned int num() const { return m_num; }
    unsigned char prefix() const { return m_prefix; }

private:
    unsigned int m_num;
    unsigned char m_prefix;
};

extern const X86SegmentRegister* X86_ES;
extern const X86SegmentRegister* X86_CS;
extern const X86SegmentRegister* X86_SS;
extern const X86SegmentRegister* X86_DS;
extern const X86SegmentRegister* X86_FS;
extern const X86SegmentRegister* X86_GS;

class X86TargetModifier : public Insn::Operand::TargetModifier {
public:
    enum Type {
        NEAR = 1,
        SHORT,
        FAR,
        TO
    };

    explicit X86TargetModifier(Type type);
    ~X86TargetModifier();

    Type type() const { return m_type; }

    void put(std::ostream& os) const;

private:
    Type m_type;
};

extern const X86TargetModifier* X86_NEAR;
extern const X86TargetModifier* X86_SHORT;
extern const X86TargetModifier* X86_FAR;
extern const X86TargetModifier* X86_TO;

}}} // namespace yasm::arch::x86

#endif

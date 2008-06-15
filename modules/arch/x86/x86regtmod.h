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
#include <libyasmx/arch.h>
#include <libyasmx/insn.h>

namespace yasm
{
namespace arch
{
namespace x86
{

class X86Arch;

class X86Register : public Register
{
public:
    // Register type.
    enum Type
    {
        REG8 = 0,
        REG8X,      // 64-bit mode only, REX prefix version of REG8
        REG16,
        REG32,
        REG64,      // 64-bit mode only
        FPUREG,
        MMXREG,
        XMMREG,
        CRREG,
        DRREG,
        TRREG,
        RIP,        // 64-bit mode only, always RIP (reg num ignored)
        TYPE_COUNT  // Number of types, must always be last in enum
    };

    X86Register(Type type, unsigned int num) : m_type(type), m_num(num) {}
    ~X86Register() {}

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

class X86RegisterGroup : public RegisterGroup
{
public:
    X86RegisterGroup(X86Arch& arch, X86Register** regs, unsigned long size)
        : m_arch(arch), m_regs(regs), m_size(size) {}
    ~X86RegisterGroup() {}

    /// Get a specific register of a register group, based on the register
    /// group and the index within the group.
    /// @param regindex     register index
    /// @return 0 if regindex is not valid for that register group,
    ///         otherwise the specific register.
    const X86Register* get_reg(unsigned long regindex) const;

private:
    X86Arch& m_arch;
    X86Register** m_regs;
    unsigned long m_size;
};

class X86SegmentRegister : public SegmentRegister
{
public:
    enum Type
    {
        ES = 0,
        CS,
        SS,
        DS,
        FS,
        GS,
        TYPE_COUNT
    };

    X86SegmentRegister(Type type, unsigned char prefix)
        : m_type(type), m_prefix(prefix) {}
    ~X86SegmentRegister() {}

    /// Print a segment register.  For debugging purposes.
    /// @param os   output stream
    void put(std::ostream& os) const;

    Type type() const { return m_type; }
    unsigned int num() const { return static_cast<unsigned int>(m_type); }
    unsigned char prefix() const { return m_prefix; }

private:
    Type m_type;
    unsigned char m_prefix;
};

class X86TargetModifier : public Insn::Operand::TargetModifier
{
public:
    enum Type
    {
        NEAR = 0,
        SHORT,
        FAR,
        TO,
        TYPE_COUNT
    };

    explicit X86TargetModifier(Type type) : m_type(type) {}
    ~X86TargetModifier() {}

    Type type() const { return m_type; }

    void put(std::ostream& os) const;

private:
    Type m_type;
};

}}} // namespace yasm::arch::x86

#endif

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

namespace yasm
{
namespace arch
{
namespace x86
{

namespace X86Register
{
    // Register types.
    enum Type
    {
        NONE = 0x0,
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
}

inline X86Register::Type
get_type(const Register& reg)
{
    return static_cast<X86Register::Type>(reg.m_type);
}

inline unsigned int
get_num(const Register& reg)
{
    return reg.m_num;
}

inline X86Register::Type
get_type(const RegisterGroup& reggroup)
{
    return static_cast<X86Register::Type>(reggroup.m_type);
}

namespace X86SegReg
{
    enum X86SegReg
    {
        ES = 0, CS = 1, SS = 2, DS = 3, FS = 4, GS = 5
    };
}

inline unsigned int
get_num(const SegmentRegister& segreg)
{
    return segreg.m_num;
}

unsigned char get_prefix(const SegmentRegister& segreg);

namespace X86TargetModifier
{
    enum Type {
        NEAR = 1,
        SHORT,
        FAR,
        TO
    };
}

inline X86TargetModifier::Type
get_type(const Insn::Operand::TargetModifier& tmod)
{
    return static_cast<X86TargetModifier::Type>(tmod.m_type);
}

}}} // namespace yasm::arch::x86

#endif

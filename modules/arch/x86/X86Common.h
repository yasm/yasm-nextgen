#ifndef YASM_X86COMMON_H
#define YASM_X86COMMON_H
//
// x86 common instruction information interface
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
#include <vector>

#include "yasmx/Insn.h"


namespace YAML { class Emitter; }

namespace yasm
{
namespace arch
{
namespace x86
{

class X86SegmentRegister;

class X86Common
{
public:
    X86Common();

    void ApplyPrefixes(unsigned int def_opersize_64,
                       const Insn::Prefixes& prefixes,
                       Diagnostic& diags,
                       unsigned char* rex = 0);
    void Finish();

    unsigned long getLen() const;
    void ToBytes(Bytes& bytes, const X86SegmentRegister* segreg) const;

    unsigned char m_addrsize;       // 0 or =mode_bits => no override
    unsigned char m_opersize;       // 0 or =mode_bits => no override
    unsigned char m_lockrep_pre;    // 0 indicates no prefix

    unsigned char m_mode_bits;
};

YAML::Emitter& operator<< (YAML::Emitter& out, const X86Common& common);

}}} // namespace yasm::arch::x86

#endif

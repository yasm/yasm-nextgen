#ifndef YASM_X86COMMON_H
#define YASM_X86COMMON_H
//
// x86 common bytecode header file
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

#include <libyasm/insn.h>

namespace yasm { namespace arch { namespace x86 {

class X86SegmentRegister;

class X86Common : public Bytecode::Contents {
public:
    X86Common();

    virtual void apply_prefixes
        (unsigned int def_opersize_64,
         const std::vector<const Insn::Prefix*>& prefixes)
    {
        apply_prefixes_common(0, def_opersize_64, prefixes);
    }

    void put(std::ostream& os, int indent_level) const;

    virtual unsigned long calc_len(Bytecode& bc,
                                   Bytecode::AddSpanFunc add_span) = 0;
    unsigned long calc_len() const;

    virtual void to_bytes(Bytecode& bc, Bytes& bytes,
                          OutputValueFunc output_value,
                          OutputRelocFunc output_reloc = 0) = 0;
    void to_bytes(Bytes& bytes, const X86SegmentRegister* segreg) const;

    unsigned char m_addrsize;       // 0 or =mode_bits => no override
    unsigned char m_opersize;       // 0 or =mode_bits => no override
    unsigned char m_lockrep_pre;    // 0 indicates no prefix

    unsigned char m_mode_bits;

protected:
    void apply_prefixes_common
        (unsigned char* rex, unsigned int def_opersize_64,
         const std::vector<const Insn::Prefix*>& prefixes);
};

}}} // namespace yasm::arch::x86

#endif

#ifndef YASM_X86GENERAL_H
#define YASM_X86GENERAL_H
//
// x86 general bytecode header file
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
#include <libyasm/bytecode.h>

#include "x86common.h"
#include "x86opcode.h"

namespace yasm { namespace arch { namespace x86 {

class X86EffAddr;

class X86General : public X86Common {
public:
    // Postponed (from parsing to later binding) action options.
    enum PostOp {
        // None
        POSTOP_NONE = 0,

        // Instructions that take a sign-extended imm8 as well as imm values
        // (eg, the arith instructions and a subset of the imul instructions)
        // should set this and put the imm8 form as the "normal" opcode (in
        // the first one or two bytes) and non-imm8 form in the second or
        // third byte of the opcode.
        POSTOP_SIGNEXT_IMM8,

        // Could become a short opcode mov with bits=64 and a32 prefix.
        POSTOP_SHORT_MOV,

        // Override any attempt at address-size override to 16 bits, and never
        // generate a prefix.  This is used for the ENTER opcode.
        POSTOP_ADDRESS16,

        // Large imm64 that can become a sign-extended imm32.
        POSTOP_SIMM32_AVAIL
    };

    X86General(const X86Opcode& opcode, std::auto_ptr<X86EffAddr> ea,
               std::auto_ptr<Value> imm, unsigned char special_prefix,
               unsigned char rex, PostOp postop);
    ~X86General();

    void finish_postop(bool default_rel);

    void apply_prefixes(unsigned int def_opersize_64,
                        const std::vector<const Insn::Prefix*>& prefixes)
    {
        apply_prefixes_common(&m_rex, def_opersize_64, prefixes);
    }

    void put(std::ostream& os, int indent_level) const;
    void finalize(Bytecode& bc, Bytecode& prev_bc);
    unsigned long calc_len(Bytecode& bc, Bytecode::AddSpanFunc add_span);
    bool expand(Bytecode& bc, unsigned long& len, int span,
                long old_val, long new_val,
                /*@out@*/ long& neg_thres,
                /*@out@*/ long& pos_thres);
    void to_bytes(Bytecode& bc, Bytes& bytes,
                  OutputValueFunc output_value,
                  OutputRelocFunc output_reloc = 0);

    X86General* clone() const;

private:
    X86General(const X86General& rhs);

    X86Opcode m_opcode;

    /*@null@*/ boost::scoped_ptr<X86EffAddr> m_ea;  // effective address

    /*@null@*/ boost::scoped_ptr<Value> m_imm;  // immediate or relative value

    unsigned char m_special_prefix;     // "special" prefix (0=none)

    // REX AMD64 extension, 0 if none,
    // 0xff if not allowed (high 8 bit reg used)
    unsigned char m_rex;

    PostOp m_postop;
};

}}} // namespace yasm::arch::x86

#endif

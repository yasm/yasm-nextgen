#ifndef YASM_X86JMP_H
#define YASM_X86JMP_H
//
// x86 jump bytecode header file
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
#include <libyasm/value.h>

#include "x86common.h"
#include "x86opcode.h"


namespace yasm { namespace arch { namespace x86 {

class X86Jmp : public X86Common {
public:
    enum OpcodeSel {
        NONE,
        SHORT,
        NEAR,
        SHORT_FORCED,
        NEAR_FORCED
    };

    X86Jmp(OpcodeSel op_sel, const X86Opcode& shortop, const X86Opcode& nearop,
           std::auto_ptr<Expr> target, const Bytecode& bc, Bytecode& precbc);
    ~X86Jmp();

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

    X86Jmp* clone() const;

private:
    X86Opcode m_shortop, m_nearop;

    Value m_target;             // jump target

    // which opcode are we using?
    // The *FORCED forms are specified in the source as such
    OpcodeSel m_op_sel;
};

}}} // namespace yasm::arch::x86

#endif

#ifndef YASM_X86JMPFAR_H
#define YASM_X86JMPFAR_H
//
// x86 jump far header file
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

// Direct (immediate) FAR jumps ONLY; indirect FAR jumps get turned into
// x86_insn bytecodes; relative jumps turn into x86_jmp bytecodes.
// This bytecode is not legal in 64-bit mode.
class X86JmpFar : public X86Common {
public:
    X86JmpFar(const X86Opcode& opcode, std::auto_ptr<Expr> segment,
              std::auto_ptr<Expr> offset, Bytecode& precbc);
    ~X86JmpFar();

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

    X86JmpFar* clone() const;

private:
    X86Opcode m_opcode;

    Value m_segment;            // target segment
    Value m_offset;             // target offset
};

}}} // namespace yasm::arch::x86

#endif

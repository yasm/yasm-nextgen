#ifndef YASM_UNWINDCODE_H
#define YASM_UNWINDCODE_H
//
// Win64 structured exception handling unwind code
//
//  Copyright (C) 2007-2008  Peter Johnson
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
#include <memory>
#include <vector>

#include <yasmx/Bytecode.h>
#include <yasmx/SymbolRef.h>
#include <yasmx/Value.h>


namespace yasm
{
class BytecodeContainer;

namespace objfmt
{
namespace win64
{

class UnwindCode : public Bytecode::Contents
{
    friend void append_unwind_code(BytecodeContainer& container,
                                   std::auto_ptr<UnwindCode> uwcode);

public:
    // Operation codes
    enum Opcode
    {
        PUSH_NONVOL = 0,
        ALLOC_LARGE = 1,
        ALLOC_SMALL = 2,
        SET_FPREG = 3,
        SAVE_NONVOL = 4,
        SAVE_NONVOL_FAR = 5,
        SAVE_XMM128 = 8,
        SAVE_XMM128_FAR = 9,
        PUSH_MACHFRAME = 10
    };

    UnwindCode(SymbolRef proc,
               SymbolRef loc,
               Opcode opcode,
               unsigned int info)
        : m_proc(proc)
        , m_loc(loc)
        , m_opcode(opcode)
        , m_info(info)
        , m_off(0)
    {
    }

    UnwindCode(SymbolRef proc,
               SymbolRef loc,
               Opcode opcode,
               unsigned int info,
               unsigned int size,
               std::auto_ptr<Expr> e)
        : m_proc(proc)
        , m_loc(loc)
        , m_opcode(opcode)
        , m_info(info)
        , m_off(size, e)
    {
    }

    ~UnwindCode();

    virtual void put(marg_ostream& os) const;
    virtual void finalize(Bytecode& bc);
    virtual unsigned long calc_len(Bytecode& bc,
                                   const Bytecode::AddSpanFunc& add_span);
    virtual bool expand(Bytecode& bc,
                        unsigned long& len,
                        int span,
                        long old_val,
                        long new_val,
                        /*@out@*/ long* neg_thres,
                        /*@out@*/ long* pos_thres);
    virtual void output(Bytecode& bc, BytecodeOutput& bc_out);
    virtual UnwindCode* clone() const;

private:
    SymbolRef m_proc;       // Start of procedure
    SymbolRef m_loc;        // Location of operation

    Opcode m_opcode;        // Operation code
    unsigned int m_info;    // Operation info
    Value m_off;            // Offset expression (used for some codes)
};

void append_unwind_code(BytecodeContainer& container,
                        std::auto_ptr<UnwindCode> uwcode);

}}} // namespace yasm::objfmt::win64

#endif

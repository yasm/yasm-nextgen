#ifndef YASM_UNWINDINFO_H
#define YASM_UNWINDINFO_H
//
// Win64 structured exception handling unwind info
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

#include "UnwindCode.h"


namespace yasm
{
class BytecodeContainer;

namespace objfmt
{
namespace win64
{

class UnwindInfo : public Bytecode::Contents
{
    friend void generate(std::auto_ptr<UnwindInfo> uwinfo,
                         BytecodeContainer& xdata,
                         unsigned long line,
                         const Arch& arch);

public:
    UnwindInfo();
    ~UnwindInfo();

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
    virtual UnwindInfo* clone() const;

    void set_proc(SymbolRef proc) { m_proc = proc; }
    SymbolRef get_proc() { return m_proc; }
    void set_prolog(SymbolRef prolog) { m_prolog = prolog; }
    void set_ehandler(SymbolRef ehandler) { m_ehandler = ehandler; }
    void set_framereg(unsigned long framereg) { m_framereg = framereg; }
    void set_frameoff(const Value& frameoff) { m_frameoff = frameoff; }
    void add_code(std::auto_ptr<UnwindCode> uwcode)
    { m_codes.push_back(uwcode.release()); }

private:
    SymbolRef m_proc;           // Start of procedure
    SymbolRef m_prolog;         // End of prologue

    SymbolRef m_ehandler;       // Error handler

    unsigned long m_framereg;   // Frame register
    Value m_frameoff;           // Frame offset

    // Codes, in increasing location offset order.
    // Inserting at the end of this vector during assembly naturally results
    // in this sorting.
    std::vector<UnwindCode*> m_codes;

    // These aren't used until inside of generate.
    Value m_prolog_size;
    Value m_codes_count;
};

void generate(std::auto_ptr<UnwindInfo> uwinfo,
              BytecodeContainer& xdata,
              unsigned long line,
              const Arch& arch);

}}} // namespace yasm::objfmt::win64

#endif

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

#include "yasmx/Config/export.h"
#include "yasmx/Bytecode.h"
#include "yasmx/SymbolRef.h"
#include "yasmx/Value.h"

#include "UnwindCode.h"


namespace yasm
{
class BytecodeContainer;

namespace objfmt
{

class YASM_STD_EXPORT UnwindInfo : public Bytecode::Contents
{
    friend YASM_STD_EXPORT
        void Generate(std::auto_ptr<UnwindInfo> uwinfo,
                      BytecodeContainer& xdata,
                      SourceLocation source,
                      const Arch& arch,
                      DiagnosticsEngine& diags);

public:
    UnwindInfo();
    ~UnwindInfo();

    virtual bool Finalize(Bytecode& bc, DiagnosticsEngine& diags);
    virtual bool CalcLen(Bytecode& bc,
                         /*@out@*/ unsigned long* len,
                         const Bytecode::AddSpanFunc& add_span,
                         DiagnosticsEngine& diags);
    virtual bool Expand(Bytecode& bc,
                        unsigned long* len,
                        int span,
                        long old_val,
                        long new_val,
                        bool* keep,
                        /*@out@*/ long* neg_thres,
                        /*@out@*/ long* pos_thres,
                        DiagnosticsEngine& diags);
    virtual bool Output(Bytecode& bc, BytecodeOutput& bc_out);
    StringRef getType() const;
    virtual UnwindInfo* clone() const;
#ifdef WITH_XML
    virtual pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    void setProc(SymbolRef proc) { m_proc = proc; }
    SymbolRef getProc() { return m_proc; }
    void setProlog(SymbolRef prolog) { m_prolog = prolog; }
    void setEHandler(SymbolRef ehandler) { m_ehandler = ehandler; }
    void setFrameReg(unsigned long framereg) { m_framereg = framereg; }
    void setFrameOff(const Value& frameoff) { m_frameoff = frameoff; }
    void AddCode(std::auto_ptr<UnwindCode> uwcode)
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

YASM_STD_EXPORT
void Generate(std::auto_ptr<UnwindInfo> uwinfo,
              BytecodeContainer& xdata,
              SourceLocation source,
              const Arch& arch,
              DiagnosticsEngine& diags);

}} // namespace yasm::objfmt

#endif

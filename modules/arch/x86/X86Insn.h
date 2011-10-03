#ifndef YASM_X86INSN_H
#define YASM_X86INSN_H
//
// x86 instruction handling
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
#include "yasmx/Config/export.h"
#include "yasmx/Insn.h"

#include "X86Arch.h"

namespace yasm
{

class SourceLocation;

namespace arch
{

struct X86InfoOperand;
struct X86InsnInfo;
class X86Opcode;

class YASM_STD_EXPORT X86Insn : public Insn
{
public:
    X86Insn(const X86Arch& arch,
            const X86InsnInfo* group,
            const X86Arch::CpuMask& active_cpu,
            unsigned char mod_data0,
            unsigned char mod_data1,
            unsigned char mod_data2,
            unsigned int num_info,
            unsigned int mode_bits,
            unsigned int suffix,
            unsigned int misc_flags,
            X86Arch::ParserSelect parser,
            bool force_strict,
            bool default_rel);
    ~X86Insn();

    X86Insn* clone() const;

protected:
    bool DoAppend(BytecodeContainer& container,
                  SourceLocation source,
                  DiagnosticsEngine& diags);
#ifdef WITH_XML
    pugi::xml_node DoWrite(pugi::xml_node out) const;
#endif // WITH_XML

private:
    bool DoAppendJmpFar(BytecodeContainer& container,
                        const X86InsnInfo& info,
                        SourceLocation source,
                        DiagnosticsEngine& diags);

    bool MatchJmpInfo(const X86InsnInfo& info,
                      unsigned int opersize,
                      X86Opcode& shortop,
                      X86Opcode& nearop) const;
    bool DoAppendJmp(BytecodeContainer& container,
                     const X86InsnInfo& jinfo,
                     SourceLocation source,
                     DiagnosticsEngine& diags);

    bool DoAppendGeneral(BytecodeContainer& container,
                         const X86InsnInfo& info,
                         const unsigned int* size_lookup,
                         SourceLocation source,
                         DiagnosticsEngine& diags);

    const X86InsnInfo* FindMatch(const unsigned int* size_lookup, int bypass)
        const;
    bool MatchInfo(const X86InsnInfo& info,
                   const unsigned int* size_lookup,
                   int bypass) const;
    bool MatchOperand(const Operand& op,
                      const X86InfoOperand& info_op,
                      const Operand& op0,
                      const unsigned int* size_lookup,
                      int bypass) const;
    void MatchError(const unsigned int* size_lookup,
                    SourceLocation source,
                    DiagnosticsEngine& diags) const;

    // architecture
    const X86Arch& m_arch;

    // instruction parse group - NULL if empty instruction (just prefixes)
    /*@null@*/ const X86InsnInfo* m_group;

    // CPU feature flags enabled at the time of parsing the instruction
    X86Arch::CpuMask m_active_cpu;

    // Modifier data
    unsigned char m_mod_data[3];

    // Number of elements in the instruction parse group
    unsigned int m_num_info:8;

    // BITS setting active at the time of parsing the instruction
    unsigned int m_mode_bits:8;

    // Suffix flags
    unsigned int m_suffix:9;

    // Tests against BITS==64 and AVX
    unsigned int m_misc_flags:5;

    // Parser enabled at the time of parsing the instruction
    unsigned int m_parser:2;

    // Strict forced setting at the time of parsing the instruction
    unsigned int m_force_strict:1;

    // Default rel setting at the time of parsing the instruction
    unsigned int m_default_rel:1;
};

}} // namespace yasm::arch

#endif

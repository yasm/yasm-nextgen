#ifndef YASM_X86ARCH_H
#define YASM_X86ARCH_H
//
// x86 Architecture header file
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
#include <bitset>

#include <libyasm/arch.h>


namespace yasm {

class Object;
class NameValues;

namespace arch { namespace x86 {

// Available CPU feature flags
enum CPUFeature {
    CPU_Any = 0,        // Any old cpu will do
    CPU_086 = CPU_Any,
    CPU_186,            // i186 or better required
    CPU_286,            // i286 or better required
    CPU_386,            // i386 or better required
    CPU_486,            // i486 or better required
    CPU_586,            // i585 or better required
    CPU_686,            // i686 or better required
    CPU_P3,             // Pentium3 or better required
    CPU_P4,             // Pentium4 or better required
    CPU_IA64,           // IA-64 or better required
    CPU_K6,             // AMD K6 or better required
    CPU_Athlon,         // AMD Athlon or better required
    CPU_Hammer,         // AMD Sledgehammer or better required
    CPU_FPU,            // FPU support required
    CPU_MMX,            // MMX support required
    CPU_SSE,            // Streaming SIMD extensions required
    CPU_SSE2,           // Streaming SIMD extensions 2 required
    CPU_SSE3,           // Streaming SIMD extensions 3 required
    CPU_3DNow,          // 3DNow! support required
    CPU_Cyrix,          // Cyrix-specific instruction
    CPU_AMD,            // AMD-specific inst. (older than K6)
    CPU_SMM,            // System Management Mode instruction
    CPU_Prot,           // Protected mode only instruction
    CPU_Undoc,          // Undocumented instruction
    CPU_Obs,            // Obsolete instruction
    CPU_Priv,           // Priveleged instruction
    CPU_SVM,            // Secure Virtual Machine instruction
    CPU_PadLock,        // VIA PadLock instruction
    CPU_EM64T,          // Intel EM64T or better
    CPU_SSSE3,          // Streaming SIMD extensions 3 required
    CPU_SSE41,          // Streaming SIMD extensions 4.1 required
    CPU_SSE42,          // Streaming SIMD extensions 4.2 required
    CPU_SSE4a,          // AMD Streaming SIMD extensions 4a required
    CPU_SSE5,           // AMD Streaming SIMD extensions 5 required

    // Technically not CPU capabilities, they do affect what instructions are
    // available.  These are tested against BITS==64.
    CPU_64 = 120,       // Only available in 64-bit mode
    CPU_Not64 = 121     // Not available (invalid) in 64-bit mode
};

class X86Arch : public Arch {
public:
    typedef std::bitset<64> CpuMask;
    enum ParserSelect {
        PARSER_NASM = 0,
        PARSER_GAS = 1,
        PARSER_UNKNOWN
    };

    /// Constructor.
    X86Arch();

    /// Destructor.
    ~X86Arch();

    std::string get_name() const;
    std::string get_keyword() const;
    std::string get_type() const;
    void add_directives(Directives& dir, const std::string& parser);

    bool set_parser(const std::string& parser);
    unsigned int get_wordsize() const;
    unsigned int get_min_insn_len() const;
    bool set_machine(const std::string& machine);
    std::string get_machine() const;
    std::map<std::string, std::string> get_machines() const;
    unsigned int get_address_size() const;

    bool set_var(const std::string& var, unsigned long val);

    InsnPrefix parse_check_insnprefix
        (const char *id, size_t id_len, unsigned long line) const;
    RegTmod parse_check_regtmod(const char *id, size_t id_len) const;

    const unsigned char** get_fill() const;

    void floatnum_tobytes(const FloatNum& flt, Bytes& bytes,
                          size_t valsize, size_t shift, int warn) const;
    void intnum_tobytes(const IntNum& intn, Bytes& bytes,
                        size_t valsize, int shift, int warn) const;

    std::auto_ptr<EffAddr> ea_create(std::auto_ptr<Expr> e) const;

    std::auto_ptr<Insn> create_empty_insn() const;

    ParserSelect parser() const { return m_parser; }

private:
    void parse_cpu(const std::string& cpuid);

    // Directives
    void dir_cpu(Object& object, const NameValues& namevals,
                 const NameValues& objext_valparams, unsigned long line);
    void dir_bits(Object& object, const NameValues& namevals,
                  const NameValues& objext_valparams, unsigned long line);
    void dir_code16(Object& object, const NameValues& namevals,
                    const NameValues& objext_namevals, unsigned long line);
    void dir_code32(Object& object, const NameValues& namevals,
                    const NameValues& objext_namevals, unsigned long line);
    void dir_code64(Object& object, const NameValues& namevals,
                    const NameValues& objext_namevals, unsigned long line);

    // What instructions/features are enabled?
    CpuMask m_active_cpu;

    bool m_amd64_machine;
    ParserSelect m_parser;
    unsigned int m_mode_bits;
    bool m_force_strict;
    bool m_default_rel;
};

}}} // namespace yasm::arch::x86

#endif

//
// x86 architecture description
//
//  Copyright (C) 2002-2007  Peter Johnson
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
#include "x86arch.h"

#include "util.h"

#include <libyasm/bytes.h>
#include <libyasm/compose.h>
#include <libyasm/directive.h>
#include <libyasm/errwarn.h>
#include <libyasm/expr.h>
#include <libyasm/floatnum.h>
#include <libyasm/intnum.h>
#include <libyasm/object.h>
#include <libyasm/name_value.h>
#include <libyasm/nocase.h>
#include <libyasm/registry.h>

#include "x86effaddr.h"
#include "x86regtmod.h"


namespace yasm { namespace arch { namespace x86 {

X86Arch::X86Arch()
    : m_amd64_machine(false),
      m_parser(PARSER_UNKNOWN),
      m_mode_bits(0),
      m_force_strict(false),
      m_default_rel(false)
{
    // default to all instructions/features enabled
    m_active_cpu.set();
}

bool
X86Arch::set_parser(const std::string& parser)
{
    if (String::nocase_equal(parser, "nasm"))
        m_parser = PARSER_NASM;
    else if (String::nocase_equal(parser, "gas") ||
             String::nocase_equal(parser, "gnu"))
        m_parser = PARSER_GAS;
    else
        return false;
    return true;
}

bool
X86Arch::set_machine(const std::string& machine)
{
    if (String::nocase_equal(machine, "x86"))
        m_amd64_machine = false;
    else if (String::nocase_equal(machine, "amd64"))
        m_amd64_machine = true;
    else
        return false;
    return true;
}

X86Arch::~X86Arch()
{
}

unsigned int
X86Arch::get_wordsize() const
{
    return 16;
}

unsigned int
X86Arch::get_min_insn_len() const
{
    return 1;
}

std::string
X86Arch::get_machine() const
{
    if (m_amd64_machine)
        return "amd64";
    else
        return "x86";
}

std::map<std::string, std::string>
X86Arch::get_machines() const
{
    std::map<std::string, std::string> machines;
    machines["x86"] = "IA-32 and derivatives";
    machines["amd64"] = "AMD64";
    return machines;
}

unsigned int
X86Arch::get_address_size() const
{
    if (m_mode_bits != 0)
        return m_mode_bits;
    if (m_amd64_machine)
        return 64;
    else
        return 32;
}

bool
X86Arch::set_var(const std::string& var, unsigned long val)
{
    if (String::nocase_equal(var, "mode_bits"))
        m_mode_bits = (unsigned int)val;
    else if (String::nocase_equal(var, "force_strict"))
        m_force_strict = (val != 0);
    else if (String::nocase_equal(var, "default_rel")) {
        if (m_mode_bits != 64)
            warn_set(WARN_GENERAL,
                     N_("ignoring default rel in non-64-bit mode"));
        else
            m_default_rel = (val != 0);
    } else
        return true;
    return false;
}

void
X86Arch::dir_cpu(Object& object, const NameValues& namevals,
                 const NameValues& objext_namevals, unsigned long line)
{
    for (NameValues::const_iterator nv=namevals.begin(), end=namevals.end();
         nv != end; ++nv) {
        if (nv->is_string())
            parse_cpu(nv->get_string());
        else if (nv->is_expr()) {
            std::auto_ptr<Expr> e = nv->get_expr(object, line);
            IntNum* intcpu = e->get_intnum();
            if (!intcpu)
                throw SyntaxError(String::compose(
                    N_("invalid argument to [%1]"), "CPU"));
            else {
                std::ostringstream strcpu;
                strcpu << intcpu->get_uint();
                parse_cpu(strcpu.str());
            }
        } else
            throw SyntaxError(String::compose(N_("invalid argument to [%1]"),
                                              "CPU"));
    }
}

void
X86Arch::dir_bits(Object& object, const NameValues& namevals,
                  const NameValues& objext_namevals, unsigned long line)
{
    NameValues::const_iterator nv = namevals.begin();
    if (nv != namevals.end() && nv->is_expr()) {
        std::auto_ptr<Expr> e = nv->get_expr(object, line);
        IntNum* intcpu = e->get_intnum();
        if (intcpu) {
            unsigned long v = intcpu->get_uint();
            if (v == 16 || v == 32 || v == 64) {
                m_mode_bits = v;
                return;
            }
        }
    }

    throw ValueError(String::compose(N_("invalid argument to [%1]"), "BITS"));
}

void
X86Arch::dir_code16(Object& object, const NameValues& namevals,
                    const NameValues& objext_namevals, unsigned long line)
{
    m_mode_bits = 16;
}

void
X86Arch::dir_code32(Object& object, const NameValues& namevals,
                    const NameValues& objext_namevals, unsigned long line)
{
    m_mode_bits = 32;
}

void
X86Arch::dir_code64(Object& object, const NameValues& namevals,
                    const NameValues& objext_namevals, unsigned long line)
{
    m_mode_bits = 64;
}

const unsigned char **
X86Arch::get_fill() const
{
    // Fill patterns that GAS uses.
    static const unsigned char *fill16[16] = {
        NULL,                           // unused
        (const unsigned char *)
        "\x90",                         // 1 - nop
        (const unsigned char *)
        "\x89\xf6",                     // 2 - mov si, si
        (const unsigned char *)
        "\x8d\x74\x00",                 // 3 - lea si, [si+byte 0]
        (const unsigned char *)
        "\x8d\xb4\x00\x00",             // 4 - lea si, [si+word 0]
        (const unsigned char *)
        "\x90"                          // 5 - nop
        "\x8d\xb4\x00\x00",             //     lea si, [si+word 0]
        (const unsigned char *)
        "\x89\xf6"                      // 6 - mov si, si
        "\x8d\xbd\x00\x00",             //     lea di, [di+word 0]
        (const unsigned char *)
        "\x8d\x74\x00"                  // 7 - lea si, [si+byte 0]
        "\x8d\xbd\x00\x00",             //     lea di, [di+word 0]
        (const unsigned char *)
        "\x8d\xb4\x00\x00"              // 8 - lea si, [si+word 0]
        "\x8d\xbd\x00\x00",             //     lea di, [di+word 0]
        (const unsigned char *)
        "\xeb\x07\x90\x90\x90\x90\x90"  // 9 - jmp $+9; nop fill
        "\x90\x90",
        (const unsigned char *)
        "\xeb\x08\x90\x90\x90\x90\x90"  // 10 - jmp $+10; nop fill
        "\x90\x90\x90",
        (const unsigned char *)
        "\xeb\x09\x90\x90\x90\x90\x90"  // 11 - jmp $+11; nop fill
        "\x90\x90\x90\x90",
        (const unsigned char *)
        "\xeb\x0a\x90\x90\x90\x90\x90"  // 12 - jmp $+12; nop fill
        "\x90\x90\x90\x90\x90",
        (const unsigned char *)
        "\xeb\x0b\x90\x90\x90\x90\x90"  // 13 - jmp $+13; nop fill
        "\x90\x90\x90\x90\x90\x90",
        (const unsigned char *)
        "\xeb\x0c\x90\x90\x90\x90\x90"  // 14 - jmp $+14; nop fill
        "\x90\x90\x90\x90\x90\x90\x90",
        (const unsigned char *)
        "\xeb\x0d\x90\x90\x90\x90\x90"  // 15 - jmp $+15; nop fill
        "\x90\x90\x90\x90\x90\x90\x90\x90"
    };
    static const unsigned char *fill32[16] = {
        NULL,                           // unused
        (const unsigned char *)
        "\x90",                         // 1 - nop
        (const unsigned char *)
        "\x89\xf6",                     // 2 - mov esi, esi
        (const unsigned char *)
        "\x8d\x76\x00",                 // 3 - lea esi, [esi+byte 0]
        (const unsigned char *)
        "\x8d\x74\x26\x00",             // 4 - lea esi, [esi*1+byte 0]
        (const unsigned char *)
        "\x90"                          // 5 - nop
        "\x8d\x74\x26\x00",             //     lea esi, [esi*1+byte 0]
        (const unsigned char *)
        "\x8d\xb6\x00\x00\x00\x00",     // 6 - lea esi, [esi+dword 0]
        (const unsigned char *)
        "\x8d\xb4\x26\x00\x00\x00\x00", // 7 - lea esi, [esi*1+dword 0]
        (const unsigned char *)
        "\x90"                          // 8 - nop
        "\x8d\xb4\x26\x00\x00\x00\x00", //     lea esi, [esi*1+dword 0]
#if 0
        // GAS uses these
        (const unsigned char *)
        "\x89\xf6"                      // 9 - mov esi, esi
        "\x8d\xbc\x27\x00\x00\x00\x00", //     lea edi, [edi*1+dword 0]
        (const unsigned char *)
        "\x8d\x76\x00"                  // 10 - lea esi, [esi+byte 0]
        "\x8d\xbc\x27\x00\x00\x00\x00", //      lea edi, [edi+dword 0]
        (const unsigned char *)
        "\x8d\x74\x26\x00"              // 11 - lea esi, [esi*1+byte 0]
        "\x8d\xbc\x27\x00\x00\x00\x00", //      lea edi, [edi*1+dword 0]
        (const unsigned char *)
        "\x8d\xb6\x00\x00\x00\x00"      // 12 - lea esi, [esi+dword 0]
        "\x8d\xbf\x00\x00\x00\x00",     //      lea edi, [edi+dword 0]
        (const unsigned char *)
        "\x8d\xb6\x00\x00\x00\x00"      // 13 - lea esi, [esi+dword 0]
        "\x8d\xbc\x27\x00\x00\x00\x00", //      lea edi, [edi*1+dword 0]
        (const unsigned char *)
        "\x8d\xb4\x26\x00\x00\x00\x00"  // 14 - lea esi, [esi*1+dword 0]
        "\x8d\xbc\x27\x00\x00\x00\x00", //      lea edi, [edi*1+dword 0]
#else
        // But on newer processors, these are recommended
        (const unsigned char *)
        "\xeb\x07\x90\x90\x90\x90\x90"  // 9 - jmp $+9; nop fill
        "\x90\x90",
        (const unsigned char *)
        "\xeb\x08\x90\x90\x90\x90\x90"  // 10 - jmp $+10; nop fill
        "\x90\x90\x90",
        (const unsigned char *)
        "\xeb\x09\x90\x90\x90\x90\x90"  // 11 - jmp $+11; nop fill
        "\x90\x90\x90\x90",
        (const unsigned char *)
        "\xeb\x0a\x90\x90\x90\x90\x90"  // 12 - jmp $+12; nop fill
        "\x90\x90\x90\x90\x90",
        (const unsigned char *)
        "\xeb\x0b\x90\x90\x90\x90\x90"  // 13 - jmp $+13; nop fill
        "\x90\x90\x90\x90\x90\x90",
        (const unsigned char *)
        "\xeb\x0c\x90\x90\x90\x90\x90"  // 14 - jmp $+14; nop fill
        "\x90\x90\x90\x90\x90\x90\x90",
#endif
        (const unsigned char *)
        "\xeb\x0d\x90\x90\x90\x90\x90"  // 15 - jmp $+15; nop fill
        "\x90\x90\x90\x90\x90\x90\x90\x90"
    };
    static const unsigned char *fill64[16] = {
        NULL,                           // unused
        (const unsigned char *)
        "\x90",                         // 1 - nop
        (const unsigned char *)
        "\x66\x90",                     // 2 - o16; nop
        (const unsigned char *)
        "\x66\x66\x90",                 // 3 - o16; o16; nop
        (const unsigned char *)
        "\x66\x66\x66\x90",             // 4 - o16; o16; o16; nop
        (const unsigned char *)
        "\x66\x66\x90\x66\x90",         // 5
        (const unsigned char *)
        "\x66\x66\x90\x66\x66\x90",     // 6
        (const unsigned char *)
        "\x66\x66\x66\x90\x66\x66\x90", // 7
        (const unsigned char *)
        "\x66\x66\x66\x90\x66\x66\x66"  // 8
        "\x90",
        (const unsigned char *)
        "\x66\x66\x90\x66\x66\x90\x66"  // 9
        "\x66\x90",
        (const unsigned char *)
        "\x66\x66\x66\x90\x66\x66\x90"  // 10
        "\x66\x66\x90",
        (const unsigned char *)
        "\x66\x66\x66\x90\x66\x66\x66"  // 11
        "\x90\x66\x66\x90",
        (const unsigned char *)
        "\x66\x66\x66\x90\x66\x66\x66"  // 12
        "\x90\x66\x66\x66\x90",
        (const unsigned char *)
        "\x66\x66\x66\x90\x66\x66\x90"  // 13
        "\x66\x66\x90\x66\x66\x90",
        (const unsigned char *)
        "\x66\x66\x66\x90\x66\x66\x66"  // 14
        "\x90\x66\x66\x90\x66\x66\x90",
        (const unsigned char *)
        "\x66\x66\x66\x90\x66\x66\x66"  // 15
        "\x90\x66\x66\x66\x90\x66\x66\x90"
    };
    switch (m_mode_bits) {
        case 16:
            return fill16;
        case 32:
            return fill32;
        case 64:
            return fill64;
        default:
            throw ValueError(N_("Invalid mode_bits in x86_get_fill"));
    }
}

std::string
X86Arch::get_name() const
{
    return "x86";
}

std::string
X86Arch::get_keyword() const
{
    return "x86 (IA-32 and derivatives), AMD64";
}

std::string
X86Arch::get_type() const
{
    return "Arch";
}

void
X86Arch::add_directives(Directives& dirs, const std::string& parser)
{
    if (String::nocase_equal(parser, "nasm")) {
        dirs.add("cpu",
                 BIND::bind(&X86Arch::dir_cpu, this, _1, _2, _3, _4),
                 Directives::ARG_REQUIRED);
        dirs.add("bits",
                 BIND::bind(&X86Arch::dir_bits, this, _1, _2, _3, _4),
                 Directives::ARG_REQUIRED);
    } else if (String::nocase_equal(parser, "gas") ||
               String::nocase_equal(parser, "gnu")) {
        dirs.add(".code16",
                 BIND::bind(&X86Arch::dir_code16, this, _1, _2, _3, _4),
                 Directives::ANY);
        dirs.add(".code32",
                 BIND::bind(&X86Arch::dir_code32, this, _1, _2, _3, _4),
                 Directives::ANY);
        dirs.add(".code64",
                 BIND::bind(&X86Arch::dir_code64, this, _1, _2, _3, _4),
                 Directives::ANY);
    }
}


void
X86Arch::floatnum_tobytes(const FloatNum& flt, Bytes& bytes, size_t destsize,
                          size_t valsize, size_t shift, int warn) const
{
    if (!flt.is_valid_size(valsize))
        throw FloatingPointError(N_("invalid floating point constant size"));

    Bytes::size_type sz = bytes.size();
    bytes.resize(sz+destsize);
    flt.get_sized(&bytes[sz], destsize, valsize, shift, false, warn);
}

void
X86Arch::intnum_tobytes(const IntNum& intn, Bytes& bytes, size_t destsize,
                        size_t valsize, int shift, Location loc,
                        int warn) const
{
    Bytes::size_type sz = bytes.size();
    bytes.resize(sz+destsize);
    intn.get_sized(&bytes[sz], destsize, valsize, shift, false, warn);
}

std::auto_ptr<EffAddr>
X86Arch::ea_create(std::auto_ptr<Expr> e) const
{
    return std::auto_ptr<EffAddr>(new X86EffAddr(*this, e));
}

RegisterModule<Arch, X86Arch> registerX86Arch("x86");
bool static_ref = true;

}}} // namespace yasm::arch::x86

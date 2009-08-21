//
// x86 common instruction information interface
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
#include "X86Common.h"

#include "util.h"

#include "YAML/emitter.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"

#include "X86Prefix.h"
#include "X86Register.h"


namespace yasm
{
namespace arch
{
namespace x86
{

X86Common::X86Common()
    : m_addrsize(0),
      m_opersize(0),
      m_lockrep_pre(0),
      m_mode_bits(0)
{
}

void
X86Common::ApplyPrefixes(unsigned int def_opersize_64,
                         const Insn::Prefixes& prefixes,
                         unsigned char* rex)
{
    bool first = true;

    for (Insn::Prefixes::const_iterator i=prefixes.begin(), end=prefixes.end();
         i != end; ++i)
    {
        const X86Prefix* prefix = static_cast<const X86Prefix*>(*i);
        switch (prefix->getType())
        {
            case X86Prefix::LOCKREP:
                if (m_lockrep_pre != 0)
                    setWarn(WARN_GENERAL,
                        N_("multiple LOCK or REP prefixes, using leftmost"));
                m_lockrep_pre = prefix->getValue();
                break;
            case X86Prefix::ADDRSIZE:
                m_addrsize = prefix->getValue();
                break;
            case X86Prefix::OPERSIZE:
                m_opersize = prefix->getValue();
                if (m_mode_bits == 64 && m_opersize == 64 &&
                    def_opersize_64 != 64)
                {
                    if (!rex)
                        setWarn(WARN_GENERAL,
                                N_("ignoring REX prefix on jump"));
                    else if (*rex == 0xff)
                        setWarn(WARN_GENERAL,
                            N_("REX prefix not allowed on this instruction, ignoring"));
                    else
                        *rex = 0x48;
                }
                break;
            case X86Prefix::SEGREG:
                // This is a hack.. we should really be putting this in the
                // the effective address!
                m_lockrep_pre = prefix->getValue();
                break;
            case X86Prefix::REX:
                if (!rex)
                    setWarn(WARN_GENERAL, N_("ignoring REX prefix on jump"));
                else if (*rex == 0xff)
                    setWarn(WARN_GENERAL,
                        N_("REX prefix not allowed on this instruction, ignoring"));
                else
                {
                    if (*rex != 0)
                    {
                        if (first)
                            setWarn(WARN_GENERAL,
                                N_("overriding generated REX prefix"));
                        else
                            setWarn(WARN_GENERAL,
                                N_("multiple REX prefixes, using leftmost"));
                    }
                    // Here we assume that we can't get this prefix in non
                    // 64 bit mode due to checks in ParseCheckInsnPrefix().
                    m_mode_bits = 64;
                    *rex = prefix->getValue();
                }
                first = false;
                break;
        }
    }
}

void
X86Common::Finish()
{
    // Change 0 opersize to mode_bits.
    // 64-bit mode opersize defaults to 32-bit.
    // Don't change addrsize here as it needs to be auto-detected by
    // X86EffAddr::checkea().
    if (m_opersize == 0)
        m_opersize = (m_mode_bits == 64 ? 32 : m_mode_bits);
}

YAML::Emitter&
operator<< (YAML::Emitter& out, const X86Common& common)
{
    out << YAML::Flow << YAML::BeginMap;
    out << YAML::Key << "addrsize";
    out << YAML::Value << static_cast<unsigned int>(common.m_addrsize);
    out << YAML::Key << "opersize";
    out << YAML::Value << static_cast<unsigned int>(common.m_opersize);
    out << YAML::Key << "lockrep" << YAML::Value;
    out << YAML::Hex << static_cast<unsigned int>(common.m_lockrep_pre);
    out << YAML::Key << "bits";
    out << YAML::Value << static_cast<unsigned int>(common.m_mode_bits);
    out << YAML::EndMap;
    return out;
}

unsigned long
X86Common::getLen() const
{
    unsigned long len = 0;

    if (m_addrsize != 0 && m_addrsize != m_mode_bits)
        len++;
    if ((m_mode_bits != 64 && m_opersize != m_mode_bits) ||
        (m_mode_bits == 64 && m_opersize == 16))
        len++;
    if (m_lockrep_pre != 0)
        len++;

    return len;
}

void
X86Common::ToBytes(Bytes& bytes, const X86SegmentRegister* segreg) const
{
    if (segreg != 0)
        Write8(bytes, segreg->getPrefix());
    if (m_addrsize != 0 && m_addrsize != m_mode_bits)
        Write8(bytes, 0x67);
    if ((m_mode_bits != 64 && m_opersize != m_mode_bits) ||
        (m_mode_bits == 64 && m_opersize == 16))
        Write8(bytes, 0x66);
    if (m_lockrep_pre != 0)
        Write8(bytes, m_lockrep_pre);
}

}}} // namespace yasm::arch::x86

//
// IdentifierInfo implementation.
//
//  Copyright (C) 2009  Peter Johnson
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
#include "yasmx/Parse/IdentifierTable.h"


void
IdentifierInfo::DoInsnLookup(const Arch& arch,
                             SourceLocation source,
                             Diagnostic& diags)
{
    if (m_flags & DID_INSN_LOOKUP)
        return;
    m_flags &= ~(IS_INSN | IS_PREFIX);
    Arch::InsnPrefix ip = arch.ParseCheckInsnPrefix(getName(), source, diags);
    switch (ip.getType())
    {
        case Arch::InsnPrefix::INSN:
            m_info = const_cast<Arch::InsnInfo*>(ip.getInsn());
            m_flags |= IS_INSN;
            break;
        case Arch::InsnPrefix::PREFIX:
            m_info = const_cast<Prefix*>(ip.getPrefix());
            m_flags |= IS_PREFIX;
            break;
        default:
            break;
    }
    m_flags |= DID_INSN_LOOKUP;
}

void
IdentifierInfo::DoRegLookup(const Arch& arch,
                            SourceLocation source,
                            Diagnostic& diags)
{
    if (m_flags & DID_REG_LOOKUP)
        return;
    m_flags &= ~(IS_REGISTER | IS_REGGROUP | IS_SEGREG | IS_TARGETMOD);
    Arch::RegTmod regtmod = arch.ParseCheckRegTmod(getName(), source, diags);
    switch (regtmod.getType())
    {
        case Arch::RegTmod::REG:
            m_info = const_cast<Register*>(regtmod.getReg());
            m_flags |= IS_REGISTER;
            break;
        case Arch::RegTmod::REGGROUP:
            m_info = const_cast<RegisterGroup*>(regtmod.getRegGroup());
            m_flags |= IS_REGGROUP;
            break;
        case Arch::RegTmod::SEGREG:
            m_info = const_cast<SegmentRegister*>(regtmod.getSegReg());
            m_flags |= IS_SEGREG;
            break;
        case Arch::RegTmod::TARGETMOD:
            m_info = const_cast<TargetModifier*>(regtmod.getTargetMod());
            m_flags |= IS_TARGETMOD;
            break;
        default:
            break;
    }
    m_flags |= DID_REG_LOOKUP;
}

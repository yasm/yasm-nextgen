//
// Registers
//
//  Copyright (C) 2005-2008  Peter Johnson
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
#include "register.h"

#include <ostream>

#include "arch.h"


namespace yasm
{

unsigned int
Register::get_size() const
{
    if (!m_arch)
        return 0;
    return m_arch->get_size(*this);
}

std::ostream&
operator<< (std::ostream& os, const Register& reg)
{
    if (reg.m_arch)
        reg.m_arch->put(os, reg);
    else
        os << "NONE";
    return os;
}

Register
RegisterGroup::get_reg(unsigned int regindex) const
{
    if (!m_arch)
        return no_reg();
    return m_arch->get_reg(*this, regindex);
}

std::ostream&
operator<< (std::ostream &os, const RegisterGroup &reggroup)
{
    if (reggroup.m_arch)
        reggroup.m_arch->put(os, reggroup);
    else
        os << "NONE";
    return os;
}

std::ostream&
operator<< (std::ostream &os, const SegmentRegister &segreg)
{
    if (segreg.m_arch)
        segreg.m_arch->put(os, segreg);
    else
        os << "NONE";
    return os;
}

} // namespace yasm

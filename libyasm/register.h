#ifndef YASM_REGISTER_H
#define YASM_REGISTER_H
///
/// @file libyasm/register.h
/// @brief YASM registers.
///
/// @license
///  Copyright (C) 2002-2008  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
///  - Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
///  - Redistributions in binary form must reproduce the above copyright
///    notice, this list of conditions and the following disclaimer in the
///    documentation and/or other materials provided with the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
/// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
/// @endlicense
///
#include <iosfwd>


namespace yasm
{

class Arch;

/// General-purpose or specialized register.
class Register
{
public:
    /// Get the equivalent size in bits.
    /// @return 0 if there is no suitable equivalent size, otherwise the size.
    unsigned int get_size() const;

    /// Determine if the register is a placeholder.
    /// @return True if register is a placeholder, false otherwise.
    bool empty() const { return m_arch == 0; }

    //@{
    /// Data must be public so this is a POD type (required for unions).
    /// But it should not be accessed outside of Arch, so m_ prefix it.
    const Arch* m_arch;
    unsigned int m_type:4;
    unsigned int m_num:12;
    //@}
};

/// Get a register placeholder for no register.
inline Register
no_reg()
{
    Register reg;
    reg.m_arch = 0;
    reg.m_type = 0;
    reg.m_num = 0;
    return reg;
}

/// Print a register.  For debugging purposes.
/// @param os   output stream
/// @param reg  register
std::ostream& operator<< (std::ostream& os, const Register& reg);

/// Group of registers accessible by index instead of just name.
class RegisterGroup
{
public:
    /// Get a specific register of a register group, based on the register
    /// group and the index within the group.
    /// @param regindex     register index
    /// @return Specific register.
    /// @exception ValueError invalid index for group
    Register get_reg(unsigned int regindex) const;

    /// Determine if the register group is a placeholder.
    /// @return True if register group is a placeholder, false otherwise.
    bool empty() const { return m_arch == 0; }

    //@{
    /// Data must be public so this is a POD type (required for unions).
    /// But it should not be accessed outside of Arch, so m_ prefix it.
    const Arch* m_arch;
    unsigned int m_type;
    //@}
};

/// Get a register group placeholder for no register group.
inline RegisterGroup
no_reggroup()
{
    RegisterGroup reggroup;
    reggroup.m_arch = 0;
    reggroup.m_type = 0;
    return reggroup;
}

/// Print a register group.  For debugging purposes.
/// @param os   output stream
std::ostream& operator<< (std::ostream &os, const RegisterGroup &reggroup);

/// Segment register.
class SegmentRegister
{
public:
    /// Determine if the segment register is a placeholder.
    /// @return True if segment register is a placeholder, false otherwise.
    bool empty() const { return m_arch == 0; }

    //@{
    /// Data must be public so this is a POD type (required for unions).
    /// But it should not be accessed outside of Arch, so m_ prefix it.
    const Arch* m_arch;
    unsigned int m_num;
    //@}
};

/// Get a segment register placeholder for no segment register.
inline SegmentRegister
no_segreg()
{
    SegmentRegister segreg;
    segreg.m_arch = 0;
    segreg.m_num = 0;
    return segreg;
}

/// Print a segment register.  For debugging purposes.
/// @param os   output stream
std::ostream& operator<< (std::ostream &os, const SegmentRegister &segreg);

} // namespace yasm

#endif

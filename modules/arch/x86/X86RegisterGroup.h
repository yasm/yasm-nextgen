#ifndef YASM_X86REGISTERGROUP_H
#define YASM_X86REGISTERGROUP_H
//
// x86 register group header file
//
//  Copyright (C) 2001-2008  Peter Johnson
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
#include "yasmx/Arch.h"

#include "X86Register.h"


namespace yasm
{
namespace arch
{

class YASM_STD_EXPORT X86RegisterGroup : public RegisterGroup
{
public:
    X86RegisterGroup(X86Register** regs, unsigned long size)
        : m_regs(regs), m_size(size) {}
    ~X86RegisterGroup() {}

    /// Get a specific register of a register group, based on the register
    /// group and the index within the group.
    /// @param regindex     register index
    /// @return 0 if regindex is not valid for that register group,
    ///         otherwise the specific register.
    const X86Register* getReg(unsigned long regindex) const;

    /// Write a YAML representation.  For debugging purposes.
    /// @param out          YAML emitter
    void Write(YAML::Emitter& out) const;

private:
    X86Register** m_regs;
    unsigned long m_size;
};

}} // namespace yasm::arch

#endif

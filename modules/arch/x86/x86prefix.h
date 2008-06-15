#ifndef YASM_X86PREFIX_H
#define YASM_X86PREFIX_H
//
// x86 prefix header file
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
#include <libyasmx/insn.h>

namespace yasm
{
namespace arch
{
namespace x86
{

class X86Prefix : public Insn::Prefix
{
public:
    // Low 8 bits are used for the prefix value, stored in same data area.
    enum Type
    {
        LOCKREP = 1<<8,
        ADDRSIZE = 2<<8,
        OPERSIZE = 3<<8,
        SEGREG = 4<<8,
        REX = 5<<8
    };

    X86Prefix(Type type, unsigned char value);
    ~X86Prefix();
    void put(std::ostream& os) const;

    Type get_type() const { return m_type; }
    unsigned char get_value() const { return m_value; }

private:
    Type m_type;
    unsigned char m_value;
};

}}} // namespace yasm::arch::x86

#endif

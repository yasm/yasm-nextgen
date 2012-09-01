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
#include "yasmx/Config/export.h"
#include "yasmx/Insn.h"

namespace yasm
{
namespace arch
{

class YASM_STD_EXPORT X86Prefix : public Prefix
{
public:
    // Low 8 bits are used for the prefix value, stored in same data area.
    enum Type
    {
        LOCKREP = 1<<8,
        ADDRSIZE = 2<<8,
        OPERSIZE = 3<<8,
        SEGREG = 4<<8,
        REX = 5<<8,
        ACQREL = 6<<8       // TSX hint prefixes
    };

    X86Prefix(Type type, unsigned char value);
    ~X86Prefix();
    void Put(raw_ostream& os) const;
#ifdef WITH_XML
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    Type getType() const { return m_type; }
    unsigned char getValue() const { return m_value; }

    bool is(Type type) const { return m_type == type; }
    bool isNot(Type type) const { return m_type != type; }

private:
    Type m_type;
    unsigned char m_value;
};

}} // namespace yasm::arch

#endif

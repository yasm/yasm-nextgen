#ifndef YASM_X86OPCODE_H
#define YASM_X86OPCODE_H
//
// x86 core bytecode header file
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
#include <libyasmx/marg_ostream_fwd.h>

namespace yasm
{

class Bytes;

namespace arch
{
namespace x86
{

class X86Opcode
{
    friend marg_ostream& operator<< (marg_ostream& os,
                                     const X86Opcode& opcode);

public:
    X86Opcode() : m_len(0) {}

    X86Opcode(unsigned int len, const unsigned char* opcode)
        : m_len(len)
    {
        if (opcode)
        {
            m_opcode[0] = opcode[0];
            m_opcode[1] = opcode[1];
            m_opcode[2] = opcode[2];
        }
    }

    void to_bytes(Bytes& bytes) const;

    /// Switch to the "alternate" one-byte opcode.  Some optimizations
    /// store two opcodes in the three bytes of opcode storage available;
    /// one or two bytes of "primary" opcode, followed by one byte of
    /// "alternate" opcode.  The primary opcode is nominally the active one.
    /// This function makes the alternate opcode the active one by copying
    /// over the first opcode and setting the opcode length to 1.
    void make_alt_1();

    /// Switch to the "alternate" two-byte opcode.  Copies the last two
    /// opcode bytes to the first two, and sets the length to 2.
    /// This is primarily used for VEX opcodes.
    void make_alt_2();

    bool is_empty() const { return m_len == 0; }
    unsigned int get_len() const { return m_len; }

    void add(int byte, unsigned char val) { m_opcode[byte] += val; }
    void mask(int byte, unsigned char mask) { m_opcode[byte] &= mask; }
    void merge(int byte, unsigned char val) { m_opcode[byte] |= val; }

    unsigned char get(int byte) { return m_opcode[byte]; }

private:
    unsigned char m_opcode[3];      // opcode
    unsigned char m_len;
};

marg_ostream& operator<< (marg_ostream& os, const X86Opcode& opcode);

}}} // namespace yasm::arch::x86

#endif

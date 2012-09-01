#ifndef YASM_X86REGISTER_H
#define YASM_X86REGISTER_H
//
// x86 register header file
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
#include "yasmx/Arch.h"

namespace yasm
{
namespace arch
{

class YASM_STD_EXPORT X86Register : public Register
{
public:
    // Register type.
    enum Type
    {
        REG8 = 0,
        REG8X,      // 64-bit mode only, REX prefix version of REG8
        REG16,
        REG32,
        REG64,      // 64-bit mode only
        FPUREG,
        MMXREG,
        XMMREG,
        YMMREG,
        CRREG,
        DRREG,
        TRREG,
        RIP,        // 64-bit mode only, always RIP (reg num ignored)
        TYPE_COUNT  // Number of types, must always be last in enum
    };

    X86Register(Type type, unsigned int num) : m_type(type), m_num(num) {}
    ~X86Register();

    /// Get the equivalent size of a register in bits.
    /// @param reg  register
    /// @return 0 if there is no suitable equivalent size, otherwise the
    ///         size.
    unsigned int getSize() const;

    /// Print a register.  For debugging purposes.
    /// @param os   output stream
    void Put(raw_ostream& os) const;

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    Type getType() const { return m_type; }
    unsigned int getNum() const { return m_num; }

    bool is(Type type) const { return m_type == type; }
    bool isNot(Type type) const { return m_type != type; }

private:
    // Register type.
    Type m_type;

    // Register number.
    // Note 8-15 are only valid for some registers, and only in 64-bit mode.
    unsigned int m_num;
};

class YASM_STD_EXPORT X86SegmentRegister : public SegmentRegister
{
public:
    enum Type
    {
        kES = 0,
        kCS,
        kSS,
        kDS,
        kFS,
        kGS,
        TYPE_COUNT
    };

    X86SegmentRegister(Type type, unsigned char prefix)
        : m_type(type), m_prefix(prefix) {}
    ~X86SegmentRegister();

    /// Print a segment register.  For debugging purposes.
    /// @param os   output stream
    void Put(raw_ostream& os) const;

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif

    Type getType() const { return m_type; }
    unsigned int getNum() const { return static_cast<unsigned int>(m_type); }
    unsigned char getPrefix() const { return m_prefix; }

    bool is(Type type) const { return m_type == type; }
    bool isNot(Type type) const { return m_type != type; }

private:
    Type m_type;
    unsigned char m_prefix;
};

}} // namespace yasm::arch

#endif

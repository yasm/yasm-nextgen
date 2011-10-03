#ifndef YASM_X86EFFADDR_H
#define YASM_X86EFFADDR_H
//
// x86 effective address header file
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
#include "yasmx/EffAddr.h"

#include "X86Register.h"

namespace yasm
{

class DiagnosticsEngine;

namespace arch
{

enum X86RexBitPos
{
    X86_REX_W = 3,
    X86_REX_R = 2,
    X86_REX_X = 1,
    X86_REX_B = 0
};

/// Sets REX (4th bit) and 3 LS bits from register size/number.  Will not
/// modify REX if not in 64-bit mode or if it wasn't needed to express reg.
/// @param rexbit   indicates bit of REX to use if REX is needed
/// @return True if successful, false if invalid mix of register and REX
///         (diag::err_high8_rex_conflict should be generated).
YASM_STD_EXPORT
bool setRexFromReg(unsigned char* rex,
                   unsigned char* low3,
                   X86Register::Type reg_type,
                   unsigned int reg_num,
                   unsigned int bits,
                   X86RexBitPos rexbit);

inline bool
setRexFromReg(unsigned char *rex,
              unsigned char *low3,
              const X86Register* reg,
              unsigned int bits,
              X86RexBitPos rexbit)
{
    return setRexFromReg(rex, low3, reg->getType(), reg->getNum(), bits,
                         rexbit);
}

// Effective address type
class YASM_STD_EXPORT X86EffAddr : public EffAddr
{
public:
    // How the spare (register) bits in Mod/RM are handled:
    // Even if valid_modrm=0, the spare bits are still valid (don't overwrite!)
    // They're set in bytecode_create_insn().
    unsigned char m_modrm;
    unsigned char m_sib;

    // 1 if SIB byte needed, 0 if not, 0xff if unknown
    unsigned char m_need_sib;

    // VSIB uses the normal SIB byte, but this flag enables it.
    unsigned char m_vsib_mode;  // 0 if not, 1 if XMM, 2 if YMM

    bool m_valid_modrm:1;   // 1 if Mod/RM byte currently valid, 0 if not
    bool m_need_modrm:1;    // 1 if Mod/RM byte needed, 0 if not
    bool m_valid_sib:1;     // 1 if SIB byte currently valid, 0 if not

    /// Basic constructor.
    X86EffAddr();

    /// Expression constructor.
    /// @param xform_rip_plus   Transform foo+rip into foo wrt rip; used
    ///                         for GAS parser
    /// @param e                Expression
    X86EffAddr(bool xform_rip_plus, std::auto_ptr<Expr> e);

    /// Register setter.
    /// @return True if successful, false if invalid mix of register and REX
    ///         (diag::err_high8_rex_conflict should be generated).
    bool setReg(const X86Register* reg, unsigned char* rex, unsigned int bits);

    /// Immediate setter.
    void setImm(std::auto_ptr<Expr> imm, unsigned int im_len);

    /// Finalize the EA displacement and init the spare field.
    void Init(unsigned int spare);

    /// Make the EA only a displacement.
    void setDispOnly();

    X86EffAddr* clone() const;

#ifdef WITH_XML
    pugi::xml_node DoWrite(pugi::xml_node out) const;
#endif // WITH_XML

    // Check an effective address.  Returns true if EA was successfully
    // determined, false if indeterminate EA.
    bool Check(unsigned char* addrsize,
               unsigned int bits,
               bool address16_op,
               unsigned char* rex,
               bool* ip_rel,
               DiagnosticsEngine& diags);

    /// Finalize the effective address.
    bool Finalize(DiagnosticsEngine& diags);

private:
    /// Copy constructor.
    X86EffAddr(const X86EffAddr& rhs);

    // Calculate the displacement length, if possible.
    // Takes several extra inputs so it can be used by both 32-bit and 16-bit
    // expressions:
    //  wordsize=16 for 16-bit, =32 for 32-bit.
    //  noreg=true if the *ModRM byte* has no registers used.
    //  dispreq=true if a displacement value is *required* (even if =0).
    // Returns false if not successfully calculated.
    bool CalcDispLen(unsigned int wordsize,
                     bool noreg,
                     bool dispreq,
                     DiagnosticsEngine& diags);

    bool Check3264(unsigned int addrsize,
                   unsigned int bits,
                   unsigned char* rex,
                   bool* ip_rel,
                   DiagnosticsEngine& diags);
    bool Check16(unsigned int bits,
                 bool address16_op,
                 bool* ip_rel,
                 DiagnosticsEngine& diags);
};

}} // namespace yasm::arch

#endif

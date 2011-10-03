#ifndef YASM_DWARFCFI_H
#define YASM_DWARFCFI_H
//
// DWARF Call Frame Information
//
//  Copyright (C) 2010-2011  Peter Johnson
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
#include "yasmx/Support/ptr_vector.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/Location.h"

namespace yasm
{
class Arch;
class BytecodeContainer;
class Directives;
class DirectiveInfo;
class NameValue;
class Object;
class ObjectFormat;

namespace dbgfmt
{
class DwarfCfiFde;
class DwarfDebug;

class YASM_STD_EXPORT DwarfCfiOutput
{
public:
    DwarfCfiOutput(BytecodeContainer& container_,
                   DiagnosticsEngine& diags_,
                   const DwarfDebug& debug_,
                   Object& object_,
                   bool eh_frame_)
        : container(container_)
        , diags(diags_)
        , debug(debug_)
        , object(object_)
        , eh_frame(eh_frame_)
    {}

    BytecodeContainer& container;
    DiagnosticsEngine& diags;
    const DwarfDebug& debug;
    Object& object;
    bool eh_frame;
};

class YASM_STD_EXPORT DwarfCfiInsn
{
public:
    ~DwarfCfiInsn();

    void setSource(SourceLocation source) { m_source = source; }
    void Output(DwarfCfiOutput& out);

    static DwarfCfiInsn* MakeAdvanceLoc(Location from, Location to);
    static DwarfCfiInsn* MakeOffset(unsigned int reg, const IntNum& off);
    static DwarfCfiInsn* MakeRestore(unsigned int reg);
    static DwarfCfiInsn* MakeUndefined(unsigned int reg);
    static DwarfCfiInsn* MakeSameValue(unsigned int reg);
    static DwarfCfiInsn* MakeRegister(unsigned int reg1, unsigned int reg2);
    static DwarfCfiInsn* MakeRememberState()
    { return new DwarfCfiInsn(DW_CFA_remember_state); }
    static DwarfCfiInsn* MakeRestoreState()
    { return new DwarfCfiInsn(DW_CFA_restore_state); }
    static DwarfCfiInsn* MakeDefCfa(unsigned int reg, const IntNum& off);
    static DwarfCfiInsn* MakeDefCfaRegister(unsigned int reg);
    static DwarfCfiInsn* MakeDefCfaOffset(const IntNum& off)
    { return new DwarfCfiInsn(DW_CFA_def_cfa_offset, off); }
    static DwarfCfiInsn* MakeGNUWindowSave()
    { return new DwarfCfiInsn(DW_CFA_GNU_window_save); }

    static DwarfCfiInsn* MakeEscape(std::vector<Expr>* esc);
    static DwarfCfiInsn* MakeValEncodedAddr(unsigned int reg,
                                          unsigned int encoding,
                                          Expr e);

    enum Op
    {
        DW_CFA_advance_loc = 0x40,  // low 6 bits: delta
        DW_CFA_offset = 0x80,       // low 6 bits: register, op1: ULEB128 off
        DW_CFA_restore = 0x70,      // low 6 bits: register
        DW_CFA_nop = 0,
        DW_CFA_set_loc,             // op1: address
        DW_CFA_advance_loc1,        // op1: 1-byte delta
        DW_CFA_advance_loc2,        // op1: 2-byte delta
        DW_CFA_advance_loc4,        // op1: 4-byte delta
        DW_CFA_offset_extended,     // op1: ULEB128 register, op2: ULEB128 off
        DW_CFA_restore_extended,    // op1: ULEB128 register
        DW_CFA_undefined,           // op1: ULEB128 register
        DW_CFA_same_value,          // op1: ULEB128 register
        DW_CFA_register,            // op1: ULEB128 register, op2: ULEB128 reg
        DW_CFA_remember_state,
        DW_CFA_restore_state,
        DW_CFA_def_cfa,             // op1: ULEB128 register, op2: ULEB128 off
        DW_CFA_def_cfa_register,    // op1: ULEB128 register
        DW_CFA_def_cfa_offset,      // op1: ULEB128 offset
        // DWARF 3
        DW_CFA_def_cfa_expression,
        DW_CFA_expression,          // op1: ULEB128 register, op2: DW_FORM expr
        DW_CFA_offset_extended_sf,  // op1: ULEB128 register, op2: SLEB128 off
        DW_CFA_def_cfa_sf,          // op1: ULEB128 register, op2: SLEB128 off
        DW_CFA_def_cfa_offset_sf,   // op1: SLEB128 offset
        DW_CFA_val_offset,
        DW_CFA_val_offset_sf,
        DW_CFA_val_expression,
        DW_CFA_GNU_window_save = 0x2d,
        DW_CFA_GNU_args_size,       // op1: ULEB128 size
        CFI_escape = 0x100,         // escape
        CFI_val_encoded_addr        // val encoded addr
    };

    Op getOp() const { return m_op; }

    bool operator== (const DwarfCfiInsn& oth) const;
    bool operator!= (const DwarfCfiInsn& oth) const { return !(*this == oth); }

private:
    DwarfCfiInsn(Op op);
    DwarfCfiInsn(Op op, const IntNum& off);

    Op m_op;
    Location m_from, m_to;
    unsigned int m_regs[2];
    std::vector<Expr> m_esc;
    IntNum m_off;
    SourceLocation m_source;
};

class YASM_STD_EXPORT DwarfCfiCie
{
public:
    DwarfCfiCie(DwarfCfiFde* fde);

    void Output(DwarfCfiOutput& out, unsigned int align);

    DwarfCfiFde* m_fde;
    SymbolRef m_start;
    size_t m_num_insns;
};

class YASM_STD_EXPORT DwarfCfiFde
{
public:
    DwarfCfiFde(const DwarfDebug& debug,
                Location start,
                SourceLocation source);
    ~DwarfCfiFde();

    void Output(DwarfCfiOutput& out, DwarfCfiCie& cie, unsigned int align);
    void Close(Location end) { m_end = end; }

    SourceLocation m_source;
    Location m_start;
    Location m_end;
    stdx::ptr_vector<DwarfCfiInsn> m_insns;
    stdx::ptr_vector_owner<DwarfCfiInsn> m_insns_owner;
    Expr m_personality;
    Expr m_lsda;
    SourceLocation m_personality_source;
    SourceLocation m_lsda_source;
    unsigned char m_personality_encoding;
    unsigned char m_lsda_encoding;
    unsigned int m_return_column;
    bool m_signal_frame;
};

}} // namespace yasm::dbgfmt

#endif

/**
 * \file libyasm/insn.h
 * \brief YASM mnenomic instruction.
 *
 * \license
 *  Copyright (C) 2002-2007  Peter Johnson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * \endlicense
 */
#ifndef YASM_INSN_H
#define YASM_INSN_H

#include <vector>
#include <iostream>

#include "bytecode.h"

namespace yasm {

class Expr;
class Register;
class SegmentRegister;

class EffAddr;
#if 0
/** Base class for an effective address.
 * Architectures should derive their own implementation from this.
 */
class EffAddr {
public:
    /** Set segment override for an effective address.
     * Some architectures (such as x86) support segment overrides on effective
     * addresses.  A override of an override will result in a warning.
     * \param segreg        segment register (0 if none)
     */
    void set_segreg(const SegmentRegister* segreg);

    Value disp;         /**< address displacement */

    /** Segment register override (0 if none). */
    const SegmentRegister* segreg;

    /** 1 if length of disp must be >0. */
    unsigned int need_nonzero_len:1;

    /** 1 if a displacement should be present in the output. */
    unsigned int need_disp:1;

    /** 1 if reg*2 should not be split into reg+reg. (0 if not).
     * This flag indicates (for architectures that support complex effective
     * addresses such as x86) if various types of complex effective addresses
     * can be split into different forms in order to minimize instruction
     * length.
     */
    unsigned int nosplit:1;

    /** 1 if effective address is /definitely/ an effective address.
     * This is used in e.g. the GAS parser to differentiate
     * between "expr" (which might or might not be an effective address) and
     * "expr(,1)" (which is definitely an effective address).
     */
    unsigned int strong:1;
};
#endif

/** Base class for "instruction" bytecodes.  These are the mnenomic
 * (rather than raw) representation of instructions.  Architectures should
 * derive their own implementation from this.
 */
struct Insn : public Bytecode::Contents {
public:
    /** An instruction operand. */
    class Operand {
    public:
        /** Base class for target modifiers. */
        class TargetModifier {
            TargetModifier() {}
            virtual ~TargetModifier() {}
        };

        /** Create an instruction operand from a register.
         * \param reg   register
         */
        Operand(const Register* reg);

        /** Create an instruction operand from a segment register.
         * \param segreg        segment register
         */
        Operand(const SegmentRegister* segreg);

        /** Create an instruction operand from an effective address.
         * \param ea    effective address
         */
        Operand(/*@only@*/ EffAddr* ea);

        /** Create an instruction operand from an immediate expression.
         * Looks for cases of a single register and creates a register variant of
         * #yasm_insn_operand.
         * \param val   immediate expression
         * \return Newly allocated operand.
         */
        Operand(/*@only@*/ Expr* val);

        /** Destructor. */
        ~Operand();

        void put(std::ostream& os, int indent_level) const;
        void finalize();
    private:
        /** Operand type. */
        enum yasm_insn_operand_type {
            REG = 1,    /**< A register. */
            SEGREG,     /**< A segment register. */
            MEMORY,     /**< An effective address (memory reference). */
            IMM         /**< An immediate or jump target. */
        } m_type;

        /** Operand data. */
        union {
            const SegmentRegister* segreg;  /**< Segment register. */
            const Register* reg;            /**< Register. */
            EffAddr* ea;    /**< Effective address for memory references. */
            Expr* val;      /**< Value of immediate or jump target. */
        } m_data;

        const TargetModifier* m_targetmod;  /**< Arch target modifier, 0 if none. */

        /** Specified size of the operand, in bits.  0 if not user-specified. */
        unsigned int m_size:16;

        /** Nonzero if dereference.  Used for "*foo" in GAS.
         * The reason for this is that by default in GAS, an unprefixed value
         * is a memory address, except for jumps/calls, in which case it needs a
         * "*" prefix to become a memory address (otherwise it's an immediate).
         * This isn't knowable in the parser stage, so the parser sets this flag
         * to indicate the "*" prefix has been used, and the arch needs to adjust
         * the operand type appropriately depending on the instruction type.
         */
        unsigned int m_deref:1;

        /** Nonzero if strict.  Used for "strict foo" in NASM.
         * This is used to inhibit optimization on otherwise "sized" values.
         * For example, the user may just want to be explicit with the size on
         * "push dword 4", but not actually want to force the immediate size to
         * 4 bytes (rather wanting the optimizer to optimize it down to 1 byte as
         * though "dword" was not specified).  To indicate the immediate should
         * actually be forced to 4 bytes, the user needs to write
         * "push strict dword 4", which sets this flag.
         */
        unsigned int m_strict:1;
    };

    /** Base class for instruction prefixes. */
    class Prefix {
        Prefix() {}
        virtual ~Prefix() {}
    };

    Insn() {}
    virtual ~Insn() {}

    /** Add operand to the end of an instruction.
     * \param op            operand
     */
    void add_operand(const Operand& op)
    { m_operands.push_back(op); }

    /** Associate a prefix with an instruction.
     * \param prefix        data that identifies the prefix
     */
    void add_prefix(const Prefix* prefix)
    { m_prefixes.push_back(prefix); }

    /** Associate a segment prefix with an instruction.
     * \param segreg        data that identifies the segment register
     */
    void add_seg_prefix(const SegmentRegister* segreg)
    { m_segregs.push_back(segreg); }

    /** Print a list of instruction operands.  For debugging purposes.
     * \param os            output stream
     * \param indent_level  indentation level
     */
    virtual void put(std::ostream& os, int indent_level) const = 0;

    /** Finalize the common parts of an instruction. */
    virtual void finalize() = 0;

protected:
    /** Operands. */
    std::vector<Operand> m_operands;

    /** Prefixes. */
    std::vector<const Prefix*> m_prefixes;

    /** Array of segment prefixes. */
    std::vector<const SegmentRegister*> m_segregs;
};

} // namespace yasm

#endif

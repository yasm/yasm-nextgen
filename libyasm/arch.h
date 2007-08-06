#ifndef YASM_ARCH_H
#define YASM_ARCH_H
///
/// @file libyasm/arch.h
/// @brief YASM architecture interface.
///
/// @license
///  Copyright (C) 2002-2007  Peter Johnson
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
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include <boost/noncopyable.hpp>

#include "insn.h"


namespace yasm {

class Expr;
class FloatNum;
class IntNum;

class Register {
public:
    /// Get the equivalent size of a register in bits.
    /// @param reg  register
    /// @return 0 if there is no suitable equivalent size, otherwise the
    ///         size.
    virtual unsigned int get_size() const = 0;

    /// Print a register.  For debugging purposes.
    /// @param os   output stream
    virtual void put(std::ostream& os) const = 0;
};

class RegisterGroup {
public:
    /// Get a specific register of a register group, based on the register
    /// group and the index within the group.
    /// @param regindex     register index
    /// @return 0 if regindex is not valid for that register group,
    ///         otherwise the specific register.
    virtual const Register* get_reg(unsigned long regindex) const = 0;
};

class SegmentRegister {
    /// Print a segment register.  For debugging purposes.
    /// @param os   output stream
    virtual void put(std::ostream& os) const = 0;
};

/// Architecture interface.
class Arch : private boost::noncopyable {
public:
    /// Return value for parse_check_insnprefix().
    struct InsnPrefix {
        Bytecode* bc;
        const Insn::Prefix* prefix;
    };

    /// Return value for parse_check_regtmod().
    struct RegTmod {
        const Register* reg;
        const RegisterGroup* reggroup;
        const SegmentRegister* segreg;
        const Insn::Operand::TargetModifier* tmod;
    };
#if 0
    /** NULL-terminated list of directives.  NULL if none. */
    /*@null@*/ const yasm_directive *directives;
#endif
    /// Constructor.
    /// To make architecture truly usable, set_machine() and set_parser()
    /// need to be called.
    Arch();

    /// Destructor.
    virtual ~Arch() {}

    /// Set parser to use.
    /// @param parser       keyword of parser to use
    /// @return False if unrecognized parser.
    virtual bool set_parser(const std::string& parser) = 0;

    /// Get the one-line description of an architecture.
    /// @return One-line description of architecture.
    virtual std::string get_name() const = 0;

    /// Get the keyword used to select an architecture.
    /// @return Architecture keyword.
    virtual std::string get_keyword() const = 0;

    /// Get the word size of an architecture.
    /// @return Word size (in bits).
    virtual unsigned int get_wordsize() const = 0;

    /// Get the minimum instruction length of an architecture.
    /// @return Minimum instruction length (in bytes).
    virtual unsigned int get_min_insn_len() const = 0;

    /// Set active machine.
    /// @param machine      keyword of machine to use; must be one in the
    ///                     list returned by get_machines().
    /// @return False if unrecognized machine.
    virtual bool set_machine(const std::string& machine) = 0;

    /// Get architecture's active machine name.
    /// @return Active machine name.
    virtual std::string get_machine() const = 0;

    /// Get available machines.
    /// A number of different machine types may be associated with a single
    /// architecture.  These may be specific CPU's, but the ABI used to
    /// interface with the architecture should be the primary differentiator
    /// between machines.  Some object formats (ELF) use the machine to
    /// determine parameters within the generated output.
    /// @return Map of all machines.  The key is the keyword used to select
    ///         the machine with set_machine(), and the value is a one-line
    ///         description of the machine.
    virtual std::map<std::string, std::string> get_machines() const = 0;

    /// Get architecture's active address size, in bits.
    /// @return Active address size (in bits).
    virtual unsigned int get_address_size() const = 0;

    /// Set any arch-specific variables.  For example, "mode_bits" in x86.
    /// @param var  variable name
    /// @param val  value to set
    /// @return False on success, true on failure (variable does not exist).
    virtual bool set_var(const std::string& var, unsigned long val) = 0;

    /// Check an generic identifier to see if it matches architecture
    /// specific names for instructions or instruction prefixes.
    /// Unrecognized identifiers should return #NOTINSNPREFIX so they can be
    /// treated as normal symbols.
    /// @param id           identifier as in the input file
    /// @param id_len       length of id string
    /// @param line         virtual line
    /// @param bc           for instructions, yasm_insn-based bytecode is
    ///                     returned (and NULL otherwise)
    /// @param prefix       for prefixes, yasm_arch-specific value is
    ///                     returned (and 0 otherwise)
    /// @return Identifier type (#NOTINSNPREFIX if unrecognized)
    virtual InsnPrefix parse_check_insnprefix
        (const char *id, size_t id_len, unsigned long line) = 0;

    /// Check an generic identifier to see if it matches architecture
    /// specific names for registers or target modifiers.  Unrecognized
    /// identifiers should return #NOTREGTMOD.
    /// @param id           identifier as in the input file
    /// @param id_len       length of id string
    /// @return Identifier type (#NOTREGTMOD if unrecognized)
    virtual RegTmod parse_check_regtmod(const char *id, size_t id_len) = 0;

    /// Get NOP fill patterns for 1-15 bytes of fill.
    /// @return 16-entry array of arrays; [0] is unused,
    ///         [1] - [15] point to arrays of 1-15 bytes (respectively)
    ///         in length.
    virtual const unsigned char** get_fill() const = 0;

    /// Output #FloatNum to buffer.  Puts the value into the least
    /// significant bits of the destination, or may be shifted into more
    /// significant bits by the shift parameter.  The destination bits are
    /// cleared before being set.
    /// Architecture-specific because of endianness.
    /// @param flt           floating point value
    /// @param buf           buffer to write into
    /// @param destsize      destination size (in bytes)
    /// @param valsize       size (in bits)
    /// @param shift         left shift (in bits)
    /// @param warn          enables standard overflow/underflow warnings
    virtual void floatnum_tobytes(const FloatNum& flt,
                                  unsigned char* buf, size_t destsize,
                                  size_t valsize, size_t shift,
                                  int warn) const = 0;

    /// Output #IntNum to buffer.  Puts the value into the least
    /// significant bits of the destination, or may be shifted into more
    /// significant bits by the shift parameter.  The destination bits are
    /// cleared before being set.
    /// @param intn         integer value
    /// @param buf          buffer to write into
    /// @param destsize     destination size (in bytes)
    /// @param valsize      size (in bits)
    /// @param shift        left shift (in bits); may be negative to specify
    ///                     right shift (standard warnings include
    ///                     truncation to boundary)
    /// @param bc           bytecode being output ("parent" of value)
    /// @param warn         enables standard warnings (value doesn't fit into
    ///                     valsize bits)
    virtual void intnum_tobytes(const IntNum& intn,
                                unsigned char* buf, size_t destsize,
                                size_t valsize, int shift,
                                const Bytecode& bc, int warn) const = 0;

    /// Create an effective address from an expression.
    /// @param e    expression
    /// @return Newly allocated effective address.
    virtual std::auto_ptr<EffAddr> ea_create(std::auto_ptr<Expr> e) = 0;

    /// Create a bytecode that represents a single empty (0 length)
    /// instruction.  This is used for handling solitary prefixes.
    /// @param line         virtual line (from yasm_linemap)
    /// @return Newly allocated bytecode.
    virtual std::auto_ptr<Bytecode>
        create_empty_insn(unsigned long line) const = 0;
};

} // namespace yasm

#endif

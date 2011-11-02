#ifndef YASM_ARCH_H
#define YASM_ARCH_H
///
/// @file
/// @brief Architecture interface.
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
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "llvm/ADT/StringRef.h"
#include "yasmx/Config/export.h"
#include "yasmx/DebugDumper.h"
#include "yasmx/Location.h"
#include "yasmx/Module.h"


namespace llvm { class APFloat; class raw_ostream; }

namespace yasm
{

class ArchModule;
class BytecodeContainer;
class Bytes;
class Diagnostic;
class Directives;
class EffAddr;
class Expr;
class IntNum;
class Insn;
class ParserImpl;
class Prefix;
class Preprocessor;
class SourceLocation;
class TargetModifier;
class Token;

/// A register.
class YASM_LIB_EXPORT Register
{
public:
    Register() {}
    virtual ~Register();

    /// Get the equivalent size of a register in bits.
    /// @param reg  register
    /// @return 0 if there is no suitable equivalent size, otherwise the
    ///         size.
    virtual unsigned int getSize() const = 0;

    /// Get the register number.  This is typically the binary encoding of
    /// the register used in the instruction encoding.
    virtual unsigned int getNum() const = 0;

    /// Print the register name.
    /// @param os   output stream
    virtual void Put(llvm::raw_ostream& os) const = 0;

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    virtual pugi::xml_node Write(pugi::xml_node out) const = 0;
#endif // WITH_XML

private:
    Register(const Register&);                  // not implemented
    const Register& operator=(const Register&); // not implemented
};

inline llvm::raw_ostream& operator<<
(llvm::raw_ostream& os, const Register& reg)
{
    reg.Put(os);
    return os;
}

/// A group of registers.
class YASM_LIB_EXPORT RegisterGroup
{
public:
    RegisterGroup() {}
    virtual ~RegisterGroup();

    /// Get a specific register of a register group, based on the register
    /// group and the index within the group.
    /// @param regindex     register index
    /// @return 0 if regindex is not valid for that register group,
    ///         otherwise the specific register.
    virtual const Register* getReg(unsigned long regindex) const = 0;

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    virtual pugi::xml_node Write(pugi::xml_node out) const = 0;
#endif // WITH_XML

private:
    RegisterGroup(const RegisterGroup&);                  // not implemented
    const RegisterGroup& operator=(const RegisterGroup&); // not implemented
};

/// A segment register.
class YASM_LIB_EXPORT SegmentRegister
{
public:
    SegmentRegister() {}
    virtual ~SegmentRegister();

    /// Get the register number.  This is typically the binary encoding of
    /// the register used in the instruction encoding.
    virtual unsigned int getNum() const = 0;

    /// Print a segment register.  For debugging purposes.
    /// @param os   output stream
    virtual void Put(llvm::raw_ostream& os) const = 0;

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    virtual pugi::xml_node Write(pugi::xml_node out) const = 0;
#endif // WITH_XML

private:
    SegmentRegister(const SegmentRegister&);                  // not implemented
    const SegmentRegister& operator=(const SegmentRegister&); // not implemented
};

inline llvm::raw_ostream& operator<<
(llvm::raw_ostream& os, const SegmentRegister& segreg)
{
    segreg.Put(os);
    return os;
}

/// Architecture interface.
/// To make architecture truly usable, setMachine() and setParser()
/// need to be called.
class YASM_LIB_EXPORT Arch
{
public:
    /// Opaque class to be used as a ParseCheckInsnPrefix() return value
    /// for instructions.
    class InsnInfo;

    /// Return value for ParseCheckInsnPrefix().
    class YASM_LIB_EXPORT InsnPrefix
    {
    public:
        enum Type
        {
            NONE,
            INSN,
            PREFIX
        };

        InsnPrefix() : m_type(NONE) {}
        InsnPrefix(const InsnInfo* insn)
            : m_type(INSN), m_insn(insn)
        {}
        InsnPrefix(const Prefix* prefix)
            : m_type(PREFIX), m_prefix(prefix)
        {}

        Type getType() const { return m_type; }
        bool isType(Type type) const { return m_type == type; }

        const InsnInfo* getInsn() const
        {
            return (m_type == INSN ? m_insn : 0);
        }
        const Prefix* getPrefix() const
        {
            return (m_type == PREFIX ? m_prefix : 0);
        }
    private:
        Type m_type;
        union
        {
            const InsnInfo* m_insn;
            const Prefix* m_prefix;
        };
    };

    /// Return value for ParseCheckRegTmod().
    class YASM_LIB_EXPORT RegTmod
    {
    public:
        enum Type
        {
            NONE,
            REG,
            REGGROUP,
            SEGREG,
            TARGETMOD
        };

        RegTmod() : m_type(NONE) {}
        RegTmod(const Register* reg)
            : m_type(REG), m_reg(reg)
        {}
        RegTmod(const RegisterGroup* reggroup)
            : m_type(REGGROUP), m_reggroup(reggroup)
        {}
        RegTmod(const SegmentRegister* segreg)
            : m_type(SEGREG), m_segreg(segreg)
        {}
        RegTmod(const TargetModifier* tmod)
            : m_type(TARGETMOD), m_tmod(tmod)
        {}

        Type getType() const { return m_type; }
        bool isType(Type type) const { return m_type == type; }

        const Register* getReg() const
        {
            return (m_type == REG ? m_reg : 0);
        }

        const RegisterGroup* getRegGroup() const
        {
            return (m_type == REGGROUP ? m_reggroup : 0);
        }

        const SegmentRegister* getSegReg() const
        {
            return (m_type == SEGREG ? m_segreg : 0);
        }

        const TargetModifier* getTargetMod() const
        {
            return (m_type == TARGETMOD ? m_tmod : 0);
        }

    private:
        Type m_type;
        union
        {
            const Register* m_reg;
            const RegisterGroup* m_reggroup;
            const SegmentRegister* m_segreg;
            const TargetModifier* m_tmod;
        };
    };

    /// Constructor.
    Arch(const ArchModule& module) : m_module(module) {}

    /// Destructor.
    virtual ~Arch();

    /// Get module.
    const ArchModule& getModule() const { return m_module; }

    /// Add directive handlers.
    virtual void AddDirectives(Directives& dirs, llvm::StringRef parser);

    /// Set parser to use.
    /// @param parser       keyword of parser to use
    /// @return False if unrecognized parser.
    virtual bool setParser(llvm::StringRef parser) = 0;

    /// Set active machine.
    /// @param machine      keyword of machine to use; must be one in the
    ///                     list returned by get_machines().
    /// @return False if unrecognized machine.
    virtual bool setMachine(llvm::StringRef machine) = 0;

    /// Get architecture's active machine name.
    /// @return Active machine name.
    virtual llvm::StringRef getMachine() const = 0;

    /// Get architecture's active address size, in bits.
    /// @return Active address size (in bits).
    virtual unsigned int getAddressSize() const = 0;

    /// Set any arch-specific variables.  For example, "mode_bits" in x86.
    /// @param var  variable name
    /// @param val  value to set
    /// @return True on success, false on failure (variable does not exist).
    virtual bool setVar(llvm::StringRef var, unsigned long val) = 0;

    /// Determine if a custom parser (ParseInsn) should be used.  The default
    /// implementation returns false.
    /// @note This can be parser-dependent, so call setParser() first.
    /// @return True if ParseInsn() should be used for instruction parsing.
    virtual bool hasParseInsn() const;

    /// Custom instruction parser.  Parses an instruction and appends it to a
    /// BytecodeContainer.  This function starts parsing with the current
    /// token (parser.m_token), getting additional tokens from parser.m_preproc
    /// as necessary.  It stops parsing when it finds a token with the end of
    /// statement flag set.
    /// @note The default implementation asserts.
    /// @param container        bytecode container for resulting instruction
    /// @param parser           parser implementation
    /// @return False if error occurred.
    virtual bool ParseInsn(BytecodeContainer& container,
                           ParserImpl& parser) const;

    /// Check an generic identifier to see if it matches architecture
    /// specific names for instructions or instruction prefixes.
    /// Unrecognized identifiers should return empty so they can be
    /// treated as normal symbols.
    /// @param id           identifier as in the input file
    /// @param line         virtual line
    /// @param bc           for instructions, yasm_insn-based bytecode is
    ///                     returned (and NULL otherwise)
    /// @param prefix       for prefixes, yasm_arch-specific value is
    ///                     returned (and 0 otherwise)
    /// @return Identifier type (empty if unrecognized)
    virtual InsnPrefix ParseCheckInsnPrefix(llvm::StringRef id,
                                            SourceLocation source,
                                            Diagnostic& diags) const = 0;

    /// Check an generic identifier to see if it matches architecture
    /// specific names for registers or target modifiers.  Unrecognized
    /// identifiers should return empty.
    /// @param id           identifier as in the input file
    /// @return Identifier type (empty if unrecognized)
    virtual RegTmod ParseCheckRegTmod(llvm::StringRef id,
                                      SourceLocation source,
                                      Diagnostic& diags) const = 0;

    /// Get NOP fill patterns for 1-15 bytes of fill.
    /// @return 16-entry array of arrays; [0] is unused,
    ///         [1] - [15] point to arrays of 1-15 bytes (respectively)
    ///         in length.
    virtual const unsigned char** getFill() const = 0;

    /// Set endianness of a buffer appropriately for this architecture.
    virtual void setEndian(Bytes& bytes) const = 0;

    /// Create an effective address from an expression.
    /// @param e    expression
    /// @return Newly allocated effective address.
    virtual std::auto_ptr<EffAddr> CreateEffAddr(std::auto_ptr<Expr> e) const
        = 0;

    /// Create an instruction that represents a single empty (0 length)
    /// instruction.  This is used for handling solitary prefixes.
    /// @return Newly allocated instruction.
    virtual std::auto_ptr<Insn> CreateEmptyInsn() const = 0;

    /// Create an instruction based on an InsnInfo provided by
    /// ParseCheckInsnPrefix().
    virtual std::auto_ptr<Insn> CreateInsn(const InsnInfo* info) const = 0;

private:
    Arch(const Arch&);                  // not implemented
    const Arch& operator=(const Arch&); // not implemented

    const ArchModule& m_module;
};

/// Arch module interface.
class YASM_LIB_EXPORT ArchModule : public Module
{
public:
    enum { module_type = 1 };

    /// Destructor.
    virtual ~ArchModule();

    /// Get the module type.
    /// @return "Arch".
    llvm::StringRef getType() const;

    /// Get the word size of an architecture.
    /// @return Word size (in bits).
    virtual unsigned int getWordSize() const = 0;

    /// Get the minimum instruction length of an architecture.
    /// @return Minimum instruction length (in bytes).
    virtual unsigned int getMinInsnLen() const = 0;

    /// Vector of machine keyword/name pairs.  The first element in the pair
    /// is the keyword used to select the machine with set_machine(), and the
    /// second element is a one-line description of the machine.
    typedef std::vector<std::pair<llvm::StringRef, llvm::StringRef> > MachineNames;

    /// Get available machines.
    /// A number of different machine types may be associated with a single
    /// architecture.  These may be specific CPU's, but the ABI used to
    /// interface with the architecture should be the primary differentiator
    /// between machines.  Some object formats (ELF) use the machine to
    /// determine parameters within the generated output.
    /// @return Machine keyword/name pairs.
    virtual MachineNames getMachines() const = 0;

    /// Arch factory function.
    /// @return New architecture.
    virtual std::auto_ptr<Arch> Create() const = 0;
};

template <typename ArchImpl>
class ArchModuleImpl : public ArchModule
{
public:
    ArchModuleImpl() {}
    ~ArchModuleImpl() {}

    llvm::StringRef getName() const { return ArchImpl::getName(); }
    llvm::StringRef getKeyword() const { return ArchImpl::getKeyword(); }

    unsigned int getWordSize() const { return ArchImpl::getWordSize(); }
    unsigned int getMinInsnLen() const
    { return ArchImpl::getMinInsnLen(); }
    MachineNames getMachines() const { return ArchImpl::getMachines(); }

    std::auto_ptr<Arch> Create() const
    {
        return std::auto_ptr<Arch>(new ArchImpl(*this));
    }
};

} // namespace yasm

#endif

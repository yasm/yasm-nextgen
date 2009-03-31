#ifndef YASM_INSN_H
#define YASM_INSN_H
///
/// @file
/// @brief Mnenomic instructions interface.
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
#include <iosfwd>
#include <memory>
#include <vector>

#include "yasmx/Config/export.h"
#include "yasmx/Support/marg_ostream_fwd.h"

#include "yasmx/EffAddr.h"
#include "yasmx/Expr.h"


namespace yasm
{

class BytecodeContainer;
class Bytes;
class Register;
class SegmentRegister;

/// Base class for instructions.  Architectures should
/// derive their own implementation from this.
class YASM_LIB_EXPORT Insn
{
public:
    typedef std::auto_ptr<Insn> Ptr;

    /// An instruction operand.
    class YASM_LIB_EXPORT Operand
    {
    public:
        /// Operand type.
        enum Type
        {
            NONE = 0,   ///< Nothing.
            REG,        ///< A register.
            SEGREG,     ///< A segment register.
            MEMORY,     ///< An effective address (memory reference).
            IMM         ///< An immediate or jump target.
        };

        /// Base class for target modifiers.
        class YASM_LIB_EXPORT TargetModifier
        {
        public:
            TargetModifier() {}
            virtual ~TargetModifier();
            virtual void put(std::ostream& os) const = 0;

        private:
            // not implemented (noncopyable class)
            TargetModifier(const TargetModifier&);
            const TargetModifier& operator=(const TargetModifier&);
        };

        /// Create an instruction operand from a register.
        /// @param reg  register
        Operand(const Register* reg);

        /// Create an instruction operand from a segment register.
        /// @param segreg       segment register
        Operand(const SegmentRegister* segreg);

        /// Create an instruction operand from an effective address.
        /// @param ea   effective address
        Operand(std::auto_ptr<EffAddr> ea);

        /// Create an instruction operand from an immediate expression.
        /// Looks for cases of a single register and creates a register
        /// variant.
        /// @param val  immediate expression
        /// @return Newly allocated operand.
        Operand(std::auto_ptr<Expr> val);

        /// Explicit copy creator.
        /// There's no copy constructor or assignment operator as we want
        /// to use the default bit-copy ones.  Even though this class
        /// contains more complex structures, we don't want to be copying
        /// the contents all the time.
        Operand clone() const;

        /// Explicit release.
        /// Doesn't destroy contents, just ensures what contents are there
        /// won't be destroyed via the destructor.
        void release()
        {
            m_type = NONE;
            m_reg = 0;
        }

        /// Explicit destructor.
        /// Similar to clone(), we do smart copying and destruction in
        /// #Expr implementation to prevent over-copying of possibly deep
        /// expression trees.
        void destroy();

        void put(marg_ostream& os) const;
        void finalize();

        /// Match type.
        bool is_type(Type type) const
        { return m_type == static_cast<unsigned int>(type); }

        /// Get the type.
        Type get_type() const { return static_cast<Type>(m_type); }

        /// Helper functions to get specific types.

        const Register* get_reg() const
        {
            return (m_type == REG ? m_reg : 0);
        }

        const SegmentRegister* get_segreg() const
        {
            return (m_type == SEGREG ? m_segreg : 0);
        }

        EffAddr* get_memory() const
        {
            return (m_type == MEMORY ? m_ea : 0);
        }

        Expr* get_imm() const
        {
            return (m_type == IMM ? m_val : 0);
        }

        /// Helper functions to release specific types
        std::auto_ptr<EffAddr> release_memory();
        std::auto_ptr<Expr> release_imm();

        /// Release segment expression, 0 if none.
        std::auto_ptr<Expr> release_seg();

        /// Get segment expression, 0 if none.
        Expr* get_seg() { return m_seg; }
        const Expr* get_seg() const { return m_seg; }

        /// Set segment expression.
        void set_seg(std::auto_ptr<Expr> seg);

        /// Get arch target modifier, 0 if none.
        const TargetModifier* get_targetmod() const { return m_targetmod; }

        /// Set target modifier.
        void set_targetmod(const TargetModifier* tmod) { m_targetmod = tmod; }

        /// Get operand size, in bits.  0 if not user specified.
        unsigned int get_size() const { return m_size; }

        /// Set operand size, in bits.
        void set_size(unsigned int size) { m_size = size; }

        /// Is the operand dereferenced, as in "*foo" in GAS?
        bool is_deref() const { return m_deref; }

        /// Is the operand strict, as in "strict foo" in NASM?
        bool is_strict() const { return m_strict; }

        /// Set the strictness of the operand.
        void make_strict(bool strict=true) { m_strict = strict; }

    private:
        /// Operand data.
        union
        {
            const SegmentRegister* m_segreg;    ///< Segment register.
            const Register* m_reg;              ///< Register.
            EffAddr* m_ea;      ///< Effective address for memory references.
            Expr* m_val;        ///< Value of immediate or jump target.
        };

        /// Segment expression.
        Expr* m_seg;

        /// Arch target modifier, 0 if none.
        const TargetModifier* m_targetmod;

        /// Specified size of the operand, in bits.
        /// 0 if not user-specified.
        unsigned int m_size:16;

        /// Nonzero if dereference.  Used for "*foo" in GAS.
        /// The reason for this is that by default in GAS, an unprefixed value
        /// is a memory address, except for jumps/calls, in which case it
        /// needs a "*" prefix to become a memory address (otherwise it's an
        /// immediate).  This isn't knowable in the parser stage, so the
        /// parser sets this flag to indicate the "*" prefix has been used,
        /// and the arch needs to adjust the operand type appropriately
        /// depending on the instruction type.
        unsigned int m_deref:1;

        /// Nonzero if strict.  Used for "strict foo" in NASM.
        /// This is used to inhibit optimization on otherwise "sized" values.
        /// For example, the user may just want to be explicit with the size
        /// on "push dword 4", but not actually want to force the immediate
        /// size to 4 bytes (rather wanting the optimizer to optimize it down
        /// to 1 byte as though "dword" was not specified).  To indicate the
        /// immediate should actually be forced to 4 bytes, the user needs to
        /// write "push strict dword 4", which sets this flag.
        unsigned int m_strict:1;

        /// Operand type.
        unsigned int m_type:4;
    };

    /// Base class for instruction prefixes.
    class YASM_LIB_EXPORT Prefix
    {
    public:
        Prefix() {}
        virtual ~Prefix();
        virtual void put(std::ostream& os) const = 0;

    private:
        Prefix(const Prefix&);                  // not implemented
        const Prefix& operator=(const Prefix&); // not implemented
    };

    typedef std::vector<Operand> Operands;
    typedef std::vector<const Prefix*> Prefixes;
    typedef std::vector<const SegmentRegister*> SegRegs;

    Insn();
    virtual ~Insn();

    /// Add operand to the end of an instruction.
    /// @param op           operand
    void add_operand(const Operand& op)
    {
        m_operands.push_back(op);
    }

    /// Associate a prefix with an instruction.
    /// @param prefix       data that identifies the prefix
    void add_prefix(const Prefix* prefix)
    {
        m_prefixes.push_back(prefix);
    }

    /// Associate a segment prefix with an instruction.
    /// @param segreg       data that identifies the segment register
    void add_seg_prefix(const SegmentRegister* segreg)
    {
        m_segregs.push_back(segreg);
    }

    /// Print a list of instruction operands.  For debugging purposes.
    /// @note A base version of this function is provided.
    /// @param os           output stream
    /// @param indent_level indentation level
    virtual void put(marg_ostream& os) const = 0;

    /// Append instruction to a bytecode container.
    void append(BytecodeContainer& container, unsigned long line);

    virtual Insn* clone() const = 0;

protected:
    /// Copy constructor.
    Insn(const Insn& rhs);

    /// Append instruction to a section.
    virtual void do_append(BytecodeContainer& container,
                           unsigned long line) = 0;

    /// Operands.
    Operands m_operands;

    /// Prefixes.
    Prefixes m_prefixes;

    /// Array of segment prefixes.
    SegRegs m_segregs;

private:
    const Insn& operator=(const Insn&);
};

inline std::ostream&
operator<< (std::ostream& os, const Insn::Operand::TargetModifier& tmod)
{
    tmod.put(os);
    return os;
}

inline std::ostream&
operator<< (std::ostream& os, const Insn::Prefix& prefix)
{
    prefix.put(os);
    return os;
}

inline marg_ostream&
operator<< (marg_ostream& os, const Insn::Operand& operand)
{
    operand.put(os);
    return os;
}

inline marg_ostream&
operator<< (marg_ostream& os, const Insn& insn)
{
    insn.put(os);
    return os;
}

} // namespace yasm

#endif

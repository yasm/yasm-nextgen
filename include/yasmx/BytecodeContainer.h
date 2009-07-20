#ifndef YASM_BYTECODECONTAINER_H
#define YASM_BYTECODECONTAINER_H
///
/// @file
/// @brief Bytecode container interface.
///
/// @license
///  Copyright (C) 2001-2007  Peter Johnson
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

#include "yasmx/Config/export.h"
#include "yasmx/Support/ptr_vector.h"


namespace YAML { class Emitter; }

namespace yasm
{

class Bytecode;
class Errwarns;
class Expr;
class Section;
class Object;

/// A bytecode container.
class YASM_LIB_EXPORT BytecodeContainer
{
    friend class Object;

public:
    /// Constructor.
    BytecodeContainer();

    /// Destructor.
    virtual ~BytecodeContainer();

    /// If container is a section, get it as such.
    /// @return Section if this container is a section, else NULL.
    /*@null@*/ virtual Section* AsSection();
    /*@null@*/ virtual const Section* AsSection() const;

    /// Get object owner of a section.
    /// @return Object this section is a part of.
    Object* getObject() const { return m_object; }

    /// Add bytecode to the end of the container.
    /// @param bc       bytecode (may be NULL)
    void AppendBytecode(/*@null@*/ std::auto_ptr<Bytecode> bc);

    /// Add gap space to the end of the container.
    /// @param size     number of bytes of gap
    /// @return Reference to gap bytecode.
    Bytecode& AppendGap(unsigned long size, unsigned long line);

    /// Start a new bytecode at the end of the container.  Factory function.
    /// @return Reference to new bytecode.
    Bytecode& StartBytecode();

    /// Ensure the last bytecode in the container has no tail.  If the last
    /// bytecode has no tail, simply returns it; otherwise creates and returns
    /// a fresh bytecode.
    /// @return Reference to last bytecode.
    Bytecode& FreshBytecode();

    typedef stdx::ptr_vector<Bytecode>::iterator bc_iterator;
    typedef stdx::ptr_vector<Bytecode>::const_iterator const_bc_iterator;

    bc_iterator bytecodes_begin() { return m_bcs.begin(); }
    const_bc_iterator bytecodes_begin() const { return m_bcs.begin(); }
    bc_iterator bytecodes_end() { return m_bcs.end(); }
    const_bc_iterator bytecodes_end() const { return m_bcs.end(); }

    Bytecode& bytecodes_first() { return m_bcs.front(); }
    const Bytecode& bytecodes_first() const { return m_bcs.front(); }
    Bytecode& bytecodes_last() { return m_bcs.back(); }
    const Bytecode& bytecodes_last() const { return m_bcs.back(); }

    /// Finalize all bytecodes after parsing.
    /// @param errwarns     error/warning set
    /// @note Errors/warnings are stored into errwarns.
    void Finalize(Errwarns& errwarns);

    /// Update all bytecode offsets.
    /// @param errwarns     error/warning set
    /// @note Errors/warnings are stored into errwarns.
    void UpdateOffsets(Errwarns& errwarns);

    /// Write a YAML representation.  For debugging purposes.
    /// @param out          YAML emitter
    void Write(YAML::Emitter& out) const;

    /// Dump a YAML representation to stderr.  For debugging purposes.
    void Dump() const;

private:
    // not implemented (noncopyable class)
    BytecodeContainer(const BytecodeContainer&);
    const BytecodeContainer& operator=(const BytecodeContainer&);

    /*@dependent@*/ Object* m_object;   ///< Pointer to parent object

    /// The bytecodes for the section's contents.
    stdx::ptr_vector<Bytecode> m_bcs;
    stdx::ptr_vector_owner<Bytecode> m_bcs_owner;

    bool m_last_gap;        ///< Last bytecode is a gap bytecode
};

/// Dump a YAML representation of bytecode container.  For debugging purposes.
/// @param out          YAML emitter
/// @param container    container
/// @return Emitter.
inline YAML::Emitter&
operator<< (YAML::Emitter& out, const BytecodeContainer& container)
{
    container.Write(out);
    return out;
}

} // namespace yasm

#endif

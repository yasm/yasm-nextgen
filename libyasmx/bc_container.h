#ifndef YASM_BC_CONTAINER_H
#define YASM_BC_CONTAINER_H
///
/// @file libyasm/bc_container.h
/// @brief YASM bytecode container interface.
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

#include <boost/noncopyable.hpp>

#include "marg_ostream_fwd.h"
#include "ptr_vector.h"


namespace yasm
{

class Bytecode;
class Errwarns;
class Expr;
class Section;
class Object;

class BytecodeContainer : private boost::noncopyable
{
    friend class Object;

public:
    /// Constructor.
    BytecodeContainer();

    /// Destructor.
    virtual ~BytecodeContainer();

    /// If container is a section, get it as such.
    /// @return Section if this container is a section, else NULL.
    /*@null@*/ virtual Section* as_section();
    /*@null@*/ virtual const Section* as_section() const;

    /// Get object owner of a section.
    /// @return Object this section is a part of.
    Object* get_object() const { return m_object; }

    /// Print a bytecode container.  For debugging purposes.
    /// @param os           output stream
    void put(marg_ostream& os) const;

    /// Add bytecode to the end of the container.
    /// @param bc       bytecode (may be NULL)
    void append_bytecode(/*@null@*/ std::auto_ptr<Bytecode> bc);

    /// Add gap space to the end of the container.
    /// @param size     number of bytes of gap
    void append_gap(unsigned int size, unsigned long line);

    /// Start a new bytecode at the end of the container.  Factory function.
    /// @return Reference to new bytecode.
    Bytecode& start_bytecode();

    /// Ensure the last bytecode in the container has no tail.  If the last
    /// bytecode has no tail, simply returns it; otherwise creates and returns
    /// a fresh bytecode.
    /// @return Reference to last bytecode.
    Bytecode& fresh_bytecode();

    typedef stdx::ptr_vector<Bytecode>::iterator bc_iterator;
    typedef stdx::ptr_vector<Bytecode>::const_iterator const_bc_iterator;

    bc_iterator bcs_begin() { return m_bcs.begin(); }
    const_bc_iterator bcs_begin() const { return m_bcs.begin(); }
    bc_iterator bcs_end() { return m_bcs.end(); }
    const_bc_iterator bcs_end() const { return m_bcs.end(); }

    Bytecode& bcs_first() { return m_bcs.front(); }
    const Bytecode& bcs_first() const { return m_bcs.front(); }
    Bytecode& bcs_last() { return m_bcs.back(); }
    const Bytecode& bcs_last() const { return m_bcs.back(); }

    /// Finalize all bytecodes after parsing.
    /// @param errwarns     error/warning set
    /// @note Errors/warnings are stored into errwarns.
    void finalize(Errwarns& errwarns);

    /// Update all bytecode offsets.
    /// @param errwarns     error/warning set
    /// @note Errors/warnings are stored into errwarns.
    void update_offsets(Errwarns& errwarns);

private:
    /*@dependent@*/ Object* m_object;   ///< Pointer to parent object

    /// The bytecodes for the section's contents.
    stdx::ptr_vector<Bytecode> m_bcs;
    stdx::ptr_vector_owner<Bytecode> m_bcs_owner;

    bool m_last_gap;        ///< Last bytecode is a gap bytecode
};

/// Print a bytecode container.  For debugging purposes.
/// @param os           output stream
inline marg_ostream&
operator<< (marg_ostream& os, const BytecodeContainer& container)
{
    container.put(os);
    return os;
}

} // namespace yasm

#endif

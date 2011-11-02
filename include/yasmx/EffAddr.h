#ifndef YASM_EFFADDR_H
#define YASM_EFFADDR_H
///
/// @file
/// @brief Effective address interface.
///
/// @license
///  Copyright (C) 2007  Peter Johnson
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
#include "yasmx/DebugDumper.h"
#include "yasmx/Value.h"


namespace yasm
{

class SegmentRegister;

/// Base class for an effective address.
/// Architectures should derive their own implementation from this.
class YASM_LIB_EXPORT EffAddr
{
public:
    typedef std::auto_ptr<EffAddr> Ptr;

    /// Constructor.
    explicit EffAddr(std::auto_ptr<Expr> e);

    /// Destructor.
    virtual ~EffAddr();

    Value m_disp;       ///< address displacement

    /// Segment register override (0 if none).
    /// Some architectures (such as x86) support segment overrides on
    /// effective addresses.
    const SegmentRegister* m_segreg;

    /// 1 if length of disp must be >0.
    bool m_need_nonzero_len:1;

    /// 1 if a displacement should be present in the output.
    bool m_need_disp:1;

    /// 1 if reg*2 should not be split into reg+reg. (0 if not).
    /// This flag indicates (for architectures that support complex effective
    /// addresses such as x86) if various types of complex effective addresses
    /// can be split into different forms in order to minimize instruction
    /// length.
    bool m_nosplit:1;

    /// 1 if effective address is /definitely/ an effective address.
    /// This is used in e.g. the GAS parser to differentiate
    /// between "expr" (which might or might not be an effective address) and
    /// "expr(,1)" (which is definitely an effective address).
    bool m_strong:1;

    /// 1 if effective address is forced PC-relative.
    bool m_pc_rel:1;

    /// 1 if effective address is forced non-PC-relative.
    bool m_not_pc_rel:1;

    /// Clone an effective address.
    virtual EffAddr* clone() const = 0;

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

protected:
    /// Copy constructor so that derived classes can sanely have one.
    EffAddr(const EffAddr& rhs);

#ifdef WITH_XML
    /// Write derived class XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    virtual pugi::xml_node DoWrite(pugi::xml_node out) const = 0;
#endif // WITH_XML

private:
    const EffAddr& operator=(const EffAddr&);
};

} // namespace yasm

#endif

///
/// ORG bytecode
///
///  Copyright (C) 2005-2007  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
/// 1. Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
/// 2. Redistributions in binary form must reproduce the above copyright
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
///
#include "util.h"

#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/marg_ostream.h"
#include "yasmx/BytecodeContainer.h"
#include "yasmx/BytecodeContainer_util.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"


namespace
{

using namespace yasm;

class OrgBytecode : public Bytecode::Contents
{
public:
    OrgBytecode(unsigned long start, unsigned long fill);
    ~OrgBytecode();

    /// Prints the implementation-specific data (for debugging purposes).
    void Put(marg_ostream& os) const;

    /// Finalizes the bytecode after parsing.
    void Finalize(Bytecode& bc);

    /// Calculates the minimum size of a bytecode.
    unsigned long CalcLen(Bytecode& bc, const Bytecode::AddSpanFunc& add_span);

    /// Recalculates the bytecode's length based on an expanded span
    /// length.
    bool Expand(Bytecode& bc,
                unsigned long& len,
                int span,
                long old_val,
                long new_val,
                /*@out@*/ long* neg_thres,
                /*@out@*/ long* pos_thres);

    /// Convert a bytecode into its byte representation.
    void Output(Bytecode& bc, BytecodeOutput& bc_out);

    SpecialType getSpecial() const;

    OrgBytecode* clone() const;

private:
    unsigned long m_start;      ///< target starting offset within section
    unsigned long m_fill;       ///< fill value
};


OrgBytecode::OrgBytecode(unsigned long start, unsigned long fill)
    : m_start(start),
      m_fill(fill)
{
}

OrgBytecode::~OrgBytecode()
{
}

void
OrgBytecode::Put(marg_ostream& os) const
{
    os << "_Org_\n";
    os << "Start=" << m_start << '\n';
    os << "Fill=" << m_fill << '\n';
}

void
OrgBytecode::Finalize(Bytecode& bc)
{
}

unsigned long
OrgBytecode::CalcLen(Bytecode& bc, const Bytecode::AddSpanFunc& add_span)
{
    unsigned long len = 0;
    long neg_thres = 0;
    long pos_thres = m_start;

    Expand(bc, len, 0, 0, static_cast<long>(bc.getTailOffset()), &neg_thres,
           &pos_thres);

    return len;
}

bool
OrgBytecode::Expand(Bytecode& bc,
                    unsigned long& len,
                    int span,
                    long old_val,
                    long new_val,
                    /*@out@*/ long* neg_thres,
                    /*@out@*/ long* pos_thres)
{
    // Check for overrun
    if (static_cast<unsigned long>(new_val) > m_start)
        throw Error(N_("ORG overlap with already existing data"));

    // Generate space to start offset
    len = m_start - new_val;
    return true;
}

void
OrgBytecode::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    // Sanity check for overrun
    if (bc.getTailOffset() > m_start)
        throw Error(N_("ORG overlap with already existing data"));
    unsigned long len = m_start - bc.getTailOffset();
    Bytes& bytes = bc_out.getScratch();
    // XXX: handle more than 8 bit?
    bytes.insert(bytes.end(), len, static_cast<unsigned char>(m_fill));
    bc_out.Output(bytes);
}

OrgBytecode::SpecialType
OrgBytecode::getSpecial() const
{
    return SPECIAL_OFFSET;
}

OrgBytecode*
OrgBytecode::clone() const
{
    return new OrgBytecode(m_start, m_fill);
}

} // anonymous namespace

namespace yasm
{

void
AppendOrg(BytecodeContainer& container,
          unsigned long start,
          unsigned long fill,
          unsigned long line)
{
    Bytecode& bc = container.FreshBytecode();
    bc.Transform(Bytecode::Contents::Ptr(new OrgBytecode(start, fill)));
    bc.setLine(line);
}

} // namespace yasm

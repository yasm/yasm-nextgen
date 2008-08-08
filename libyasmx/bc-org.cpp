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

#include "bc_container.h"
#include "bc_container_util.h"
#include "bc_output.h"
#include "bytecode.h"
#include "bytes.h"
#include "errwarn.h"
#include "marg_ostream.h"


namespace
{

using namespace yasm;

class OrgBytecode : public Bytecode::Contents
{
public:
    OrgBytecode(unsigned long start, unsigned long fill);
    ~OrgBytecode();

    /// Prints the implementation-specific data (for debugging purposes).
    void put(marg_ostream& os) const;

    /// Finalizes the bytecode after parsing.
    void finalize(Bytecode& bc);

    /// Calculates the minimum size of a bytecode.
    unsigned long calc_len(Bytecode& bc, Bytecode::AddSpanFunc add_span);

    /// Recalculates the bytecode's length based on an expanded span
    /// length.
    bool expand(Bytecode& bc,
                unsigned long& len,
                int span,
                long old_val,
                long new_val,
                /*@out@*/ long& neg_thres,
                /*@out@*/ long& pos_thres);

    /// Convert a bytecode into its byte representation.
    void output(Bytecode& bc, BytecodeOutput& bc_out);

    SpecialType get_special() const;

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
OrgBytecode::put(marg_ostream& os) const
{
    os << "_Org_\n";
    os << "Start=" << m_start << '\n';
    os << "Fill=" << m_fill << '\n';
}

void
OrgBytecode::finalize(Bytecode& bc)
{
}

unsigned long
OrgBytecode::calc_len(Bytecode& bc, Bytecode::AddSpanFunc add_span)
{
    unsigned long len = 0;
    long neg_thres = 0;
    long pos_thres = m_start;

    expand(bc, len, 0, 0, static_cast<long>(bc.tail_offset()), neg_thres,
           pos_thres);

    return len;
}

bool
OrgBytecode::expand(Bytecode& bc,
                    unsigned long& len,
                    int span,
                    long old_val,
                    long new_val,
                    /*@out@*/ long& neg_thres,
                    /*@out@*/ long& pos_thres)
{
    // Check for overrun
    if (static_cast<unsigned long>(new_val) > m_start)
        throw Error(N_("ORG overlap with already existing data"));

    // Generate space to start offset
    len = m_start - new_val;
    return true;
}

void
OrgBytecode::output(Bytecode& bc, BytecodeOutput& bc_out)
{
    // Sanity check for overrun
    if (bc.tail_offset() > m_start)
        throw Error(N_("ORG overlap with already existing data"));
    unsigned long len = m_start - bc.tail_offset();
    Bytes& bytes = bc_out.get_scratch();
    // XXX: handle more than 8 bit?
    bytes.insert(bytes.end(), len, static_cast<unsigned char>(m_fill));
    bc_out.output(bytes);
}

OrgBytecode::SpecialType
OrgBytecode::get_special() const
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
append_org(BytecodeContainer& container,
           unsigned long start,
           unsigned long fill,
           unsigned long line)
{
    Bytecode& bc = container.fresh_bytecode();
    bc.transform(Bytecode::Contents::Ptr(new OrgBytecode(start, fill)));
    bc.set_line(line);
}

} // namespace yasm

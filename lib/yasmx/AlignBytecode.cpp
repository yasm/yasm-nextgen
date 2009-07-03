///
/// Align bytecode
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
#include "yasmx/BytecodeContainer_util.h"

#include "util.h"

#include "yasmx/Support/Compose.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/marg_ostream.h"
#include "yasmx/Support/scoped_ptr.h"
#include "yasmx/BytecodeContainer.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"


namespace
{

using namespace yasm;

class AlignBytecode : public Bytecode::Contents
{
public:
    AlignBytecode(const Expr& boundary,
                  const Expr& fill,
                  const Expr& maxskip,
                  /*@null@*/ const unsigned char** code_fill);
    ~AlignBytecode();

    /// Prints the implementation-specific data (for debugging purposes).
    void Put(marg_ostream& os) const;

    /// Finalizes the bytecode after parsing.
    void Finalize(Bytecode& bc);

    /// Calculates the minimum size of a bytecode.
    unsigned long CalcLen(Bytecode& bc, const Bytecode::AddSpanFunc& add_span);

    /// Recalculates the bytecode's length based on an expanded span
    /// length.
    bool Expand(Bytecode& bc, unsigned long& len, int span,
                long old_val, long new_val,
                /*@out@*/ long* neg_thres,
                /*@out@*/ long* pos_thres);

    /// Convert a bytecode into its byte representation.
    void Output(Bytecode& bc, BytecodeOutput& bc_out);

    SpecialType getSpecial() const;

    AlignBytecode* clone() const;

private:
    Expr m_boundary;    ///< alignment boundary

    /// What to fill intervening locations with, empty if using code_fill
    Expr m_fill;

    /// Maximum number of bytes to skip, empty if no maximum.
    Expr m_maxskip;

    /// Code fill, NULL if using 0 fill
    /*@null@*/ const unsigned char** m_code_fill;
};


AlignBytecode::AlignBytecode(const Expr& boundary,
                             const Expr& fill,
                             const Expr& maxskip,
                             /*@null@*/ const unsigned char** code_fill)
    : m_boundary(boundary),
      m_fill(fill),
      m_maxskip(maxskip),
      m_code_fill(code_fill)
{
}

AlignBytecode::~AlignBytecode()
{
}

void
AlignBytecode::Put(marg_ostream& os) const
{
    os << "_Align_\n";
    os << "Boundary=" << m_boundary << '\n';
    if (!m_fill.isEmpty())
        os << "Fill=" << m_fill << '\n';
    if (!m_maxskip.isEmpty())
        os << "Max Skip=" << m_maxskip << '\n';
}

void
AlignBytecode::Finalize(Bytecode& bc)
{
    if (!m_boundary.isIntNum())
        throw NotConstantError(N_("align boundary must be a constant"));
    if (!m_fill.isEmpty() && !m_fill.isIntNum())
        throw NotConstantError(N_("align fill must be a constant"));
    if (!m_maxskip.isEmpty() && !m_maxskip.isIntNum())
        throw NotConstantError(N_("align maximum skip must be a constant"));
}

unsigned long
AlignBytecode::CalcLen(Bytecode& bc, const Bytecode::AddSpanFunc& add_span)
{
    unsigned long len = 0;
    long neg_thres = 0;
    long pos_thres = 0;

    Expand(bc, len, 0, 0, static_cast<long>(bc.getTailOffset()), &neg_thres,
           &pos_thres);
    return len;
}

bool
AlignBytecode::Expand(Bytecode& bc, unsigned long& len, int span,
                      long old_val, long new_val,
                      /*@out@*/ long* neg_thres, /*@out@*/ long* pos_thres)
{
    unsigned long boundary = m_boundary.getIntNum().getUInt();

    if (boundary == 0)
    {
        len = 0;
        *pos_thres = new_val;
        return false;
    }

    unsigned long end = static_cast<unsigned long>(new_val);
    if (end & (boundary-1))
        end = (end & ~(boundary-1)) + boundary;

    *pos_thres = static_cast<long>(end);
    len = end - static_cast<unsigned long>(new_val);

    if (!m_maxskip.isEmpty())
    {
        unsigned long maxskip = m_maxskip.getIntNum().getUInt();
        if (len > maxskip)
        {
            *pos_thres = static_cast<long>(end-maxskip)-1;
            len = 0;
        }
    }
    return true;
}

void
AlignBytecode::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    unsigned long len;
    unsigned long boundary = m_boundary.getIntNum().getUInt();
    Bytes& bytes = bc_out.getScratch();

    if (boundary == 0)
        return;
    else
    {
        unsigned long tail = bc.getTailOffset();
        unsigned long end = tail;
        if (tail & (boundary-1))
            end = (tail & ~(boundary-1)) + boundary;
        len = end - tail;
        if (len == 0)
            return;
        if (!m_maxskip.isEmpty())
        {
            unsigned long maxskip = m_maxskip.getIntNum().getUInt();
            if (len > maxskip)
                return;
        }
    }

    if (!m_fill.isEmpty())
    {
        unsigned long v = m_fill.getIntNum().getUInt();
        bytes.insert(bytes.end(), len, static_cast<unsigned char>(v));
    }
    else if (m_code_fill)
    {
        unsigned long maxlen = 15;
        while (!m_code_fill[maxlen] && maxlen>0)
            maxlen--;
        if (maxlen == 0)
            throw Error(N_("could not find any code alignment size"));

        // Fill with maximum code fill as much as possible
        while (len > maxlen)
        {
            bytes.insert(bytes.end(),
                         &m_code_fill[maxlen][0],
                         &m_code_fill[maxlen][maxlen]);
            len -= maxlen;
        }

        if (!m_code_fill[len])
        {
            throw ValueError(String::Compose(N_("invalid alignment size %1"),
                                             len));
        }
        // Handle rest of code fill
        bytes.insert(bytes.end(),
                     &m_code_fill[len][0],
                     &m_code_fill[len][len]);
    }
    else
    {
        // Just fill with 0
        bytes.insert(bytes.end(), len, 0);
    }
    bc_out.Output(bytes);
}

AlignBytecode::SpecialType
AlignBytecode::getSpecial() const
{
    return SPECIAL_OFFSET;
}

AlignBytecode*
AlignBytecode::clone() const
{
    return new AlignBytecode(m_boundary, m_fill, m_maxskip, m_code_fill);
}

} // anonymous namespace

namespace yasm
{

void
AppendAlign(BytecodeContainer& container,
            const Expr& boundary,
            const Expr& fill,
            const Expr& maxskip,
            /*@null@*/ const unsigned char** code_fill,
            unsigned long line)
{
    Bytecode& bc = container.FreshBytecode();
    bc.Transform(Bytecode::Contents::Ptr(
        new AlignBytecode(boundary, fill, maxskip, code_fill)));
    bc.setLine(line);
}

} // namespace yasm

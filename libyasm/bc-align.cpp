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
#include "util.h"

#include <cstdlib>
#include <iomanip>
#include <ostream>

#include <boost/scoped_ptr.hpp>

#include "bytecode.h"
#include "bytes.h"
#include "compose.h"
#include "errwarn.h"
#include "expr.h"
#include "intnum.h"


namespace {

using namespace yasm;

class AlignBytecode : public Bytecode::Contents {
public:
    AlignBytecode(std::auto_ptr<Expr> boundary,
                  /*@null@*/ std::auto_ptr<Expr> fill,
                  /*@null@*/ std::auto_ptr<Expr> maxskip,
                  /*@null@*/ const unsigned char** code_fill);
    ~AlignBytecode();

    /// Prints the implementation-specific data (for debugging purposes).
    void put(std::ostream& os, int indent_level) const;

    /// Finalizes the bytecode after parsing.
    void finalize(Bytecode& bc);

    /// Calculates the minimum size of a bytecode.
    unsigned long calc_len(Bytecode& bc, Bytecode::AddSpanFunc add_span);

    /// Recalculates the bytecode's length based on an expanded span
    /// length.
    bool expand(Bytecode& bc, unsigned long& len, int span,
                long old_val, long new_val,
                /*@out@*/ long& neg_thres,
                /*@out@*/ long& pos_thres);

    /// Convert a bytecode into its byte representation.
    void to_bytes(Bytecode& bc, Bytes& bytes, OutputValueFunc output_value,
                  OutputRelocFunc output_reloc = 0);

    SpecialType get_special() const;

    AlignBytecode* clone() const;

private:
    boost::scoped_ptr<Expr> m_boundary;     ///< alignment boundary

    /// What to fill intervening locations with, NULL if using code_fill
    boost::scoped_ptr<Expr> m_fill;

    /// Maximum number of bytes to skip, NULL if no maximum.
    boost::scoped_ptr<Expr> m_maxskip;

    /// Code fill, NULL if using 0 fill
    /*@null@*/ const unsigned char** m_code_fill;
};


AlignBytecode::AlignBytecode(std::auto_ptr<Expr> boundary,
                             /*@null@*/ std::auto_ptr<Expr> fill,
                             /*@null@*/ std::auto_ptr<Expr> maxskip,
                             /*@null@*/ const unsigned char** code_fill)
    : m_boundary(boundary.release()),
      m_fill(fill.release()),
      m_maxskip(maxskip.release()),
      m_code_fill(code_fill)
{
}

AlignBytecode::~AlignBytecode()
{
}

void
AlignBytecode::put(std::ostream& os, int indent_level) const
{
    os << std::setw(indent_level) << "" << "_Align_\n";
    os << std::setw(indent_level) << "" << "Boundary=" << *m_boundary << '\n';
    if (m_fill.get() != 0)
        os << std::setw(indent_level) << "" << "Fill=" << *m_fill << '\n';
    if (m_maxskip.get() != 0)
        os << std::setw(indent_level) << "" << "Max Skip=" << *m_maxskip
           << '\n';
}

void
AlignBytecode::finalize(Bytecode& bc)
{
    if (!m_boundary->get_intnum())
        throw NotConstantError(N_("align boundary must be a constant"));
    if (m_fill.get() != 0 && !m_fill->get_intnum())
        throw NotConstantError(N_("align fill must be a constant"));
    if (m_maxskip.get() != 0 && !m_maxskip->get_intnum())
        throw NotConstantError(N_("align maximum skip must be a constant"));
}

unsigned long
AlignBytecode::calc_len(Bytecode& bc, Bytecode::AddSpanFunc add_span)
{
    unsigned long len = 0;
    long neg_thres = 0;
    long pos_thres = 0;

    expand(bc, len, 0, 0, (long)bc.get_offset(), neg_thres, pos_thres);
    return len;
}

bool
AlignBytecode::expand(Bytecode& bc, unsigned long& len, int span,
                      long old_val, long new_val,
                      /*@out@*/ long& neg_thres, /*@out@*/ long& pos_thres)
{
    unsigned long boundary = m_boundary->get_intnum()->get_uint();

    if (boundary == 0) {
        len = 0;
        pos_thres = new_val;
        return false;
    }

    unsigned long end = (unsigned long)new_val;
    if ((unsigned long)new_val & (boundary-1))
        end = ((unsigned long)new_val & ~(boundary-1)) + boundary;

    pos_thres = (long)end;
    len = end - (unsigned long)new_val;

    if (m_maxskip.get() != 0) {
        unsigned long maxskip = m_maxskip->get_intnum()->get_uint();
        if (len > maxskip) {
            pos_thres = (long)end-maxskip-1;
            len = 0;
        }
    }
    return true;
}

void
AlignBytecode::to_bytes(Bytecode& bc, Bytes& bytes,
                        OutputValueFunc output_value,
                        OutputRelocFunc output_reloc)
{
    unsigned long len;
    unsigned long boundary = m_boundary->get_intnum()->get_uint();

    if (boundary == 0)
        return;
    else {
        unsigned long end = bc.get_offset();
        if (bc.get_offset() & (boundary-1))
            end = (bc.get_offset() & ~(boundary-1)) + boundary;
        len = end - bc.get_offset();
        if (len == 0)
            return;
        if (m_maxskip.get() != 0) {
            unsigned long maxskip = m_maxskip->get_intnum()->get_uint();
            if (len > maxskip)
                return;
        }
    }

    if (m_fill.get() != 0) {
        unsigned long v = m_fill->get_intnum()->get_uint();
        bytes.insert(bytes.end(), len, static_cast<unsigned char>(v));
    } else if (m_code_fill) {
        unsigned long maxlen = 15;
        while (!m_code_fill[maxlen] && maxlen>0)
            maxlen--;
        if (maxlen == 0)
            throw Error(N_("could not find any code alignment size"));

        // Fill with maximum code fill as much as possible
        while (len > maxlen) {
            bytes.insert(bytes.end(),
                         &m_code_fill[maxlen][0],
                         &m_code_fill[maxlen][len]);
            len -= maxlen;
        }

        if (!m_code_fill[len]) {
            throw ValueError(String::compose(N_("invalid alignment size %1"),
                                             len));
        }
        // Handle rest of code fill
        bytes.insert(bytes.end(),
                     &m_code_fill[len][0],
                     &m_code_fill[len][len]);
    } else {
        // Just fill with 0
        bytes.insert(bytes.end(), len, 0);
    }
}

AlignBytecode::SpecialType
AlignBytecode::get_special() const
{
    return SPECIAL_OFFSET;
}

AlignBytecode*
AlignBytecode::clone() const
{
    std::auto_ptr<Expr> fill(0);
    std::auto_ptr<Expr> maxskip(0);

    if (m_fill.get() != 0)
        fill.reset(m_fill->clone());
    if (m_maxskip.get() != 0)
        fill.reset(m_maxskip->clone());
    return new AlignBytecode(std::auto_ptr<Expr>(m_boundary->clone()),
                             fill, maxskip, m_code_fill);
}

} // anonymous namespace

namespace yasm {

std::auto_ptr<Bytecode>
create_align(std::auto_ptr<Expr> boundary,
             /*@null@*/ std::auto_ptr<Expr> fill,
             /*@null@*/ std::auto_ptr<Expr> maxskip,
             /*@null@*/ const unsigned char** code_fill,
             unsigned long line)
{
    Bytecode::Contents::Ptr contents(new AlignBytecode(boundary, fill,
                                                       maxskip, code_fill));
    return std::auto_ptr<Bytecode>(new Bytecode(contents, line));
}

} // namespace yasm

///
/// Reserve bytecode
///
///  Copyright (C) 2001-2007  Peter Johnson
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

#include <iomanip>
#include <ostream>

#include "bytecode.h"
#include "expr.h"


namespace {

using namespace yasm;

class ReserveBytecode : public Bytecode::Contents {
public:
    ReserveBytecode(std::auto_ptr<Expr> numitems, unsigned int itemsize);
    ~ReserveBytecode();

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

    /*@null@*/ const Expr* reserve_numitems
        (/*@out@*/ unsigned int& itemsize) const;

    SpecialType get_special() const;

    ReserveBytecode* clone() const;

private:
    std::auto_ptr<Expr> m_numitems;     ///< number of items to reserve
    unsigned int m_itemsize;            ///< size of each item (in bytes)
};


ReserveBytecode::ReserveBytecode(std::auto_ptr<Expr> numitems,
                                 unsigned int itemsize)
    : m_numitems(numitems),
      m_itemsize(itemsize)
{
}

ReserveBytecode::~ReserveBytecode()
{
}

void
ReserveBytecode::put(std::ostream& os, int indent_level) const
{
    os << std::setw(indent_level) << "" << "_Reserve_\n";
    os << std::setw(indent_level) << "" << "Num Items=";
    if (m_numitems.get() != 0)
        os << *m_numitems;
    else
        os << '1';
    os << '\n';
    os << std::setw(indent_level) << "" << "Item Size=" << m_itemsize << '\n';
}

void
ReserveBytecode::finalize(Bytecode& bc)
{
    // multiply reserve expression into multiple
    bc.multiply_multiple(m_numitems);
}

unsigned long
ReserveBytecode::calc_len(Bytecode& bc, Bytecode::AddSpanFunc add_span)
{
    // As the reserve expression has been transferred to the multiple,
    // this is simply the item size.
    return m_itemsize;
}

bool
ReserveBytecode::expand(Bytecode& bc, unsigned long& len, int span,
                        long old_val, long new_val,
                        /*@out@*/ long& neg_thres,
                        /*@out@*/ long& pos_thres)
{
    return Bytecode::Contents::expand(bc, len, span, old_val, new_val,
                                      neg_thres, pos_thres);
}

void
ReserveBytecode::to_bytes(Bytecode& bc, Bytes& bytes,
                          OutputValueFunc output_value,
                          OutputRelocFunc output_reloc)
{
    Bytecode::Contents::to_bytes(bc, bytes, output_value, output_reloc);
}

ReserveBytecode::SpecialType
ReserveBytecode::get_special() const
{
    return SPECIAL_RESERVE;
}

ReserveBytecode*
ReserveBytecode::clone() const
{
    if (m_numitems.get() != 0)
        return new ReserveBytecode(std::auto_ptr<Expr>(m_numitems->clone()),
                                   m_itemsize);
    else
        return new ReserveBytecode(std::auto_ptr<Expr>(0), m_itemsize);
}

const Expr*
ReserveBytecode::reserve_numitems(unsigned int& itemsize) const
{
    itemsize = m_itemsize;
    return m_numitems.get();
}

} // anonymous namespace

namespace yasm {

Bytecode::Contents::Ptr
create_reserve(std::auto_ptr<Expr> numitems, unsigned int itemsize)
{
    return Bytecode::Contents::Ptr(new ReserveBytecode(numitems, itemsize));
}

} // namespace yasm

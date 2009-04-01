#ifndef BIN_DATA_H
#define BIN_DATA_H
//
// Flat-format binary object format associated data
//
//  Copyright (C) 2002-2008  Peter Johnson
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#include <string>

#include <yasmx/Support/scoped_ptr.h>
#include <yasmx/AssocData.h>
#include <yasmx/IntNum.h>
#include <yasmx/Section.h>
#include <yasmx/Symbol.h>


namespace yasm
{

class Expr;

namespace objfmt
{
namespace bin
{

struct BinSection : public AssocData
{
    static const char* key;

    BinSection();
    ~BinSection();
    void put(marg_ostream& os) const;

    // User-provided alignment
    bool has_align, has_valign;
    IntNum align;
    IntNum valign;

    // User-provided starts
    util::scoped_ptr<Expr> start;
    util::scoped_ptr<Expr> vstart;
    unsigned long start_line;
    unsigned long vstart_line;

    // User-provided follows
    std::string follows;
    std::string vfollows;

    // Calculated (final) starts, used only during output()
    bool has_istart, has_ivstart;

    // Calculated (final) length, used only during output()
    bool has_length;
    IntNum length;
};

// Symbol data is used only for the special symbols section<sectname>.start,
// section<sectname>.vstart, and section<sectname>.length
class BinSymbol : public AssocData
{
public:
    static const char* key;

    enum SpecialSym
    {
        START,
        VSTART,
        LENGTH
    };
    BinSymbol(const Section& sect,
              const BinSection& bsd,
              SpecialSym which);
    ~BinSymbol();
    void put(marg_ostream& os) const;
    bool get_value(/*@out@*/ IntNum* val) const;

private:
    const Section& m_sect;          // referenced section
    const BinSection& m_bsd;        // data for referenced section
    SpecialSym m_which;
};

void bin_simplify(Expr& e);

inline const BinSection*
get_bin_sect(const Section& sect)
{
    return static_cast<const BinSection*>(sect.get_assoc_data(BinSection::key));
}

inline BinSection*
get_bin_sect(Section& sect)
{
    return static_cast<BinSection*>(sect.get_assoc_data(BinSection::key));
}

inline const BinSymbol*
get_bin_sym(const Symbol& sym)
{
    return static_cast<const BinSymbol*>(sym.get_assoc_data(BinSymbol::key));
}

inline BinSymbol*
get_bin_sym(Symbol& sym)
{
    return static_cast<BinSymbol*>(sym.get_assoc_data(BinSymbol::key));
}

inline bool
get_ssym_value(const Symbol& sym, /*@out@*/ IntNum* val)
{
    const BinSymbol* bsym = get_bin_sym(sym);
    if (!bsym)
        return false;
    return bsym->get_value(val);
}

}}} // namespace yasm::objfmt::bin

#endif

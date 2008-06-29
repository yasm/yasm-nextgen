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

#include <boost/scoped_ptr.hpp>

#include <libyasmx/assoc_data.h>
#include <libyasmx/intnum.h>
#include <libyasmx/symbol.h>

namespace yasm
{

class Expr;

namespace objfmt
{
namespace bin
{

struct BinSectionData : public AssocData
{
    BinSectionData();
    ~BinSectionData();
    void put(marg_ostream& os) const;

    bool bss;               // aka nobits

    // User-provided alignment
    bool has_align, has_valign;
    IntNum align;
    IntNum valign;

    // User-provided starts
    boost::scoped_ptr<Expr> start;
    boost::scoped_ptr<Expr> vstart;

    // User-provided follows
    std::string follows;
    std::string vfollows;

    // Calculated (final) starts, used only during output()
    bool has_istart, has_ivstart;
    IntNum istart;
    IntNum ivstart;

    // Calculated (final) length, used only during output()
    bool has_length;
    IntNum length;
};

// Symbol data is used only for the special symbols section<sectname>.start,
// section<sectname>.vstart, and section<sectname>.length
class BinSymbolData : public AssocData
{
public:
    enum SpecialSym
    {
        START,
        VSTART,
        LENGTH
    };
    BinSymbolData(const BinSectionData& bsd, SpecialSym which);
    ~BinSymbolData();
    void put(marg_ostream& os) const;
    const IntNum* get_value() const;

private:
    const BinSectionData& m_bsd;    // data for referenced section
    SpecialSym m_which;
};

void expr_xform(Expr* e, const void* assoc_key);

static inline const IntNum*
get_ssym_value(const Symbol* sym, const void* assoc_key)
{
    const BinSymbolData* bsymd =
        static_cast<const BinSymbolData*>(sym->get_assoc_data(assoc_key));
    if (!bsymd)
        return 0;
    return bsymd->get_value();
}

}}} // namespace yasm::objfmt::bin

#endif

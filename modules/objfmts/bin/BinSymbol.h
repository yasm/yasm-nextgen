#ifndef YASM_BINSYMBOL_H
#define YASM_BINSYMBOL_H
//
// Flat-format binary object format symbol data
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

#include "yasmx/AssocData.h"
#include "yasmx/Symbol.h"


class IntNum;
class Section;

struct BinSection;

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
    bool getValue(/*@out@*/ IntNum* val) const;

private:
    const Section& m_sect;          // referenced section
    const BinSection& m_bsd;        // data for referenced section
    SpecialSym m_which;
};

void BinSimplify(Expr& e);

inline bool
getBinSSymValue(const Symbol& sym, /*@out@*/ IntNum* val)
{
    const BinSymbol* bsym = sym.getAssocData<BinSymbol>();
    if (!bsym)
        return false;
    return bsym->getValue(val);
}

#endif

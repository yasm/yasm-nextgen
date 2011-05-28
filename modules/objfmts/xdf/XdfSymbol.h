#ifndef YASM_XDFSYMBOL_H
#define YASM_XDFSYMBOL_H
//
// Extended Dynamic Object format symbol
//
//  Copyright (C) 2004-2007  Peter Johnson
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
#include "yasmx/Config/export.h"
#include "yasmx/AssocData.h"
#include "yasmx/Symbol.h"
#include "yasmx/SymbolRef.h"


namespace yasm
{
namespace objfmt
{

struct YASM_STD_EXPORT XdfSymbol : public AssocData
{
    static const char* key;

    XdfSymbol(unsigned long index_) : index(index_) {}
    ~XdfSymbol();
    pugi::xml_node Write(pugi::xml_node out) const;

    enum Flags
    {
        XDF_EXTERN = 1,
        XDF_GLOBAL = 2,
        XDF_EQU    = 4
    };

    unsigned long index;                //< assigned XDF symbol table index
};

}} // namespace yasm::objfmt

#endif

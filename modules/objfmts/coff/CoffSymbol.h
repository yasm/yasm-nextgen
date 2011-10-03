#ifndef YASM_COFFSYMBOL_H
#define YASM_COFFSYMBOL_H
//
// COFF object format symbol data
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
#include <vector>

#include "yasmx/Config/export.h"
#include "yasmx/AssocData.h"
#include "yasmx/Symbol.h"


namespace yasm
{

class Bytes;
class DiagnosticsEngine;
class StringTable;

namespace objfmt
{

struct YASM_STD_EXPORT CoffSymbol : public AssocData
{
    static const char* key;

    enum StorageClass
    {
        SCL_EFCN = 0xff,    ///< physical end of function
        SCL_NULL = 0,
        SCL_AUTO = 1,       ///< automatic variable
        SCL_EXT = 2,        ///< external symbol
        SCL_STAT = 3,       ///< static
        SCL_REG = 4,        ///< register variable
        SCL_EXTDEF = 5,     ///< external definition
        SCL_LABEL = 6,      ///< label
        SCL_ULABEL = 7,     ///< undefined label
        SCL_MOS = 8,        ///< member of structure
        SCL_ARG = 9,        ///< function argument
        SCL_STRTAG = 10,    ///< structure tag
        SCL_MOU = 11,       ///< member of union
        SCL_UNTAG = 12,     ///< union tag
        SCL_TPDEF = 13,     ///< type definition
        SCL_USTATIC = 14,   ///< undefined static
        SCL_ENTAG = 15,     ///< enumeration tag
        SCL_MOE = 16,       ///< member of enumeration
        SCL_REGPARM = 17,   ///< register parameter
        SCL_FIELD = 18,     ///< bit field
        SCL_AUTOARG = 19,   ///< auto argument
        SCL_LASTENT = 20,   ///< dummy entry (end of block)
        SCL_BLOCK = 100,    ///< ".bb" or ".eb"
        SCL_FCN = 101,      ///< ".bf" or ".ef"
        SCL_EOS = 102,      ///< end of structure
        SCL_FILE = 103,     ///< file name
        SCL_LINE = 104,     ///< line # reformatted as symbol table entry
        SCL_ALIAS = 105,    ///< duplicate tag
        SCL_HIDDEN = 106    ///< ext symbol in dmert public lib
    };

    struct AuxEntry
    {
        std::string fname;      ///< filename aux entry data
    };

    enum AuxType
    {
        AUX_NONE = 0,
        AUX_SECT,
        AUX_FILE
    };

    CoffSymbol(StorageClass sclass, AuxType auxtype = AUX_NONE);
    ~CoffSymbol();
#ifdef WITH_XML
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML
    void Write(Bytes& bytes,
               const Symbol& sym,
               DiagnosticsEngine& diags,
               StringTable& strtab) const;

    bool m_forcevis;                ///< force visibility in symbol table
    unsigned long m_index;          ///< assigned COFF symbol table index
    StorageClass m_sclass;          ///< storage class
    unsigned int m_type;            ///< type
    AuxType m_auxtype;              ///< type of aux entries
    std::vector<AuxEntry> m_aux;    ///< aux entries
};

}} // namespace yasm::objfmt

#endif

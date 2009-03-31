#ifndef YASM_COFFSECTION_H
#define YASM_COFFSECTION_H
//
// COFF object format section data
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
#include <libyasmx/AssocData.h>
#include <libyasmx/marg_ostream_fwd.h>
#include <libyasmx/Section.h>
#include <libyasmx/SymbolRef.h>

namespace yasm
{

class Bytes;

namespace objfmt
{
namespace coff
{

struct CoffSection : public AssocData
{
    static const char* key;

    CoffSection(SymbolRef sym);
    ~CoffSection();
    void put(marg_ostream& os) const;
    void write(Bytes& bytes, const Section& sect) const;

    static const unsigned long TEXT;
    static const unsigned long DATA;
    static const unsigned long BSS;
    static const unsigned long INFO;
    static const unsigned long STD_MASK;
    static const unsigned long ALIGN_MASK;
    static const unsigned int ALIGN_SHIFT;

    // Win32-specific flags
    static const unsigned long NRELOC_OVFL;
    static const unsigned long DISCARD;
    static const unsigned long NOCACHE;
    static const unsigned long NOPAGE;
    static const unsigned long SHARED;
    static const unsigned long EXECUTE;
    static const unsigned long READ;
    static const unsigned long WRITE;
    static const unsigned long WIN32_MASK;

    SymbolRef m_sym;            ///< symbol created for this section
    unsigned int m_scnum;       ///< section number (1=first section)
    unsigned long m_flags;      ///< section flags (see above)
    unsigned long m_size;       ///< size of raw data (section data) in bytes
    unsigned long m_relptr;     ///< file ptr to relocation
    unsigned long m_strtab_name;///< strtab offset of name if name > 8 chars
    bool m_nobase;              ///< Use no-base (NB) relocs?
    bool m_isdebug;             ///< is a debug section?
    bool m_setalign;            ///< Set alignment in section flags
};

inline CoffSection*
get_coff(Section& sect)
{
    return static_cast<CoffSection*>(sect.get_assoc_data(CoffSection::key));
}

inline const CoffSection*
get_coff(const Section& sect)
{
    return static_cast<const CoffSection*>
        (sect.get_assoc_data(CoffSection::key));
}

}}} // namespace yasm::objfmt::coff

#endif

#ifndef YASM_MACHSYMBOL_H
#define YASM_MACHSYMBOL_H
//
// Mach-O symbol
//
//  Copyright (C) 2007 Henryk Richter, built upon xdf objfmt (C) Peter Johnson
//  Copyright (C) 2010 Peter Johnson
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
#include "yasmx/IntNum.h"


namespace yasm
{
class Bytes;
class Diagnostic;
class StringTable;
class Symbol;

namespace objfmt
{

struct YASM_STD_EXPORT MachSymbol : public AssocData
{
    static const char* key;

    static MachSymbol& Build(Symbol& sym);

    MachSymbol();
    ~MachSymbol();
#ifdef WITH_XML
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML
    void Finalize(const Symbol& sym, Diagnostic& diags);

    void Write(Bytes& bytes,
               const Symbol& sym,
               StringTable& strtab,
               int long_int_size) const;

    enum {
        // Symbol table type field bit masks
        N_STAB  = 0xe0,         // mask indicating stab entry
        N_PEXT  = 0x10,         // private external bit
        N_TYPE  = 0x0e,         // mask for all the type bits
        N_EXT   = 0x01,         // external (global) bit

        // Symbol table type field values
        N_UNDF  = 0x00,         // undefined
        N_ABS   = 0x02,         // absolute address
        N_SECT  = 0x0e,         // symbol is defined in a section
        N_PBUD  = 0x0c,         // prebound undefined (defined in a dylib)
        N_INDR  = 0x0a,         // indirect

        NO_SECT = 0             // no section for symbol in nlist
    };

    // Extra description field values
    enum {
        // reference type bit mask
        REFERENCE_TYPE = 0x0007,
        // reference types
        REFERENCE_FLAG_UNDEFINED_NON_LAZY           = 0x0000,
        REFERENCE_FLAG_UNDEFINED_LAZY               = 0x0001,
        REFERENCE_FLAG_DEFINED                      = 0x0002,
        REFERENCE_FLAG_PRIVATE_DEFINED              = 0x0003,
        REFERENCE_FLAG_PRIVATE_UNDEFINED_NON_LAZY   = 0x0004,
        REFERENCE_FLAG_PRIVATE_UNDEFINED_LAZY       = 0x0005,

        // other flags
        N_NO_DEAD_STRIP = 0x0020,   // symbol is not to be dead stripped
        N_WEAK_REF      = 0x0040,   // symbol is weak referenced
        N_WEAK_DEF      = 0x0080    // coalesced symbol is a weak definition
    };

    void setDesc(unsigned int desc)
    {
        m_desc_override = true;
        m_desc_value = desc;
    }

    // Get computed type (only valid after Finalize() is called).
    unsigned int getType() const { return m_n_type; }

    // flags
    bool m_private_extern;      // private external
    bool m_no_dead_strip;       // don't dead strip symbol
    bool m_weak_ref;            // weak referenced
    bool m_weak_def;            // weak definition
    unsigned int m_ref_flag;    // reference flag

    // Symbol is required to be in symbol table, e.g. if it's used in a reloc.
    bool m_required;
    unsigned long m_index;      // index in output order

private:
    bool m_desc_override;       // m_desc overrides other desc flags
    unsigned int m_desc_value;  // extra description (raw value)

    // finalized (computed) values
    unsigned int m_n_type;
    unsigned int m_n_sect;
    unsigned int m_n_desc;
    IntNum m_value;
};

}} // namespace yasm::objfmt

#endif

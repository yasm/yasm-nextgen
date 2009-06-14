#ifndef YASM_COFFOBJECT_H
#define YASM_COFFOBJECT_H
//
// COFF (DJGPP) object format
//
//  Copyright (C) 2007-2008  Peter Johnson
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
#include <yasmx/ObjectFormat.h>

namespace yasm
{

class DirHelpers;
class IntNum;

namespace objfmt
{
namespace coff
{

class CoffSection;
class CoffSymbol;

class CoffObject : public ObjectFormat
{
public:
    enum Machine
    {
        MACHINE_UNKNOWN = 0,
        MACHINE_I386  = 0x014C,
        MACHINE_AMD64 = 0x8664
    };

    CoffObject(bool set_vma = true,
               bool win32 = false,
               bool win64 = false);
    virtual ~CoffObject();

    virtual std::string get_name() const;
    virtual std::string get_keyword() const;
    virtual void add_directives(Directives& dirs, const std::string& parser);

    virtual bool ok_object(Object* object) const;
    virtual void initialize();

    virtual std::string get_extension() const;
    virtual unsigned int get_default_x86_mode_bits() const;

    virtual std::vector<std::string> get_dbgfmt_keywords() const;
    virtual std::string get_default_dbgfmt_keyword() const;

    virtual void init_symbols(const std::string& parser);
#if 0
    virtual bool taste(std::istream& is,
                       /*@out@*/ std::string* arch_keyword,
                       /*@out@*/ std::string* machine);
    virtual void read(std::istream& is);
#endif
    virtual void output(std::ostream& os, bool all_syms, Errwarns& errwarns);

    virtual Section* add_default_section();
    virtual Section* append_section(const std::string& name,
                                    unsigned long line);

    Machine get_machine() const { return m_machine; }

    bool is_win32() const { return m_win32; }
    bool is_win64() const { return m_win64; }

protected:
    /// Initialize section (and COFF data) based on section name.
    /// @return True if section name recognized, false otherwise.
    virtual bool init_section(const std::string& name,
                              Section& section,
                              CoffSection* coffsect);
    virtual void dir_section_init_helpers(DirHelpers& helpers,
                                          CoffSection* csd,
                                          IntNum* align,
                                          bool* has_align,
                                          unsigned long line);

private:
    // When this is false, all section VMA's are set to 0 rather than as the
    // same as the LMA.  According to the DJGPP COFF Spec, this should be true
    // (VMA=LMA), and indeed DJGPP's GCC output shows VMA=LMA.  However, NASM
    // outputs VMA=0 (as if this was false), and GNU objdump output looks a lot
    // nicer with VMA=0.  For Win32 PE/COFF, this should be false.
    bool m_set_vma;

    bool m_win32;               // win32 or win64 output?
    bool m_win64;               // win64 output?

    enum Flags
    {
        F_LNNO   = 0x0004,     ///< line number info NOT present
        F_LSYMS  = 0x0008,     ///< local symbols NOT present
        F_AR32WR = 0x0100      ///< 32-bit little endian file
    };

    Machine m_machine;              // COFF machine to use

    CoffSymbol* m_file_coffsym;     // Data for .file symbol

    void dir_gas_section(Object& object,
                         NameValues& namevals,
                         NameValues& objext_namevals,
                         unsigned long line);
    void dir_section(Object& object,
                     NameValues& namevals,
                     NameValues& objext_namevals,
                     unsigned long line);
    void dir_ident(Object& object,
                   NameValues& namevals,
                   NameValues& objext_namevals,
                   unsigned long line);
};

}}} // namespace yasm::objfmt::coff

#endif

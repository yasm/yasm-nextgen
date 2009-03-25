#ifndef WIN32_OBJFMT_H
#define WIN32_OBJFMT_H
//
// Win32 object format
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
#include "modules/objfmts/coff/coff-objfmt.h"

namespace yasm
{
namespace objfmt
{
namespace win32
{

using yasm::objfmt::coff::CoffSection;

class Win32Object : public yasm::objfmt::coff::CoffObject
{
public:
    Win32Object();
    virtual ~Win32Object();

    virtual std::string get_name() const;
    virtual std::string get_keyword() const;
    virtual void add_directives(Directives& dirs, const std::string& parser);

    //virtual bool ok_object(Object* object) const;
    //virtual void initialize();

    virtual std::string get_extension() const;
    virtual unsigned int get_default_x86_mode_bits() const;

    virtual std::vector<std::string> get_dbgfmt_keywords() const;
    //virtual std::string get_default_dbgfmt_keyword() const;

    //virtual void init_symbols(const std::string& parser);
    //virtual bool taste(std::istream& is,
    //                   /*@out@*/ std::string* arch_keyword,
    //                   /*@out@*/ std::string* machine);
    //virtual void read(std::istream& is);
    //virtual void output(std::ostream& os, bool all_syms, Errwarns& errwarns);

protected:
    virtual bool init_section(const std::string& name,
                              Section& section,
                              CoffSection* coffsect);
    virtual void dir_section_init_helpers(DirHelpers& helpers,
                                          CoffSection* csd,
                                          IntNum* align,
                                          bool* has_align,
                                          unsigned long line);

protected:
    void dir_export(Object& object,
                    NameValues& namevals,
                    NameValues& objext_namevals,
                    unsigned long line);

private:
    void dir_safeseh(Object& object,
                     NameValues& namevals,
                     NameValues& objext_namevals,
                     unsigned long line);
};

}}} // namespace yasm::objfmt::win32

#endif

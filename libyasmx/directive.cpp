///
/// Directive implementation
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
#include "directive.h"

#include "util.h"

#include <map>

#include "compose.h"
#include "errwarn.h"
#include "name_value.h"
#include "nocase.h"


namespace yasm
{

class Directives::Impl
{
public:
    Impl() {}
    ~Impl() {}

    class Dir
    {
    public:
        Dir(Directive handler, Directives::Flags flags)
            : m_handler(handler), m_flags(flags)
        {}
        ~Dir() {}
        void operator() (Object& object,
                         const std::string& name,
                         NameValues& namevals,
                         NameValues& objext_namevals,
                         unsigned long line);

    private:
        Directive m_handler;
        Directives::Flags m_flags;
    };

    typedef std::map<std::string, Dir> DirMap;
    DirMap m_dirs;
};

Directives::Directives()
    : m_impl(new Impl)
{
}

Directives::~Directives()
{
}

void
Directives::add(const char* name, Directive handler, Flags flags)
{
    m_impl->m_dirs.insert(std::make_pair(String::lowercase(name),
                                         Impl::Dir(handler, flags)));
}

Directive
Directives::operator[] (const std::string& name) const
{
    Impl::DirMap::iterator p = m_impl->m_dirs.find(String::lowercase(name));
    if (p == m_impl->m_dirs.end())
        throw ValueError(String::compose(N_("unrecognized directive `%1'"),
                                         name));

    return BIND::bind(&Impl::Dir::operator(), REF::ref(p->second),
                      _1, name, _2, _3, _4);
}

void
Directives::Impl::Dir::operator() (Object& object,
                                   const std::string& name,
                                   NameValues& namevals,
                                   NameValues& objext_namevals,
                                   unsigned long line)
{
    if ((m_flags & (ARG_REQUIRED|ID_REQUIRED)) && namevals.empty())
        throw SyntaxError(String::compose(
            N_("directive `%1' requires an argument"), name));

    if (!namevals.empty())
    {
        if ((m_flags & ID_REQUIRED) && !namevals.front().is_id())
            throw SyntaxError(String::compose(
                N_("directive `%1' requires an identifier parameter"), name));
    }

    m_handler(object, namevals, objext_namevals, line);
}

} // namespace yasm

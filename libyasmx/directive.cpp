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
#include "expr.h"
#include "intnum.h"
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
                         const NameValues& namevals,
                         const NameValues& objext_namevals,
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
                                   const NameValues& namevals,
                                   const NameValues& objext_namevals,
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


class DirHelpers::Impl
{
public:
    Impl() {}
    ~Impl() {}

    typedef std::map<std::string, FUNCTION::function<void (const NameValue&)> >
        HelperMap;
    HelperMap m_value_helpers, m_novalue_helpers;
};

DirHelpers::DirHelpers()
    : m_impl(new Impl)
{
}

DirHelpers::~DirHelpers()
{
}

void
DirHelpers::add(const char* name, bool needsvalue,
                FUNCTION::function<void (const NameValue&)> helper)
{
    if (needsvalue)
        m_impl->m_value_helpers.insert(std::make_pair(name, helper));
    else
        m_impl->m_novalue_helpers.insert(std::make_pair(name, helper));
}

bool
DirHelpers::operator()
    (NameValues::const_iterator nv_first,
     NameValues::const_iterator nv_last,
     FUNCTION::function<bool (const NameValue&)> helper_nameval)
{
    bool anymatched = false;

    for (NameValues::const_iterator nv=nv_first; nv != nv_last; ++nv)
    {
        bool matched = false;

        if (nv->get_name().empty() && nv->is_id())
        {
            Impl::HelperMap::iterator helper =
                m_impl->m_novalue_helpers.find(nv->get_id());
            if (helper != m_impl->m_novalue_helpers.end())
            {
                helper->second(*nv);
                matched = true;
                anymatched = true;
            }
        }
        else if (!nv->get_name().empty())
        {
            Impl::HelperMap::iterator helper =
                m_impl->m_value_helpers.find(nv->get_name());
            if (helper != m_impl->m_value_helpers.end())
            {
                helper->second(*nv);
                matched = true;
                anymatched = true;
            }
        }

        if (!matched)
        {
            if (helper_nameval(*nv))
                anymatched = true;
        }
    }

    return anymatched;
}

void
dir_intn(const NameValue& nv,
         Object* obj,
         unsigned long line,
         IntNum* out,
         bool* out_set)
{
    std::auto_ptr<Expr> e(nv.get_expr(*obj, line));
    /*@null@*/ IntNum* local;

    if ((e.get() == 0) || ((local = e->get_intnum()) == 0))
        throw NotConstantError(String::compose(
            N_("argument to `%1' is not an integer"), nv.get_name()));

    *out = *local;
    *out_set = true;
}

void
dir_expr(const NameValue& nv,
         Object* obj,
         unsigned long line,
         std::auto_ptr<Expr>* out,
         bool* out_set)
{
    if (!nv.is_expr())
        throw ValueError(String::compose(
            N_("argument to `%1' is not an expression"), nv.get_name()));
    *out = nv.get_expr(*obj, line);
    *out_set = true;
}

void
dir_string(const NameValue& nv, std::string* out, bool* out_set)
{
    if (!nv.is_string())
        throw ValueError(String::compose(
            N_("argument to `%1' is not a string or identifier"),
            nv.get_name()));
    *out = nv.get_string();
    *out_set = true;
}

bool
dir_nameval_warn(const NameValue& nv)
{
    if (!nv.get_name().empty())
    {
        warn_set(WARN_GENERAL,
                 String::compose(N_("Unrecognized qualifier `%1'"),
                                 nv.get_name()));
        return false;
    }

    if (nv.is_id())
        warn_set(WARN_GENERAL,
                 String::compose(N_("Unrecognized qualifier `%1'"),
                                 nv.get_id()));
    else if (nv.is_string())
        warn_set(WARN_GENERAL, N_("Unrecognized string qualifier"));
    else
        warn_set(WARN_GENERAL, N_("Unrecognized numeric qualifier"));

    return false;
}

} // namespace yasm

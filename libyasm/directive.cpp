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
#include "util.h"

#include <map>
#include <sstream>

#include <boost/bind.hpp>

#include "directive.h"
#include "errwarn.h"
#include "expr.h"
#include "intnum.h"


namespace yasm {

class DirectiveManager::Impl {
public:
    Impl() {}
    ~Impl() {}

    class Dir {
    public:
        Dir(Directive handler, DirectiveManager::Flags flags)
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
        DirectiveManager::Flags m_flags;
    };

    typedef std::map<std::string, std::map<std::string, Dir> > Dirs;
    Dirs m_dirs;
};

DirectiveManager::DirectiveManager()
    : m_impl(new Impl)
{
}

DirectiveManager::~DirectiveManager()
{
}

void
DirectiveManager::add(const std::string& name, const std::string& parser,
                      Directive handler, Flags flags)
{
    m_impl->m_dirs[parser].insert(std::make_pair(name,
                                                 Impl::Dir(handler, flags)));
}

Directive
DirectiveManager::get(const std::string& name,
                      const std::string& parser) const
{
    Impl::Dirs::iterator p = m_impl->m_dirs.find(parser);
    if (p == m_impl->m_dirs.end()) {
        std::ostringstream emsg;
        emsg << N_("unrecognized parser") << " `" << parser << "'";
        throw Error(emsg.str());
    }

    std::map<std::string, Impl::Dir>::iterator q = p->second.find(parser);
    if (q == p->second.end()) {
        std::ostringstream emsg;
        emsg << N_("unrecognized directive") << " `" << name << "'";
        throw Error(emsg.str());
    }

    return boost::bind<void>(q->second, _1, name, _2, _3, _4);
}

void
DirectiveManager::Impl::Dir::operator() (Object& object,
                                         const std::string& name,
                                         const NameValues& namevals,
                                         const NameValues& objext_namevals,
                                         unsigned long line)
{
    if ((m_flags & (ARG_REQUIRED|ID_REQUIRED)) && namevals.empty()) {
        std::ostringstream emsg;
        emsg << N_("directive") << " `" << name << "' "
             << N_("requires an argument");
        throw SyntaxError(emsg.str());
    }

    if (!namevals.empty()) {
        if ((m_flags & ID_REQUIRED) && !namevals.front().is_id()) {
            std::ostringstream emsg;
            emsg << N_("directive") << " `" << name << "' "
                 << N_("requires an identifier parameter");
            throw SyntaxError(emsg.str());
        }
    }

    m_handler(object, namevals, objext_namevals, line);
}


class DirHelperManager::Impl {
public:
    Impl() {}
    ~Impl() {}

    typedef std::map<std::string, boost::function<void (const NameValue&)> >
        HelperMap;
    HelperMap m_value_helpers, m_novalue_helpers;
};

DirHelperManager::DirHelperManager()
    : m_impl(new Impl)
{
}

DirHelperManager::~DirHelperManager()
{
}

void
DirHelperManager::add(const std::string& name, bool needsvalue,
                      boost::function<void (const NameValue&)> helper)
{
    if (needsvalue)
        m_impl->m_value_helpers.insert(std::make_pair(name, helper));
    else
        m_impl->m_novalue_helpers.insert(std::make_pair(name, helper));
}

bool
DirHelperManager::operator()
    (NameValues::const_iterator nv_first,
     NameValues::const_iterator nv_last,
     boost::function<bool (const NameValue&)> helper_nameval)
{
    bool anymatched = false;

    for (NameValues::const_iterator nv=nv_first; nv != nv_last; ++nv) {
        bool matched = false;

        if (nv->get_name().empty() && nv->is_id()) {
            Impl::HelperMap::iterator helper =
                m_impl->m_novalue_helpers.find(nv->get_id());
            if (helper != m_impl->m_novalue_helpers.end()) {
                helper->second(*nv);
                matched = true;
                anymatched = true;
            }
        } else if (!nv->get_name().empty()) {
            Impl::HelperMap::iterator helper =
                m_impl->m_value_helpers.find(nv->get_id());
            if (helper != m_impl->m_novalue_helpers.end()) {
                helper->second(*nv);
                matched = true;
                anymatched = true;
            }
        }

        if (!matched) {
            if (helper_nameval(*nv))
                anymatched = true;
        }
    }

    return anymatched;
}

void
dir_intn(const NameValue& nv, Object& obj, unsigned long line, IntNum& out,
         bool& out_set)
{
    std::auto_ptr<Expr> e(nv.get_expr(obj, line));
    /*@null@*/ IntNum* local;

    if ((e.get() == 0) || ((local = e->get_intnum()) == 0)) {
        std::ostringstream emsg;
        emsg << "`" << nv.get_name() << "': "
             << N_("argument must be an integer");
        throw NotConstantError(emsg.str());
    }

    out = *local;
    out_set = true;
}

void
dir_string(const NameValue& nv, std::string& out, bool& out_set)
{
    if (!nv.is_string()) {
        std::ostringstream emsg;
        emsg << "`" << nv.get_name() << "': "
             << N_("argument must be a string or identifier");
        throw ValueError(emsg.str());
    }
    out = nv.get_string();
    out_set = true;
}

bool
dir_nameval_warn(const NameValue& nv)
{
    if (!nv.get_name().empty()) {
        std::ostringstream wmsg;
        wmsg << N_("Unrecognized qualifier") << " `" << nv.get_name() << "'";
        warn_set(WARN_GENERAL, wmsg.str());
        return false;
    }

    if (nv.is_id()) {
        std::ostringstream wmsg;
        wmsg << N_("Unrecognized qualifier") << " `" << nv.get_id() << "'";
        warn_set(WARN_GENERAL, wmsg.str());
    } else if (nv.is_string()) {
        warn_set(WARN_GENERAL, N_("Unrecognized string qualifier"));
    } else {
        warn_set(WARN_GENERAL, N_("Unrecognized numeric qualifier"));
    }

    return false;
}

} // namespace yasm

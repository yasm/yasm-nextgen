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
#include "object.h"
#include "symbol.h"


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

NameValue::NameValue(const std::string& name, const std::string& id,
                     char id_prefix)
    : m_name(name),
      m_type(ID),
      m_idstr(id),
      m_expr(0),
      m_id_prefix(id_prefix)
{
}

NameValue::NameValue(const std::string& name, const std::string& str)
    : m_name(name),
      m_type(STRING),
      m_idstr(str),
      m_expr(0),
      m_id_prefix('\0')
{
}

NameValue::NameValue(const std::string& name, std::auto_ptr<Expr> e)
    : m_name(name),
      m_type(EXPR),
      m_expr(e.release()),
      m_id_prefix('\0')
{
}

NameValue::NameValue(const std::string& id, char id_prefix)
    : m_name(""),
      m_type(ID),
      m_idstr(id),
      m_expr(0),
      m_id_prefix(id_prefix)
{
}

NameValue::NameValue(const std::string& str)
    : m_name(""),
      m_type(STRING),
      m_idstr(str),
      m_expr(0),
      m_id_prefix('\0')
{
}

NameValue::NameValue(std::auto_ptr<Expr> e)
    : m_name(""),
      m_type(EXPR),
      m_expr(e.release()),
      m_id_prefix('\0')
{
}

NameValue::~NameValue()
{
}

/*@null@*/ std::auto_ptr<Expr>
NameValue::get_expr(Object& object, unsigned long line) const
{
    switch (m_type) {
        case ID:
        {
            Symbol* sym = object.get_sym(get_id());
            sym->use(line);
            return std::auto_ptr<Expr>(new Expr(sym, line));
        }
        case EXPR:
            return std::auto_ptr<Expr>(m_expr->clone());
        default:
            return std::auto_ptr<Expr>(0);
    }
}

std::string
NameValue::get_string() const
{
    switch (m_type) {
        case ID:
        case STRING:
            return m_idstr;
        default:
            throw Error(N_("name/value not convertible to string"));
    }
}

std::string
NameValue::get_id() const
{
    if (m_type != ID)
        throw Error(N_("name/value not convertible to identifier"));

    if (m_idstr[0] == m_id_prefix)
        return m_idstr.substr(1);
    else
        return m_idstr;
}

} // namespace yasm

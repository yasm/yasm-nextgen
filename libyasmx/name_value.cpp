///
/// Name/value implementation
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
#include "name_value.h"

#include "util.h"

#include <ostream>

#include "errwarn.h"
#include "expr.h"
#include "object.h"
#include "symbol.h"


namespace yasm
{

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

NameValue::NameValue(const NameValue& oth)
    : m_name(oth.m_name),
      m_type(oth.m_type),
      m_idstr(oth.m_idstr),
      m_expr(oth.m_expr->clone()),
      m_id_prefix(oth.m_id_prefix)
{
}

NameValue::~NameValue()
{
}

void
NameValue::swap(NameValue& oth)
{
    m_name.swap(oth.m_name);
    std::swap(m_type, oth.m_type);
    m_idstr.swap(oth.m_idstr);
    std::swap(m_expr, oth.m_expr);
    std::swap(m_id_prefix, oth.m_id_prefix);
}

/*@null@*/ std::auto_ptr<Expr>
NameValue::get_expr(Object& object, unsigned long line) const
{
    switch (m_type)
    {
        case ID:
        {
            SymbolRef sym = object.get_sym(get_id());
            sym->use(line);
            return std::auto_ptr<Expr>(new Expr(sym, line));
        }
        case EXPR:
            return std::auto_ptr<Expr>(m_expr->clone());
        default:
            return std::auto_ptr<Expr>(0);
    }
}

/*@null@*/ std::auto_ptr<Expr>
NameValue::release_expr(Object& object, unsigned long line)
{
    switch (m_type)
    {
        case ID:
        {
            SymbolRef sym = object.get_sym(get_id());
            sym->use(line);
            return std::auto_ptr<Expr>(new Expr(sym, line));
        }
        case EXPR:
            return m_expr;
        default:
            return std::auto_ptr<Expr>(0);
    }
}

std::string
NameValue::get_string() const
{
    switch (m_type)
    {
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

std::ostream&
operator<< (std::ostream& os, const NameValue& nv)
{
    os << "(\"" << nv.get_name() << "\",";
    switch (nv.m_type)
    {
        case NameValue::ID:
            os << nv.m_idstr;
            break;
        case NameValue::STRING:
            os << '"' << nv.m_idstr << '"';
            break;
        case NameValue::EXPR:
            os << *(nv.m_expr);
            break;
    }
    os << ')';
    return os;
}

NameValues::~NameValues()
{
    // Destroy contained NameValues.  By doing this here, we can guarantee
    // the ptr_vector_owner destructor will be called before the base class
    // (ptr_vector) destructor.
    stdx::ptr_vector_owner<NameValue> owner(*this);
}

std::ostream&
operator<< (std::ostream& os, const NameValues& namevals)
{
    if (namevals.empty())
    {
        os << "(none)";
        return os;
    }

    for (NameValues::const_iterator i=namevals.begin(), end=namevals.end();
         i != end; ++i)
    {
        if (i != namevals.begin())
            os << ',';
        os << *i;
    }
    return os;
}

} // namespace yasm

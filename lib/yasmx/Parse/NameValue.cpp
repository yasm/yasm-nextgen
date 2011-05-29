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
#include "yasmx/Parse/NameValue.h"

#include "yasmx/Expr.h"
#include "yasmx/Object.h"
#include "yasmx/Symbol.h"


using namespace yasm;

NameValue::NameValue(llvm::StringRef name, llvm::StringRef id, char id_prefix)
    : m_name(name),
      m_type(ID),
      m_idstr(id),
      m_expr(0),
      m_id_prefix(id_prefix)
{
}

NameValue::NameValue(llvm::StringRef name, llvm::StringRef str)
    : m_name(name),
      m_type(STRING),
      m_idstr(str),
      m_expr(0),
      m_id_prefix('\0')
{
}

NameValue::NameValue(llvm::StringRef name, std::auto_ptr<Expr> e)
    : m_name(name),
      m_type(EXPR),
      m_expr(e.release()),
      m_id_prefix('\0')
{
}

NameValue::NameValue(llvm::StringRef id, char id_prefix)
    : m_type(ID),
      m_idstr(id),
      m_expr(0),
      m_id_prefix(id_prefix)
{
}

NameValue::NameValue(llvm::StringRef str)
    : m_type(STRING),
      m_idstr(str),
      m_expr(0),
      m_id_prefix('\0')
{
}

NameValue::NameValue(std::auto_ptr<Expr> e)
    : m_type(EXPR),
      m_expr(e.release()),
      m_id_prefix('\0')
{
}

NameValue::NameValue(const Token& token)
    : m_type(TOKEN),
      m_token(token),
      m_id_prefix('\0')
{
    setValueRange(token.getLocation());
}

NameValue::NameValue(const NameValue& oth)
    : m_name(oth.m_name),
      m_type(oth.m_type),
      m_idstr(oth.m_idstr),
      m_expr(oth.m_expr->clone()),
      m_id_prefix(oth.m_id_prefix),
      m_name_source(oth.m_name_source),
      m_equals_source(oth.m_equals_source),
      m_value_range(oth.m_value_range)
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
    std::swap(m_name_source, oth.m_name_source);
    std::swap(m_equals_source, oth.m_equals_source);
    std::swap(m_value_range, oth.m_value_range);
}

Expr
NameValue::getExpr(Object& object) const
{
    switch (m_type)
    {
        case ID:
        {
            SymbolRef sym = object.getSymbol(getId());
            sym->Use(m_value_range.getBegin());
            return Expr(sym);
        }
        case EXPR:
            return *m_expr;
        default:
            assert(false && "name/value not convertible to expression");
            return Expr();
    }
}

std::auto_ptr<Expr>
NameValue::ReleaseExpr(Object& object)
{
    switch (m_type)
    {
        case ID:
        {
            SymbolRef sym = object.getSymbol(getId());
            sym->Use(m_value_range.getBegin());
            return std::auto_ptr<Expr>(new Expr(sym));
        }
        case EXPR:
            return m_expr;
        default:
            return std::auto_ptr<Expr>(0);
    }
}

llvm::StringRef
NameValue::getString() const
{
    switch (m_type)
    {
        case ID:
        case STRING:
            return m_idstr;
        default:
            assert(false && "name/value not convertible to string");
            return "";
    }
}

llvm::StringRef
NameValue::getId() const
{
    assert (m_type == ID && "name/value not convertible to identifier");
    if (m_idstr[0] == m_id_prefix)
        return llvm::StringRef(m_idstr).substr(1);
    else
        return m_idstr;
}

const Token&
NameValue::getToken() const
{
    assert(m_type == TOKEN && "name/value not token");
    return m_token;
}

#ifdef WITH_XML
pugi::xml_node
NameValue::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("NameValue");
    append_child(root, "Name", m_name);
    pugi::xml_node val = root.append_child("Value");
    pugi::xml_attribute type = val.append_attribute("type");
    switch (m_type)
    {
        case ID:
            type = "id";
            append_child(val, "Id", m_idstr);
            append_child(val, "Prefix", llvm::StringRef(&m_id_prefix, 1));
            break;
        case STRING:
            type = "string";
            append_data(val, m_idstr);
            break;
        case EXPR:
            type = "expr";
            append_data(val, *m_expr);
            break;
        case TOKEN:
            type = "token";
            append_data(val, m_token.getKind());
            break;
    }
    return root;
}
#endif // WITH_XML

NameValues::~NameValues()
{
    // Destroy contained NameValues.  By doing this here, we can guarantee
    // the ptr_vector_owner destructor will be called before the base class
    // (ptr_vector) destructor.
    stdx::ptr_vector_owner<NameValue> owner(*this);
}

#ifdef WITH_XML
pugi::xml_node
NameValues::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("NameValues");
    for (const_iterator i=begin(), endi=end(); i != endi; ++i)
        append_data(root, *i);
    return root;
}
#endif // WITH_XML

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
#include "yasmx/NameValue.h"

#include "util.h"

#include "llvm/Support/raw_ostream.h"
#include "YAML/emitter.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Expr.h"
#include "yasmx/Object.h"
#include "yasmx/Symbol.h"


namespace yasm
{

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
    : m_name(""),
      m_type(ID),
      m_idstr(id),
      m_expr(0),
      m_id_prefix(id_prefix)
{
}

NameValue::NameValue(llvm::StringRef str)
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

Expr
NameValue::getExpr(Object& object, clang::SourceLocation source) const
{
    switch (m_type)
    {
        case ID:
        {
            SymbolRef sym = object.getSymbol(getId());
            sym->Use(source);
            return Expr(sym);
        }
        case EXPR:
            return *m_expr;
        default:
            throw Error(N_("name/value not convertible to expression"));
    }
}

std::auto_ptr<Expr>
NameValue::ReleaseExpr(Object& object, clang::SourceLocation source)
{
    switch (m_type)
    {
        case ID:
        {
            SymbolRef sym = object.getSymbol(getId());
            sym->Use(source);
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
            throw Error(N_("name/value not convertible to string"));
    }
}

llvm::StringRef
NameValue::getId() const
{
    if (m_type != ID)
        throw Error(N_("name/value not convertible to identifier"));

    if (m_idstr[0] == m_id_prefix)
        return m_idstr.substr(1);
    else
        return m_idstr;
}

void
NameValue::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << m_name;
    switch (m_type)
    {
        case ID:
            out << YAML::Key << "id" << YAML::Value << m_idstr;
            out << YAML::Key << "prefix" << YAML::Value << m_id_prefix;
            break;
        case STRING:
            out << YAML::Key << "string" << YAML::Value << m_idstr;
            break;
        case EXPR:
            out << YAML::Key << "expr" << YAML::Value << *m_expr;
            break;
    }
    out << YAML::EndMap;
}

void
NameValue::Dump() const
{
    YAML::Emitter out;
    Write(out);
    llvm::errs() << out.c_str() << '\n';
}

NameValues::~NameValues()
{
    // Destroy contained NameValues.  By doing this here, we can guarantee
    // the ptr_vector_owner destructor will be called before the base class
    // (ptr_vector) destructor.
    stdx::ptr_vector_owner<NameValue> owner(*this);
}

void
NameValues::Write(YAML::Emitter& out) const
{
    out << YAML::Flow << YAML::BeginSeq;
    for (const_iterator i=begin(), endi=end(); i != endi; ++i)
        out << *i;
    out << YAML::EndSeq;
}

void
NameValues::Dump() const
{
    YAML::Emitter out;
    Write(out);
    llvm::errs() << out.c_str() << '\n';
}

} // namespace yasm

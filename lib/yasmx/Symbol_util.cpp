//
// Symbol utility implementation.
//
//  Copyright (C) 2001-2008  Michael Urman, Peter Johnson
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
#include "yasmx/Symbol_util.h"

#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Parse/Directive.h"
#include "yasmx/Support/scoped_ptr.h"
#include "yasmx/AssocData.h"
#include "yasmx/Expr.h"
#include "yasmx/Object.h"
#include "yasmx/Symbol.h"


using namespace yasm;

namespace {
class ObjextNameValues : public AssocData
{
public:
    static const char* key;

    ObjextNameValues(NameValues& nvs) { m_nvs.swap(nvs); }
    ~ObjextNameValues();

#ifdef WITH_XML
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    const NameValues& get() const { return m_nvs; }
    NameValues& get() { return m_nvs; }

private:
    NameValues m_nvs;
};
} // anonymous namespace

const char* ObjextNameValues::key = "ObjextNameValues";

ObjextNameValues::~ObjextNameValues()
{
}

#ifdef WITH_XML
pugi::xml_node
ObjextNameValues::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("Objext");
    root.append_attribute("key") = key;
    append_data(root, m_nvs);
    return root;
}
#endif // WITH_XML

namespace {
class CommonSize : public AssocData
{
public:
    static const char* key;

    CommonSize(const Expr& e) : m_expr(e) {}
    ~CommonSize();

#ifdef WITH_XML
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

    const Expr* get() const { return &m_expr; }
    Expr* get() { return &m_expr; }

private:
    Expr m_expr;
};
} // anonymous namespace

const char* CommonSize::key = "CommonSize";

CommonSize::~CommonSize()
{
}

#ifdef WITH_XML
pugi::xml_node
CommonSize::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("CommonSize");
    root.append_attribute("key") = key;
    append_child(root, "Size", m_expr);
    return root;
}
#endif // WITH_XML

void
yasm::setObjextNameValues(Symbol& sym, NameValues& objext_namevals)
{
    sym.AddAssocData(std::auto_ptr<ObjextNameValues>
                     (new ObjextNameValues(objext_namevals)));
}

const NameValues*
yasm::getObjextNameValues(const Symbol& sym)
{
    const ObjextNameValues* x = sym.getAssocData<ObjextNameValues>();
    if (!x)
        return 0;
    return &x->get();
}

NameValues*
yasm::getObjextNameValues(Symbol& sym)
{
    ObjextNameValues* x = sym.getAssocData<ObjextNameValues>();
    if (!x)
        return 0;
    return &x->get();
}

void
yasm::setCommonSize(Symbol& sym, const Expr& common_size)
{
    sym.AddAssocData(std::auto_ptr<CommonSize>(new CommonSize(common_size)));
}

const Expr*
yasm::getCommonSize(const Symbol& sym)
{
    const CommonSize* x = sym.getAssocData<CommonSize>();
    if (!x)
        return 0;
    return x->get();
}

Expr*
yasm::getCommonSize(Symbol& sym)
{
    CommonSize* x = sym.getAssocData<CommonSize>();
    if (!x)
        return 0;
    return x->get();
}

void
yasm::DirExtern(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    Object& object = info.getObject();
    NameValue& nv = info.getNameValues().front();
    SymbolRef sym = object.getSymbol(nv.getId());
    sym->CheckedDeclare(Symbol::EXTERN, nv.getValueRange().getBegin(), diags);

    if (!info.getObjextNameValues().empty())
        setObjextNameValues(*sym, info.getObjextNameValues());
}

void
yasm::DirExternMulti(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    Object& object = info.getObject();
    NameValues& nvs = info.getNameValues();
    for (NameValues::const_iterator i=nvs.begin(), end=nvs.end(); i != end; ++i)
    {
        if (i->isId())
        {
            SymbolRef sym = object.getSymbol(i->getId());
            sym->CheckedDeclare(Symbol::EXTERN, i->getValueRange().getBegin(),
                                diags);
        }
        else
            diags.Report(i->getValueRange().getBegin(),
                         diag::err_expected_ident);
    }
}

void
yasm::DirGlobal(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    Object& object = info.getObject();
    NameValue& nv = info.getNameValues().front();
    SymbolRef sym = object.getSymbol(nv.getId());
    sym->CheckedDeclare(Symbol::GLOBAL, nv.getValueRange().getBegin(), diags);

    if (!info.getObjextNameValues().empty())
        setObjextNameValues(*sym, info.getObjextNameValues());
}

void
yasm::DirGlobalMulti(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    Object& object = info.getObject();
    NameValues& nvs = info.getNameValues();
    for (NameValues::const_iterator i=nvs.begin(), end=nvs.end(); i != end; ++i)
    {
        if (i->isId())
        {
            SymbolRef sym = object.getSymbol(i->getId());
            sym->CheckedDeclare(Symbol::GLOBAL, i->getValueRange().getBegin(),
                                diags);
        }
        else
            diags.Report(i->getValueRange().getBegin(),
                         diag::err_expected_ident);
    }
}

void
yasm::DirCommon(DirectiveInfo& info, DiagnosticsEngine& diags)
{
    NameValues& namevals = info.getNameValues();
    if (namevals.size() < 2)
    {
        diags.Report(info.getSource(), diag::err_no_size);
        return;
    }

    NameValue& size_nv = namevals[1];
    if (!size_nv.isExpr())
    {
        diags.Report(info.getSource(), diag::err_size_expression)
            << size_nv.getValueRange();
        return;
    }

    Object& object = info.getObject();
    NameValue& name_nv = namevals.front();
    SymbolRef sym = object.getSymbol(name_nv.getId());
    sym->CheckedDeclare(Symbol::COMMON, name_nv.getValueRange().getBegin(),
                        diags);

    setCommonSize(*sym, size_nv.getExpr(object));

    if (!info.getObjextNameValues().empty())
        setObjextNameValues(*sym, info.getObjextNameValues());
}

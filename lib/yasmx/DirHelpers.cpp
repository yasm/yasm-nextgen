///
/// Directive helpers implementation
///
///  Copyright (C) 2001-2008  Peter Johnson
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
#include "yasmx/DirHelpers.h"

#include "util.h"

#include "llvm/ADT/StringMap.h"
#include "yasmx/Support/Compose.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/nocase.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/NameValue.h"


namespace yasm
{

class DirHelpers::Impl
{
public:
    Impl() {}
    ~Impl() {}

    typedef llvm::StringMap<FUNCTION::function<void (NameValue&)> > HelperMap;
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
DirHelpers::Add(llvm::StringRef name,
                bool needsvalue,
                FUNCTION::function<void (NameValue&)> helper)
{
    if (needsvalue)
        m_impl->m_value_helpers[String::Lowercase(name)] = helper;
    else
        m_impl->m_novalue_helpers[String::Lowercase(name)] = helper;
}

bool
DirHelpers::operator()
    (NameValues::iterator nv_first,
     NameValues::iterator nv_last,
     FUNCTION::function<bool (NameValue&)> helper_nameval)
{
    bool anymatched = false;

    for (NameValues::iterator nv=nv_first; nv != nv_last; ++nv)
    {
        bool matched = false;

        if (nv->getName().empty() && nv->isId())
        {
            Impl::HelperMap::iterator helper =
                m_impl->m_novalue_helpers.find(String::Lowercase(nv->getId()));
            if (helper != m_impl->m_novalue_helpers.end())
            {
                helper->second(*nv);
                matched = true;
                anymatched = true;
            }
        }
        else if (!nv->getName().empty())
        {
            Impl::HelperMap::iterator helper =
                m_impl->m_value_helpers.find(String::Lowercase(nv->getName()));
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
DirIntNum(NameValue& nv,
          Object* obj,
          clang::SourceLocation source,
          IntNum* out,
          bool* out_set)
{
    std::auto_ptr<Expr> e(nv.ReleaseExpr(*obj, source));

    if ((e.get() == 0) || !e->isIntNum())
        throw NotConstantError(String::Compose(
            N_("argument to `%1' is not an integer"), nv.getName()));

    *out = e->getIntNum();
    *out_set = true;
}

void
DirExpr(NameValue& nv,
        Object* obj,
        clang::SourceLocation source,
        std::auto_ptr<Expr>* out,
        bool* out_set)
{
    if (!nv.isExpr())
        throw ValueError(String::Compose(
            N_("argument to `%1' is not an expression"), nv.getName()));
    *out = nv.ReleaseExpr(*obj, source);
    *out_set = true;
}

void
DirString(NameValue& nv, std::string* out, bool* out_set)
{
    if (!nv.isString())
        throw ValueError(String::Compose(
            N_("argument to `%1' is not a string or identifier"),
            nv.getName()));
    *out = nv.getString();
    *out_set = true;
}

bool
DirNameValueWarn(NameValue& nv)
{
    if (!nv.getName().empty())
    {
        setWarn(WARN_GENERAL,
                String::Compose(N_("Unrecognized qualifier `%1'"),
                                nv.getName()));
        return false;
    }

    if (nv.isId())
        setWarn(WARN_GENERAL,
                String::Compose(N_("Unrecognized qualifier `%1'"),
                                nv.getId()));
    else if (nv.isString())
        setWarn(WARN_GENERAL, N_("Unrecognized string qualifier"));
    else
        setWarn(WARN_GENERAL, N_("Unrecognized numeric qualifier"));

    return false;
}

} // namespace yasm

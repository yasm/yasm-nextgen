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

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"
#include "yasmx/Diagnostic.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/NameValue.h"


using namespace yasm;

static llvm::APInt staticbv(IntNum::BITVECT_NATIVE_SIZE, 0);

namespace yasm
{

class DirHelpers::Impl
{
public:
    typedef llvm::StringMap<DirHelper> HelperMap;
    HelperMap m_value_helpers, m_novalue_helpers;
};

} // namespace yasm

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
                DirHelper helper)
{
    if (needsvalue)
        m_impl->m_value_helpers[llvm::LowercaseString(name)] = helper;
    else
        m_impl->m_novalue_helpers[llvm::LowercaseString(name)] = helper;
}

bool
DirHelpers::operator()
    (NameValues::iterator nv_first,
     NameValues::iterator nv_last,
     clang::SourceLocation dir_source,
     Diagnostic& diags,
     FUNCTION::function<bool (NameValue& nv,
                              clang::SourceLocation dir_source,
                              Diagnostic& diags)>
         helper_nameval)
{
    bool anymatched = false;

    for (NameValues::iterator nv=nv_first; nv != nv_last; ++nv)
    {
        bool matched = false;

        if (nv->getName().empty() && nv->isId())
        {
            Impl::HelperMap::iterator helper =
                m_impl->m_novalue_helpers.find(llvm::LowercaseString(nv->getId()));
            if (helper != m_impl->m_novalue_helpers.end())
            {
                helper->second(*nv, diags);
                matched = true;
                anymatched = true;
            }
        }
        else if (!nv->getName().empty())
        {
            Impl::HelperMap::iterator helper =
                m_impl->m_value_helpers.find(llvm::LowercaseString(nv->getName()));
            if (helper != m_impl->m_value_helpers.end())
            {
                helper->second(*nv, diags);
                matched = true;
                anymatched = true;
            }
        }

        if (!matched)
        {
            if (helper_nameval(*nv, dir_source, diags))
                anymatched = true;
        }
    }

    return anymatched;
}

void
yasm::DirIntNumPower2(NameValue& nv,
                      Diagnostic& diags,
                      Object* obj,
                      IntNum* out,
                      bool* out_set)
{
    std::auto_ptr<Expr> e(nv.ReleaseExpr(*obj));

    if ((e.get() == 0) || !e->isIntNum())
    {
        diags.Report(nv.getNameSource(), diag::err_value_integer)
            << nv.getValueSource();
        return;
    }

    if (!e->getIntNum().getBV(&staticbv)->isPowerOf2())
    {
        diags.Report(nv.getNameSource(), diag::err_value_power2)
            << nv.getValueSource();
        return;
    }

    *out = e->getIntNum();
    *out_set = true;
}

void
yasm::DirIntNum(NameValue& nv,
                Diagnostic& diags,
                Object* obj,
                IntNum* out,
                bool* out_set)
{
    std::auto_ptr<Expr> e(nv.ReleaseExpr(*obj));

    if ((e.get() == 0) || !e->isIntNum())
    {
        diags.Report(nv.getNameSource(), diag::err_value_integer)
            << nv.getValueSource();
        return;
    }

    *out = e->getIntNum();
    *out_set = true;
}

void
yasm::DirExpr(NameValue& nv,
              Diagnostic& diags,
              Object* obj,
              std::auto_ptr<Expr>* out,
              bool* out_set)
{
    if (!nv.isExpr())
    {
        diags.Report(nv.getNameSource(), diag::err_value_expression)
            << nv.getValueSource();
        return;
    }
    *out = nv.ReleaseExpr(*obj);
    *out_set = true;
}

void
yasm::DirString(NameValue& nv,
                Diagnostic& diags,
                std::string* out,
                bool* out_set)
{
    if (!nv.isString())
    {
        diags.Report(nv.getNameSource(), diag::err_value_string_or_id)
            << nv.getValueSource();
        return;
    }
    *out = nv.getString();
    *out_set = true;
}

bool
yasm::DirNameValueWarn(NameValue& nv,
                       clang::SourceLocation dir_source,
                       Diagnostic& diags)
{
    if (nv.getNameSource().isValid())
    {
        diags.Report(dir_source, diag::warn_unrecognized_qualifier)
            << nv.getNameSource();
        return false;
    }

    diags.Report(dir_source, diag::warn_unrecognized_qualifier)
        << nv.getValueSource();

    return false;
}

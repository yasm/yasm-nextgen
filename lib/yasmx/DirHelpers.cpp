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
DirHelpers::add(const char* name, bool needsvalue,
                FUNCTION::function<void (NameValue&)> helper)
{
    if (needsvalue)
        m_impl->m_value_helpers[name] = helper;
    else
        m_impl->m_novalue_helpers[name] = helper;
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
dir_intn(NameValue& nv,
         Object* obj,
         unsigned long line,
         IntNum* out,
         bool* out_set)
{
    std::auto_ptr<Expr> e(nv.release_expr(*obj, line));

    if ((e.get() == 0) || !e->is_intnum())
        throw NotConstantError(String::compose(
            N_("argument to `%1' is not an integer"), nv.get_name()));

    *out = e->get_intnum();
    *out_set = true;
}

void
dir_expr(NameValue& nv,
         Object* obj,
         unsigned long line,
         std::auto_ptr<Expr>* out,
         bool* out_set)
{
    if (!nv.is_expr())
        throw ValueError(String::compose(
            N_("argument to `%1' is not an expression"), nv.get_name()));
    *out = nv.release_expr(*obj, line);
    *out_set = true;
}

void
dir_string(NameValue& nv, std::string* out, bool* out_set)
{
    if (!nv.is_string())
        throw ValueError(String::compose(
            N_("argument to `%1' is not a string or identifier"),
            nv.get_name()));
    *out = nv.get_string();
    *out_set = true;
}

bool
dir_nameval_warn(NameValue& nv)
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

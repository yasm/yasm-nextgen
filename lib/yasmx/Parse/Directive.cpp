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
#include "yasmx/Parse/Directive.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"
#include "yasmx/Basic/Diagnostic.h"


using namespace yasm;

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
        Dir() {}
        Dir(Directive handler, Directives::Flags flags)
            : m_handler(handler), m_flags(flags)
        {}
        ~Dir() {}
        void operator() (StringRef name,
                         DirectiveInfo& info,
                         DiagnosticsEngine& diags);

    private:
        Directive m_handler;
        Directives::Flags m_flags;
    };

    typedef llvm::StringMap<Dir, llvm::BumpPtrAllocator, false> DirMap;
    DirMap m_dirs;
};

} // namespace yasm

Directives::Directives()
    : m_impl(new Impl)
{
}

Directives::~Directives()
{
}

void
Directives::Add(StringRef name, Directive handler, Flags flags)
{
    m_impl->m_dirs.GetOrCreateValue(name, Impl::Dir(handler, flags))
        .setCaseInsensitive();
}

bool
Directives::get(Directive* dir, StringRef name) const
{
    Impl::DirMap::iterator p = m_impl->m_dirs.find(name);
    if (p == m_impl->m_dirs.end())
        return false;

    *dir =
        TR1::bind(&Impl::Dir::operator(), TR1::ref(p->second), name, _1, _2);
    return true;
}

void
Directives::Impl::Dir::operator() (StringRef name,
                                   DirectiveInfo& info,
                                   DiagnosticsEngine& diags)
{
    NameValues& namevals = info.getNameValues();
    if ((m_flags & (ARG_REQUIRED|ID_REQUIRED)) && namevals.empty())
    {
        diags.Report(info.getSource(), diag::err_directive_no_args);
        return;
    }

    if (!namevals.empty() && (m_flags & ID_REQUIRED) &&
        !namevals.front().isId())
    {
        diags.Report(info.getSource(), diag::err_value_id)
            << namevals.front().getValueRange();
        return;
    }

    m_handler(info, diags);
}

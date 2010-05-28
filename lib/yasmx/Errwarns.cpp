//
// Error and warning set reporting.
//
//  Copyright (C) 2001-2007  Peter Johnson
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
#include "yasmx/Errwarns.h"

#include "util.h"

#include <algorithm>

#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/ADT/StringRef.h"
#include "yasmx/Support/errwarn.h"


using namespace yasm;

namespace {
class EWData
{
public:
    EWData(clang::SourceRange source, const Error& err);
    EWData(clang::SourceRange source, const std::string& wmsg);
    ~EWData();

    enum { ERROR, WARNING, PARSERERROR } m_type;

    clang::SourceRange m_source;
    clang::SourceRange m_xrefsource;
    std::string m_message;
    std::string m_xrefmsg;
};

class EWDataCompare
{
    const clang::SourceManager& m_mgr;

public:
    EWDataCompare(const clang::SourceManager& mgr) : m_mgr(mgr) {}
    bool operator() (const EWData& lhs,
                     const EWData& rhs)
    {
        return m_mgr.isBeforeInTranslationUnit(lhs.m_source.getBegin(),
                                               rhs.m_source.getBegin());
    }
};
} // anonymous namespace

EWData::EWData(clang::SourceRange source, const Error& err)
    : m_type(ERROR),
      m_source(source),
      m_xrefsource(err.m_xrefsource),
      m_message(err.m_message),
      m_xrefmsg(err.m_xrefmsg)
{
    if (err.m_parse_error)
        m_type = PARSERERROR;
}

EWData::EWData(clang::SourceRange source, const std::string& wmsg)
    : m_type(WARNING),
      m_source(source),
      m_message(wmsg)
{
}

EWData::~EWData()
{
}

namespace yasm {
class Errwarns::Impl
{
public:
    Impl();
    ~Impl();

    std::vector<EWData> m_errwarns;
    int m_ecount, m_wcount;
};
} // namespace yasm

Errwarns::Impl::Impl()
    : m_ecount(0), m_wcount(0)
{
}

Errwarns::Impl::~Impl()
{
}

Errwarns::Errwarns()
    : m_impl(new Impl)
{
}

Errwarns::~Errwarns()
{
}

void
Errwarns::Propagate(clang::SourceRange source, const Error& err)
{
    clang::SourceRange real_source =
        err.m_source.isValid() ? err.m_source : source;
    m_impl->m_errwarns.push_back(EWData(real_source, err));
    m_impl->m_ecount++;
    Propagate(source);  // propagate warnings
}

void
Errwarns::Propagate(clang::SourceRange source)
{
    WarnClass wclass;
    std::string wmsg;
    clang::SourceRange wsource;

    while ((wclass = FetchWarn(&wmsg, &wsource)) != WARN_NONE)
    {
        clang::SourceRange real_source =
            wsource.isValid() ? wsource : source;
        m_impl->m_errwarns.push_back(EWData(real_source, wmsg));
        m_impl->m_wcount++;
    }
}

unsigned int
Errwarns::getNumErrors(bool warning_as_error) const
{
    if (warning_as_error)
        return m_impl->m_ecount + m_impl->m_wcount;
    else
        return m_impl->m_ecount;
}

void
Errwarns::OutputAll(const clang::SourceManager& source_mgr,
                    int warning_as_error,
                    PrintErrorFunc print_error,
                    PrintWarningFunc print_warning)
{
    // If we're treating warnings as errors, tell the user about it.
    if (warning_as_error == 1)
        print_error(source_mgr, clang::SourceRange(),
                    gettext_hook(N_("warnings being treated as errors")),
                    clang::SourceRange(), "");

    // Sort the error/warnings before output.
    std::stable_sort(m_impl->m_errwarns.begin(), m_impl->m_errwarns.end(),
                     EWDataCompare(source_mgr));

    // Output error/warnings.
    for (std::vector<EWData>::iterator i=m_impl->m_errwarns.begin(),
         end=m_impl->m_errwarns.end(); i != end; ++i)
    {
        // Don't output a PARSERERROR if there's another error on the same
        // line.
        if (i->m_type == EWData::PARSERERROR
            && i->m_source == (i+1)->m_source
            && (i+1)->m_type == EWData::ERROR)
            continue;

        // Output error/warning
        if (i->m_type == EWData::ERROR
            || i->m_type == EWData::PARSERERROR)
            print_error(source_mgr, i->m_source, i->m_message,
                        i->m_xrefsource, i->m_xrefmsg);
        else
            print_warning(source_mgr, i->m_source, i->m_message);
    }
}

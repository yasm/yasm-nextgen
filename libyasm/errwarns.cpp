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
#include "errwarns.h"

#include "util.h"

#include <algorithm>
#include <list>

#include "errwarn.h"
#include "linemap.h"


namespace yasm {

Errwarns::Errwarns()
    : m_ecount(0),
      m_wcount(0)
{
}

Errwarns::~Errwarns()
{
}

Errwarns::Data::Data(unsigned long line, const Error& err)
    : m_type(ERROR),
      m_line(line),
      m_xrefline(err.m_xrefline),
      m_message(err.m_message),
      m_xrefmsg(err.m_xrefmsg)
{
    if (err.m_parse_error)
        m_type = PARSERERROR;
}

Errwarns::Data::Data(unsigned long line, const std::string& wmsg)
    : m_type(WARNING),
      m_line(line),
      m_xrefline(0),
      m_message(wmsg)
{
}

Errwarns::Data::~Data()
{
}

void
Errwarns::propagate(unsigned long line, const Error& err)
{
    m_errwarns.push_back(Data(line, err));
    m_ecount++;
    propagate(line);    // propagate warnings
}

void
Errwarns::propagate(unsigned long line)
{
    WarnClass wclass;
    std::string wmsg;

    while ((wclass = warn_fetch(wmsg)) != WARN_NONE) {
        m_errwarns.push_back(Data(line, wmsg));
        m_wcount++;
    }
}

unsigned int
Errwarns::num_errors(bool warning_as_error) const
{
    if (warning_as_error)
        return m_ecount+m_wcount;
    else
        return m_ecount;
}

void
Errwarns::output_all(const Linemap& lm, int warning_as_error,
                     yasm_print_error_func print_error,
                     yasm_print_warning_func print_warning)
{
    // If we're treating warnings as errors, tell the user about it.
    if (warning_as_error == 1)
        print_error("", 0,
                    gettext_hook(N_("warnings being treated as errors")),
                    NULL, 0, NULL);

    // Sort the error/warnings in virtual line order before output.
    std::stable_sort(m_errwarns.begin(), m_errwarns.end());

    // Output error/warnings.
    for (std::vector<Data>::iterator i=m_errwarns.begin(),
         end=m_errwarns.end(); i != end; ++i) {
        // Get the physical location
        std::string filename, xref_filename;
        unsigned long line, xref_line;
        lm.lookup(i->m_line, filename, line);

        // Get the cross-reference physical location
        if (i->m_xrefline != 0)
            lm.lookup(i->m_xrefline, xref_filename, xref_line);
        else {
            xref_filename = "";
            xref_line = 0;
        }

        // Don't output a PARSERERROR if there's another error on the same
        // line.
        if (i->m_type == Data::PARSERERROR && i->m_line == (i+1)->m_line
            && (i+1)->m_type == Data::ERROR)
            continue;

        // Output error/warning
        if (i->m_type == Data::ERROR || i->m_type == Data::PARSERERROR)
            print_error(filename, line, i->m_message, xref_filename,
                        xref_line, i->m_xrefmsg);
        else
            print_warning(filename, line, i->m_message);
    }
}

} // namespace yasm

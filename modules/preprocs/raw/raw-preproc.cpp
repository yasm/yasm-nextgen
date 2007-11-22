//
// Raw preprocessor (preforms NO preprocessing)
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
#include "util.h"

#include <libyasm/errwarn.h>
#include <libyasm/factory.h>
#include <libyasm/linemap.h>
#include <libyasm/preproc.h>

namespace yasm {
namespace preproc {
namespace raw {

class RawPreproc : public Preprocessor {
public:
    RawPreproc() {}
    ~RawPreproc() {}

    std::string get_name() const { return "Disable preprocessing"; }
    std::string get_keyword() const { return "raw"; }

    std::istream& get_stream()
    {
        return *m_is;
    }

    void init(std::istream& is, const std::string& in_filename,
              Linemap& linemap, Errwarns& errwarns);

    std::size_t input(/*@out@*/ char* buf, std::size_t max_size);

    std::string get_included_file() { return ""; }
    void add_include_file(const std::string& filename) {}
    void predefine_macro(const std::string& macronameval) {}
    void undefine_macro(const std::string& macroname) {}
    void define_builtin(const std::string& macronameval) {}

private:
    std::istream* m_is;
    Linemap* m_linemap;
    Errwarns* m_errwarns;
};

void
RawPreproc::init(std::istream& is, const std::string& in_filename,
                 Linemap& linemap, Errwarns& errwarns)
{
    m_is = &is;
    m_linemap = &linemap;
    m_errwarns = &errwarns;
}

std::size_t
RawPreproc::input(/*@out@*/ char* buf, std::size_t max_size)
{
    std::size_t n = m_is->readsome(buf, max_size);

    if (m_is->bad()) {
        m_errwarns->propagate(m_linemap->get_current(),
                              IOError(N_("error when reading from file")));
    }

    return n;
}

registerModule<Preprocessor, RawPreproc> registerRawPreproc("raw");
bool static_ref = true;

}}} // namespace yasm::preproc::raw

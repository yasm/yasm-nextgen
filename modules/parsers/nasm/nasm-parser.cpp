//
// NASM-compatible parser
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
#include "nasm-parser.h"

#include <util.h>

#include <libyasm/arch.h>
#include <libyasm/directive.h>
#include <libyasm/expr.h>
#include <libyasm/factory.h>
#include <libyasm/object.h>
#include <libyasm/section.h>
#include <libyasm/nocase.h>


namespace yasm { namespace parser { namespace nasm {

NasmParser::NasmParser()
{
}

NasmParser::~NasmParser()
{
}

std::string
NasmParser::get_name() const
{
    return "NASM-compatible parser";
}

std::string
NasmParser::get_keyword() const
{
    return "nasm";
}

void
NasmParser::parse(Object& object, Preprocessor& preproc, bool save_input,
                  Linemap& linemap, Errwarns& errwarns)
{
    m_object = &object;
    m_preproc = &preproc;
    m_linemap = &linemap;
    m_errwarns = &errwarns;
    m_arch = object.get_arch();
    m_wordsize = m_arch->get_wordsize();

    m_locallabel_base = "";

    m_prev_bc = &(m_object->get_cur_section()->bcs_first());

    m_save_input = save_input;

    m_peek_token = NONE;

    m_absstart.reset(0);
    m_abspos.reset(0);

    m_state = INITIAL;

    do_parse();

    // Check for undefined symbols
    object.symbols_finalize(errwarns, false);
}

std::vector<std::string>
NasmParser::get_preproc_keywords() const
{
    // valid preprocessors to use with this parser
    static const char* keywords[] = {"raw", "nasm"};
    return std::vector<std::string>(keywords, keywords+NELEMS(keywords));
}

std::string
NasmParser::get_default_preproc_keyword() const
{
    //return "nasm";
    return "raw";
}

void
NasmParser::add_directives(Directives& dirs, const std::string& parser)
{
    if (!String::nocase_equal(parser, "nasm"))
        return;
    dirs.add("absolute",
             BIND::bind(&NasmParser::dir_absolute, this, _1, _2, _3, _4),
             Directives::ARG_REQUIRED);
    dirs.add("align",
             BIND::bind(&NasmParser::dir_align, this, _1, _2, _3, _4),
             Directives::ARG_REQUIRED);
    dirs.add("default",
             BIND::bind(&NasmParser::dir_default, this, _1, _2, _3, _4));
}

registerModule<Parser, NasmParser> registerRawPreproc("nasm");
bool static_ref = true;

}}} // namespace yasm::parser::nasm

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

#include <libyasmx/arch.h>
#include <libyasmx/directive.h>
#include <libyasmx/expr.h>
#include <libyasmx/nocase.h>
#include <libyasmx/object.h>
#include <libyasmx/section.h>
#include <libyasmx/registry.h>


namespace yasm
{
namespace parser
{
namespace nasm
{

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
NasmParser::parse(Object& object,
                  Preprocessor& preproc,
                  bool save_input,
                  Directives& dirs,
                  Linemap& linemap,
                  Errwarns& errwarns)
{
    m_object = &object;
    m_preproc = &preproc;
    m_dirs = &dirs;
    m_linemap = &linemap;
    m_errwarns = &errwarns;
    m_arch = object.get_arch();
    m_wordsize = m_arch->get_wordsize();

    m_locallabel_base = "";

    m_bc = 0;

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
    static const Directives::Init<NasmParser> nasm_dirs[] =
    {
        {"absolute", &NasmParser::dir_absolute, Directives::ARG_REQUIRED},
        {"align", &NasmParser::dir_align, Directives::ARG_REQUIRED},
        {"default", &NasmParser::dir_default, Directives::ANY},
    };

    if (String::nocase_equal(parser, "nasm"))
        dirs.add_array(this, nasm_dirs, NELEMS(nasm_dirs));
}

void
do_register()
{
    register_module<Parser, NasmParser>("nasm");
}

}}} // namespace yasm::parser::nasm

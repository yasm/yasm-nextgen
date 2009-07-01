//
// GAS-compatible parser
//
//  Copyright (C) 2005-2007  Peter Johnson
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the author nor the names of other contributors
//    may be used to endorse or promote products derived from this
//    software without specific prior written permission.
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
#include "GasParser.h"

#include <util.h>

#include <yasmx/Support/errwarn.h>
#include <yasmx/Support/nocase.h>
#include <yasmx/Support/registry.h>
#include <yasmx/Arch.h>
#include <yasmx/Directive.h>
#include <yasmx/Errwarns.h>
#include <yasmx/Object.h>
#include <yasmx/Preprocessor.h>
#include <yasmx/Symbol_util.h>


namespace yasm
{
namespace parser
{
namespace gas
{

GasParser::GasParser(const ParserModule& module, Errwarns& errwarns)
    : Parser(module, errwarns)
    , m_rept_owner(m_rept)
{
    static const GasDirLookup gas_dirs_init[] =
    {
        // FIXME: Whether this is power-of-two or not depends on arch and objfmt
        {".align",      &GasParser::dir_align,  0},
        {".p2align",    &GasParser::dir_align,  1},
        {".balign",     &GasParser::dir_align,  0},
        {".org",        &GasParser::dir_org,    0},
        // data visibility directives
        {".local",      &GasParser::dir_local,  0},
        {".comm",       &GasParser::dir_comm,   0},
        {".lcomm",      &GasParser::dir_comm,   1},
        // integer data declaration directives
        {".byte",       &GasParser::dir_data,   1},
        {".2byte",      &GasParser::dir_data,   2},
        {".4byte",      &GasParser::dir_data,   4},
        {".8byte",      &GasParser::dir_data,   8},
        {".16byte",     &GasParser::dir_data,   16},
        // TODO: These should depend on arch
        {".short",      &GasParser::dir_data,   2},
        {".int",        &GasParser::dir_data,   4},
        {".long",       &GasParser::dir_data,   4},
        {".hword",      &GasParser::dir_data,   2},
        {".quad",       &GasParser::dir_data,   8},
        {".octa",       &GasParser::dir_data,   16},
        // XXX: At least on x86, this is 2 bytes
        {".value",      &GasParser::dir_data,   2},
        // ASCII data declaration directives
        {".ascii",      &GasParser::dir_ascii,  0},   // no terminating zero
        {".asciz",      &GasParser::dir_ascii,  1},   // add terminating zero
        {".string",     &GasParser::dir_ascii,  1},   // add terminating zero
        // LEB128 integer data declaration directives
        {".sleb128",    &GasParser::dir_leb128, 1},   // signed
        {".uleb128",    &GasParser::dir_leb128, 0},   // unsigned
        // floating point data declaration directives
        {".float",      &GasParser::dir_data,   4},
        {".single",     &GasParser::dir_data,   4},
        {".double",     &GasParser::dir_data,   8},
        {".tfloat",     &GasParser::dir_data,   10},
        // section directives
        {".bss",        &GasParser::dir_bss_section,    0},
        {".data",       &GasParser::dir_data_section,   0},
        {".text",       &GasParser::dir_text_section,   0},
        {".section",    &GasParser::dir_section,        0},
        // macro directives
        {".rept",       &GasParser::dir_rept,   0},
        {".endr",       &GasParser::dir_endr,   0},
        // empty space/fill directives
        {".skip",       &GasParser::dir_skip,   0},
        {".space",      &GasParser::dir_skip,   0},
        {".fill",       &GasParser::dir_fill,   0},
        {".zero",       &GasParser::dir_zero,   0},
        // other directives
        {".equ",        &GasParser::dir_equ,    0},
        {".file",       &GasParser::dir_file,   0},
        {".line",       &GasParser::dir_line,   0},
        {".set",        &GasParser::dir_equ,    0}
    };

    for (unsigned int i=0; i<NELEMS(gas_dirs_init); ++i)
        m_gas_dirs[gas_dirs_init[i].name] = &gas_dirs_init[i];
}

GasParser::~GasParser()
{
}

void
GasParser::parse(Object& object,
                 Preprocessor& preproc,
                 bool save_input,
                 Directives& dirs,
                 Linemap& linemap)
{
    init_mixin(object, preproc, save_input, dirs, linemap);

    m_locallabel_base = "";

    m_dir_fileline = FL_NONE;
    m_dir_file.clear();
    m_dir_line = 0;
    m_seen_line_marker = false;

    m_state = INITIAL;

    stdx::ptr_vector<GasRept>().swap(m_rept);   // clear the m_rept stack

    for (int i=0; i<10; i++)
        m_local[i] = 0;

    m_is_cpp_preproc =
        String::nocase_equal(preproc.get_module().get_keyword(), "cpp");
    m_is_nasm_preproc =
        String::nocase_equal(preproc.get_module().get_keyword(), "nasm");

    // Set up arch-sized directives
    m_sized_gas_dirs[0].name = ".word";
    m_sized_gas_dirs[0].handler = &GasParser::dir_data;
    m_sized_gas_dirs[0].param = m_arch->get_module().get_wordsize()/8;
    for (unsigned int i=0; i<NELEMS(m_sized_gas_dirs); ++i)
        m_gas_dirs[m_sized_gas_dirs[i].name] = &m_sized_gas_dirs[i];

    do_parse();

    // Check for ending inside a rept
    if (!m_rept.empty())
    {
        m_errwarns.propagate(m_rept.back().startline,
                             SyntaxError(N_("rept without matching endr")));
    }

    // Check for ending inside a comment
    if (m_state == COMMENT)
    {
        warn_set(WARN_GENERAL, N_("end of file in comment"));
        // XXX: Minus two to compensate for already having moved past the EOF
        // in the linemap.
        m_errwarns.propagate(get_cur_line()-2);
    }

    // Convert all undefined symbols into extern symbols
    object.symbols_finalize(m_errwarns, true);
}

std::vector<const char*>
GasParser::get_preproc_keywords()
{
    // valid preprocessors to use with this parser
    static const char* keywords[] = {"raw", "cpp", "nasm"};
    return std::vector<const char*>(keywords, keywords+NELEMS(keywords));
}

void
GasParser::add_directives(Directives& dirs, const char* parser)
{
    if (String::nocase_equal(parser, "gas"))
    {
        dirs.add(".extern", &dir_extern, Directives::ID_REQUIRED);
        dirs.add(".global", &dir_global, Directives::ID_REQUIRED);
        dirs.add(".globl",  &dir_global, Directives::ID_REQUIRED);
    }
}

void
do_register()
{
    register_module<ParserModule, ParserModuleImpl<GasParser> >("gas");
    register_module<ParserModule, ParserModuleImpl<GasParser> >("gnu");
}

}}} // namespace yasm::parser::gas

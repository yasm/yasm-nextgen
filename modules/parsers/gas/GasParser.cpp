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

#include "util.h"

#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/nocase.h"
#include "yasmx/Support/registry.h"
#include "yasmx/Arch.h"
#include "yasmx/Directive.h"
#include "yasmx/Errwarns.h"
#include "yasmx/Object.h"
#include "yasmx/Preprocessor.h"
#include "yasmx/Symbol_util.h"


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
        {".align",      &GasParser::ParseDirAlign,  0},
        {".p2align",    &GasParser::ParseDirAlign,  1},
        {".balign",     &GasParser::ParseDirAlign,  0},
        {".org",        &GasParser::ParseDirOrg,    0},
        // data visibility directives
        {".local",      &GasParser::ParseDirLocal,  0},
        {".comm",       &GasParser::ParseDirComm,   0},
        {".lcomm",      &GasParser::ParseDirComm,   1},
        // integer data declaration directives
        {".byte",       &GasParser::ParseDirData,   1},
        {".2byte",      &GasParser::ParseDirData,   2},
        {".4byte",      &GasParser::ParseDirData,   4},
        {".8byte",      &GasParser::ParseDirData,   8},
        {".16byte",     &GasParser::ParseDirData,   16},
        // TODO: These should depend on arch
        {".short",      &GasParser::ParseDirData,   2},
        {".int",        &GasParser::ParseDirData,   4},
        {".long",       &GasParser::ParseDirData,   4},
        {".hword",      &GasParser::ParseDirData,   2},
        {".quad",       &GasParser::ParseDirData,   8},
        {".octa",       &GasParser::ParseDirData,   16},
        // XXX: At least on x86, this is 2 bytes
        {".value",      &GasParser::ParseDirData,   2},
        // ASCII data declaration directives
        {".ascii",      &GasParser::ParseDirAscii,  0}, // no terminating zero
        {".asciz",      &GasParser::ParseDirAscii,  1}, // add terminating zero
        {".string",     &GasParser::ParseDirAscii,  1}, // add terminating zero
        // LEB128 integer data declaration directives
        {".sleb128",    &GasParser::ParseDirLeb128, 1}, // signed
        {".uleb128",    &GasParser::ParseDirLeb128, 0}, // unsigned
        // floating point data declaration directives
        {".float",      &GasParser::ParseDirData,   4},
        {".single",     &GasParser::ParseDirData,   4},
        {".double",     &GasParser::ParseDirData,   8},
        {".tfloat",     &GasParser::ParseDirData,   10},
        // section directives
        {".bss",        &GasParser::ParseDirBssSection,     0},
        {".data",       &GasParser::ParseDirDataSection,    0},
        {".text",       &GasParser::ParseDirTextSection,    0},
        {".section",    &GasParser::ParseDirSection,        0},
        // macro directives
        {".rept",       &GasParser::ParseDirRept,   0},
        {".endr",       &GasParser::ParseDirEndr,   0},
        // empty space/fill directives
        {".skip",       &GasParser::ParseDirSkip,   0},
        {".space",      &GasParser::ParseDirSkip,   0},
        {".fill",       &GasParser::ParseDirFill,   0},
        {".zero",       &GasParser::ParseDirZero,   0},
        // other directives
        {".equ",        &GasParser::ParseDirEqu,    0},
        {".file",       &GasParser::ParseDirFile,   0},
        {".line",       &GasParser::ParseDirLine,   0},
        {".set",        &GasParser::ParseDirEqu,    0}
    };

    for (unsigned int i=0; i<NELEMS(gas_dirs_init); ++i)
        m_gas_dirs[gas_dirs_init[i].name] = &gas_dirs_init[i];
}

GasParser::~GasParser()
{
}

void
GasParser::Parse(Object& object,
                 Preprocessor& preproc,
                 bool save_input,
                 Directives& dirs,
                 Linemap& linemap)
{
    InitMixin(object, preproc, save_input, dirs, linemap);

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
        String::NocaseEqual(preproc.getModule().getKeyword(), "cpp");
    m_is_nasm_preproc =
        String::NocaseEqual(preproc.getModule().getKeyword(), "nasm");

    // Set up arch-sized directives
    m_sized_gas_dirs[0].name = ".word";
    m_sized_gas_dirs[0].handler = &GasParser::ParseDirData;
    m_sized_gas_dirs[0].param = m_arch->getModule().getWordSize()/8;
    for (unsigned int i=0; i<NELEMS(m_sized_gas_dirs); ++i)
        m_gas_dirs[m_sized_gas_dirs[i].name] = &m_sized_gas_dirs[i];

    DoParse();

    // Check for ending inside a rept
    if (!m_rept.empty())
    {
        m_errwarns.Propagate(m_rept.back().startline,
                             SyntaxError(N_("rept without matching endr")));
    }

    // Check for ending inside a comment
    if (m_state == COMMENT)
    {
        setWarn(WARN_GENERAL, N_("end of file in comment"));
        // XXX: Minus two to compensate for already having moved past the EOF
        // in the linemap.
        m_errwarns.Propagate(getCurLine()-2);
    }

    // Convert all undefined symbols into extern symbols
    object.FinalizeSymbols(m_errwarns, true);
}

std::vector<const char*>
GasParser::getPreprocessorKeywords()
{
    // valid preprocessors to use with this parser
    static const char* keywords[] = {"raw", "cpp", "nasm"};
    return std::vector<const char*>(keywords, keywords+NELEMS(keywords));
}

void
GasParser::AddDirectives(Directives& dirs, const char* parser)
{
    if (String::NocaseEqual(parser, "gas"))
    {
        dirs.Add(".extern", &DirExtern, Directives::ID_REQUIRED);
        dirs.Add(".global", &DirGlobal, Directives::ID_REQUIRED);
        dirs.Add(".globl",  &DirGlobal, Directives::ID_REQUIRED);
    }
}

void
DoRegister()
{
    RegisterModule<ParserModule, ParserModuleImpl<GasParser> >("gas");
    RegisterModule<ParserModule, ParserModuleImpl<GasParser> >("gnu");
}

}}} // namespace yasm::parser::gas

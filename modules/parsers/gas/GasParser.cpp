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

#include "yasmx/Parse/Directive.h"
#include "yasmx/Support/registry.h"
#include "yasmx/Arch.h"
#include "yasmx/Object.h"
#include "yasmx/Op.h"
#include "yasmx/Symbol_util.h"


using namespace yasm;
using namespace yasm::parser;

GasParser::GasParser(const ParserModule& module,
                     Diagnostic& diags,
                     SourceManager& sm,
                     HeaderSearch& headers)
    : ParserImpl(module, m_gas_preproc)
    , m_gas_preproc(diags, sm, headers)
    , m_intel(false)
    , m_reg_prefix(true)
    , m_previous_section(0)
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
        // alternate integer data declaration directives
        {".dc",         &GasParser::ParseDirData,   2},
        {".dc.b",       &GasParser::ParseDirData,   1},
        {".dc.w",       &GasParser::ParseDirData,   2},
        {".dc.l",       &GasParser::ParseDirData,   4},
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
        {".float",      &GasParser::ParseDirFloat,   4},
        {".single",     &GasParser::ParseDirFloat,   4},
        {".double",     &GasParser::ParseDirFloat,   8},
        {".tfloat",     &GasParser::ParseDirFloat,   10},
        // alternate floating point data declaration directives
        {".dc.s",       &GasParser::ParseDirFloat,   4},
        {".dc.d",       &GasParser::ParseDirFloat,   8},
        {".dc.x",       &GasParser::ParseDirFloat,   10},
        // section directives
        {".bss",        &GasParser::ParseDirBssSection,     0},
        {".data",       &GasParser::ParseDirDataSection,    0},
        {".text",       &GasParser::ParseDirTextSection,    0},
        {".section",    &GasParser::ParseDirSection,        0},
        {".pushsection",&GasParser::ParseDirSection,        1},
        {".popsection", &GasParser::ParseDirPopSection,     0},
        {".previous",   &GasParser::ParseDirPrevious,       0},
        // macro directives
        {".include",    &GasParser::ParseDirInclude,    0},
#if 0
        {".macro",      &GasParser::ParseDirMacro,      0},
        {".endm",       &GasParser::ParseDirEndm,       0},
#endif
        {".rept",       &GasParser::ParseDirRept,       0},
        {".endr",       &GasParser::ParseDirEndr,       0},
        // empty space/fill directives
        {".skip",       &GasParser::ParseDirSkip,   1},
        {".space",      &GasParser::ParseDirSkip,   1},
        {".fill",       &GasParser::ParseDirFill,   0},
        {".zero",       &GasParser::ParseDirZero,   0},
        // alternate empty space/fill directives
        {".dcb",        &GasParser::ParseDirSkip,   2},
        {".dcb.b",      &GasParser::ParseDirSkip,   1},
        {".dcb.w",      &GasParser::ParseDirSkip,   2},
        {".dcb.l",      &GasParser::ParseDirSkip,   4},
        {".ds",         &GasParser::ParseDirSkip,   2},
        {".ds.b",       &GasParser::ParseDirSkip,   1},
        {".ds.w",       &GasParser::ParseDirSkip,   2},
        {".ds.l",       &GasParser::ParseDirSkip,   4},
        {".ds.p",       &GasParser::ParseDirSkip,   12},
        // "float" alternate empty space/fill directives
        {".dcb.s",      &GasParser::ParseDirFloatFill,  4},
        {".dcb.d",      &GasParser::ParseDirFloatFill,  8},
        {".dcb.x",      &GasParser::ParseDirFloatFill,  10},
        {".ds.s",       &GasParser::ParseDirSkip,   4},
        {".ds.d",       &GasParser::ParseDirSkip,   8},
        // XXX: gas uses 12 for this for some reason, but match it
        {".ds.x",       &GasParser::ParseDirSkip,   12},
        // conditional compilation directives
        {".else",       &GasParser::ParseDirElse,   0},
        {".elsec",      &GasParser::ParseDirElse,   0},
        {".elseif",     &GasParser::ParseDirElseif, 0},
        {".endif",      &GasParser::ParseDirEndif,  0},
        {".endc",       &GasParser::ParseDirEndif,  0},
        {".if",         &GasParser::ParseDirIf,     Op::NE},
        {".ifb",        &GasParser::ParseDirIfb,    0},
        {".ifdef",      &GasParser::ParseDirIfdef,  0},
        {".ifeq",       &GasParser::ParseDirIf,     Op::EQ},
        {".ifeqs",      &GasParser::ParseDirIfeqs,  0},
        {".ifge",       &GasParser::ParseDirIf,     Op::GE},
        {".ifgt",       &GasParser::ParseDirIf,     Op::GT},
        {".ifle",       &GasParser::ParseDirIf,     Op::LE},
        {".iflt",       &GasParser::ParseDirIf,     Op::LT},
        {".ifnb",       &GasParser::ParseDirIfb,    1},
        {".ifndef",     &GasParser::ParseDirIfdef,  1},
        {".ifnotdef",   &GasParser::ParseDirIfdef,  1},
        {".ifne",       &GasParser::ParseDirIf,     Op::NE},
        {".ifnes",      &GasParser::ParseDirIfeqs,  1},
        // other directives
        {".att_syntax",     &GasParser::ParseDirSyntax, 0},
        {".intel_syntax",   &GasParser::ParseDirSyntax, 1},
        {".equ",        &GasParser::ParseDirEqu,    0},
        {".file",       &GasParser::ParseDirFile,   0},
        {".line",       &GasParser::ParseDirLine,   0},
        {".set",        &GasParser::ParseDirEqu,    0}
    };

    for (size_t i=0; i<sizeof(gas_dirs_init)/sizeof(gas_dirs_init[0]); ++i)
        m_gas_dirs[gas_dirs_init[i].name] = &gas_dirs_init[i];
}

GasParser::~GasParser()
{
}

void
GasParser::Parse(Object& object, Directives& dirs, Diagnostic& diags)
{
    m_object = &object;
    m_dirs = &dirs;
    m_arch = object.getArch();

    m_locallabel_base = "";

    m_dir_fileline = FL_NONE;
    m_dir_file.clear();
    m_dir_line = 0;
    m_seen_line_marker = false;

    m_local.clear();
    m_cond_stack.clear();

    // Set up arch-sized directives
    m_sized_gas_dirs[0].name = ".word";
    m_sized_gas_dirs[0].handler = &GasParser::ParseDirData;
    m_sized_gas_dirs[0].param = m_arch->getModule().getWordSize()/8;
    for (size_t i=0; i<sizeof(m_sized_gas_dirs)/sizeof(m_sized_gas_dirs[0]);
         ++i)
        m_gas_dirs[m_sized_gas_dirs[i].name] = &m_sized_gas_dirs[i];

    m_preproc.EnterMainSourceFile();
    m_preproc.Lex(&m_token);
    DoParse();

    // Check for ending inside a rept
#if 0
    if (!m_rept.empty())
    {
        m_errwarns.Propagate(m_rept.back().startline,
                             SyntaxError(N_("rept without matching endr")));
    }
#endif

    // Convert all undefined symbols into extern symbols
    object.ExternUndefinedSymbols();
}

void
GasParser::AddDirectives(Directives& dirs, llvm::StringRef parser)
{
    if (parser.equals_lower("gas") || parser.equals_lower("gnu"))
    {
        dirs.Add(".extern", &DirExternMulti, Directives::ID_REQUIRED);
        dirs.Add(".global", &DirGlobalMulti, Directives::ID_REQUIRED);
        dirs.Add(".globl",  &DirGlobalMulti, Directives::ID_REQUIRED);
    }
}

void
yasm_parser_gas_DoRegister()
{
    RegisterModule<ParserModule, ParserModuleImpl<GasParser> >("gas");
    RegisterModule<ParserModule, ParserModuleImpl<GasParser> >("gnu");
}

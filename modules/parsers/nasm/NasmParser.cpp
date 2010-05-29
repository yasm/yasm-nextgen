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
#include "NasmParser.h"

#include "util.h"

#include "yasmx/Support/registry.h"
#include "yasmx/Arch.h"
#include "yasmx/Directive.h"
#include "yasmx/Expr.h"
#include "yasmx/Object.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol_util.h"


using namespace yasm;
using namespace yasm::parser;

NasmParser::NasmParser(const ParserModule& module,
                       Diagnostic& diags,
                       clang::SourceManager& sm,
                       HeaderSearch& headers)
    : Parser(module)
    , ParserImpl(m_nasm_preproc)
    , m_nasm_preproc(diags, sm, headers)
{
}

NasmParser::~NasmParser()
{
}

void
NasmParser::Parse(Object& object, Directives& dirs, Diagnostic& diags)
{
    m_object = &object;
    m_dirs = &dirs;
    m_arch = object.getArch();
    m_wordsize = m_arch->getModule().getWordSize();

    // Set up pseudo instructions.
    for (int i=0; i<8; ++i)
    {
        m_data_insns[i].type = PseudoInsn::DECLARE_DATA;
        m_reserve_insns[i].type = PseudoInsn::RESERVE_SPACE;
    }
    m_data_insns[DB].size = m_reserve_insns[DB].size = 1;               // b
    m_data_insns[DT].size = m_reserve_insns[DT].size = 80/8;            // t
    m_data_insns[DY].size = m_reserve_insns[DY].size = 256/8;           // y
    m_data_insns[DHW].size = m_reserve_insns[DHW].size = m_wordsize/8/2;// hw
    m_data_insns[DW].size = m_reserve_insns[DW].size = m_wordsize/8;    // w
    m_data_insns[DD].size = m_reserve_insns[DD].size = m_wordsize/8*2;  // d
    m_data_insns[DQ].size = m_reserve_insns[DQ].size = m_wordsize/8*4;  // q
    m_data_insns[DO].size = m_reserve_insns[DO].size = m_wordsize/8*8;  // o

    m_locallabel_base = "";

    m_bc = 0;

    m_absstart.Clear();
    m_abspos.Clear();

    // Get first token
    m_preproc.EnterMainSourceFile();
    m_preproc.Lex(&m_token);
    DoParse();

    // Check for undefined symbols
    object.FinalizeSymbols(m_preproc.getDiagnostics());
}

void
NasmParser::AddDirectives(Directives& dirs, llvm::StringRef parser)
{
    static const Directives::Init<NasmParser> nasm_dirs[] =
    {
        {"absolute", &NasmParser::DirAbsolute, Directives::ARG_REQUIRED},
        {"align", &NasmParser::DirAlign, Directives::ARG_REQUIRED},
    };

    if (parser.equals_lower("nasm"))
    {
        dirs.AddArray(this, nasm_dirs, NELEMS(nasm_dirs));
        dirs.Add("extern", &DirExtern, Directives::ID_REQUIRED);
        dirs.Add("global", &DirGlobal, Directives::ID_REQUIRED);
        dirs.Add("common", &DirCommon, Directives::ID_REQUIRED);
    }
}

void
yasm_parser_nasm_DoRegister()
{
    RegisterModule<ParserModule, ParserModuleImpl<NasmParser> >("nasm");
}

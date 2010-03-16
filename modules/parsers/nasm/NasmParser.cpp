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

#include "yasmx/Support/nocase.h"
#include "yasmx/Support/registry.h"
#include "yasmx/Arch.h"
#include "yasmx/Directive.h"
#include "yasmx/Expr.h"
#include "yasmx/Object.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol_util.h"


namespace yasm
{
namespace parser
{
namespace nasm
{

NasmParser::NasmParser(const ParserModule& module, Errwarns& errwarns)
    : Parser(module, errwarns)
{
}

NasmParser::~NasmParser()
{
}

void
NasmParser::Parse(Object& object, Preprocessor& preproc, Directives& dirs)
{
    InitMixin(object, preproc, dirs);

    m_locallabel_base = "";

    m_bc = 0;

    m_absstart.Clear();
    m_abspos.Clear();

    m_state = INITIAL;

    DoParse();

    // Check for undefined symbols
    object.FinalizeSymbols(m_errwarns, false);
}

std::vector<llvm::StringRef>
NasmParser::getPreprocessorKeywords()
{
    // valid preprocessors to use with this parser
    static const char* keywords[] = {"raw", "nasm"};
    return std::vector<llvm::StringRef>(keywords, keywords+NELEMS(keywords));
}

void
NasmParser::AddDirectives(Directives& dirs, llvm::StringRef parser)
{
    static const Directives::Init<NasmParser> nasm_dirs[] =
    {
        {"absolute", &NasmParser::DirAbsolute, Directives::ARG_REQUIRED},
        {"align", &NasmParser::DirAlign, Directives::ARG_REQUIRED},
        {"default", &NasmParser::DirDefault, Directives::ANY},
    };

    if (String::NocaseEqual(parser, "nasm"))
    {
        dirs.AddArray(this, nasm_dirs, NELEMS(nasm_dirs));
        dirs.Add("extern", &DirExtern, Directives::ID_REQUIRED);
        dirs.Add("global", &DirGlobal, Directives::ID_REQUIRED);
        dirs.Add("common", &DirCommon, Directives::ID_REQUIRED);
    }
}

void
DoRegister()
{
    RegisterModule<ParserModule, ParserModuleImpl<NasmParser> >("nasm");
}

}}} // namespace yasm::parser::nasm

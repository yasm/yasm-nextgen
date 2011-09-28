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

#include "config.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "yasmx/Parse/Directive.h"
#include "yasmx/Support/registry.h"
#include "yasmx/Arch.h"
#include "yasmx/Expr.h"
#include "yasmx/Object.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol_util.h"

#include "nasm.h"
#include "nasmlib.h"
#include "nasm-eval.h"
#include "nasm-pp.h"

#include "nasm-macros.i"

using namespace yasm;
using namespace yasm::parser;

static unsigned int nasm_errors;

static void
nasm_efunc(int severity, const char *fmt, ...)
{
    va_list va;

    fprintf(stderr, "%s:%ld: ", nasm::nasm_src_get_fname(),
            nasm::nasm_src_get_linnum());

    va_start(va, fmt);
    switch (severity & ERR_MASK) {
        case ERR_WARNING:
            vfprintf(stderr, fmt, va);
            fputc('\n', stderr);
            break;
        case ERR_NONFATAL:
            vfprintf(stderr, fmt, va);
            fputc('\n', stderr);
            ++nasm_errors;
            break;
        case ERR_FATAL:
        case ERR_PANIC:
            vfprintf(stderr, fmt, va);
            fputc('\n', stderr);
            exit(1);
            /*@notreached@*/
            break;
        case ERR_DEBUG:
            break;
    }
    va_end(va);
}

NasmParser::NasmParser(const ParserModule& module,
                       Diagnostic& diags,
                       SourceManager& sm,
                       HeaderSearch& headers)
    : ParserImpl(module, m_nasm_preproc)
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

    // XXX: HACK: run through nasm preproc and replace main file contents
    nasm::yasm_preproc = &m_preproc;
    nasm::yasm_object = &object;
    SourceManager& sm = m_preproc.getSourceManager();
    nasm_errors = 0;
    nasm::nasmpp.reset(sm.getMainFileID(), 2, nasm_efunc, nasm::nasm_evaluate);

    // pass down command line options
    for (std::vector<NasmPreproc::Predef>::iterator
         i = m_nasm_preproc.m_predefs.begin(),
         end = m_nasm_preproc.m_predefs.end();
         i != end; ++i)
    {
        char *def = const_cast<char*>(i->m_string.c_str());
        switch (i->m_type)
        {
            case NasmPreproc::Predef::PREINC:
                nasm::pp_pre_include(def);
                break;
            case NasmPreproc::Predef::PREDEF:
                nasm::pp_pre_define(def);
                break;
            case NasmPreproc::Predef::UNDEF:
                nasm::pp_pre_undefine(def);
                break;
            case NasmPreproc::Predef::BUILTIN:
                nasm::pp_builtin_define(def);
                break;
        }
    }

    // add version macros
    int major = 0, minor = 0, subminor = 0, patchlevel = 0, matched;
    matched = sscanf(PACKAGE_VERSION, "%d.%d.%d.%d", &major, &minor, &subminor,
                     &patchlevel);

    if (matched == 3)
        patchlevel = 0;

    char* nasm_version_mac[8];
    for (int i=0; i<7; ++i)
        nasm_version_mac[i] = new char[100];
    sprintf(nasm_version_mac[0], "%%define __YASM_MAJOR__ %d", major);
    sprintf(nasm_version_mac[1], "%%define __YASM_MINOR__ %d", minor);
    sprintf(nasm_version_mac[2], "%%define __YASM_SUBMINOR__ %d", subminor);
    sprintf(nasm_version_mac[3], "%%define __YASM_BUILD__ %d", patchlevel);
    sprintf(nasm_version_mac[4], "%%define __YASM_PATCHLEVEL__ %d", patchlevel);

    /* Version id (hex number) */
    sprintf(nasm_version_mac[5],
            "%%define __YASM_VERSION_ID__ 0%02x%02x%02x%02xh",
            major, minor, subminor, patchlevel);

    /* Version string */
    sprintf(nasm_version_mac[6], "%%define __YASM_VER__ \"%s\"",
            PACKAGE_VERSION);
    nasm_version_mac[7] = NULL;
    nasm::pp_extra_stdmac(const_cast<const char**>(nasm_version_mac));

    // add standard macros
    nasm::pp_extra_stdmac(nasm_standard_mac);

    // preprocess input
    std::string result;
    long prior_linnum = 0;
    char *file_name = 0;
    int lineinc = 0;
    while (char* line = nasm::nasmpp.getline())
    {
        long linnum = prior_linnum += lineinc;
        int altline = nasm::nasm_src_get(&linnum, &file_name);
        if (altline != 0) {
            lineinc = (altline != -1 || lineinc != 1);
            llvm::SmallString<64> linestr;
            llvm::raw_svector_ostream los(linestr);
            los << "%line " << linnum << '+' << lineinc << ' ' << file_name
                << '\n';
            result += los.str();
            prior_linnum = linnum;
        }
        result += line;
        result += '\n';
    }
    nasm::nasmpp.cleanup(1);
    for (int i=0; i<7; ++i)
        delete[] nasm_version_mac[i];
    if (nasm_errors > 0)
    {
        diags.Report(SourceLocation(), diag::fatal_pp_errors);
        return;
    }

    //fputs(result.c_str(), stdout);    // for debugging

    // override main file with preprocessed source
    const char* filename =
        sm.getBuffer(sm.getMainFileID())->getBufferIdentifier();
    sm.clearIDTables();
    sm.createMainFileIDForMemBuffer(
        llvm::MemoryBuffer::getMemBufferCopy(result, filename));

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
        dirs.AddArray(this, nasm_dirs);
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

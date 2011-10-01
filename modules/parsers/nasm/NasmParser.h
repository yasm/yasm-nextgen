#ifndef YASM_NASMPARSER_H
#define YASM_NASMPARSER_H
//
// NASM-compatible parser header file
//
//  Copyright (C) 2002-2007  Peter Johnson
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
#include <memory>

#include "llvm/ADT/APFloat.h"
#include "yasmx/Config/export.h"
#include "yasmx/Parse/Parser.h"
#include "yasmx/Parse/ParserImpl.h"
#include "yasmx/Insn.h"

#include "NasmPreproc.h"


namespace yasm
{

class Arch;
class Bytecode;
class DirectiveInfo;
class Directives;
class Expr;
class NameValues;

namespace parser
{

class YASM_STD_EXPORT NasmParseDirExprTerm : public ParseExprTerm
{
public:
    virtual ~NasmParseDirExprTerm();
    bool operator() (Expr& e, ParserImpl& parser, bool* handled) const;
};

class YASM_STD_EXPORT NasmParseDataExprTerm : public ParseExprTerm
{
public:
    virtual ~NasmParseDataExprTerm();
    bool operator() (Expr& e, ParserImpl& parser, bool* handled) const;
};

class YASM_STD_EXPORT NasmParser : public ParserImpl
{
public:
    NasmParser(const ParserModule& module,
               Diagnostic& diags,
               SourceManager& sm,
               HeaderSearch& headers);
    ~NasmParser();

    void AddDirectives(Directives& dirs, llvm::StringRef parser);

    static llvm::StringRef getName() { return "NASM-compatible parser"; }
    static llvm::StringRef getKeyword() { return "nasm"; }

    void Parse(Object& object, Directives& dirs, Diagnostic& diags);

private:
    friend class NasmParseDirExprTerm;
    friend class NasmParseDataExprTerm;

    struct PseudoInsn
    {
        enum Type
        {
            DECLARE_DATA,
            RESERVE_SPACE,
            INCBIN,
            EQU,
            TIMES
        };
        Type type;
        unsigned int size;
    };

    void CheckPseudoInsn(IdentifierInfo* ii);
    bool CheckKeyword(IdentifierInfo* ii);

    void DefineLabel(SymbolRef sym, SourceLocation source, bool local);

    void DoParse();
    bool ParseLine();
    bool ParseDirective(/*@out@*/ NameValues& nvs);
    bool ParseExp();
    Insn::Ptr ParseInsn();

    unsigned int getSizeOverride(Token& tok);
    Operand ParseOperand();

    Operand ParseMemoryAddress();

    bool ParseSegOffExpr(Expr& e, const ParseExprTerm* parse_term = 0);
    bool ParseExpr(Expr& e, const ParseExprTerm* parse_term = 0);
    bool ParseExpr0(Expr& e, const ParseExprTerm* parse_term);
    bool ParseExpr1(Expr& e, const ParseExprTerm* parse_term);
    bool ParseExpr2(Expr& e, const ParseExprTerm* parse_term);
    bool ParseExpr3(Expr& e, const ParseExprTerm* parse_term);
    bool ParseExpr4(Expr& e, const ParseExprTerm* parse_term);
    bool ParseExpr5(Expr& e, const ParseExprTerm* parse_term);
    bool ParseExpr6(Expr& e, const ParseExprTerm* parse_term);

    SymbolRef ParseSymbol(IdentifierInfo* ii, bool* local = 0);

    void DirAbsolute(DirectiveInfo& info, Diagnostic& diags);
    void DirAlign(DirectiveInfo& info, Diagnostic& diags);

    void DoDirective(llvm::StringRef name, DirectiveInfo& info);

    Object* m_object;
    Arch* m_arch;
    Directives* m_dirs;
    unsigned int m_wordsize;

    NasmPreproc m_nasm_preproc;

    PseudoInsn m_data_insns[8], m_reserve_insns[8];

    // Indexes into m_data_insns and m_reserve_insns.
    enum { DB = 0, DT, DY, DHW, DW, DD, DQ, DO };

    // last "base" label for local (.) labels
    std::string m_locallabel_base;

    BytecodeContainer* m_container;
    /*@null@*/ Bytecode* m_bc;

    // Starting point of the absolute section.  Empty if not in an absolute
    // section.
    Expr m_absstart;

    // Current location inside an absolute section (including the start).
    // Empty if not in an absolute section.
    Expr m_abspos;

    // Delta to add to abspos when the current line completes.
    Expr m_absinc;

    // Current TIMES expression.  Empty if not in a TIMES.
    Expr m_times;

    // Original container when in a TIMES expression.
    // TIMES replaces m_container, saving the old one here.
    BytecodeContainer* m_times_outer_container;
};

}} // namespace yasm::parser

#endif

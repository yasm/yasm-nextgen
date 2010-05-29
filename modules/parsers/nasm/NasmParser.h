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
#include "yasmx/Parse/ParserImpl.h"
#include "yasmx/Insn.h"
#include "yasmx/Parser.h"

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

class YASM_STD_EXPORT NasmParser : public ParserImpl
{
public:
    NasmParser(const ParserModule& module,
               Diagnostic& diags,
               clang::SourceManager& sm,
               HeaderSearch& headers);
    ~NasmParser();

    void AddDirectives(Directives& dirs, llvm::StringRef parser);

    static llvm::StringRef getName() { return "NASM-compatible parser"; }
    static llvm::StringRef getKeyword() { return "nasm"; }

    void Parse(Object& object, Directives& dirs, Diagnostic& diags);

    enum ExprType
    {
        NORM_EXPR,
        DIR_EXPR,       // Can't have seg:off or WRT anywhere
        DV_EXPR         // Can't have registers anywhere
    };

private:
    struct PseudoInsn
    {
        enum Type
        {
            DECLARE_DATA,
            RESERVE_SPACE,
            INCBIN,
            EQU
        };
        Type type;
        unsigned int size;
    };

    void CheckPseudoInsn(IdentifierInfo* ii);
    bool CheckKeyword(IdentifierInfo* ii);

    void DefineLabel(SymbolRef sym, clang::SourceLocation source, bool local);

    void DoParse();
    bool ParseLine();
    bool ParseDirective(/*@out@*/ NameValues& nvs);
    bool ParseTimes(clang::SourceLocation times_source);
    bool ParseExp();
    Insn::Ptr ParseInsn();

    unsigned int getSizeOverride(Token& tok);
    Operand ParseOperand();

    Operand ParseMemoryAddress();

    bool ParseExpr(Expr& e, ExprType type);
    bool ParseBExpr(Expr& e, ExprType type);
    bool ParseExpr0(Expr& e, ExprType type);
    bool ParseExpr1(Expr& e, ExprType type);
    bool ParseExpr2(Expr& e, ExprType type);
    bool ParseExpr3(Expr& e, ExprType type);
    bool ParseExpr4(Expr& e, ExprType type);
    bool ParseExpr5(Expr& e, ExprType type);
    bool ParseExpr6(Expr& e, ExprType type);

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
};

}} // namespace yasm::parser

#endif

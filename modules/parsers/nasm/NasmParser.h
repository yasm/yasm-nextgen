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
#include "yasmx/Mixin/ParserMixin.h"
#include "yasmx/Support/scoped_ptr.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Insn.h"
#include "yasmx/IntNum.h"
#include "yasmx/Parser.h"


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
namespace nasm
{

#define YYCTYPE char

struct yystype
{
    std::string str;
    std::auto_ptr<IntNum> intn;
    std::auto_ptr<llvm::APFloat> flt;
    Insn::Ptr insn;
    union
    {
        unsigned int int_info;
        const Prefix* prefix;
        const SegmentRegister* segreg;
        const Register* reg;
        const TargetModifier* targetmod;
    };
};
#define YYSTYPE yystype

class NasmParser
    : public Parser
    , public ParserMixin<NasmParser, YYSTYPE, YYCTYPE>
{
public:
    NasmParser(const ParserModule& module, Errwarns& errwarns);
    ~NasmParser();

    void AddDirectives(Directives& dirs, llvm::StringRef parser);

    static llvm::StringRef getName() { return "NASM-compatible parser"; }
    static llvm::StringRef getKeyword() { return "nasm"; }
    static std::vector<llvm::StringRef> getPreprocessorKeywords();
    static llvm::StringRef getDefaultPreprocessorKeyword() { return "raw"; }

    void Parse(Object& object, Preprocessor& preproc, Directives& dirs);

    enum TokenType
    {
        INTNUM = 258,
        FLTNUM,
        DIRECTIVE_NAME,
        FILENAME,
        STRING,
        SIZE_OVERRIDE,
        DECLARE_DATA,
        RESERVE_SPACE,
        INCBIN,
        EQU,
        TIMES,
        SEG,
        WRT,
        ABS,
        REL,
        NOSPLIT,
        STRICT,
        INSN,
        PREFIX,
        REG,
        SEGREG,
        TARGETMOD,
        LEFT_OP,
        RIGHT_OP,
        SIGNDIV,
        SIGNMOD,
        START_SECTION_ID,
        ID,
        LOCAL_ID,
        SPECIAL_ID,
        NONLOCAL_ID,
        LINE,
        NONE                // special token for lookahead
    };

    enum ExprType
    {
        NORM_EXPR,
        DIR_EXPR,       // Can't have seg:off or WRT anywhere
        DV_EXPR         // Can't have registers anywhere
    };

    static bool isEolTok(int tok) { return (tok == 0); }
    static llvm::StringRef DescribeToken(int tok);

    int Lex(YYSTYPE* lvalp);

private:
    int HandleDotLabel(YYSTYPE* lvalp, YYCTYPE* tok, size_t toklen,
                       size_t zeropos);
    void DefineLabel(llvm::StringRef name, bool local);

    void DoParse();
    void ParseLine();
    bool ParseDirective(/*@out@*/ NameValues& nvs);
    void ParseTimes();
    bool ParseExp();
    Insn::Ptr ParseInsn();

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

    void DirAbsolute(DirectiveInfo& info);
    void DirAlign(DirectiveInfo& info);
    void DirDefault(DirectiveInfo& info);

    void DoDirective(llvm::StringRef name, DirectiveInfo& info);

    // last "base" label for local (.) labels
    std::string m_locallabel_base;

    /*@null@*/ Bytecode* m_bc;

    enum State
    {
        INITIAL,
        DIRECTIVE,
        SECTION_DIRECTIVE,
        DIRECTIVE2,
        LINECHG,
        LINECHG2,
        INSTRUCTION
    } m_state;

    // Starting point of the absolute section.  Empty if not in an absolute
    // section.
    Expr m_absstart;

    // Current location inside an absolute section (including the start).
    // Empty if not in an absolute section.
    Expr m_abspos;
};

#define INTNUM_val              (m_tokval.intn)
#define FLTNUM_val              (m_tokval.flt)
#define DIRECTIVE_NAME_val      (m_tokval.str)
#define FILENAME_val            (m_tokval.str)
#define STRING_val              (m_tokval.str)
#define SIZE_OVERRIDE_val       (m_tokval.int_info)
#define DECLARE_DATA_val        (m_tokval.int_info)
#define RESERVE_SPACE_val       (m_tokval.int_info)
#define INSN_val                (m_tokval.insn)
#define PREFIX_val              (m_tokval.prefix)
#define REG_val                 (m_tokval.reg)
#define SEGREG_val              (m_tokval.segreg)
#define TARGETMOD_val           (m_tokval.targetmod)
#define ID_val                  (m_tokval.str)

}}} // namespace yasm::parser::nasm

#endif

#ifndef YASM_NASM_PARSER_H
#define YASM_NASM_PARSER_H
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

#include <boost/scoped_ptr.hpp>

#include <libyasm/bytecode.h>
#include <libyasm/floatnum.h>
#include <libyasm/insn.h>
#include <libyasm/intnum.h>
#include <libyasm/linemap.h>
#include <libyasm/parser.h>


namespace yasm
{

class Arch;
class Bytecode;
class Directives;
class Expr;
class FloatNum;
class IntNum;
class NameValues;

namespace parser
{
namespace nasm
{

#define YYCTYPE char

#define MAX_SAVED_LINE_LEN  80

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
    LINE,
    NONE                // special token for lookahead
};

struct yystype
{
    std::string str;
    std::auto_ptr<IntNum> intn;
    std::auto_ptr<FloatNum> flt;
    Insn::Ptr insn;
    union
    {
        unsigned int int_info;
        const Insn::Prefix* prefix;
        const SegmentRegister* segreg;
        const Register* reg;
        const Insn::Operand::TargetModifier* targetmod;
    };
};
#define YYSTYPE yystype

bool is_eol_tok(int tok);

class NasmParser : public Parser
{
public:
    NasmParser();
    ~NasmParser();

    std::string get_name() const;
    std::string get_keyword() const;
    void add_directives(Directives& dirs, const std::string& parser);

    std::vector<std::string> get_preproc_keywords() const;
    std::string get_default_preproc_keyword() const;

    void parse(Object& object,
               Preprocessor& preproc,
               bool save_input,
               Directives& dirs,
               Linemap& linemap,
               Errwarns& errwarns);

private:
    enum ExprType
    {
        NORM_EXPR,
        DIR_EXPR,
        DV_EXPR
    };

    unsigned long get_cur_line() const { return m_linemap->get_current(); }

    int lex(YYSTYPE* lvalp);
    void fill(YYCTYPE* &cursor);
    size_t fill_input(unsigned char* buf, size_t max);
    YYCTYPE* save_line(YYCTYPE* cursor);

    int get_next_token()
    {
        m_token = lex(&m_tokval);
        return m_token;
    }
    void get_peek_token();
    bool is_eol() { return is_eol_tok(m_token); }

    // Eat all remaining tokens to EOL, discarding all of them.  If there's any
    // intervening tokens, generates an error (junk at end of line).
    void demand_eol();

    void expect(int token);

    void define_label(const std::string& name, bool local);

    void do_parse();
    void parse_line();
    bool parse_directive_namevals(/*@out@*/ NameValues& nvs);
    void parse_times();
    bool parse_exp();
    Insn::Ptr parse_instr();

    Insn::Operand parse_operand();

    std::auto_ptr<EffAddr> parse_memaddr();

    std::auto_ptr<Expr> parse_expr(ExprType type);
    std::auto_ptr<Expr> parse_bexpr(ExprType type);
    std::auto_ptr<Expr> parse_expr0(ExprType type);
    std::auto_ptr<Expr> parse_expr1(ExprType type);
    std::auto_ptr<Expr> parse_expr2(ExprType type);
    std::auto_ptr<Expr> parse_expr3(ExprType type);
    std::auto_ptr<Expr> parse_expr4(ExprType type);
    std::auto_ptr<Expr> parse_expr5(ExprType type);
    std::auto_ptr<Expr> parse_expr6(ExprType type);

    void dir_absolute(Object& object,
                      const NameValues& namevals,
                      const NameValues& objext_namevals,
                      unsigned long line);
    void dir_align(Object& object,
                   const NameValues& namevals,
                   const NameValues& objext_namevals,
                   unsigned long line);
    void dir_default(Object& object,
                     const NameValues& namevals,
                     const NameValues& objext_namevals,
                     unsigned long line);

    void directive(const std::string& name,
                   const NameValues& namevals,
                   const NameValues& objext_namevals);

    Object* m_object;
    BytecodeContainer* m_container;
    Preprocessor* m_preproc;
    Directives* m_dirs;
    Linemap* m_linemap;
    Errwarns* m_errwarns;

    Arch* m_arch;
    unsigned int m_wordsize;

    // last "base" label for local (.) labels
    std::string m_locallabel_base;

    /*@null@*/ Bytecode* m_bc;

    bool m_save_input;

    YYCTYPE *m_bot, *m_tok, *m_ptr, *m_cur, *m_lim;

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

    int m_token;        // enum TokenType or any character
    yystype m_tokval;
    char m_tokch;       // first character of token

    // one token of lookahead; used sparingly
    int m_peek_token;   // NONE if none
    yystype m_peek_tokval;
    char m_peek_tokch;

    // Starting point of the absolute section.  NULL if not in an absolute
    // section.
    /*@null@*/ boost::scoped_ptr<Expr> m_absstart;

    // Current location inside an absolute section (including the start).
    // NULL if not in an absolute section.
    /*@null@*/ std::auto_ptr<Expr> m_abspos;
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

#define p_expr_new_tree(l,o,r)  yasm_expr_create_tree(l,o,r,cur_line)
#define p_expr_new_branch(o,r)  yasm_expr_create_branch(o,r,cur_line)
#define p_expr_new_ident(r)     yasm_expr_create_ident(r,cur_line)

}}} // namespace yasm::parser::nasm

#endif

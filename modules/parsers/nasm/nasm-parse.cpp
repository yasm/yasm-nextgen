//
// NASM-compatible parser
//
//  Copyright (C) 2001-2007  Peter Johnson, Michael Urman
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
#include <util.h>

#include <math.h>

#include <libyasm/arch.h>
#include <libyasm/bc_container_util.h>
#include <libyasm/bitcount.h>
#include <libyasm/compose.h>
#include <libyasm/directive.h>
#include <libyasm/effaddr.h>
#include <libyasm/errwarn.h>
#include <libyasm/errwarns.h>
#include <libyasm/expr.h>
#include <libyasm/name_value.h>
#include <libyasm/nocase.h>
#include <libyasm/object.h>
#include <libyasm/preproc.h>
#include <libyasm/section.h>
#include <libyasm/symbol.h>

#include "modules/parsers/nasm/nasm-parser.h"


namespace yasm
{
namespace parser
{
namespace nasm
{

inline bool
is_eol_tok(int tok)
{
    return (tok == 0);
}

void
NasmParser::get_peek_token()
{
    char savech = m_tokch;
    if (m_peek_token != NONE)
        throw InternalError(N_("only can have one token of lookahead"));
    m_peek_token = lex(&m_peek_tokval);
    m_peek_tokch = m_tokch;
    m_tokch = savech;
}

void
NasmParser::demand_eol()
{
    if (is_eol())
        return;

    char tokch = m_tokch;

    do {
        get_next_token();
    } while (!is_eol());

    throw SyntaxError(String::compose(
        N_("junk at end of line, first unrecognized character is `%1'"),
        tokch));

}

void
NasmParser::expect(int token)
{
    if (m_token == token)
        return;

    static char strch[] = "` '";
    const char *str;

    switch (token)
    {
        case INTNUM:            str = "integer"; break;
        case FLTNUM:            str = "floating point value"; break;
        case DIRECTIVE_NAME:    str = "directive name"; break;
        case FILENAME:          str = "filename"; break;
        case STRING:            str = "string"; break;
        case SIZE_OVERRIDE:     str = "size override"; break;
        case DECLARE_DATA:      str = "DB/DW/etc."; break;
        case RESERVE_SPACE:     str = "RESB/RESW/etc."; break;
        case INCBIN:            str = "INCBIN"; break;
        case EQU:               str = "EQU"; break;
        case TIMES:             str = "TIMES"; break;
        case SEG:               str = "SEG"; break;
        case WRT:               str = "WRT"; break;
        case NOSPLIT:           str = "NOSPLIT"; break;
        case STRICT:            str = "STRICT"; break;
        case INSN:              str = "instruction"; break;
        case PREFIX:            str = "instruction prefix"; break;
        case REG:               str = "register"; break;
        case SEGREG:            str = "segment register"; break;
        case TARGETMOD:         str = "target modifier"; break;
        case LEFT_OP:           str = "<<"; break;
        case RIGHT_OP:          str = ">>"; break;
        case SIGNDIV:           str = "//"; break;
        case SIGNMOD:           str = "%%"; break;
        case START_SECTION_ID:  str = "$$"; break;
        case ID:                str = "identifier"; break;
        case LOCAL_ID:          str = ".identifier"; break;
        case SPECIAL_ID:        str = "..identifier"; break;
        case LINE:              str = "%line"; break;
        default:
            strch[1] = token;
            str = strch;
            break;
    }
    throw ParseError(String::compose("expected %1", str));
}

void
NasmParser::do_parse()
{
    unsigned long cur_line = get_cur_line();
    std::string line;
    Bytecode::Ptr bc(new Bytecode());

    while (m_preproc->get_line(line))
    {
        if (m_abspos.get() != 0)
            m_bc = bc.get();
        else
            m_bc = &m_object->get_cur_section()->fresh_bytecode();
        m_bc->set_line(cur_line);
        Location loc = {m_bc, m_bc->get_fixed_len()};

        try
        {
            m_bot = m_tok = m_ptr = m_cur = &line[0];
            m_lim = &line[line.length()+1];

            get_next_token();
            if (!is_eol())
            {
                parse_line();
                demand_eol();
            }
#if 0
            if (m_abspos.get() != 0)
            {
                // If we're inside an absolute section, just add to the
                // absolute position rather than appending bytecodes to a
                // section.  Only RES* are allowed in an absolute section,
                // so this is easy.
                if (m_bc->has_contents())
                {
                    unsigned int itemsize;
                    const Expr* numitems = m_bc->reserve_numitems(itemsize);
                    if (numitems)
                    {
                        Expr::Ptr e(new Expr(numitems->clone(),
                                             Op::MUL,
                                             new IntNum(itemsize),
                                             cur_line));
                        const Expr* multiple = m_bc->get_multiple_expr();
                        if (multiple)
                            e.reset(new Expr(e.release(),
                                             Op::MUL,
                                             multiple->clone(),
                                             cur_line));
                        m_abspos.reset(new Expr(m_abspos.release(),
                                                Op::ADD,
                                                e,
                                                cur_line));
                    }
                    else
                        throw SyntaxError(
                            N_("only RES* allowed within absolute section"));
                    bc.reset(new Bytecode());
                }
            }
#endif
            if (m_save_input)
            {
                m_linemap->add_source(loc, line);
            }
            m_errwarns->propagate(cur_line);
        }
        catch (Error& err)
        {
            m_errwarns->propagate(cur_line, err);
        }

        cur_line = m_linemap->goto_next();
    }
}

// All parse_* functions expect to be called with m_token being their first
// token.  They should return with m_token being the token *after* their
// information.

void
NasmParser::parse_line()
{
    m_container = m_object->get_cur_section();

    if (parse_exp())
        return;

    switch (m_token)
    {
        case LINE: // LINE INTNUM '+' INTNUM FILENAME
        {
            get_next_token();

            expect(INTNUM);
            std::auto_ptr<IntNum> line(INTNUM_val);
            get_next_token();

            expect('+');
            get_next_token();

            expect(INTNUM);
            std::auto_ptr<IntNum> incr(INTNUM_val);
            get_next_token();

            expect(FILENAME);
            std::string filename;
            std::swap(filename, FILENAME_val);
            get_next_token();

            // %line indicates the line number of the *next* line, so subtract
            // out the increment when setting the line number.
            m_linemap->set(filename, line->get_uint() - incr->get_uint(),
                           incr->get_uint());
            break;
        }
        case '[': // [ directive ]
        {
            m_state = DIRECTIVE;
            get_next_token();

            expect(DIRECTIVE_NAME);
            std::string dirname;
            std::swap(dirname, DIRECTIVE_NAME_val);
            get_next_token();

            NameValues dir_nvs, ext_nvs;
            if (m_token != ']' && m_token != ':' &&
                !parse_directive_namevals(dir_nvs))
            {
                throw SyntaxError(String::compose(
                    N_("invalid arguments to [%s]"), dirname));
            }
            if (m_token == ':')
            {
                get_next_token();
                if (!parse_directive_namevals(ext_nvs))
                {
                    throw SyntaxError(String::compose(
                        N_("invalid arguments to [%1]"), dirname));
                }
            }
            directive(dirname, dir_nvs, ext_nvs);
            expect(']');
            get_next_token();
            break;
        }
        case TIMES: // TIMES expr exp
            get_next_token();
            parse_times();
            return;
        case ID:
        case SPECIAL_ID:
        case LOCAL_ID:
        {
            bool local = (m_token != ID);
            std::string name;
            std::swap(name, ID_val);

            get_next_token();
            if (is_eol())
            {
                // label alone on the line
                warn_set(WARN_ORPHAN_LABEL,
                    N_("label alone on a line without a colon might be in error"));
                define_label(name, local);
                break;
            }
            if (m_token == ':')
                get_next_token();

            if (m_token == EQU)
            {
                // label EQU expr
                get_next_token();
                Expr::Ptr e = parse_expr(NORM_EXPR);
                if (e.get() == 0)
                {
                    throw SyntaxError(String::compose(
                        N_("expression expected after %1"), "EQU"));
                }
                m_object->get_sym(name).define_equ(e, get_cur_line());
                break;
            }

            define_label(name, local);
            if (is_eol())
                break;
            if (m_token == TIMES)
            {
                get_next_token();
                return parse_times();
            }
            if (!parse_exp())
                throw SyntaxError(N_("instruction expected after label"));
            return;
        }
        default:
            throw SyntaxError(
                N_("label or instruction expected at start of line"));
    }
}

bool
NasmParser::parse_directive_namevals(/*@out@*/ NameValues& nvs)
{
    for (;;)
    {
        std::string id;
        std::auto_ptr<NameValue> nv(0);

        // Look for value first
        if (m_token == ID)
        {
            get_peek_token();
            if (m_peek_token == '=')
            {
                std::swap(id, ID_val);
                get_next_token(); // id
                get_next_token(); // '='
            }
        }

        // Look for parameter
        switch (m_token)
        {
            case STRING:
                nv.reset(new NameValue(id, STRING_val));
                get_next_token();
                break;
            case ID:
                // We need a peek token, but avoid error if we have one
                // already; we need to work whether or not we hit the
                // "value=" if test above.
                //
                // We cheat and peek ahead to see if this is just an ID or
                // the ID is part of an expression.  We assume a + or - means
                // that it's part of an expression (e.g. "x+y" is parsed as
                // the expression "x+y" and not as "x", "+y").
                if (m_peek_token == NONE)
                    get_peek_token();
                switch (m_peek_token)
                {
                    case '|': case '^': case '&': case LEFT_OP: case RIGHT_OP:
                    case '+': case '-':
                    case '*': case '/': case '%': case SIGNDIV: case SIGNMOD:
                        break;
                    default:
                        // Just an id
                        nv.reset(new NameValue(id, ID_val, '$'));
                        get_next_token();
                        goto next;
                }
                /*@fallthrough@*/
            default:
            {
                Expr::Ptr e = parse_expr(DIR_EXPR);
                if (e.get() == 0)
                    return false;
                nv.reset(new NameValue(id, e));
                break;
            }
        }
next:
        nvs.push_back(nv.release());
        if (m_token == ',')
            get_next_token();
        if (m_token == ']' || m_token == ':' || is_eol())
            return true;
    }
}

void
NasmParser::parse_times()
{
    Expr::Ptr multiple = parse_expr(DV_EXPR);
    if (multiple.get() == 0)
    {
        throw SyntaxError(String::compose(N_("expression expected after %1"),
                                          "TIMES"));
    }
    BytecodeContainer* orig_container = m_container;
    m_container = &append_multiple(*m_container, multiple, get_cur_line());
    try
    {
        if (!parse_exp())
            throw SyntaxError(N_("instruction expected after TIMES expression"));
    }
    catch (...)
    {
        m_container = orig_container;
        throw;
    }
}

bool
NasmParser::parse_exp()
{
    Insn::Ptr insn = parse_instr();
    if (insn.get() != 0)
    {
        insn->append(*m_container);
        return true;
    }

    switch (m_token)
    {
        case DECLARE_DATA:
        {
            unsigned int size = DECLARE_DATA_val/8;
            get_next_token();

            for (;;)
            {
                if (m_token == STRING)
                {
                    // Peek ahead to see if we're in an expr.  If we're not,
                    // then generate a real string dataval.
                    get_peek_token();
                    if (m_peek_token == ',' || is_eol_tok(m_peek_token))
                    {
                        append_data(*m_container, STRING_val, size, false);
                        get_next_token();
                        goto dv_done;
                    }
                }
                {
                    Expr::Ptr e = parse_expr(DV_EXPR);
                    if (e.get() != 0)
                        append_data(*m_container, e, size,
                                    *m_object->get_arch());
                    else
                        throw SyntaxError(N_("expression or string expected"));
                }
dv_done:
                if (is_eol())
                    break;
                expect(',');
                get_next_token();
                if (is_eol())   // allow trailing , on list
                    break;
            }
            return true;
        }
        case RESERVE_SPACE:
        {
            unsigned int size = RESERVE_SPACE_val/8;
            get_next_token();
            Expr::Ptr e = parse_expr(DV_EXPR);
            if (e.get() == 0)
            {
                throw SyntaxError(String::compose(
                    N_("expression expected after %1"), "RESx"));
            }
            BytecodeContainer& multc =
                append_multiple(*m_container, e, get_cur_line());
            multc.append_gap(size, get_cur_line());
            return true;
        }
        case INCBIN:
        {
            Expr::Ptr start(0), maxlen(0);

            get_next_token();

            if (m_token != STRING)
                throw SyntaxError(N_("filename string expected after INCBIN"));
            std::string filename;
            std::swap(filename, STRING_val);
            get_next_token();

            // optional start expression
            if (m_token == ',')
                get_next_token();
            if (is_eol())
                goto incbin_done;
            start = parse_expr(DV_EXPR);
            if (start.get() == 0)
                throw SyntaxError(N_("expression expected for INCBIN start"));

            // optional maxlen expression
            if (m_token == ',')
                get_next_token();
            if (is_eol())
                goto incbin_done;
            maxlen = parse_expr(DV_EXPR);
            if (maxlen.get() == 0)
            {
                throw SyntaxError(
                    N_("expression expected for INCBIN maximum length"));
            }

incbin_done:
            append_incbin(*m_container, filename, start, maxlen,
                          get_cur_line());
            return true;
        }
        default:
            break;
    }
    return false;
}

Insn::Ptr
NasmParser::parse_instr()
{
    switch (m_token)
    {
        case INSN:
        {
            Insn::Ptr insn(INSN_val);
            get_next_token();
            if (is_eol())
                return insn;    // no operands

            // parse operands
            for (;;)
            {
                insn->add_operand(parse_operand());

                if (is_eol())
                    break;
                expect(',');
                get_next_token();
            }
            return insn;
        }
        case PREFIX:
        {
            const Insn::Prefix* prefix = PREFIX_val;
            get_next_token();
            Insn::Ptr insn = parse_instr();
            if (insn.get() != 0)
                insn->add_prefix(prefix);
            return insn;
        }
        case SEGREG:
        {
            const SegmentRegister* segreg = SEGREG_val;
            get_next_token();
            Insn::Ptr insn = parse_instr();
            if (insn.get() != 0)
                insn->add_seg_prefix(segreg);
            return insn;
        }
        default:
            return Insn::Ptr(0);
    }
}

Insn::Operand
NasmParser::parse_operand()
{
    switch (m_token)
    {
        case '[':
        {
            get_next_token();
            EffAddr::Ptr ea = parse_memaddr();

            expect(']');
            get_next_token();

            return Insn::Operand(ea);
        }
        case SEGREG:
        {
            Insn::Operand op(SEGREG_val);
            get_next_token();
            return op;
        }
        case REG:
        {
            Insn::Operand op(REG_val);
            get_next_token();
            return op;
        }
        case STRICT:
        {
            get_next_token();
            Insn::Operand op = parse_operand();
            op.make_strict();
            return op;
        }
        case SIZE_OVERRIDE:
        {
            unsigned int size = SIZE_OVERRIDE_val;
            get_next_token();
            Insn::Operand op = parse_operand();
            const Register* reg = op.get_reg();
            if (reg && reg->get_size() != size)
                throw TypeError(N_("cannot override register size"));
            else
            {
                // Silently override others unless a warning is turned on.
                // This is to allow overrides such as:
                //   %define arg1 dword [bp+4]
                //   cmp word arg1, 2
                // Which expands to:
                //   cmp word dword [bp+4], 2
                unsigned int opsize = op.get_size();
                if (opsize != 0)
                {
                    if (opsize != size)
                        warn_set(WARN_SIZE_OVERRIDE, String::compose(
                            N_("overriding operand size from %1-bit to %2-bit"),
                            opsize, size));
                    else
                        warn_set(WARN_SIZE_OVERRIDE,
                                 N_("double operand size override"));
                }
                op.set_size(size);
            }
            return op;
        }
        case TARGETMOD:
        {
            const Insn::Operand::TargetModifier* tmod = TARGETMOD_val;
            get_next_token();
            Insn::Operand op = parse_operand();
            op.set_targetmod(tmod);
            return op;
        }
        default:
        {
            Expr::Ptr e = parse_expr(NORM_EXPR);
            if (e.get() == 0)
                throw SyntaxError(N_("expression syntax error"));
            return Insn::Operand(e);
        }
    }
}

// memory addresses
EffAddr::Ptr
NasmParser::parse_memaddr()
{
    switch (m_token)
    {
        case SEGREG:
        {
            const SegmentRegister* segreg = SEGREG_val;
            get_next_token();
            if (m_token != ':')
                throw SyntaxError(N_("`:' required after segment register"));
            get_next_token();
            EffAddr::Ptr ea = parse_memaddr();
            ea->set_segreg(segreg);
            return ea;
        }
        case SIZE_OVERRIDE:
        {
            unsigned int size = SIZE_OVERRIDE_val;
            get_next_token();
            EffAddr::Ptr ea = parse_memaddr();
            ea->m_disp.m_size = size;
            return ea;
        }
        case NOSPLIT:
        {
            get_next_token();
            EffAddr::Ptr ea = parse_memaddr();
            ea->m_nosplit = true;
            return ea;
        }
        case REL:
        {
            get_next_token();
            EffAddr::Ptr ea = parse_memaddr();
            ea->m_pc_rel = true;
            ea->m_not_pc_rel = false;
            return ea;
        }
        case ABS:
        {
            get_next_token();
            EffAddr::Ptr ea = parse_memaddr();
            ea->m_pc_rel = false;
            ea->m_not_pc_rel = true;
            return ea;
        }
        default:
        {
            Expr::Ptr e = parse_expr(NORM_EXPR);
            if (e.get() == 0)
                throw SyntaxError(N_("memory address expected"));
            return m_object->get_arch()->ea_create(e);
        }
    }
}

// Expression grammar parsed is:
//
// expr  : bexpr [ : bexpr ]
// bexpr : expr0 [ WRT expr6 ]
// expr0 : expr1 [ {|} expr1...]
// expr1 : expr2 [ {^} expr2...]
// expr2 : expr3 [ {&} expr3...]
// expr3 : expr4 [ {<<,>>} expr4...]
// expr4 : expr5 [ {+,-} expr5...]
// expr5 : expr6 [ {*,/,%,//,%%} expr6...]
// expr6 : { ~,+,-,SEG } expr6
//       | (expr)
//       | symbol
//       | $
//       | number

#define parse_expr_common(leftfunc, tok, rightfunc, op) \
    do {                                                \
        Expr::Ptr e = leftfunc(type);                   \
        if (e.get() == 0)                               \
            return e;                                   \
                                                        \
        while (m_token == tok)                          \
        {                                               \
            get_next_token();                           \
            Expr::Ptr f = rightfunc(type);              \
            if (f.get() == 0)                           \
                return f;                               \
            e.reset(new Expr(e, op, f));                \
        }                                               \
        return e;                                       \
    } while(0)

Expr::Ptr
NasmParser::parse_expr(ExprType type)
{
    switch (type)
    {
        case NORM_EXPR:
            parse_expr_common(parse_bexpr, ':', parse_bexpr, Op::SEGOFF);
        case DV_EXPR:
            // dataval expressions can't handle seg:off
            return parse_bexpr(type);
        case DIR_EXPR:
            // directive expressions can't handle seg:off or WRT
            return parse_expr0(type);
    }
    /*@notreached@*/
    return Expr::Ptr(0);
}

Expr::Ptr
NasmParser::parse_bexpr(ExprType type)
{
    parse_expr_common(parse_expr0, WRT, parse_expr6, Op::WRT);
}

Expr::Ptr
NasmParser::parse_expr0(ExprType type)
{
    parse_expr_common(parse_expr1, '|', parse_expr1, Op::OR);
}

Expr::Ptr
NasmParser::parse_expr1(ExprType type)
{
    parse_expr_common(parse_expr2, '^', parse_expr2, Op::XOR);
}

Expr::Ptr
NasmParser::parse_expr2(ExprType type)
{
    parse_expr_common(parse_expr3, '&', parse_expr3, Op::AND);
}

Expr::Ptr
NasmParser::parse_expr3(ExprType type)
{
    Expr::Ptr e = parse_expr4(type);
    if (e.get() == 0)
        return e;

    while (m_token == LEFT_OP || m_token == RIGHT_OP)
    {
        int op = m_token;
        get_next_token();
        Expr::Ptr f = parse_expr4(type);
        if (f.get() == 0)
            return f;

        switch (op)
        {
            case LEFT_OP: e.reset(new Expr(e, Op::SHL, f)); break;
            case RIGHT_OP: e.reset(new Expr(e, Op::SHR, f)); break;
        }
    }
    return e;
}

Expr::Ptr
NasmParser::parse_expr4(ExprType type)
{
    Expr::Ptr e = parse_expr5(type);
    if (e.get() == 0)
        return e;

    while (m_token == '+' || m_token == '-')
    {
        int op = m_token;
        get_next_token();
        Expr::Ptr f = parse_expr5(type);
        if (f.get() == 0)
            return f;

        switch (op)
        {
            case '+': e.reset(new Expr(e, Op::ADD, f)); break;
            case '-': e.reset(new Expr(e, Op::SUB, f)); break;
        }
    }
    return e;
}

Expr::Ptr
NasmParser::parse_expr5(ExprType type)
{
    Expr::Ptr e = parse_expr6(type);
    if (e.get() == 0)
        return e;

    while (m_token == '*' || m_token == '/' || m_token == '%'
           || m_token == SIGNDIV || m_token == SIGNMOD)
    {
        int op = m_token;
        get_next_token();
        Expr::Ptr f = parse_expr6(type);
        if (f.get() == 0)
            return f;

        switch (op)
        {
            case '*': e.reset(new Expr(e, Op::MUL, f)); break;
            case '/': e.reset(new Expr(e, Op::DIV, f)); break;
            case '%': e.reset(new Expr(e, Op::MOD, f)); break;
            case SIGNDIV: e.reset(new Expr(e, Op::SIGNDIV, f)); break;
            case SIGNMOD: e.reset(new Expr(e, Op::SIGNMOD, f)); break;
        }
    }
    return e;
}

Expr::Ptr
NasmParser::parse_expr6(ExprType type)
{
    Expr::Ptr e(0);

    /* directives allow very little and handle IDs specially */
    if (type == DIR_EXPR)
    {
        switch (m_token)
        {
        case '~':
            get_next_token();
            e = parse_expr6(type);
            if (e.get() == 0)
                return e;
            return Expr::Ptr(new Expr(Op::NOT, e));
        case '(':
            get_next_token();
            e = parse_expr(type);
            if (e.get() == 0)
                return e;
            if (m_token != ')')
                throw SyntaxError(N_("missing parenthesis"));
            break;
        case INTNUM:
            e.reset(new Expr(INTNUM_val));
            break;
        case REG:
            e.reset(new Expr(REG_val));
            break;
        case ID:
            e.reset(new Expr(m_object->get_sym(ID_val).use(get_cur_line())));
            break;
        default:
            return e;
        }
    }
    else switch (m_token)
    {
        case '+':
            get_next_token();
            return parse_expr6(type);
        case '-':
            get_next_token();
            e = parse_expr6(type);
            if (e.get() == 0)
                return e;
            return Expr::Ptr(new Expr(Op::NEG, e));
        case '~':
            get_next_token();
            e = parse_expr6(type);
            if (e.get() == 0)
                return e;
            return Expr::Ptr(new Expr(Op::NOT, e));
        case SEG:
            get_next_token();
            e = parse_expr6(type);
            if (e.get() == 0)
                return e;
            return Expr::Ptr(new Expr(Op::SEG, e));
        case '(':
            get_next_token();
            e = parse_expr(type);
            if (e.get() == 0)
                return e;
            if (m_token != ')')
                throw SyntaxError(N_("missing parenthesis"));
            break;
        case INTNUM:
            e.reset(new Expr(INTNUM_val));
            break;
        case FLTNUM:
            e.reset(new Expr(FLTNUM_val));
            break;
        case REG:
            if (type == DV_EXPR)
                throw SyntaxError(N_("data values can't have registers"));
            e.reset(new Expr(REG_val));
            break;
#if 0
        case STRING:
            e = p_expr_new_ident(yasm_expr_int(
                yasm_intnum_create_charconst_nasm(STRING_val.contents)));
            yasm_xfree(STRING_val.contents);
            break;
#endif
        case ID:
        case SPECIAL_ID:
        case LOCAL_ID:
            e.reset(new Expr(m_object->get_sym(ID_val).use(get_cur_line())));
            break;
        case '$':
            // "$" references the current assembly position
            if (m_abspos.get() != 0)
                e.reset(m_abspos->clone());
            else
            {
                std::auto_ptr<Symbol> sym(new Symbol("$"));
                m_bc = &m_container->fresh_bytecode();
                Location loc = {m_bc, m_bc->get_fixed_len()};
                sym->define_curpos(loc, get_cur_line());
                e.reset(new Expr(sym.get()));
                m_object->add_non_table_symbol(sym);
            }
            break;
        case START_SECTION_ID:
            // "$$" references the start of the current section
            if (m_absstart.get() != 0)
                e.reset(m_absstart->clone());
            else
            {
                std::auto_ptr<Symbol> sym(new Symbol("$$"));
                Location loc = {&m_container->bcs_first(), 0};
                sym->define_label(loc, get_cur_line());
                e.reset(new Expr(sym.get()));
                m_object->add_non_table_symbol(sym);
            }
            break;
        default:
            return e;
    }
    get_next_token();
    return e;
}

void
NasmParser::define_label(const std::string& name, bool local)
{
    if (!local)
        m_locallabel_base = name;

    Symbol& sym = m_object->get_sym(name);
    if (m_abspos.get() != 0)
        sym.define_equ(Expr::Ptr(m_abspos->clone()), get_cur_line());
    else
    {
        m_bc = &m_container->fresh_bytecode();
        Location loc = {m_bc, m_bc->get_fixed_len()};
        sym.define_label(loc, get_cur_line());
    }
}

void
NasmParser::dir_absolute(Object& object, const NameValues& namevals,
                         const NameValues& objext_namevals, unsigned long line)
{
    m_absstart.reset(namevals.front().get_expr(object, line).release());
    m_abspos.reset(m_absstart->clone());
    object.set_cur_section(0);
}

void
NasmParser::dir_align(Object& object, const NameValues& namevals,
                      const NameValues& objext_namevals, unsigned long line)
{
    // Really, we shouldn't end up with an align directive in an absolute
    // section (as it's supposed to be only used for nop fill), but handle
    // it gracefully anyway.
    if (m_abspos.get() != 0)
    {
        Expr::Ptr boundval = namevals.front().get_expr(object, line);
        Expr::Ptr e(new Expr(
            new Expr(m_absstart->clone(), Op::SUB, m_abspos->clone(), line),
            Op::AND,
            new Expr(boundval, Op::SUB, new IntNum(1), line),
            line));
        m_abspos.reset(new Expr(m_abspos, Op::ADD, e, line));
    }
    else
    {
        Section* cur_section = object.get_cur_section();
        Expr::Ptr boundval = namevals.front().get_expr(object, line);
        IntNum* boundintn;

        // Largest .align in the section specifies section alignment.
        // Note: this doesn't match NASM behavior, but is a lot more
        // intelligent!
        if (boundval.get() != 0 && (boundintn = boundval->get_intnum()))
        {
            unsigned long boundint = boundintn->get_uint();

            // Alignments must be a power of two.
            if (is_exp2(boundint))
            {
                if (boundint > cur_section->get_align())
                    cur_section->set_align(boundint);
            }
        }

        // As this directive is called only when nop is used as fill, always
        // use arch (nop) fill.
        append_align(*cur_section, boundval, Expr::Ptr(0), Expr::Ptr(0),
                     /*cur_section->is_code() ?*/
                     object.get_arch()->get_fill()/* : 0*/,
                     line);
    }
}

void
NasmParser::dir_default(Object& object, const NameValues& namevals,
                        const NameValues& objext_namevals, unsigned long line)
{
    for (NameValues::const_iterator nv=namevals.begin(), end=namevals.end();
         nv != end; ++nv)
    {
        if (nv->is_id())
        {
            std::string id = nv->get_id();
            if (String::nocase_equal(id, "rel") == 0)
                object.get_arch()->set_var("default_rel", 1);
            else if (String::nocase_equal(id, "abs") == 0)
                object.get_arch()->set_var("default_rel", 0);
            else
                throw SyntaxError(String::compose(
                    N_("unrecognized default `%1'"), id));
        }
        else
            throw SyntaxError(N_("unrecognized default value"));
    }
}

void
NasmParser::directive(const std::string& name,
                      const NameValues& namevals,
                      const NameValues& objext_namevals)
{
    (*m_dirs)[name](*m_object, namevals, objext_namevals, get_cur_line());
    Section* cursect = m_object->get_cur_section();
    if (m_absstart.get() != 0 && cursect)
    {
        // We switched to a new section.  Get out of absolute section mode.
        m_absstart.reset(0);
        m_abspos.reset(0);
    }
}

}}} // namespace yasm::parser::nasm

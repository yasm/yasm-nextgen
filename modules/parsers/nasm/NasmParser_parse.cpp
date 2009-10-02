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
#include "util.h"

#include <math.h>

#include "clang/Basic/SourceManager.h"
#include "yasmx/Support/bitcount.h"
#include "yasmx/Arch.h"
#include "yasmx/BytecodeContainer_util.h"
#include "yasmx/Directive.h"
#include "yasmx/EffAddr.h"
#include "yasmx/Errwarns.h"
#include "yasmx/Expr.h"
#include "yasmx/NameValue.h"
#include "yasmx/Object.h"
#include "yasmx/Preprocessor.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"

#include "modules/parsers/nasm/NasmParser.h"


namespace yasm
{
namespace parser
{
namespace nasm
{

llvm::StringRef
NasmParser::DescribeToken(int token)
{
    static char strch[] = "` '";
    const char *str;

    switch (token)
    {
        case 0:                 str = "end of line"; break;
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
        case NONLOCAL_ID:       str = "..@identifier"; break;
        case LINE:              str = "%line"; break;
        default:
            strch[1] = token;
            str = strch;
            break;
    }

    return str;
}

void
NasmParser::DoParse()
{
    std::string line;
    Bytecode::Ptr bc(new Bytecode());

    while (m_preproc->getLine(&line, &m_source))
    {
        if (!m_abspos.isEmpty())
            m_bc = bc.get();
        else
            m_bc = &m_object->getCurSection()->FreshBytecode();

        try
        {
            m_bot = m_tok = m_ptr = m_cur = &line[0];
            m_lim = &line[line.length()+1];

            getNextToken();
            if (!isEol())
            {
                bool ok = ParseLine();
                if (!ok)
                {
                    DemandEolNoThrow();
                    m_state = INITIAL;
                }
                else
                    DemandEol();
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
                        const Expr* multiple = m_bc->getMultipleExpr();
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
            m_errwarns.Propagate(m_source);
        }
        catch (Error& err)
        {
            m_errwarns.Propagate(m_source, err);
            DemandEolNoThrow();
            m_state = INITIAL;
        }
    }
}

// All Parse* functions expect to be called with m_token being their first
// token.  They should return with m_token being the token *after* their
// information.

bool
NasmParser::ParseLine()
{
    m_container = m_object->getCurSection();

    if (ParseExp())
        return true;

    switch (m_token)
    {
        case LINE: // LINE INTNUM '+' INTNUM FILENAME
        {
            getNextToken();

            if (!Expect(INTNUM, diag::err_expected_integer))
                return false;
            std::auto_ptr<IntNum> line(INTNUM_val);
            getNextToken();

            if (!ExpectAndConsume('+', diag::err_expected_plus))
                return false;

            if (!Expect(INTNUM, diag::err_expected_integer))
                return false;
            std::auto_ptr<IntNum> incr(INTNUM_val);
            getNextToken();

            if (!Expect(FILENAME, diag::err_expected_filename))
                return false;
            std::string filename;
            std::swap(filename, FILENAME_val);
            getNextToken();

            // %line indicates the line number of the *next* line, so subtract
            // out the increment when setting the line number.
            // FIXME: handle incr
            clang::SourceManager& smgr = m_preproc->getSourceManager();
            smgr.AddLineNote(m_source, line->getUInt(),
                             smgr.getLineTableFilenameID(filename.data(),
                                                         filename.size()));
            break;
        }
        case '[': // [ directive ]
        {
            clang::SourceLocation lhsrc = getTokenSource();
            m_state = DIRECTIVE;
            getNextToken();

            if (!Expect(DIRECTIVE_NAME, diag::err_expected_directive_name))
                return false;
            std::string dirname;
            std::swap(dirname, DIRECTIVE_NAME_val);
            getNextToken();

            // catch [directive<eol> early (XXX: better way to do this?)
            if (isEol())
            {
                Diag(getTokenSource(), diag::err_expected_rsquare);
                Diag(lhsrc, diag::note_matching) << "[";
                return false;
            }

            DirectiveInfo info(*m_object, getFullSource());
            if (m_token != ']' && m_token != ':' &&
                !ParseDirective(info.getNameValues()))
                return false;
            if (m_token == ':')
            {
                getNextToken();
                if (!ParseDirective(info.getObjextNameValues()))
                    return false;
            }
            DoDirective(dirname, info);
            if (!ExpectAndConsume(']', diag::err_expected_rsquare))
            {
                Diag(lhsrc, diag::note_matching) << "[";
                return false;
            }
            break;
        }
        case TIMES: // TIMES expr exp
            getNextToken();
            return ParseTimes();
        case ID:
        case SPECIAL_ID:
        case NONLOCAL_ID:
        case LOCAL_ID:
        {
            bool local = (m_token != ID);
            std::string name;
            std::swap(name, ID_val);

            getNextToken();
            if (isEol())
            {
                // label alone on the line
                Diag(getTokenSource(), diag::warn_orphan_label);
                DefineLabel(name, local);
                break;
            }
            if (m_token == ':')
                getNextToken();

            if (m_token == EQU)
            {
                // label EQU expr
                getNextToken();
                Expr e;
                if (!ParseExpr(e, NORM_EXPR))
                {
                    Diag(getTokenSource(),
                         diag::err_expected_expression_after_id)
                        << "EQU";
                    return false;
                }
                m_object->getSymbol(name)->DefineEqu(e, m_source);
                break;
            }

            DefineLabel(name, local);
            if (isEol())
                break;
            if (m_token == TIMES)
            {
                getNextToken();
                return ParseTimes();
            }
            if (!ParseExp())
            {
                Diag(getTokenSource(), diag::err_expected_insn_after_label);
                return false;
            }
            break;
        }
        default:
            Diag(getTokenSource(), diag::err_expected_insn_label_after_eol);
            return false;
    }
    return true;
}

bool
NasmParser::ParseDirective(/*@out@*/ NameValues& nvs)
{
    for (;;)
    {
        std::string id;
        std::auto_ptr<NameValue> nv(0);

        // Look for value first
        if (m_token == ID)
        {
            getPeekToken();
            if (m_peek_token == '=')
            {
                std::swap(id, ID_val);
                getNextToken(); // id
                getNextToken(); // '='
            }
        }

        // Look for parameter
        switch (m_token)
        {
            case STRING:
                nv.reset(new NameValue(id, STRING_val));
                getNextToken();
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
                    getPeekToken();
                switch (m_peek_token)
                {
                    case '|': case '^': case '&': case LEFT_OP: case RIGHT_OP:
                    case '+': case '-':
                    case '*': case '/': case '%': case SIGNDIV: case SIGNMOD:
                        break;
                    default:
                        // Just an id
                        nv.reset(new NameValue(id, ID_val, '$'));
                        getNextToken();
                        goto next;
                }
                /*@fallthrough@*/
            default:
            {
                Expr::Ptr e(new Expr);
                if (!ParseExpr(*e, DIR_EXPR))
                    return false;
                nv.reset(new NameValue(id, e));
                break;
            }
        }
next:
        nvs.push_back(nv.release());
        if (m_token == ',')
            getNextToken();
        if (m_token == ']' || m_token == ':' || isEol())
            return true;
    }
}

bool
NasmParser::ParseTimes()
{
    Expr::Ptr multiple(new Expr);
    if (!ParseBExpr(*multiple, DV_EXPR))
    {
        Diag(getTokenSource(), diag::err_expected_expression_after_id)
            << "TIMES";
        return false;
    }
    BytecodeContainer* orig_container = m_container;
    m_container = &AppendMultiple(*m_container, multiple, m_source);

    clang::SourceLocation cursource = getTokenSource();
    if (!ParseExp())
    {
        Diag(cursource, diag::err_expected_insn_after_times);
        m_container = orig_container;
        return false;
    }
    m_container = orig_container;
    return true;
}

bool
NasmParser::ParseExp()
{
    Insn::Ptr insn = ParseInsn();
    if (insn.get() != 0)
    {
        insn->Append(*m_container, m_source);
        return true;
    }

    switch (m_token)
    {
        case DECLARE_DATA:
        {
            unsigned int size = DECLARE_DATA_val/8;
            getNextToken();

            for (;;)
            {
                if (m_token == STRING)
                {
                    // Peek ahead to see if we're in an expr.  If we're not,
                    // then generate a real string dataval.
                    getPeekToken();
                    if (m_peek_token == ',' || isEolTok(m_peek_token))
                    {
                        AppendData(*m_container, STRING_val, size, false);
                        getNextToken();
                        goto dv_done;
                    }
                }
                {
                    Expr::Ptr e(new Expr);
                    if (ParseBExpr(*e, DV_EXPR))
                        AppendData(*m_container, e, size,
                                    *m_object->getArch(), m_source);
                    else
                    {
                        Diag(getTokenSource(),
                             diag::err_expected_expression_or_string);
                        return false;
                    }
                }
dv_done:
                if (isEol())
                    break;
                ExpectAndConsume(',', diag::err_expected_comma);
                if (isEol())   // allow trailing , on list
                    break;
            }
            return true;
        }
        case RESERVE_SPACE:
        {
            unsigned int size = RESERVE_SPACE_val/8;
            getNextToken();
            Expr::Ptr e(new Expr);
            if (!ParseBExpr(*e, DV_EXPR))
            {
                Diag(getTokenSource(), diag::err_expected_expression_after_id)
                    << "RESx";
                return false;
            }
            BytecodeContainer& multc =
                AppendMultiple(*m_container, e, m_source);
            multc.AppendGap(size, m_source);
            return true;
        }
        case INCBIN:
        {
            Expr::Ptr start(0), maxlen(0);

            getNextToken();

            if (!Expect(STRING, diag::err_expected_string))
                return false;
            std::string filename;
            std::swap(filename, STRING_val);
            getNextToken();

            // optional start expression
            if (m_token == ',')
                getNextToken();
            if (isEol())
                goto incbin_done;
            start.reset(new Expr);
            if (!ParseBExpr(*start, DV_EXPR))
            {
                Diag(getTokenSource(), diag::err_expected_expression);
                return false;
            }

            // optional maxlen expression
            if (m_token == ',')
                getNextToken();
            if (isEol())
                goto incbin_done;
            maxlen.reset(new Expr);
            if (!ParseBExpr(*maxlen, DV_EXPR))
            {
                Diag(getTokenSource(), diag::err_expected_expression);
                return false;
            }

incbin_done:
            AppendIncbin(*m_container, filename, start, maxlen, m_source);
            return true;
        }
        default:
            break;
    }
    return false;
}

Insn::Ptr
NasmParser::ParseInsn()
{
    switch (m_token)
    {
        case INSN:
        {
            Insn::Ptr insn(INSN_val);
            getNextToken();
            if (isEol())
                return insn;    // no operands

            // parse operands
            for (;;)
            {
                insn->AddOperand(ParseOperand());

                if (isEol())
                    break;
                ExpectAndConsume(',', diag::err_expected_comma);
            }
            return insn;
        }
        case PREFIX:
        {
            const Prefix* prefix = PREFIX_val;
            getNextToken();
            Insn::Ptr insn = ParseInsn();
            if (insn.get() == 0)
                insn = m_arch->CreateEmptyInsn();
            insn->AddPrefix(prefix);
            return insn;
        }
        case SEGREG:
        {
            const SegmentRegister* segreg = SEGREG_val;
            getNextToken();
            Insn::Ptr insn = ParseInsn();
            if (insn.get() == 0)
                insn = m_arch->CreateEmptyInsn();
            insn->AddSegPrefix(segreg);
            return insn;
        }
        default:
            return Insn::Ptr(0);
    }
}

Operand
NasmParser::ParseOperand()
{
    switch (m_token)
    {
        case '[':
        {
            clang::SourceLocation lhsrc = getTokenSource();
            getNextToken();
            Operand op = ParseMemoryAddress();

            if (!ExpectAndConsume(']', diag::err_expected_rsquare))
                Diag(lhsrc, diag::note_matching) << "[";

            return op;
        }
        case SEGREG:
        {
            Operand op(SEGREG_val);
            getNextToken();
            return op;
        }
        case REG:
        {
            Operand op(REG_val);
            getNextToken();
            return op;
        }
        case STRICT:
        {
            getNextToken();
            Operand op = ParseOperand();
            op.setStrict();
            return op;
        }
        case SIZE_OVERRIDE:
        {
            unsigned int size = SIZE_OVERRIDE_val;
            getNextToken();
            Operand op = ParseOperand();
            const Register* reg = op.getReg();
            if (reg && reg->getSize() != size)
            {
                Diag(getTokenSource(), diag::err_register_size_override);
                return op;
            }
            else
            {
                // Silently override others unless a warning is turned on.
                // This is to allow overrides such as:
                //   %define arg1 dword [bp+4]
                //   cmp word arg1, 2
                // Which expands to:
                //   cmp word dword [bp+4], 2
                unsigned int opsize = op.getSize();
                if (opsize != 0)
                {
                    if (opsize != size)
                        Diag(getTokenSource(), diag::warn_operand_size_override)
                            << opsize << size;
                    else
                        Diag(getTokenSource(),
                             diag::warn_operand_size_duplicate);
                }
                op.setSize(size);
            }
            return op;
        }
        case TARGETMOD:
        {
            const TargetModifier* tmod = TARGETMOD_val;
            getNextToken();
            Operand op = ParseOperand();
            op.setTargetMod(tmod);
            return op;
        }
        default:
        {
            Expr::Ptr e(new Expr);
            if (!ParseBExpr(*e, NORM_EXPR))
            {
                Diag(getTokenSource(), diag::err_expected_operand);
                return Operand(e);
            }
            if (m_token != ':')
                return Operand(e);
            getNextToken();
            Expr::Ptr off(new Expr);
            if (!ParseBExpr(*off, NORM_EXPR))
            {
                Diag(getTokenSource(), diag::err_expected_expression_after)
                    << ":";
                return Operand(e);
            }
            Operand op(off);
            op.setSeg(e);
            return op;
        }
    }
}

// memory addresses
Operand
NasmParser::ParseMemoryAddress()
{
    switch (m_token)
    {
        case SEGREG:
        {
            const SegmentRegister* segreg = SEGREG_val;
            getNextToken();
            if (!ExpectAndConsume(':', diag::err_expected_colon_after_segreg))
                return Operand(segreg);
            Operand op = ParseMemoryAddress();
            op.getMemory()->setSegReg(segreg);
            return op;
        }
        case SIZE_OVERRIDE:
        {
            unsigned int size = SIZE_OVERRIDE_val;
            getNextToken();
            Operand op = ParseMemoryAddress();
            op.getMemory()->m_disp.setSize(size);
            return op;
        }
        case NOSPLIT:
        {
            getNextToken();
            Operand op = ParseMemoryAddress();
            op.getMemory()->m_nosplit = true;
            return op;
        }
        case REL:
        {
            getNextToken();
            Operand op = ParseMemoryAddress();
            EffAddr* ea = op.getMemory();
            ea->m_pc_rel = true;
            ea->m_not_pc_rel = false;
            return op;
        }
        case ABS:
        {
            getNextToken();
            Operand op = ParseMemoryAddress();
            EffAddr* ea = op.getMemory();
            ea->m_pc_rel = false;
            ea->m_not_pc_rel = true;
            return op;
        }
        default:
        {
            Expr::Ptr e(new Expr);
            if (!ParseBExpr(*e, NORM_EXPR))
            {
                Diag(getTokenSource(), diag::err_expected_memory_address);
                return Operand(e);
            }
            if (m_token != ':')
                return Operand(m_object->getArch()->CreateEffAddr(e));
            getNextToken();
            Expr::Ptr off(new Expr);
            if (!ParseBExpr(*off, NORM_EXPR))
            {
                Diag(getTokenSource(), diag::err_expected_expression_after)
                    << ":";
                return Operand(e);
            }
            Operand op(m_object->getArch()->CreateEffAddr(off));
            op.setSeg(e);
            return op;
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

#define ParseExprCommon(leftfunc, tok, rightfunc, op) \
    do {                                              \
        if (!leftfunc(e, type))                       \
            return false;                             \
                                                      \
        while (m_token == tok)                        \
        {                                             \
            getNextToken();                           \
            Expr f;                                   \
            if (!rightfunc(f, type))                  \
                return false;                         \
            e.Calc(op, f);                            \
        }                                             \
        return true;                                  \
    } while(0)

bool
NasmParser::ParseExpr(Expr& e, ExprType type)
{
    switch (type)
    {
        case DIR_EXPR:
            // directive expressions can't handle seg:off or WRT
            return ParseExpr0(e, type);
        default:
            ParseExprCommon(ParseBExpr, ':', ParseBExpr, Op::SEGOFF);
    }
    /*@notreached@*/
    return false;
}

bool
NasmParser::ParseBExpr(Expr& e, ExprType type)
{
    ParseExprCommon(ParseExpr0, WRT, ParseExpr6, Op::WRT);
}

bool
NasmParser::ParseExpr0(Expr& e, ExprType type)
{
    ParseExprCommon(ParseExpr1, '|', ParseExpr1, Op::OR);
}

bool
NasmParser::ParseExpr1(Expr& e, ExprType type)
{
    ParseExprCommon(ParseExpr2, '^', ParseExpr2, Op::XOR);
}

bool
NasmParser::ParseExpr2(Expr& e, ExprType type)
{
    ParseExprCommon(ParseExpr3, '&', ParseExpr3, Op::AND);
}

bool
NasmParser::ParseExpr3(Expr& e, ExprType type)
{
    if (!ParseExpr4(e, type))
        return false;

    while (m_token == LEFT_OP || m_token == RIGHT_OP)
    {
        int op = m_token;
        getNextToken();
        Expr f;
        if (!ParseExpr4(f, type))
            return false;

        switch (op)
        {
            case LEFT_OP: e.Calc(Op::SHL, f); break;
            case RIGHT_OP: e.Calc(Op::SHR, f); break;
        }
    }
    return true;
}

bool
NasmParser::ParseExpr4(Expr& e, ExprType type)
{
    if (!ParseExpr5(e, type))
        return false;

    while (m_token == '+' || m_token == '-')
    {
        int op = m_token;
        getNextToken();
        Expr f;
        if (!ParseExpr5(f, type))
            return false;

        switch (op)
        {
            case '+': e.Calc(Op::ADD, f); break;
            case '-': e.Calc(Op::SUB, f); break;
        }
    }
    return true;
}

bool
NasmParser::ParseExpr5(Expr& e, ExprType type)
{
    if (!ParseExpr6(e, type))
        return false;

    while (m_token == '*' || m_token == '/' || m_token == '%'
           || m_token == SIGNDIV || m_token == SIGNMOD)
    {
        int op = m_token;
        getNextToken();
        Expr f;
        if (!ParseExpr6(f, type))
            return false;

        switch (op)
        {
            case '*': e.Calc(Op::MUL, f); break;
            case '/': e.Calc(Op::DIV, f); break;
            case '%': e.Calc(Op::MOD, f); break;
            case SIGNDIV: e.Calc(Op::SIGNDIV, f); break;
            case SIGNMOD: e.Calc(Op::SIGNMOD, f); break;
        }
    }
    return true;
}

bool
NasmParser::ParseExpr6(Expr& e, ExprType type)
{
    /* directives allow very little and handle IDs specially */
    if (type == DIR_EXPR)
    {
        switch (m_token)
        {
        case '~':
            getNextToken();
            if (!ParseExpr6(e, type))
                return false;
            e.Calc(Op::NOT);
            return true;
        case '(':
        {
            clang::SourceLocation lhsrc = getTokenSource();
            getNextToken();
            if (!ParseExpr(e, type))
                return false;
            // If missing closing paren, emit an error but otherwise assume
            // it was there.
            if (!ExpectAndConsume(')', diag::err_expected_rparen))
                Diag(lhsrc, diag::note_matching) << "(";
            return true;
        }
        case INTNUM:
            e = INTNUM_val;
            break;
        case REG:
            e = *REG_val;
            break;
        case ID:
        {
            SymbolRef sym = m_object->getSymbol(ID_val);
            sym->Use(m_source);
            e = sym;
            break;
        }
        default:
            return false;
        }
    }
    else switch (m_token)
    {
        case '+':
            getNextToken();
            return ParseExpr6(e, type);
        case '-':
            getNextToken();
            if (!ParseExpr6(e, type))
                return false;
            e.Calc(Op::NEG);
            return true;
        case '~':
            getNextToken();
            if (!ParseExpr6(e, type))
                return false;
            e.Calc(Op::NOT);
            return true;
        case SEG:
            getNextToken();
            if (!ParseExpr6(e, type))
                return false;
            e.Calc(Op::SEG);
            return true;
        case '(':
        {
            clang::SourceLocation lhsrc = getTokenSource();
            getNextToken();
            if (!ParseExpr(e, type))
                return false;
            // If missing closing paren, emit an error but otherwise assume
            // it was there.
            if (!ExpectAndConsume(')', diag::err_expected_rparen))
                Diag(lhsrc, diag::note_matching) << "(";
            return true;
        }
        case INTNUM:
            e = INTNUM_val;
            break;
        case FLTNUM:
            e = FLTNUM_val;
            break;
        case REG:
            if (type == DV_EXPR)
                Diag(getTokenSource(), diag::err_data_value_register);
            e = *REG_val;
            break;
        case STRING:
        {
            const char* strbegin = STRING_val.data();
            const char* p = strbegin+STRING_val.length();
            IntNum intn(0);
            while (p > strbegin)
            {
                intn <<= 8;
                intn |= static_cast<unsigned int>(*--p);
            }
            e = intn;
            break;
        }
        case SPECIAL_ID:
        {
            SymbolRef sym =
                m_object->FindSpecialSymbol(ID_val.c_str()+2);
            if (sym)
            {
                e = sym;
                break;
            }
            /*@fallthrough@*/
        }
        case ID:
        case LOCAL_ID:
        case NONLOCAL_ID:
        {
            SymbolRef sym = m_object->getSymbol(ID_val);
            sym->Use(m_source);
            e = sym;
            break;
        }
        case '$':
            // "$" references the current assembly position
            if (!m_abspos.isEmpty())
                e = m_abspos;
            else
            {
                SymbolRef sym = m_object->AddNonTableSymbol("$");
                m_bc = &m_container->FreshBytecode();
                Location loc = {m_bc, m_bc->getFixedLen()};
                sym->DefineLabel(loc, m_source);
                e = sym;
            }
            break;
        case START_SECTION_ID:
            // "$$" references the start of the current section
            if (!m_absstart.isEmpty())
                e = m_absstart;
            else
            {
                SymbolRef sym = m_object->AddNonTableSymbol("$$");
                Location loc = {&m_container->bytecodes_first(), 0};
                sym->DefineLabel(loc, m_source);
                e = sym;
            }
            break;
        default:
            return false;
    }
    getNextToken();
    return true;
}

void
NasmParser::DefineLabel(llvm::StringRef name, bool local)
{
    if (!local)
        m_locallabel_base = name;

    SymbolRef sym = m_object->getSymbol(name);
    if (!m_abspos.isEmpty())
        sym->DefineEqu(m_abspos, m_source);
    else
    {
        m_bc = &m_container->FreshBytecode();
        Location loc = {m_bc, m_bc->getFixedLen()};
        sym->DefineLabel(loc, m_source);
    }
}

void
NasmParser::DirAbsolute(DirectiveInfo& info)
{
    Object& object = info.getObject();
    m_absstart = info.getNameValues().front().getExpr(object, info.getSource());
    m_abspos = m_absstart;
    object.setCurSection(0);
}

void
NasmParser::DirAlign(DirectiveInfo& info)
{
    Object& object = info.getObject();
    NameValues& namevals = info.getNameValues();
    clang::SourceLocation source = info.getSource();

    // Really, we shouldn't end up with an align directive in an absolute
    // section (as it's supposed to be only used for nop fill), but handle
    // it gracefully anyway.
    if (!m_abspos.isEmpty())
    {
        Expr e = SUB(m_absstart, m_abspos);
        e &= SUB(namevals.front().getExpr(object, source), 1);
        m_abspos += e;
    }
    else
    {
        Section* cur_section = object.getCurSection();
        Expr boundval = namevals.front().getExpr(object, source);

        // Largest .align in the section specifies section alignment.
        // Note: this doesn't match NASM behavior, but is a lot more
        // intelligent!
        boundval.Simplify();
        if (boundval.isIntNum())
        {
            unsigned long boundint = boundval.getIntNum().getUInt();

            // Alignments must be a power of two.
            if (isExp2(boundint))
            {
                if (boundint > cur_section->getAlign())
                    cur_section->setAlign(boundint);
            }
        }

        // As this directive is called only when nop is used as fill, always
        // use arch (nop) fill.
        AppendAlign(*cur_section, boundval, Expr(), Expr(),
                    /*cur_section->isCode() ?*/
                    object.getArch()->getFill()/* : 0*/,
                    source);
    }
}

void
NasmParser::DirDefault(DirectiveInfo& info)
{
    clang::SourceLocation source = info.getSource();
    for (NameValues::const_iterator nv=info.getNameValues().begin(),
         end=info.getNameValues().end(); nv != end; ++nv)
    {
        if (nv->isId())
        {
            llvm::StringRef id = nv->getId();
            if (id.equals_lower("rel"))
                info.getObject().getArch()->setVar("default_rel", 1);
            else if (id.equals_lower("abs"))
                info.getObject().getArch()->setVar("default_rel", 0);
            else
            {
                Diag(source, m_diags->getCustomDiagID(Diagnostic::Error,
                    "unrecognized default '%0'")) << id;
            }
        }
        else
        {
            Diag(source, m_diags->getCustomDiagID(Diagnostic::Error,
                "unrecognized default value"));
        }
    }
}

void
NasmParser::DoDirective(llvm::StringRef name, DirectiveInfo& info)
{
    (*m_dirs)[name](info);
    Section* cursect = m_object->getCurSection();
    if (!m_absstart.isEmpty() && cursect)
    {
        // We switched to a new section.  Get out of absolute section mode.
        m_absstart.Clear();
        m_abspos.Clear();
    }
}

}}} // namespace yasm::parser::nasm

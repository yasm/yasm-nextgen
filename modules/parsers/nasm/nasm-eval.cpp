/* eval.c    expression evaluator for the Netwide Assembler
 *
 * The Netwide Assembler is copyright (C) 1996 Simon Tatham and
 * Julian Hall. All rights reserved. The software is
 * redistributable under the licence given in the file "Licence"
 * distributed in the NASM archive.
 *
 * initial version 27/iii/95 by Simon Tatham
 */
#include <cctype>

#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/Object.h"
#include "yasmx/Symbol.h"

#include "nasm.h"
#include "nasmlib.h"
#include "nasm-eval.h"


using yasm::Expr;
using yasm::IntNum;

namespace nasm {

/* The assembler object (for symbol table). */
yasm::Object *yasm_object;

static scanner scan;    /* Address of scanner routine */
static efunc error;     /* Address of error reporting routine */

static struct tokenval *tokval;   /* The current token */
static int i;                     /* The t_type of tokval */

static void *scpriv;

/*
 * Recursive-descent parser. Called with a single boolean operand,
 * which is TRUE if the evaluation is critical (i.e. unresolved
 * symbols are an error condition). Must update the global `i' to
 * reflect the token after the parsed string. May return NULL.
 *
 * evaluate() should report its own errors: on return it is assumed
 * that if NULL has been returned, the error has already been
 * reported.
 */

/*
 * Grammar parsed is:
 *
 * expr  : bexpr [ WRT expr6 ]
 * bexpr : rexp0 or expr0 depending on relative-mode setting
 * rexp0 : rexp1 [ {||} rexp1...]
 * rexp1 : rexp2 [ {^^} rexp2...]
 * rexp2 : rexp3 [ {&&} rexp3...]
 * rexp3 : expr0 [ {=,==,<>,!=,<,>,<=,>=} expr0 ]
 * expr0 : expr1 [ {|} expr1...]
 * expr1 : expr2 [ {^} expr2...]
 * expr2 : expr3 [ {&} expr3...]
 * expr3 : expr4 [ {<<,>>} expr4...]
 * expr4 : expr5 [ {+,-} expr5...]
 * expr5 : expr6 [ {*,/,%,//,%%} expr6...]
 * expr6 : { ~,+,-,SEG } expr6
 *       | (bexpr)
 *       | symbol
 *       | $
 *       | number
 */

static bool rexp0(Expr*), rexp1(Expr*), rexp2(Expr*), rexp3(Expr*);

static bool expr0(Expr*), expr1(Expr*), expr2(Expr*), expr3(Expr*);
static bool expr4(Expr*), expr5(Expr*), expr6(Expr*);

static bool (*bexpr)(Expr*);

static bool rexp0(Expr* e)
{
    if (!rexp1(e))
        return false;

    while (i == TOKEN_DBL_OR)
    {
        i = scan(scpriv, tokval);
        Expr f;
        if (!rexp1(&f))
            return false;

        e->Calc(yasm::Op::LOR, f);
    }
    return true;
}

static bool rexp1(Expr* e)
{
    if (!rexp2(e))
        return false;

    while (i == TOKEN_DBL_XOR)
    {
        i = scan(scpriv, tokval);
        Expr f;
        if (!rexp2(&f))
            return false;

        e->Calc(yasm::Op::LXOR, f);
    }
    return true;
}

static bool rexp2(Expr* e)
{
    if (!rexp3(e))
        return false;
    while (i == TOKEN_DBL_AND)
    {
        i = scan(scpriv, tokval);
        Expr f;
        if (!rexp3(&f))
            return false;

        e->Calc(yasm::Op::LAND, f);
    }
    return true;
}

static bool rexp3(Expr* e)
{
    if (!expr0(e))
        return false;

    while (i == TOKEN_EQ || i == TOKEN_LT || i == TOKEN_GT ||
           i == TOKEN_NE || i == TOKEN_LE || i == TOKEN_GE)
    {
        int j = i;
        i = scan(scpriv, tokval);
        Expr f;
        if (!expr0(&f))
            return false;

        switch (j)
        {
            case TOKEN_EQ:
                e->Calc(yasm::Op::EQ, f);
                break;
            case TOKEN_LT:
                e->Calc(yasm::Op::LT, f);
                break;
            case TOKEN_GT:
                e->Calc(yasm::Op::GT, f);
                break;
            case TOKEN_NE:
                e->Calc(yasm::Op::NE, f);
                break;
            case TOKEN_LE:
                e->Calc(yasm::Op::LE, f);
                break;
            case TOKEN_GE:
                e->Calc(yasm::Op::GE, f);
                break;
        }
    }
    return true;
}

static bool expr0(Expr* e)
{
    if (!expr1(e))
        return false;

    while (i == '|')
    {
        i = scan(scpriv, tokval);
        Expr f;
        if (!expr1(&f))
            return false;

        e->Calc(yasm::Op::OR, f);
    }
    return true;
}

static bool expr1(Expr* e)
{
    if (!expr2(e))
        return false;

    while (i == '^') {
        i = scan(scpriv, tokval);
        Expr f;
        if (!expr2(&f))
            return false;

        e->Calc(yasm::Op::XOR, f);
    }
    return true;
}

static bool expr2(Expr* e)
{
    if (!expr3(e))
        return false;

    while (i == '&') {
        i = scan(scpriv, tokval);
        Expr f;
        if (!expr3(&f))
            return false;

        e->Calc(yasm::Op::AND, f);
    }
    return true;
}

static bool expr3(Expr* e)
{
    if (!expr4(e))
        return false;

    while (i == TOKEN_SHL || i == TOKEN_SHR)
    {
        int j = i;
        i = scan(scpriv, tokval);
        Expr f;
        if (!expr4(&f))
            return false;

        switch (j) {
            case TOKEN_SHL:
                e->Calc(yasm::Op::SHL, f);
                break;
            case TOKEN_SHR:
                e->Calc(yasm::Op::SHR, f);
                break;
        }
    }
    return true;
}

static bool expr4(Expr* e)
{
    if (!expr5(e))
        return false;
    while (i == '+' || i == '-')
    {
        int j = i;
        i = scan(scpriv, tokval);
        Expr f;
        if (!expr5(&f))
            return false;
        switch (j) {
          case '+':
            e->Calc(yasm::Op::ADD, f);
            break;
          case '-':
            e->Calc(yasm::Op::SUB, f);
            break;
        }
    }
    return true;
}

static bool expr5(Expr* e)
{
    if (!expr6(e))
        return false;
    while (i == '*' || i == '/' || i == '%' ||
           i == TOKEN_SDIV || i == TOKEN_SMOD)
    {
        int j = i;
        i = scan(scpriv, tokval);
        Expr f;
        if (!expr6(&f))
            return false;
        switch (j) {
          case '*':
            e->Calc(yasm::Op::MUL, f);
            break;
          case '/':
            e->Calc(yasm::Op::DIV, f);
            break;
          case '%':
            e->Calc(yasm::Op::MOD, f);
            break;
          case TOKEN_SDIV:
            e->Calc(yasm::Op::SIGNDIV, f);
            break;
          case TOKEN_SMOD:
            e->Calc(yasm::Op::SIGNMOD, f);
            break;
        }
    }
    return true;
}

static bool expr6(Expr* e)
{
    if (i == '-') {
        i = scan(scpriv, tokval);
        if (!expr6(e))
            return false;
        e->Calc(yasm::Op::NEG);
        return true;
    } else if (i == '+') {
        i = scan(scpriv, tokval);
        return expr6(e);
    } else if (i == '~') {
        i = scan(scpriv, tokval);
        if (!expr6(e))
            return false;
        e->Calc(yasm::Op::NOT);
        return true;
    } else if (i == TOKEN_SEG) {
        i = scan(scpriv, tokval);
        if (!expr6(e))
            return false;
        error(ERR_NONFATAL, "%s not supported", "SEG");
        return true;
    } else if (i == '(') {
        i = scan(scpriv, tokval);
        if (!bexpr(e))
            return false;
        if (i != ')') {
            error(ERR_NONFATAL, "expecting `)'");
            return false;
        }
        i = scan(scpriv, tokval);
        return true;
    }
    else if (i == TOKEN_NUM || i == TOKEN_ID ||
             i == TOKEN_HERE || i == TOKEN_BASE)
    {
        switch (i) {
          case TOKEN_NUM:
            *e = Expr(*tokval->t_integer);
            break;
          case TOKEN_ID:
            if (yasm_object) {
                yasm::SymbolRef sym = yasm_object->getSymbol(tokval->t_charptr);
                if (sym) {
                    sym->Use(yasm::SourceLocation());
                    *e = Expr(sym);
                } else {
                    error(ERR_NONFATAL,
                          "undefined symbol `%s' in preprocessor",
                          tokval->t_charptr);
                    *e = Expr(IntNum(1));
                }
                break;
            }
            /*fallthrough*/
          case TOKEN_HERE:
          case TOKEN_BASE:
            error(ERR_NONFATAL,
                  "cannot reference symbol `%s' in preprocessor",
                  (i == TOKEN_ID ? tokval->t_charptr :
                   i == TOKEN_HERE ? "$" : "$$"));
            *e = Expr(IntNum(1));
            break;
        }
        i = scan(scpriv, tokval);
        return true;
    } else {
        error(ERR_NONFATAL, "expression syntax error");
        return false;
    }
}

Expr *nasm_evaluate (scanner sc, void *scprivate, struct tokenval *tv,
                          int critical, efunc report_error)
{
    if (critical & CRITICAL) {
        critical &= ~CRITICAL;
        bexpr = rexp0;
    } else
        bexpr = expr0;

    scan = sc;
    scpriv = scprivate;
    tokval = tv;
    error = report_error;

    if (tokval->t_type == TOKEN_INVALID)
        i = scan(scpriv, tokval);
    else
        i = tokval->t_type;

    Expr* e = new Expr;
    if (bexpr(e))
        return e;
    delete e;
    return NULL;
}

} // namespace nasm

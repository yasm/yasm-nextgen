//
// NASM-compatible re2c lexer
//
//  Copyright (C) 2001-2007  Peter Johnson
//
//  Portions based on re2c's example code.
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

#include <yasmx/Support/Compose.h>
#include <yasmx/Support/errwarn.h>
#include <yasmx/Support/nocase.h>
#include <yasmx/Arch.h>
#include <yasmx/Errwarns.h>
#include <yasmx/Object.h>

#include "modules/parsers/nasm/NasmParser.h"


namespace yasm
{
namespace parser
{
namespace nasm
{

#define BSIZE   8192

#define YYCURSOR        cursor
#define YYLIMIT         (m_lim)
#define YYMARKER        (m_ptr)
#define YYFILL(n)       {}

#define RETURN(i)       {m_cur = cursor; m_tokch = *TOK; return i;}

#define SCANINIT()      {m_tok = cursor;}

#define TOK             (m_tok)
#define TOKLEN          (cursor-TOK)


// starting size of string buffer
#define STRBUF_ALLOC_SIZE       128

static int linechg_numcount;

/*!re2c
  any = [\001-\377];
  digit = [0-9];
  iletter = [a-zA-Z];
  bindigit = [01_];
  octdigit = [0-7_];
  hexdigit = [0-9a-fA-F_];
  ws = [ \t\r];
  quot = ["'];
*/

int
NasmParser::handle_dot_label(YYSTYPE* lvalp, YYCTYPE* tok, size_t toklen,
                             size_t zeropos)
{
    /* check for special non-local labels like ..start */
    if (tok[zeropos+1] == '.')
    {
        lvalp->str.assign(tok+zeropos, toklen-zeropos);
        /* check for special non-local ..@label */
        if (lvalp->str[zeropos+2] == '@')
            return NONLOCAL_ID;
        return SPECIAL_ID;
    }

    if (m_locallabel_base.empty())
    {
        lvalp->str.assign(tok+zeropos, toklen-zeropos);
        warn_set(WARN_GENERAL, String::compose(
            N_("no non-local label before `%1'"), lvalp->str));
    }
    else
    {
        lvalp->str = m_locallabel_base;
        lvalp->str.append(tok+zeropos, toklen-zeropos);
    }

    return LOCAL_ID;
}

int
NasmParser::lex(YYSTYPE* lvalp)
{
    YYCTYPE* cursor = m_cur;
    YYCTYPE endch;
    YYCTYPE savech;

    // Handle one token of lookahead
    if (m_peek_token != NONE)
    {
        int tok = m_peek_token;
        *lvalp = m_peek_tokval;     // structure copy
        m_tokch = m_peek_tokch;
        m_peek_token = NONE;
        return tok;
    }

    // Catch EOL (EOF from the scanner perspective)
    if (cursor == m_lim)
        return 0;

    // Jump to proper "exclusive" states
    switch (m_state)
    {
        case DIRECTIVE:
            goto directive;
        case SECTION_DIRECTIVE:
            goto section_directive;
        case DIRECTIVE2:
            goto directive2;
        case LINECHG:
            goto linechg;
        case LINECHG2:
            goto linechg2;
        default:
            break;
    }

scan:
    SCANINIT();
    if (*cursor == '\0')
        goto endofinput;

    /*!re2c
        /* standard decimal integer */
        digit+
        {
            lvalp->intn.reset(new IntNum);
            savech = TOK[TOKLEN];
            TOK[TOKLEN] = '\0';
            bool ok = lvalp->intn->set_str(TOK, 10);
            ok = ok; // silence warning
            assert(ok && "conversion failed");
            TOK[TOKLEN] = savech;
            RETURN(INTNUM);
        }
        /* 10010011b - binary number */

        [01] bindigit* 'b'
        {
            lvalp->intn.reset(new IntNum);
            TOK[TOKLEN-1] = '\0'; /* strip off 'b' */
            bool ok = lvalp->intn->set_str(TOK, 2);
            ok = ok; // silence warning
            assert(ok && "conversion failed");
            RETURN(INTNUM);
        }

        /* 777q or 777o - octal number */
        [0-7] octdigit* [qQoO]
        {
            lvalp->intn.reset(new IntNum);
            TOK[TOKLEN-1] = '\0'; /* strip off 'q' or 'o' */
            bool ok = lvalp->intn->set_str(TOK, 8);
            ok = ok; // silence warning
            assert(ok && "conversion failed");
            RETURN(INTNUM);
        }

        /* 0AAh form of hexidecimal number */
        digit hexdigit* 'h'
        {
            lvalp->intn.reset(new IntNum);
            TOK[TOKLEN-1] = '\0'; /* strip off 'h' */
            bool ok = lvalp->intn->set_str(TOK, 16);
            ok = ok; // silence warning
            assert(ok && "conversion failed");
            RETURN(INTNUM);
        }

        /* $0AA and 0xAA forms of hexidecimal number */
        (("$" digit) | '0x') hexdigit+
        {
            lvalp->intn.reset(new IntNum);
            savech = TOK[TOKLEN];
            TOK[TOKLEN] = '\0';
            bool ok;
            if (TOK[1] == 'x' || TOK[1] == 'X')
                ok = lvalp->intn->set_str(TOK+2, 16);   // skip 0 and x
            else
                ok = lvalp->intn->set_str(TOK+1, 16);   // don't skip 0
            ok = ok; // silence warning
            assert(ok && "conversion failed");
            TOK[TOKLEN] = savech;
            RETURN(INTNUM);
        }

        /* floating point value */
        digit+ "." digit* ('e' [-+]? digit+)?
        {
            savech = TOK[TOKLEN];
            TOK[TOKLEN] = '\0';
            // FIXME: Make arch-dependent
            lvalp->flt.reset(new llvm::APFloat(llvm::APFloat::x87DoubleExtended,
                                               TOK));
            TOK[TOKLEN] = savech;
            RETURN(FLTNUM);
        }

        /* string/character constant values */
        quot
        {
            endch = TOK[0];
            goto stringconst;
        }

        /* %line linenum+lineinc filename */
        "%line"
        {
            m_state = LINECHG;
            linechg_numcount = 0;
            RETURN(LINE);
        }

        /* size specifiers */
        'byte'
        {
            lvalp->int_info = 8;
            RETURN(SIZE_OVERRIDE);
        }
        'hword'
        {
            lvalp->int_info = m_wordsize/2;
            RETURN(SIZE_OVERRIDE);
        }
        'word'
        {
            lvalp->int_info = m_wordsize;
            RETURN(SIZE_OVERRIDE);
        }
        'dword' | 'long'
        {
            lvalp->int_info = m_wordsize*2;
            RETURN(SIZE_OVERRIDE);
        }
        'qword'
        {
            lvalp->int_info = m_wordsize*4;
            RETURN(SIZE_OVERRIDE);
        }
        'tword'
        {
            lvalp->int_info = 80;
            RETURN(SIZE_OVERRIDE);
        }
        'dqword'
        {
            lvalp->int_info = m_wordsize*8;
            RETURN(SIZE_OVERRIDE);
        }
        'oword'
        {
            lvalp->int_info = m_wordsize*8;
            RETURN(SIZE_OVERRIDE);
        }
        'yword'
        {
            lvalp->int_info = 256;
            RETURN(SIZE_OVERRIDE);
        }

        /* pseudo-instructions */
        'db'
        {
            lvalp->int_info = 8;
            RETURN(DECLARE_DATA);
        }
        'dhw'
        {
            lvalp->int_info = m_wordsize/2;
            RETURN(DECLARE_DATA);
        }
        'dw'
        {
            lvalp->int_info = m_wordsize;
            RETURN(DECLARE_DATA);
        }
        'dd'
        {
            lvalp->int_info = m_wordsize*2;
            RETURN(DECLARE_DATA);
        }
        'dq'
        {
            lvalp->int_info = m_wordsize*4;
            RETURN(DECLARE_DATA);
        }
        'dt'
        {
            lvalp->int_info = 80;
            RETURN(DECLARE_DATA);
        }
        'ddq'
        {
            lvalp->int_info = m_wordsize*8;
            RETURN(DECLARE_DATA);
        }
        'do'
        {
            lvalp->int_info = m_wordsize*8;
            RETURN(DECLARE_DATA);
        }
        'dy'
        {
            lvalp->int_info = 256;
            RETURN(DECLARE_DATA);
        }

        'resb'
        {
            lvalp->int_info = 8;
            RETURN(RESERVE_SPACE);
        }
        'reshw'
        {
            lvalp->int_info = m_wordsize/2;
            RETURN(RESERVE_SPACE);
        }
        'resw'
        {
            lvalp->int_info = m_wordsize;
            RETURN(RESERVE_SPACE);
        }
        'resd'
        {
            lvalp->int_info = m_wordsize*2;
            RETURN(RESERVE_SPACE);
        }
        'resq'
        {
            lvalp->int_info = m_wordsize*4;
            RETURN(RESERVE_SPACE);
        }
        'rest'
        {
            lvalp->int_info = 80;
            RETURN(RESERVE_SPACE);
        }
        'resdq'
        {
            lvalp->int_info = m_wordsize*8;
            RETURN(RESERVE_SPACE);
        }
        'reso'
        {
            lvalp->int_info = m_wordsize*8;
            RETURN(RESERVE_SPACE);
        }
        'resy'
        {
            lvalp->int_info = 256;
            RETURN(RESERVE_SPACE);
        }

        'incbin'        { RETURN(INCBIN); }

        'equ'           { RETURN(EQU); }

        'times'         { RETURN(TIMES); }

        'seg'           { RETURN(SEG); }
        'wrt'           { RETURN(WRT); }

        'abs'           { RETURN(ABS); }
        'rel'           { RETURN(REL); }

        'nosplit'       { RETURN(NOSPLIT); }
        'strict'        { RETURN(STRICT); }

        /* operators */
        "<<"                    { RETURN(LEFT_OP); }
        ">>"                    { RETURN(RIGHT_OP); }
        "//"                    { RETURN(SIGNDIV); }
        "%%"                    { RETURN(SIGNMOD); }
        "$$"                    { RETURN(START_SECTION_ID); }
        [-+|^*&/%~$():=,\[]     { RETURN(TOK[0]); }
        "]"                     { RETURN(TOK[0]); }

        /* local label (.label) */
        "." [a-zA-Z0-9_$#@~.?]+
        {
            RETURN(handle_dot_label(lvalp, TOK, TOKLEN, 0));
        }

        /* forced identifier */
        "$" [a-zA-Z0-9_$#@~.?]+
        {
            if (TOK[1] == '.')
            {
                /* handle like .label */
                RETURN(handle_dot_label(lvalp, TOK, TOKLEN, 1));
            }
            lvalp->str.assign(TOK+1, TOKLEN-1);
            RETURN(ID);
        }

        /* identifier that may be a register, instruction, etc. */
        [a-zA-Z_?@][a-zA-Z0-9_$#@~.?]*
        {
            savech = TOK[TOKLEN];
            TOK[TOKLEN] = '\0';
            if (m_state != INSTRUCTION)
            {
                Arch::InsnPrefix ip =
                    m_arch->parse_check_insnprefix(TOK, TOKLEN, get_cur_line());
                TOK[TOKLEN] = savech;
                switch (ip.get_type())
                {
                    case Arch::InsnPrefix::INSN:
                        lvalp->insn = ip.release_insn();
                        m_state = INSTRUCTION;
                        RETURN(INSN);
                    case Arch::InsnPrefix::PREFIX:
                        lvalp->prefix = ip.get_prefix();
                        RETURN(PREFIX);
                    default:
                        break;
                }
            }
            Arch::RegTmod regtmod = m_arch->parse_check_regtmod(TOK, TOKLEN);
            TOK[TOKLEN] = savech;
            switch (regtmod.get_type())
            {
                case Arch::RegTmod::REG:
                    lvalp->reg = regtmod.get_reg();
                    RETURN(REG);
                case Arch::RegTmod::SEGREG:
                    lvalp->segreg = regtmod.get_segreg();
                    RETURN(SEGREG);
                case Arch::RegTmod::TARGETMOD:
                    lvalp->targetmod = regtmod.get_tmod();
                    RETURN(TARGETMOD);
                default:
                    break;
            }
            /* Propagate errors in case we got a warning from the arch */
            m_errwarns->propagate(get_cur_line());
            /* Just an identifier, return as such. */
            lvalp->str.assign(TOK, TOKLEN);
            RETURN(ID);
        }

        ";" (any \ [\000])*     { goto scan; }

        ws+                     { goto scan; }

        [\000]                  { goto endofinput; }

        any
        {
            warn_set(WARN_UNREC_CHAR, String::compose(
                N_("ignoring unrecognized character `%1'"),
                conv_unprint(TOK[0])));
            goto scan;
        }
    */

    // %line linenum+lineinc filename
linechg:
    SCANINIT();
    if (*cursor == '\0')
        goto endofinput;

    /*!re2c
        digit+
        {
            linechg_numcount++;
            lvalp->intn.reset(new IntNum);
            savech = TOK[TOKLEN];
            TOK[TOKLEN] = '\0';
            bool ok = lvalp->intn->set_str(TOK, 10);
            ok = ok; // silence warning
            assert(ok && "conversion failed");
            TOK[TOKLEN] = savech;
            RETURN(INTNUM);
        }

        [\000]
        {
            goto endofinput;
        }

        "+"
        {
            RETURN(TOK[0]);
        }

        ws+
        {
            if (linechg_numcount == 2)
            {
                m_state = LINECHG2;
                goto linechg2;
            }
            goto linechg;
        }

        any
        {
            warn_set(WARN_UNREC_CHAR, String::compose(
                N_("ignoring unrecognized character `%1'"),
                conv_unprint(TOK[0])));
            goto linechg;
        }
    */

linechg2:
    SCANINIT();
    if (*cursor == '\0')
        goto endofinput;

    /*!re2c
        [\000]  { goto endofinput; }

        "\r"    { goto linechg2; }

        (any \ [\000])+
        {
            m_state = LINECHG;
            lvalp->str.assign(TOK, TOKLEN);
            RETURN(FILENAME);
        }
    */

    // directive: [name value]
directive:
    SCANINIT();
    if (*cursor == '\0')
        goto endofinput;

    /*!re2c
        [\]\000]    { goto endofinput; }

        [a-zA-Z_][a-zA-Z_0-9]*
        {
            lvalp->str.assign(TOK, TOKLEN);
            if (String::nocase_equal(lvalp->str, "section") ||
                String::nocase_equal(lvalp->str, "segment"))
                m_state = SECTION_DIRECTIVE;
            else
                m_state = DIRECTIVE2;
            RETURN(DIRECTIVE_NAME);
        }

        any
        {
            warn_set(WARN_UNREC_CHAR, String::compose(
                N_("ignoring unrecognized character `%1'"),
                conv_unprint(TOK[0])));
            goto directive;
        }
    */

    // section directive (the section name portion thereof)
section_directive:
    SCANINIT();
    if (*cursor == '\0')
        goto endofinput;

    /*!re2c
        [a-zA-Z0-9_$#@~.?-]+
        {
            lvalp->str.assign(TOK, TOKLEN);
            m_state = DIRECTIVE2;
            RETURN(STRING);
        }

        quot
        {
            m_state = DIRECTIVE2;
            endch = TOK[0];
            goto stringconst;
        }

        ws+
        {
            m_state = DIRECTIVE2;
            goto section_directive;
        }

        [\]\000]    { goto endofinput; }

        any
        {
            warn_set(WARN_UNREC_CHAR, String::compose(
                N_("ignoring unrecognized character `%1'"),
                conv_unprint(TOK[0])));
            goto section_directive;
        }
    */

    // inner part of directive
directive2:
    SCANINIT();
    if (*cursor == '\0')
        goto endofinput;

    /*!re2c
        /* standard decimal integer */
        digit+
        {
            lvalp->intn.reset(new IntNum);
            savech = TOK[TOKLEN];
            TOK[TOKLEN] = '\0';
            bool ok = lvalp->intn->set_str(TOK, 10);
            ok = ok; // silence warning
            assert(ok && "conversion failed");
            TOK[TOKLEN] = savech;
            RETURN(INTNUM);
        }
        /* 10010011b - binary number */

        [01] bindigit* 'b'
        {
            lvalp->intn.reset(new IntNum);
            TOK[TOKLEN-1] = '\0'; /* strip off 'b' */
            bool ok = lvalp->intn->set_str(TOK, 2);
            ok = ok; // silence warning
            assert(ok && "conversion failed");
            RETURN(INTNUM);
        }

        /* 777q or 777o - octal number */
        [0-7] octdigit* [qQoO]
        {
            lvalp->intn.reset(new IntNum);
            TOK[TOKLEN-1] = '\0'; /* strip off 'q' or 'o' */
            bool ok = lvalp->intn->set_str(TOK, 8);
            ok = ok; // silence warning
            assert(ok && "conversion failed");
            RETURN(INTNUM);
        }

        /* 0AAh form of hexidecimal number */
        digit hexdigit* 'h'
        {
            lvalp->intn.reset(new IntNum);
            TOK[TOKLEN-1] = '\0'; /* strip off 'h' */
            bool ok = lvalp->intn->set_str(TOK, 16);
            ok = ok; // silence warning
            assert(ok && "conversion failed");
            RETURN(INTNUM);
        }

        /* $0AA and 0xAA forms of hexidecimal number */
        (("$" digit) | '0x') hexdigit+
        {
            lvalp->intn.reset(new IntNum);
            savech = TOK[TOKLEN];
            TOK[TOKLEN] = '\0';
            bool ok;
            if (TOK[1] == 'x' || TOK[1] == 'X')
                ok = lvalp->intn->set_str(TOK+2, 16);   // skip 0 and x
            else
                ok = lvalp->intn->set_str(TOK+1, 16);   // don't skip 0
            assert(ok && "conversion failed");
            TOK[TOKLEN] = savech;
            RETURN(INTNUM);
        }

        /* string/character constant values */
        quot
        {
            endch = TOK[0];
            goto stringconst;
        }

        /* operators */
        "<<"                    { RETURN(LEFT_OP); }
        ">>"                    { RETURN(RIGHT_OP); }
        "//"                    { RETURN(SIGNDIV); }
        "%%"                    { RETURN(SIGNMOD); }
        [-+|^*&/%~$():=,\[]     { RETURN(TOK[0]); }

        /* handle ] for directives */
        "]"                     { goto endofinput; }

        /* forced identifier; within directive, don't strip '$', this is
         * handled later.
         */
        "$" [a-zA-Z0-9_$#@~.?]+
        {
            lvalp->str.assign(TOK, TOKLEN);
            RETURN(ID);
        }

        /* identifier; within directive, no local label mechanism */
        [a-zA-Z_.?][a-zA-Z0-9_$#@~.?]*
        {
            savech = TOK[TOKLEN];
            TOK[TOKLEN] = '\0';
            Arch::RegTmod regtmod = m_arch->parse_check_regtmod(TOK, TOKLEN);
            TOK[TOKLEN] = savech;
            lvalp->reg = regtmod.get_reg();
            if (lvalp->reg)
                RETURN(REG);
            // Propagate errors in case we got a warning from the arch
            m_errwarns->propagate(get_cur_line());
            /* Just an identifier, return as such. */
            lvalp->str.assign(TOK, TOKLEN);
            RETURN(ID);
        }

        ";" (any \ [\000])*     { goto directive2; }

        ws+                     { goto directive2; }

        [\000]                  { goto endofinput; }

        any
        {
            warn_set(WARN_UNREC_CHAR, String::compose(
                N_("ignoring unrecognized character `%1'"),
                conv_unprint(TOK[0])));
            goto scan;
        }
     */

    // string/character constant values
stringconst:
    lvalp->str.clear();

stringconst_scan:
    SCANINIT();
    if (*cursor == '\0')
        throw SyntaxError(N_("unterminated string"));

    /*!re2c
        [\000]
        {
            throw SyntaxError(N_("unterminated string"));
        }

        any
        {
            if (TOK[0] == endch)
                RETURN(STRING);
            lvalp->str += TOK[0];
            goto stringconst_scan;
        }
    */

endofinput:
    m_state = INITIAL;
    RETURN(TOK[0]);
}

}}} // namespace yasm::parser::nasm

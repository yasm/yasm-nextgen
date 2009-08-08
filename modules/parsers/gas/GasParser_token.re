//
// GAS-compatible re2c lexer
//
//  Copyright (C) 2005-2007  Peter Johnson
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the author nor the names of other contributors
//    may be used to endorse or promote products derived from this
//    software without specific prior written permission.
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

#include <cstring>

#include <yasmx/Support/Compose.h>
#include <yasmx/Support/errwarn.h>
#include <yasmx/Support/StringExtras.h>
#include <yasmx/Arch.h>
#include <yasmx/Object.h>

#include "modules/parsers/gas/GasParser.h"


namespace yasm
{
namespace parser
{
namespace gas
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

#if 0
static size_t
rept_input(yasm_parser_gas *parser_gas, /*@out@*/ YYCTYPE *buf,
           size_t max_size)
{
    gas_rept *rept = parser_gas->rept;
    size_t numleft = max_size;
    YYCTYPE *bufp = buf;

    /* If numrept is 0, copy out just the line end characters */
    if (rept->numrept == 0) {
        /* Skip first line, which contains .line */
        rept->line = STAILQ_NEXT(rept->line, link);
        if (!rept->line) {
            rept->numrept = 1;
            rept->numdone = 1;
        }
        while (rept->numrept == 0 && numleft > 0) {
            *bufp++ = rept->line->data[rept->line->len-1];
            rept->line = STAILQ_NEXT(rept->line, link);
            if (!rept->line) {
                rept->numrept = 1;
                rept->numdone = 1;
            }
        }
    }

    /* Copy out the previous fill buffer until we're *really* done */
    if (rept->numdone == rept->numrept) {
        size_t numcopy = rept->oldbuflen - rept->oldbufpos;
        if (numcopy > numleft)
            numcopy = numleft;
        memcpy(bufp, &rept->oldbuf[rept->oldbufpos], numcopy);
        numleft -= numcopy;
        bufp += numcopy;
        rept->oldbufpos += numcopy;

        if (rept->oldbufpos == rept->oldbuflen) {
            /* Delete lines, then delete rept and clear rept state */
            gas_rept_line *cur, *next;
            cur = STAILQ_FIRST(&rept->lines);
            while (cur) {
                next = STAILQ_NEXT(cur, link);
                yasm_xfree(cur->data);
                yasm_xfree(cur);
                cur = next;
            }
            yasm_xfree(rept->oldbuf);
            yasm_xfree(rept);
            parser_gas->rept = NULL;
        }
    }

    while (numleft > 0 && rept->numdone < rept->numrept) {
        /* Copy from line data to buf */
        size_t numcopy = rept->line->len - rept->linepos;
        if (numcopy > numleft)
            numcopy = numleft;
        memcpy(bufp, &rept->line->data[rept->linepos], numcopy);
        numleft -= numcopy;
        bufp += numcopy;
        rept->linepos += numcopy;

        /* Update locations if needed */
        if (rept->linepos == rept->line->len) {
            rept->line = STAILQ_NEXT(rept->line, link);
            rept->linepos = 0;
        }
        if (rept->line == NULL) {
            rept->numdone++;
            rept->line = STAILQ_FIRST(&rept->lines);
        }
    }

    return (max_size-numleft);
}

/* Bridge function to convert byte-oriented parser with line-oriented
 * preprocessor.
 */
static size_t
preproc_input(yasm_parser_gas *parser_gas, /*@out@*/ YYCTYPE *buf,
              size_t max_size)
{
    size_t tot=0;
    while (max_size > 0) {
        size_t n;

        if (!parser_gas->line) {
            parser_gas->line = yasm_preproc_get_line(parser_gas->preproc);
            if (!parser_gas->line)
                return tot; /* EOF */
            parser_gas->linepos = parser_gas->line;
            parser_gas->lineleft = strlen(parser_gas->line) + 1;
            parser_gas->line[parser_gas->lineleft-1] = '\n';
        }

        n = parser_gas->lineleft<max_size ? parser_gas->lineleft : max_size;
        strncpy((char *)buf+tot, parser_gas->linepos, n);

        if (n == parser_gas->lineleft) {
            yasm_xfree(parser_gas->line);
            parser_gas->line = NULL;
        } else {
            parser_gas->lineleft -= n;
            parser_gas->linepos += n;
        }

        tot += n;
        max_size -= n;
    }
    return tot;
}
#if 0
static size_t
fill_input(void *d, unsigned char *buf, size_t max)
{
    return yasm_preproc_input((yasm_preproc *)d, (char *)buf, max);
}
#endif
static YYCTYPE *
fill(yasm_parser_gas *parser_gas, YYCTYPE *cursor)
{
    yasm_scanner *s = &parser_gas->s;
    int first = 0;
    if(!s->eof){
        size_t cnt = s->tok - s->bot;
        if(cnt){
            memmove(s->bot, s->tok, (size_t)(s->lim - s->tok));
            s->tok = s->bot;
            s->ptr -= cnt;
            cursor -= cnt;
            s->lim -= cnt;
        }
        if (!s->bot)
            first = 1;
        if((s->top - s->lim) < BSIZE){
            YYCTYPE *buf = yasm_xmalloc((size_t)(s->lim - s->bot) + BSIZE);
            memcpy(buf, s->tok, (size_t)(s->lim - s->tok));
            s->tok = buf;
            s->ptr = &buf[s->ptr - s->bot];
            cursor = &buf[cursor - s->bot];
            s->lim = &buf[s->lim - s->bot];
            s->top = &s->lim[BSIZE];
            if (s->bot)
                yasm_xfree(s->bot);
            s->bot = buf;
        }
        if (parser_gas->rept && parser_gas->rept->ended) {
            /* Pull from rept lines instead of preproc */
            cnt = rept_input(parser_gas, s->lim, BSIZE);
        } else if((cnt = preproc_input(parser_gas, s->lim, BSIZE)) == 0) {
            s->eof = &s->lim[cnt]; *s->eof++ = '\n';
        }
        s->lim += cnt;
        if (first && parser_gas->save_input) {
            int i;
            YYCTYPE *saveline;
            parser_gas->save_last ^= 1;
            saveline = parser_gas->save_line[parser_gas->save_last];
            /* save next line into cur_line */
            for (i=0; i<79 && &s->tok[i] < s->lim && s->tok[i] != '\n'; i++)
                saveline[i] = s->tok[i];
            saveline[i] = '\0';
        }
    }
    return cursor;
}

static YYCTYPE *
save_line(yasm_parser_gas *parser_gas, YYCTYPE *cursor)
{
    yasm_scanner *s = &parser_gas->s;
    int i = 0;
    YYCTYPE *saveline;

    parser_gas->save_last ^= 1;
    saveline = parser_gas->save_line[parser_gas->save_last];

    /* save next line into cur_line */
    if ((YYLIMIT - YYCURSOR) < 80)
        YYFILL(80);
    for (i=0; i<79 && &cursor[i] < s->lim && cursor[i] != '\n'; i++)
        saveline[i] = cursor[i];
    saveline[i] = '\0';
    return cursor;
}
#endif

/*!re2c
  any = [\000-\377];
  digit = [0-9];
  iletter = [a-zA-Z];
  bindigit = [01];
  octdigit = [0-7];
  hexdigit = [0-9a-fA-F];
  ws = [ \t\r];
  dquot = ["];
*/


int
GasParser::Lex(YYSTYPE* lvalp)
{
#if 0
    /*@null@*/ gas_rept *rept = parser_gas->rept;
    gas_rept_line *new_line;
    size_t count;
    int linestart;
#endif
    YYCTYPE* cursor = m_cur;
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

#if 0
    // Handle rept
    if (rept && !rept->ended)
        goto rept_directive;
#endif

    // Jump to proper "exclusive" states
    switch (m_state)
    {
        case COMMENT:
            goto comment;
        case SECTION_DIRECTIVE:
            goto section_directive;
        case NASM_FILENAME:
            goto nasm_filename;
        default:
            break;
    }

scan:
    SCANINIT();
    if (*cursor == '\0')
        goto endofinput;

    /*!re2c
        /* standard decimal integer */
        ([1-9] digit*) | "0"
        {
            lvalp->intn.reset(new IntNum);
            lvalp->intn->setStr(TOK, TOKLEN, 10);
            RETURN(INTNUM);
        }

        /* 0b10010011 - binary number */
        '0b' bindigit+
        {
            lvalp->intn.reset(new IntNum);
            lvalp->intn->setStr(TOK+2, TOKLEN-2, 2); // skip "0b"
            RETURN(INTNUM);
        }

        /* 0777 - octal number */
        "0" octdigit+
        {
            lvalp->intn.reset(new IntNum);
            lvalp->intn->setStr(TOK, TOKLEN, 8);
            RETURN(INTNUM);
        }

        /* 0xAA - hexidecimal number */
        '0x' hexdigit+
        {
            lvalp->intn.reset(new IntNum);
            lvalp->intn->setStr(TOK+2, TOKLEN-2, 16); // skip "0x"
            RETURN(INTNUM);
        }

        /* floating point value */
        [-+]? digit* "." digit+ ('e' [-+]? digit+)?
        {
            savech = TOK[TOKLEN];
            TOK[TOKLEN] = '\0';
            // FIXME: Make arch-dependent
            lvalp->flt.reset(new llvm::APFloat(llvm::APFloat::x87DoubleExtended,
                                               TOK));
            TOK[TOKLEN] = savech;
            RETURN(FLTNUM);
        }
        [-+]? digit+ "." digit* ('e' [-+]? digit+)?
        {
            savech = TOK[TOKLEN];
            TOK[TOKLEN] = '\0';
            // FIXME: Make arch-dependent
            lvalp->flt.reset(new llvm::APFloat(llvm::APFloat::x87DoubleExtended,
                                               TOK));
            TOK[TOKLEN] = savech;
            RETURN(FLTNUM);
        }
        "0" [DdEeFfTt] [-+]? digit* ("." digit*)? ('e' [-+]? digit+)?
        {
            savech = TOK[TOKLEN];
            TOK[TOKLEN] = '\0';
            // FIXME: Make arch-dependent
            lvalp->flt.reset(new llvm::APFloat(llvm::APFloat::x87DoubleExtended,
                                               TOK+2));
            TOK[TOKLEN] = savech;
            RETURN(FLTNUM);
        }

        /* character constant values */
        [']
        {
            goto charconst;
        }

        /* string constant values */
        dquot
        {
            goto stringconst;
        }

        /* operators */
        "<<"                    { RETURN(LEFT_OP); }
        ">>"                    { RETURN(RIGHT_OP); }
        "<"                     { RETURN(LEFT_OP); }
        ">"                     { RETURN(RIGHT_OP); }
        [-+|^!*&/~$():@=,]      { RETURN(TOK[0]); }
        ";"
        {
            m_state = INITIAL;
            RETURN(TOK[0]);
        }

        /* identifier */
        [a-zA-Z_.][a-zA-Z0-9_$.]*
        {
            lvalp->str.assign(TOK, TOKLEN);
            RETURN(ID);
        }

        /* identifier with @ */
        [a-zA-Z_.]([a-zA-Z0-9_$.]*[@][a-zA-Z0-9_$.]*)+
        {
            // if @ not part of ID, move the scanner cursor to the first @
#if 0
            if (m_object->id_at_ok)
                cursor = std::strchr(TOK, '@');
#endif
            lvalp->str.assign(TOK, TOKLEN);
            RETURN(ID);
        }

        /* register or segment register */
        [%][a-zA-Z0-9]+
        {
            if (m_is_nasm_preproc && TOKLEN == 5 &&
                std::strncmp(TOK+1, "line", 4) == 0)
                RETURN(NASM_LINE_MARKER);

            Arch::RegTmod regtmod =
                m_arch->ParseCheckRegTmod(TOK+1, TOKLEN-1);
            switch (regtmod.getType())
            {
                case Arch::RegTmod::REG:
                    lvalp->reg = regtmod.getReg();
                    RETURN(REG);
                case Arch::RegTmod::REGGROUP:
                    lvalp->reggroup = regtmod.getRegGroup();
                    RETURN(REGGROUP);
                case Arch::RegTmod::SEGREG:
                    lvalp->segreg = regtmod.getSegReg();
                    RETURN(SEGREG);
                default:
                    break;
            }
            throw Error(String::Compose(N_("Unrecognized register name `%1'"),
                                        std::string(TOK+1, TOKLEN-1)));
        }

        /* local label */
        [0-9] ':'
        {
            // increment label index
            m_local[TOK[0]-'0']++;
            // build local label name
            char labelname[30];
            std::sprintf(labelname, "L%c\001%lu", TOK[0], m_local[TOK[0]-'0']);
            lvalp->str.assign(labelname);
            RETURN(LABEL);
        }

        /* local label forward reference */
        [0-9] 'f'
        {
            // build local label name
            char labelname[30];
            std::sprintf(labelname, "L%c\001%lu", TOK[0],
                         m_local[TOK[0]-'0']+1);
            lvalp->str.assign(labelname);
            RETURN(ID);
        }

        /* local label backward reference */
        [0-9] 'b'
        {
            // build local label name
            char labelname[30];
            std::sprintf(labelname, "L%c\001%lu", TOK[0], m_local[TOK[0]-'0']);
            lvalp->str.assign(labelname);
            RETURN(ID);
        }

        "/*"                    { m_state = COMMENT; goto comment; }
        "#"
        {
            if (m_is_cpp_preproc)
            {
                RETURN(CPP_LINE_MARKER);
            }
            else
                goto line_comment;
        }

        ws+                     { goto scan; }

        "\n"
        {
            m_state = INITIAL;
            RETURN(TOK[0]);
        }

        any
        {
            setWarn(WARN_UNREC_CHAR, String::Compose(
                N_("ignoring unrecognized character `%1'"),
                ConvUnprint(TOK[0])));
            goto scan;
        }
    */

    // C-style comment; nesting not supported
comment:
    SCANINIT();
    if (*cursor == '\0')
        goto endofinput;

    /*!re2c
        /* End of comment */
        "*/"    { m_state = INITIAL; goto scan; }

        "\n"
        {
            RETURN(TOK[0]);
        }

        [\000]  { goto endofinput; }
        any     { goto comment; }
    */

    // Single line comment.
line_comment:
    SCANINIT();

    /*!re2c
        "\n"                { goto scan; }
        [\000]              { goto endofinput; }
        any                 { goto line_comment; }
    */

    // .section directive (the section name portion thereof)
section_directive:
    SCANINIT();
    if (*cursor == '\0')
        goto endofinput;

    /*!re2c
        [a-zA-Z0-9_$.-]+
        {
            lvalp->str.assign(TOK, TOKLEN);
            m_state = INITIAL;
            RETURN(ID);
        }

        dquot                   { goto stringconst; }

        ws+                     { goto section_directive; }

        ","
        {
            m_state = INITIAL;
            RETURN(TOK[0]);
        }

        "\n"
        {
            m_state = INITIAL;
            RETURN(TOK[0]);
        }

        [\000]  { goto endofinput; }

        any
        {
            setWarn(WARN_UNREC_CHAR, String::Compose(
                N_("ignoring unrecognized character `%1'"),
                ConvUnprint(TOK[0])));
            goto section_directive;
        }
    */

    // filename portion of nasm preproc %line
nasm_filename:
    lvalp->str.clear();

nasm_filename_scan:
    SCANINIT();
    if (*cursor == '\0')
        return STRING;

    /*!re2c
        "\n"    { RETURN(STRING); }
        ws+     { goto nasm_filename_scan; }
        [\000]  { RETURN(STRING); }
        any
        {
            lvalp->str += TOK[0];
            goto nasm_filename_scan;
        }
    */

    // character constant values
charconst:
    //TODO

    // string constant values
stringconst:
    lvalp->str.clear();

stringconst_scan:
    SCANINIT();
    if (*cursor == '\0')
        throw SyntaxError(N_("unexpected end of file in string"));

    /*!re2c
        /* Handle escaped double-quote by copying and continuing */
        "\\\""
        {
            lvalp->str += '"';
            goto stringconst_scan;
        }

        dquot
        {
            if (!Unescape(&lvalp->str))
                setWarn(WARN_GENERAL, N_("octal value out of range"));
            RETURN(STRING);
        }

        [\000]
        {
            throw SyntaxError(N_("unexpected end of file in string"));
        }

        any
        {
            lvalp->str += TOK[0];
            goto stringconst_scan;
        }
    */
#if 0
rept_directive:
    strbuf = yasm_xmalloc(STRBUF_ALLOC_SIZE);
    strbuf_size = STRBUF_ALLOC_SIZE;
    count = 0;
    linestart = 1;


rept_scan:
    SCANINIT();

    /*!re2c
        [\n;]   {
            /* Line ending, save in lines */
            new_line = yasm_xmalloc(sizeof(gas_rept_line));
            if (cursor == s->eof) {
                yasm_xfree(strbuf);
                return 0;
            }
            strbuf_append(count++, cursor, s, s->tok[0]);
            new_line->data = strbuf;
            new_line->len = count;
            STAILQ_INSERT_TAIL(&rept->lines, new_line, link);
            /* Allocate new strbuf */
            strbuf = yasm_xmalloc(STRBUF_ALLOC_SIZE);
            strbuf_size = STRBUF_ALLOC_SIZE;
            count = 0;
            /* Mark start of line */
            linestart = 1;
            goto rept_scan;
        }
        '.rept' {
            int i;
            if (linestart) {
                /* We don't support nested right now, error */
                yasm_error_set(YASM_ERROR_GENERAL,
                               N_("nested rept not supported"));
                yasm_errwarn_propagate(parser_gas->errwarns, cur_line);
            }
            for (i=0; i<6; i++)
                strbuf_append(count++, cursor, s, s->tok[i]);
            goto rept_scan;
        }
        '.endr' {
            if (linestart) {
                /* We're done, kick off the main lexer */
                rept->line = STAILQ_FIRST(&rept->lines);
                if (!rept->line) {
                    /* Didn't get any intervening data?  Empty repeat, so
                     * don't even bother.
                     */
                    yasm_xfree(strbuf);
                    yasm_xfree(rept);
                    parser_gas->rept = NULL;
                } else {
                    rept->ended = 1;

                    /* Add .line as first line to get line numbers correct */
                    new_line = yasm_xmalloc(sizeof(gas_rept_line));
                    new_line->data = yasm_xmalloc(40);
                    sprintf((char *)new_line->data, ".line %lu;",
                            rept->startline+1);
                    new_line->len = strlen((char *)new_line->data);
                    STAILQ_INSERT_HEAD(&rept->lines, new_line, link);

                    /* Save previous fill buffer */
                    rept->oldbuf = parser_gas->s.bot;
                    rept->oldbuflen = s->lim - s->bot;
                    rept->oldbufpos = cursor - s->bot;

                    /* Reset fill */
                    s->bot = NULL;
                    s->tok = NULL;
                    s->ptr = NULL;
                    s->cur = NULL;
                    s->lim = NULL;
                    s->top = NULL;
                    s->eof = NULL;
                    cursor = NULL;
                    YYFILL(1);
                }

                goto scan;
            } else {
                int i;
                for (i=0; i<6; i++)
                    strbuf_append(count++, cursor, s, s->tok[i]);
                goto rept_scan;
            }
        }

        any     {
            if (cursor == s->eof) {
                yasm_xfree(strbuf);
                return 0;
            }
            strbuf_append(count++, cursor, s, s->tok[0]);
            linestart = 0;
            goto rept_scan;
        }
    */
#endif

endofinput:
    m_state = INITIAL;
    RETURN(TOK[0]);
}

}}} // namespace yasm::parser::gas

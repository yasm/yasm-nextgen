/* nasmlib.c    library routines for the Netwide Assembler
 *
 * The Netwide Assembler is copyright (C) 1996 Simon Tatham and
 * Julian Hall. All rights reserved. The software is
 * redistributable under the licence given in the file "Licence"
 * distributed in the NASM archive.
 */
#include <cctype>

#include "yasmx/IntNum.h"

#include "nasm.h"
#include "nasmlib.h"
/*#include "insns.h"*/          /* For MAX_KEYWORD */


using yasm::IntNum;

namespace nasm {

void *nasm_realloc(void *q, size_t size)
{
    return q ? realloc(q, size) : malloc(size);
}

char *nasm_strdup(const char *s)
{
    char *p;
    int size = strlen(s) + 1;

    p = (char*)malloc(size);
    strcpy(p, s);
    return p;
}

char *nasm_strndup(const char *s, size_t len)
{
    char *p;
    int size = len + 1;

    p = (char*)malloc(size);
    strncpy(p, s, len);
    p[len] = '\0';
    return p;
}

#ifndef nasm_stricmp
int nasm_stricmp(const char *s1, const char *s2)
{
    unsigned char c1, c2;
    int d;

    while (1) {
	c1 = tolower(*s1++);
	c2 = tolower(*s2++);
	d = c1-c2;

	if (d)
	    return d;
	if (!c1)
	    break;
    }
    return 0;
}
#endif

#ifndef nasm_strnicmp
int nasm_strnicmp(const char *s1, const char *s2, size_t n)
{
    unsigned char c1, c2;
    int d;

    while (n--) {
	c1 = tolower(*s1++);
	c2 = tolower(*s2++);
	d = c1-c2;

	if (d)
	    return d;
	if (!c1)
	    break;
    }
    return 0;
}
#endif

#define lib_isnumchar(c)   ( isalnum(c) || (c) == '$')
#define numvalue(c)  ((c)>='a' ? (c)-'a'+10 : (c)>='A' ? (c)-'A'+10 : (c)-'0')

IntNum nasm_readnum (char *str, int *error) 
{
    char *r = str, *q, *p;
    long radix;
    IntNum intn;
    char save;
    int digit;
    int sign = 0;

    *error = FALSE;

    while (isspace(*r)) r++;           /* find start of number */

    /*
     * If the number came from make_tok_num (as a result of an %assign), it
     * might have a '-' built into it (rather than in a preceeding token).
     */
    if (*r == '-')
    {
        r++;
        sign = 1;
    }

    q = r;

    while (lib_isnumchar(*q)) q++;     /* find end of number */

    /*
     * If it begins 0x, 0X or $, or ends in H, it's in hex. if it
     * ends in Q, it's octal. if it ends in B, it's binary.
     * Otherwise, it's ordinary decimal.
     */
    if (*r=='0' && (r[1]=='x' || r[1]=='X'))
        radix = 16, r += 2;
    else if (*r=='$')
        radix = 16, r++;
    else if (q[-1]=='H' || q[-1]=='h')
        radix = 16 , q--;
    else if (q[-1]=='Q' || q[-1]=='q' || q[-1]=='O' || q[-1]=='o')
        radix = 8 , q--;
    else if (q[-1]=='B' || q[-1]=='b')
        radix = 2 , q--;
    else
        radix = 10;

    /*
     * If this number has been found for us by something other than
     * the ordinary scanners, then it might be malformed by having
     * nothing between the prefix and the suffix. Check this case
     * now.
     */
    if (r >= q) {
        *error = TRUE;
        return intn;
    }

    /* Check for valid number of that radix */
    p = r;
    while (*p && p < q) {
        if (*p<'0' || (*p>'9' && *p<'A') || (digit = numvalue(*p)) >= radix) 
        {
            *error = TRUE;
            return intn;
        }
        p++;
    }

    /* Use intnum to actually do the conversion */
    save = *q;
    *q = '\0';
    switch (radix) {
        case 2:
            intn.setStr(r, 2);
            break;
        case 8:
            intn.setStr(r, 8);
            break;
        case 10:
            intn.setStr(r, 10);
            break;
        case 16:
            intn.setStr(r, 16);
            break;
        default:
            *error = TRUE;
            break;
    }
    *q = save;

    if (sign)
        intn.CalcAssert(yasm::Op::NEG);
    return intn;
}

IntNum nasm_readstrnum (char *str, size_t length, int *warn) 
{
    IntNum intn;

    *warn = FALSE;
    for (int i = length-1; i >= 0; --i)
    {
        intn <<= 8;
        intn |= static_cast<unsigned int>(str[i]);
    }

    return intn;
}

static char *file_name = NULL;
static long line_number = 0;

char *nasm_src_set_fname(char *newname) 
{
    char *oldname = file_name;
    file_name = newname;
    return oldname;
}

char *nasm_src_get_fname(void)
{
    return file_name;
}

long nasm_src_set_linnum(long newline) 
{
    long oldline = line_number;
    line_number = newline;
    return oldline;
}

long nasm_src_get_linnum(void) 
{
    return line_number;
}

int nasm_src_get(long *xline, char **xname) 
{
    if (!file_name || !*xname || strcmp(*xname, file_name)) 
    {
        nasm_free(*xname);
        *xname = file_name ? nasm_strdup(file_name) : NULL;
        *xline = line_number;
        return -2;
    }
    if (*xline != line_number) 
    {
        long tmp = line_number - *xline;
        *xline = line_number;
        return tmp;
    }
    return 0;
}

void nasm_quote(char **str) 
{
    size_t ln=strlen(*str);
    char q=(*str)[0];
    char *p;
    if (ln>1 && (*str)[ln-1]==q && (q=='"' || q=='\''))
        return;
    q = '"';
    if (strchr(*str,q))
        q = '\'';
    p = (char*)nasm_malloc(ln+3);
    strcpy(p+1, *str);
    nasm_free(*str);
    p[ln+1] = p[0] = q;
    p[ln+2] = 0;
    *str = p;
}
    
char *nasm_strcat(const char *one, const char *two) 
{
    char *rslt;
    size_t l1=strlen(one);
    rslt = (char*)nasm_malloc(l1+strlen(two)+1);
    strcpy(rslt, one);
    strcpy(rslt+l1, two);
    return rslt;
}

} // namespace nasm

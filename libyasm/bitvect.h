/* $Id: bitvect.h 1825 2007-04-22 03:32:46Z peter $ */

#ifndef YASM_BITVECT_H
#define YASM_BITVECT_H
/*****************************************************************************/
/*  MODULE NAME:  BitVector.h                           MODULE TYPE:  (adt)  */
/*****************************************************************************/
/*  MODULE IMPORTS:                                                          */
/*****************************************************************************/

namespace BitVector {

/* ToolBox.h */
/*****************************************************************************/
/*  NOTE: The type names that have been chosen here are somewhat weird on    */
/*        purpose, in order to avoid name clashes with system header files   */
/*        and your own application(s) which might - directly or indirectly - */
/*        include this definitions file.                                     */
/*****************************************************************************/

typedef  unsigned   char    N_char;
typedef  unsigned   char    N_byte;
typedef  unsigned   short   N_short;
typedef  unsigned   short   N_shortword;
typedef  unsigned   int     N_int;
typedef  unsigned   int     N_word;
typedef  unsigned   long    N_long;
typedef  unsigned   long    N_longword;

/*  Mnemonic 1:  The natural numbers,  N = { 0, 1, 2, 3, ... }               */
/*  Mnemonic 2:  Nnnn = u_N_signed,  _N_ot signed                            */

typedef  signed     char    Z_char;
typedef  signed     char    Z_byte;
typedef  signed     short   Z_short;
typedef  signed     short   Z_shortword;
typedef  signed     int     Z_int;
typedef  signed     int     Z_word;
typedef  signed     long    Z_long;
typedef  signed     long    Z_longword;

/*  Mnemonic 1:  The whole numbers,  Z = { 0, -1, 1, -2, 2, -3, 3, ... }     */
/*  Mnemonic 2:  Zzzz = Ssss_igned                                           */

typedef  void               *voidptr;
typedef  N_char             *charptr;
typedef  N_byte             *byteptr;
typedef  N_short            *shortptr;
typedef  N_shortword        *shortwordptr;
typedef  N_int              *intptr;
typedef  N_word             *wordptr;
typedef  N_long             *longptr;
typedef  N_longword         *longwordptr;

typedef  N_char             *N_charptr;
typedef  N_byte             *N_byteptr;
typedef  N_short            *N_shortptr;
typedef  N_shortword        *N_shortwordptr;
typedef  N_int              *N_intptr;
typedef  N_word             *N_wordptr;
typedef  N_long             *N_longptr;
typedef  N_longword         *N_longwordptr;

typedef  Z_char             *Z_charptr;
typedef  Z_byte             *Z_byteptr;
typedef  Z_short            *Z_shortptr;
typedef  Z_shortword        *Z_shortwordptr;
typedef  Z_int              *Z_intptr;
typedef  Z_word             *Z_wordptr;
typedef  Z_long             *Z_longptr;
typedef  Z_longword         *Z_longwordptr;

/*****************************************************************************/
/*  MODULE INTERFACE:                                                        */
/*****************************************************************************/

enum ErrCode
    {
        ErrCode_Ok = 0,    /* everything went allright                       */

        ErrCode_Type,      /* types word and size_t have incompatible sizes  */
        ErrCode_Bits,      /* bits of word and sizeof(word) are inconsistent */
        ErrCode_Word,      /* size of word is less than 16 bits              */
        ErrCode_Long,      /* size of word is greater than size of long      */
        ErrCode_Powr,      /* number of bits of word is not a power of two   */
        ErrCode_Loga,      /* error in calculation of logarithm              */

        ErrCode_Null,      /* unable to allocate memory                      */

        ErrCode_Indx,      /* index out of range                             */
        ErrCode_Ordr,      /* minimum > maximum index                        */
        ErrCode_Size,      /* bit vector size mismatch                       */
        ErrCode_Pars,      /* input string syntax error                      */
        ErrCode_Ovfl,      /* numeric overflow error                         */
        ErrCode_Same,      /* operands must be distinct                      */
        ErrCode_Expo,      /* exponent must be positive                      */
        ErrCode_Zero       /* division by zero error                         */
    };

typedef wordptr *listptr;

/* ===> MISCELLANEOUS BASIC FUNCTIONS: <=== */

const char * Error      (ErrCode error);  /* return string for err code */

ErrCode Boot       (void);                 /* 0 = ok, 1..7 = error */
void    Shutdown   (void);                            /* undo Boot */

N_word  Size       (N_int bits);  /* bit vector size (# of words)  */
N_word  Mask       (N_int bits);  /* bit vector mask (unused bits) */

/* ===> CLASS METHODS: <=== */

const char * Version    (void);          /* returns version string */

N_int   Word_Bits  (void);     /* return # of bits in machine word */
N_int   Long_Bits  (void);    /* return # of bits in unsigned long */

/* ===> CONSTRUCTOR METHODS: <=== */

/*@only@*/ wordptr Create     (N_int bits, bool clear);          /* malloc */
listptr Create_List(N_int bits, bool clear, N_int count);

wordptr Resize     (wordptr oldaddr, N_int bits);       /* realloc */

wordptr Shadow     (wordptr addr); /* make new same size but empty */
wordptr Clone      (wordptr addr);         /* make exact duplicate */

wordptr Concat     (wordptr X, wordptr Y); /* return concatenation */

/* ===> DESTRUCTOR METHODS: <=== */

void    Dispose            (/*@only@*/ /*@out@*/ charptr string);             /* string */
void    Destroy            (/*@only@*/ wordptr addr);               /* bitvec */
void    Destroy_List       (listptr list, N_int count);  /* list   */

/* ===> OBJECT METHODS: <=== */

/* ===> bit vector copy function: */

void    Copy       (wordptr X, wordptr Y);              /* X = Y   */

/* ===> bit vector initialization: */

void    Empty      (wordptr addr);                      /* X = {}  */
void    Fill       (wordptr addr);                      /* X = ~{} */
void    Flip       (wordptr addr);                      /* X = ~X  */

void    Primes     (wordptr addr);

/* ===> miscellaneous functions: */

void    Reverse    (wordptr X, wordptr Y);

/* ===> bit vector interval operations and functions: */

void    Interval_Empty     (/*@out@*/ wordptr addr, N_int lower, N_int upper);
void    Interval_Fill      (/*@out@*/ wordptr addr, N_int lower, N_int upper);
void    Interval_Flip      (/*@out@*/ wordptr addr, N_int lower, N_int upper);
void    Interval_Reverse   (/*@out@*/ wordptr addr, N_int lower, N_int upper);

bool interval_scan_inc  (wordptr addr, N_int start,
                                   N_intptr min, N_intptr max);
bool interval_scan_dec  (wordptr addr, N_int start,
                                   N_intptr min, N_intptr max);

void    Interval_Copy      (/*@out@*/ wordptr X, wordptr Y, N_int Xoffset,
                                      N_int Yoffset, N_int length);

wordptr Interval_Substitute(/*@out@*/ wordptr X, wordptr Y,
                                      N_int Xoffset, N_int Xlength,
                                      N_int Yoffset, N_int Ylength);

/* ===> bit vector test functions: */

bool is_empty   (wordptr addr);                  /* X == {} ?   */
bool is_full    (wordptr addr);                  /* X == ~{} ?  */

bool equal      (wordptr X, wordptr Y);          /* X == Y ?    */
Z_int   Lexicompare(wordptr X, wordptr Y);          /* X <,=,> Y ? */
Z_int   Compare    (wordptr X, wordptr Y);          /* X <,=,> Y ? */

/* ===> bit vector string conversion functions: */

/*@only@*/ charptr to_Hex     (wordptr addr);
ErrCode from_Hex   (/*@out@*/wordptr addr, charptr string);

ErrCode from_Oct(/*@out@*/ wordptr addr, charptr string);

/*@only@*/ charptr to_Bin     (wordptr addr);
ErrCode from_Bin   (/*@out@*/ wordptr addr, charptr string);

/*@only@*/ charptr to_Dec     (wordptr addr);
ErrCode from_Dec   (/*@out@*/ wordptr addr, charptr string);

typedef struct from_Dec_static_data from_Dec_static_data;
from_Dec_static_data *from_Dec_static_Boot(N_word bits);
void from_Dec_static_Shutdown(/*@null@*/ from_Dec_static_data *data);
ErrCode from_Dec_static(from_Dec_static_data *data,
                                  /*@out@*/ wordptr addr, charptr string);

/*@only@*/ charptr to_Enum    (wordptr addr);
ErrCode from_Enum  (/*@out@*/ wordptr addr, charptr string);

/* ===> bit vector bit operations, functions & tests: */

void    Bit_Off    (/*@out@*/ wordptr addr, N_int indx); /*  X = X \ {x}    */
void    Bit_On     (/*@out@*/ wordptr addr, N_int indx); /*  X = X + {x}    */
bool bit_flip   (/*@out@*/ wordptr addr, N_int indx); /* (X+{x})\(X*{x}) */

bool bit_test   (wordptr addr, N_int indx); /*  {x} in X ?     */

void    Bit_Copy   (/*@out@*/ wordptr addr, N_int indx, bool bit);

/* ===> bit vector bit shift & rotate functions: */

void    LSB                (/*@out@*/ wordptr addr, bool bit);
void    MSB                (/*@out@*/ wordptr addr, bool bit);
bool lsb_               (wordptr addr);
bool msb_               (wordptr addr);
bool /*@alt void@*/ rotate_left        (wordptr addr);
bool /*@alt void@*/ rotate_right       (wordptr addr);
bool /*@alt void@*/ shift_left         (wordptr addr, bool carry_in);
bool /*@alt void@*/ shift_right        (wordptr addr, bool carry_in);
void    Move_Left          (wordptr addr, N_int bits);
void    Move_Right         (wordptr addr, N_int bits);

/* ===> bit vector insert/delete bits: */

void    Insert     (wordptr addr, N_int offset, N_int count,
                              bool clear);
void    Delete     (wordptr addr, N_int offset, N_int count,
                              bool clear);

/* ===> bit vector arithmetic: */

bool /*@alt void@*/ increment  (wordptr addr);                        /*  X++  */
bool /*@alt void@*/ decrement  (wordptr addr);                        /*  X--  */

bool /*@alt void@*/ compute    (wordptr X, wordptr Y, wordptr Z, bool minus,
                                                               bool *carry);
bool /*@alt void@*/ add        (wordptr X, wordptr Y, wordptr Z, bool *carry);
bool /*@alt void@*/ sub        (wordptr X, wordptr Y, wordptr Z, bool *carry);
bool /*@alt void@*/ inc        (wordptr X, wordptr Y);
bool /*@alt void@*/ dec        (wordptr X, wordptr Y);

void    Negate     (wordptr X, wordptr Y);
void    Absolute   (wordptr X, wordptr Y);
Z_int   Sign       (wordptr addr);
ErrCode Mul_Pos    (wordptr X, wordptr Y, wordptr Z, bool strict);
ErrCode Multiply   (wordptr X, wordptr Y, wordptr Z);
ErrCode Div_Pos    (wordptr Q, wordptr X, wordptr Y, wordptr R);
ErrCode Divide     (wordptr Q, wordptr X, wordptr Y, wordptr R);
ErrCode GCD        (wordptr X, wordptr Y, wordptr Z);
ErrCode GCD2       (wordptr U, wordptr V, wordptr W,      /*   O   */
                                         wordptr X, wordptr Y);     /*   I   */
ErrCode Power      (wordptr X, wordptr Y, wordptr Z);

/* ===> direct memory access functions: */

void    Block_Store(wordptr addr, charptr buffer, N_int length);
charptr Block_Read (wordptr addr, /*@out@*/ N_intptr length);

/* ===> word array functions: */

void    Word_Store (wordptr addr, N_int offset, N_int value);
N_int   Word_Read  (wordptr addr, N_int offset);

void    Word_Insert(wordptr addr, N_int offset, N_int count,
                              bool clear);
void    Word_Delete(wordptr addr, N_int offset, N_int count,
                              bool clear);

/* ===> arbitrary size chunk functions: */

void    Chunk_Store(wordptr addr, N_int chunksize,
                              N_int offset, N_long value);
N_long  Chunk_Read (wordptr addr, N_int chunksize,
                              N_int offset);

/* ===> set operations: */

void    Set_Union            (wordptr X, wordptr Y, wordptr Z); /* X = Y + Z */
void    Set_Intersection     (wordptr X, wordptr Y, wordptr Z); /* X = Y * Z */
void    Set_Difference       (wordptr X, wordptr Y, wordptr Z); /* X = Y \ Z */
void    Set_ExclusiveOr      (wordptr X, wordptr Y, wordptr Z); /*(Y+Z)\(Y*Z)*/
void    Set_Complement       (wordptr X, wordptr Y);            /* X = ~Y    */

/* ===> set functions: */

bool Set_subset           (wordptr X, wordptr Y);            /* X in Y ?  */

N_int   Set_Norm             (wordptr addr);                    /* = | X |   */
N_int   Set_Norm2            (wordptr addr);                    /* = | X |   */
N_int   Set_Norm3            (wordptr addr);                    /* = | X |   */
Z_long  Set_Min              (wordptr addr);                    /* = min(X)  */
Z_long  Set_Max              (wordptr addr);                    /* = max(X)  */

/* ===> matrix-of-booleans operations: */

void    Matrix_Multiplication(wordptr X, N_int rowsX, N_int colsX,
                              wordptr Y, N_int rowsY, N_int colsY,
                              wordptr Z, N_int rowsZ, N_int colsZ);

void    Matrix_Product       (wordptr X, N_int rowsX, N_int colsX,
                              wordptr Y, N_int rowsY, N_int colsY,
                              wordptr Z, N_int rowsZ, N_int colsZ);

void    Matrix_Closure       (wordptr addr, N_int rows, N_int cols);

void    Matrix_Transpose     (wordptr X, N_int rowsX, N_int colsX,
                              wordptr Y, N_int rowsY, N_int colsY);

} // namespace BitVector

/*****************************************************************************/
/*  VERSION:  6.4                                                            */
/*****************************************************************************/
/*  VERSION HISTORY:                                                         */
/*****************************************************************************/
/*                                                                           */
/*    Version 6.4  03.10.04  Added C++ comp. directives. Improved "Norm()".  */
/*    Version 6.3  28.09.02  Added "Create_List()" and "GCD2()".             */
/*    Version 6.2  15.09.02  Overhauled error handling. Fixed "GCD()".       */
/*    Version 6.1  08.10.01  Make VMS linker happy: _lsb,_msb => _lsb_,_msb_ */
/*    Version 6.0  08.10.00  Corrected overflow handling.                    */
/*    Version 5.8  14.07.00  Added "Power()". Changed "Copy()".              */
/*    Version 5.7  19.05.99  Quickened "Div_Pos()". Added "Product()".       */
/*    Version 5.6  02.11.98  Leading zeros eliminated in "to_Hex()".         */
/*    Version 5.5  21.09.98  Fixed bug of uninitialized "error" in Multiply. */
/*    Version 5.4  07.09.98  Fixed bug of uninitialized "error" in Divide.   */
/*    Version 5.3  12.05.98  Improved Norm. Completed history.               */
/*    Version 5.2  31.03.98  Improved Norm.                                  */
/*    Version 5.1  09.03.98  No changes.                                     */
/*    Version 5.0  01.03.98  Major additions and rewrite.                    */
/*    Version 4.2  16.07.97  Added is_empty, is_full.                        */
/*    Version 4.1  30.06.97  Added word-ins/del, move-left/right, inc/dec.   */
/*    Version 4.0  23.04.97  Rewrite. Added bit shift and bool. matrix ops.  */
/*    Version 3.2  04.02.97  Added interval methods.                         */
/*    Version 3.1  21.01.97  Fixed bug on 64 bit machines.                   */
/*    Version 3.0  12.01.97  Added flip.                                     */
/*    Version 2.0  14.12.96  Efficiency and consistency improvements.        */
/*    Version 1.1  08.01.96  Added Resize and ExclusiveOr.                   */
/*    Version 1.0  14.12.95  First version under UNIX (with Perl module).    */
/*    Version 0.9  01.11.93  First version of C library under MS-DOS.        */
/*    Version 0.1  ??.??.89  First version in Turbo Pascal under CP/M.       */
/*                                                                           */
/*****************************************************************************/
/*  AUTHOR:                                                                  */
/*****************************************************************************/
/*                                                                           */
/*    Steffen Beyer                                                          */
/*    mailto:sb@engelschall.com                                              */
/*    http://www.engelschall.com/u/sb/download/                              */
/*                                                                           */
/*****************************************************************************/
/*  COPYRIGHT:                                                               */
/*****************************************************************************/
/*                                                                           */
/*    Copyright (c) 1995 - 2004 by Steffen Beyer.                            */
/*    All rights reserved.                                                   */
/*                                                                           */
/*****************************************************************************/
/*  LICENSE:                                                                 */
/*****************************************************************************/
/* This package is free software; you can use, modify and redistribute       */
/* it under the same terms as Perl itself, i.e., under the terms of          */
/* the "Artistic License" or the "GNU General Public License".               */
/*                                                                           */
/* The C library at the core of this Perl module can additionally            */
/* be used, modified and redistributed under the terms of the                */
/* "GNU Library General Public License".                                     */
/*                                                                           */
/*****************************************************************************/
/*  ARTISTIC LICENSE:                                                        */
/*****************************************************************************/
/*
                         The "Artistic License"

                                Preamble

The intent of this document is to state the conditions under which a
Package may be copied, such that the Copyright Holder maintains some
semblance of artistic control over the development of the package,
while giving the users of the package the right to use and distribute
the Package in a more-or-less customary fashion, plus the right to make
reasonable modifications.

Definitions:

        "Package" refers to the collection of files distributed by the
        Copyright Holder, and derivatives of that collection of files
        created through textual modification.

        "Standard Version" refers to such a Package if it has not been
        modified, or has been modified in accordance with the wishes
        of the Copyright Holder as specified below.

        "Copyright Holder" is whoever is named in the copyright or
        copyrights for the package.

        "You" is you, if you're thinking about copying or distributing
        this Package.

        "Reasonable copying fee" is whatever you can justify on the
        basis of media cost, duplication charges, time of people involved,
        and so on.  (You will not be required to justify it to the
        Copyright Holder, but only to the computing community at large
        as a market that must bear the fee.)

        "Freely Available" means that no fee is charged for the item
        itself, though there may be fees involved in handling the item.
        It also means that recipients of the item may redistribute it
        under the same conditions they received it.

1. You may make and give away verbatim copies of the source form of the
Standard Version of this Package without restriction, provided that you
duplicate all of the original copyright notices and associated disclaimers.

2. You may apply bug fixes, portability fixes and other modifications
derived from the Public Domain or from the Copyright Holder.  A Package
modified in such a way shall still be considered the Standard Version.

3. You may otherwise modify your copy of this Package in any way, provided
that you insert a prominent notice in each changed file stating how and
when you changed that file, and provided that you do at least ONE of the
following:

    a) place your modifications in the Public Domain or otherwise make them
    Freely Available, such as by posting said modifications to Usenet or
    an equivalent medium, or placing the modifications on a major archive
    site such as uunet.uu.net, or by allowing the Copyright Holder to include
    your modifications in the Standard Version of the Package.

    b) use the modified Package only within your corporation or organization.

    c) rename any non-standard executables so the names do not conflict
    with standard executables, which must also be provided, and provide
    a separate manual page for each non-standard executable that clearly
    documents how it differs from the Standard Version.

    d) make other distribution arrangements with the Copyright Holder.

4. You may distribute the programs of this Package in object code or
executable form, provided that you do at least ONE of the following:

    a) distribute a Standard Version of the executables and library files,
    together with instructions (in the manual page or equivalent) on where
    to get the Standard Version.

    b) accompany the distribution with the machine-readable source of
    the Package with your modifications.

    c) give non-standard executables non-standard names, and clearly
    document the differences in manual pages (or equivalent), together
    with instructions on where to get the Standard Version.

    d) make other distribution arrangements with the Copyright Holder.

5. You may charge a reasonable copying fee for any distribution of this
Package.  You may charge any fee you choose for support of this
Package.  You may not charge a fee for this Package itself.  However,
you may distribute this Package in aggregate with other (possibly
commercial) programs as part of a larger (possibly commercial) software
distribution provided that you do not advertise this Package as a
product of your own.  You may embed this Package's interpreter within
an executable of yours (by linking); this shall be construed as a mere
form of aggregation, provided that the complete Standard Version of the
interpreter is so embedded.

6. The scripts and library files supplied as input to or produced as
output from the programs of this Package do not automatically fall
under the copyright of this Package, but belong to whoever generated
them, and may be sold commercially, and may be aggregated with this
Package.  If such scripts or library files are aggregated with this
Package via the so-called "undump" or "unexec" methods of producing a
binary executable image, then distribution of such an image shall
neither be construed as a distribution of this Package nor shall it
fall under the restrictions of Paragraphs 3 and 4, provided that you do
not represent such an executable image as a Standard Version of this
Package.

7. C subroutines (or comparably compiled subroutines in other
languages) supplied by you and linked into this Package in order to
emulate subroutines and variables of the language defined by this
Package shall not be considered part of this Package, but are the
equivalent of input as in Paragraph 6, provided these subroutines do
not change the language in any way that would cause it to fail the
regression tests for the language.

8. Aggregation of this Package with a commercial distribution is always
permitted provided that the use of this Package is embedded; that is,
when no overt attempt is made to make this Package's interfaces visible
to the end user of the commercial distribution.  Such use shall not be
construed as a distribution of this Package.

9. The name of the Copyright Holder may not be used to endorse or promote
products derived from this software without specific prior written permission.

10. THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.

                                The End
*/
/*****************************************************************************/
/*  GNU GENERAL PUBLIC LICENSE:                                              */
/*****************************************************************************/
/* This program is free software; you can redistribute it and/or             */
/* modify it under the terms of the GNU General Public License               */
/* as published by the Free Software Foundation; either version 2            */
/* of the License, or (at your option) any later version.                    */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program; if not, write to the                             */
/* Free Software Foundation, Inc.,                                           */
/* 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.                 */
/*                                                                           */
/*****************************************************************************/
/*  GNU LIBRARY GENERAL PUBLIC LICENSE:                                      */
/*****************************************************************************/
/*                                                                           */
/*    This library is free software; you can redistribute it and/or          */
/*    modify it under the terms of the GNU Library General Public            */
/*    License as published by the Free Software Foundation; either           */
/*    version 2 of the License, or (at your option) any later version.       */
/*                                                                           */
/*    This library is distributed in the hope that it will be useful,        */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of         */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       */
/*    Library General Public License for more details.                       */
/*                                                                           */
/*    You should have received a copy of the GNU Library General Public      */
/*    License along with this library; if not, write to the                  */
/*    Free Software Foundation, Inc.,                                        */
/*    59 Temple Place, Suite 330, Boston, MA 02111-1307 USA                  */
/*                                                                           */
/*    or download a copy from ftp://ftp.gnu.org/pub/gnu/COPYING.LIB-2.0      */
/*                                                                           */
/*****************************************************************************/

#endif

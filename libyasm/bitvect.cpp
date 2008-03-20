#include "bitvect.h"

#include "util.h"

/*****************************************************************************/
/*  MODULE NAME:  BitVector.c                           MODULE TYPE:  (adt)  */
/*****************************************************************************/
/*  MODULE IMPORTS:                                                          */
/*****************************************************************************/
#include <limits>
#include <ctype.h>                                  /*  MODULE TYPE:  (sys)  */
#include <limits.h>                                 /*  MODULE TYPE:  (sys)  */
#include <string.h>                                 /*  MODULE TYPE:  (sys)  */
#include <stdlib.h>                                 /*  MODULE TYPE:  (sys)  */
/*****************************************************************************/
/*  MODULE INTERFACE:                                                        */
/*****************************************************************************/

namespace BitVector
{

/* ToolBox.h */
#define AND         &       /* binary (bitwise) operators: UPPER CASE */
#define OR          |
#define XOR         ^
#define NOT         ~
#define SHL         <<
#define SHR         >>

/*****************************************************************************/
/*  MODULE RESOURCES:                                                        */
/*****************************************************************************/

static inline N_int& bits_(const wordptr addr) { return addr[-3]; }
static inline N_int& size_(const wordptr addr) { return addr[-2]; }
static inline N_int& mask_(const wordptr addr) { return addr[-1]; }

#define  ERRCODE_TYPE  "sizeof(word) > sizeof(size_t)"
#define  ERRCODE_BITS  "bits(word) != sizeof(word)*8"
#define  ERRCODE_WORD  "bits(word) < 16"
#define  ERRCODE_LONG  "bits(word) > bits(long)"
#define  ERRCODE_POWR  "bits(word) != 2^x"
#define  ERRCODE_LOGA  "bits(word) != 2^ld(bits(word))"
#define  ERRCODE_NULL  "unable to allocate memory"
#define  ERRCODE_INDX  "index out of range"
#define  ERRCODE_ORDR  "minimum > maximum index"
#define  ERRCODE_SIZE  "bit vector size mismatch"
#define  ERRCODE_PARS  "input string syntax error"
#define  ERRCODE_OVFL  "numeric overflow error"
#define  ERRCODE_SAME  "result vector(s) must be distinct"
#define  ERRCODE_EXPO  "exponent must be positive"
#define  ERRCODE_ZERO  "division by zero error"
#define  ERRCODE_OOPS  "unexpected internal error - please contact author"

/*****************************************************************************/
/*  MODULE IMPLEMENTATION:                                                   */
/*****************************************************************************/

    /**********************************************/
    /* global implementation-intrinsic constants: */
    /**********************************************/

static const int HIDDEN_WORDS = 3;

    /*****************************************************************/
    /* global machine-dependent constants (set by "Boot"): */
    /*****************************************************************/

// # of bits in machine word (must be power of 2)
#define BITS ((N_word)(std::numeric_limits<N_word>::digits))
// BITS - 1 (mask for calculating modulo BITS)
#define MODMASK ((N_word)(BITS-1))
// # of bytes in machine word
#define BYTES ((N_word)(BITS>>3))

// mask for least significant bit
#define LSBMASK 1U
// mask for most significant bit
#define MSBMASK (LSBMASK << MODMASK)

// # of bits in unsigned long
#define LONGBITS ((N_word)(std::numeric_limits<N_long>::digits))

// logarithm to base 10 of BITS - 1
#define LOG10 ((N_word)(MODMASK * 0.30103))     // (BITS - 1) * ( ln 2 / ln 10 )
static N_word EXP10;    /* = largest possible power of 10 in signed int      */

    /********************************************************************/
    /* global bit mask table for fast access (set by "Boot"): */
    /********************************************************************/

static wordptr BITMASKTAB = 0;

    /*****************************/
    /* global macro definitions: */
    /*****************************/

#define ZERO_WORDS(target,count) \
    while (count-- > 0) *target++ = 0;

#define FILL_WORDS(target,fill,count) \
    while (count-- > 0) *target++ = fill;

#define FLIP_WORDS(target,flip,count) \
    while (count-- > 0) *target++ ^= flip;

#define COPY_WORDS(target,source,count) \
    while (count-- > 0) *target++ = *source++;

#define BACK_WORDS(target,source,count) \
    { target += count; source += count; while (count-- > 0) *--target = *--source; }

#define CLR_BIT(address,index) \
    *(address+(index/BITS)) &= NOT BITMASKTAB[index AND MODMASK];

#define SET_BIT(address,index) \
    *(address+(index/BITS)) |= BITMASKTAB[index AND MODMASK];

#define TST_BIT(address,index) \
    ((*(address+(index/BITS)) AND BITMASKTAB[index AND MODMASK]) != 0)

#define FLP_BIT(address,index,mask) \
    (mask = BITMASKTAB[index AND MODMASK]), \
    (((*(addr+(index/BITS)) ^= mask) AND mask) != 0)

#define DIGITIZE(type,value,digit) \
    value = (type) ((digit = value) / 10); \
    digit -= value * 10; \
    digit += (type) '0';

    /*********************************************************/
    /* private low-level functions (potentially dangerous!): */
    /*********************************************************/

static inline N_word power10(N_word x)
{
    N_word y = 1;

    while (x-- > 0) y *= 10;
    return(y);
}

static inline void zro_words(wordptr addr, N_word count)
{
    ZERO_WORDS(addr,count)
}

static inline void cpy_words(wordptr target, wordptr source, N_word count)
{
    COPY_WORDS(target,source,count)
}

static void mov_words(wordptr target, wordptr source, N_word count)
{
    if (target != source)
    {
        if (target < source) COPY_WORDS(target,source,count)
        else                 BACK_WORDS(target,source,count)
    }
}

static void ins_words(wordptr addr, N_word total, N_word count,
                                 bool clear)
{
    N_word length;

    if ((total > 0) && (count > 0))
    {
        if (count > total) count = total;
        length = total - count;
        if (length > 0) mov_words(addr+count,addr,length);
        if (clear)      zro_words(addr,count);
    }
}

static void del_words(wordptr addr, N_word total, N_word count,
                                 bool clear)
{
    N_word length;

    if ((total > 0) && (count > 0))
    {
        if (count > total) count = total;
        length = total - count;
        if (length > 0) mov_words(addr,addr+count,length);
        if (clear)      zro_words(addr+length,count);
    }
}

static void reverse(charptr string, N_word length)
{
    charptr last;
    N_char  temp;

    if (length > 1)
    {
        last = string + length - 1;
        while (string < last)
        {
            temp = *string;
            *string = *last;
            *last = temp;
            string++;
            last--;
        }
    }
}

static N_word int2str(charptr string, N_word value)
{
    N_word  length;
    N_word  digit;
    charptr work;

    work = string;
    if (value > 0)
    {
        length = 0;
        while (value > 0)
        {
            DIGITIZE(N_word,value,digit)
            *work++ = (N_char) digit;
            length++;
        }
        reverse(string,length);
    }
    else
    {
        length = 1;
        *work++ = (N_char) '0';
    }
    return(length);
}

static N_word str2int(charptr string, N_word *value)
{
    N_word  length;
    N_word  digit;

    *value = 0;
    length = 0;
    digit = (N_word) *string++;
    /* separate because isdigit() is likely a macro! */
    while (isdigit((int)digit) != 0)
    {
        length++;
        digit -= (N_word) '0';
        if (*value) *value *= 10;
        *value += digit;
        digit = (N_word) *string++;
    }
    return(length);
}

    /********************************************/
    /* routine to convert error code to string: */
    /********************************************/

const char * Error(ErrCode error)
{
    switch (error)
    {
        case ErrCode_Ok:   return(     NULL     ); break;
        case ErrCode_Type: return( ERRCODE_TYPE ); break;
        case ErrCode_Bits: return( ERRCODE_BITS ); break;
        case ErrCode_Word: return( ERRCODE_WORD ); break;
        case ErrCode_Long: return( ERRCODE_LONG ); break;
        case ErrCode_Powr: return( ERRCODE_POWR ); break;
        case ErrCode_Loga: return( ERRCODE_LOGA ); break;
        case ErrCode_Null: return( ERRCODE_NULL ); break;
        case ErrCode_Indx: return( ERRCODE_INDX ); break;
        case ErrCode_Ordr: return( ERRCODE_ORDR ); break;
        case ErrCode_Size: return( ERRCODE_SIZE ); break;
        case ErrCode_Pars: return( ERRCODE_PARS ); break;
        case ErrCode_Ovfl: return( ERRCODE_OVFL ); break;
        case ErrCode_Same: return( ERRCODE_SAME ); break;
        case ErrCode_Expo: return( ERRCODE_EXPO ); break;
        case ErrCode_Zero: return( ERRCODE_ZERO ); break;
        default:           return( ERRCODE_OOPS ); break;
    }
}

    /*****************************************/
    /* automatic self-configuration routine: */
    /*****************************************/

    /*******************************************************/
    /*                                                     */
    /*   MUST be called once prior to any other function   */
    /*   to initialize the machine dependent constants     */
    /*   of this package! (But call only ONCE, or you      */
    /*   will suffer memory leaks!)                        */
    /*                                                     */
    /*******************************************************/

ErrCode Boot(void)
{
    N_word sample = LSBMASK;

    if (sizeof(N_word) > sizeof(size_t)) return(ErrCode_Type);

    if (BITS != (sizeof(N_word) << 3)) return(ErrCode_Bits);

    if (BITS < 16) return(ErrCode_Word);

    if (BITS > LONGBITS) return(ErrCode_Long);

    if (BITMASKTAB) free(BITMASKTAB);

    BITMASKTAB = (wordptr) malloc((size_t) (BITS * BYTES));

    if (BITMASKTAB == NULL) return(ErrCode_Null);

    for ( sample = 0; sample < BITS; sample++ )
    {
        BITMASKTAB[sample] = (LSBMASK << sample);
    }

    EXP10 = power10(LOG10);

    return(ErrCode_Ok);
}

void Shutdown(void)
{
    if (BITMASKTAB) free(BITMASKTAB);
    BITMASKTAB = 0;
}

N_word Size(N_int bits)           /* bit vector size (# of words)  */
{
    N_word size;

    size = bits / BITS;
    if (bits AND MODMASK) size++;
    return(size);
}

N_word Mask(N_int bits)           /* bit vector mask (unused bits) */
{
    N_word mask;

    mask = bits AND MODMASK;
    if (mask) mask = (N_word) ~(~0L << mask); else mask = (N_word) ~0L;
    return(mask);
}

const char * Version(void)
{
    return("6.4");
}

N_int Word_Bits(void)
{
    return(BITS);
}

N_int Long_Bits(void)
{
    return(LONGBITS);
}

/********************************************************************/
/*                                                                  */
/*  WARNING: Do not "free()" constant character strings, i.e.,      */
/*           don't call "Dispose()" for strings returned  */
/*           by "Error()" or "Version()"!       */
/*                                                                  */
/*  ONLY call this function for strings allocated with "malloc()",  */
/*  i.e., the strings returned by the functions "to_*()"  */
/*  and "Block_Read()"!                                   */
/*                                                                  */
/********************************************************************/

void Dispose(charptr string)                      /* free string   */
{
    if (string != NULL) free((voidptr) string);
}

void Destroy(wordptr addr)                        /* free bitvec   */
{
    if (addr != NULL)
    {
        addr -= HIDDEN_WORDS;
        free((voidptr) addr);
    }
}

void Destroy_List(listptr list, N_int count)      /* free list     */
{
    listptr slot;

    if (list != NULL)
    {
        slot = list;
        while (count-- > 0)
        {
            Destroy(*slot++);
        }
        free((voidptr) list);
    }
}

wordptr Create(N_int bits, bool clear)         /* malloc        */
{
    N_word  size;
    N_word  mask;
    N_word  bytes;
    wordptr addr;
    wordptr zero;

    size = Size(bits);
    mask = Mask(bits);
    bytes = (size + HIDDEN_WORDS) * BYTES;
    addr = (wordptr) malloc((size_t) bytes);
    if (addr != NULL)
    {
        *addr++ = bits;
        *addr++ = size;
        *addr++ = mask;
        if (clear)
        {
            zero = addr;
            ZERO_WORDS(zero,size)
        }
    }
    return(addr);
}

listptr Create_List(N_int bits, bool clear, N_int count)
{
    listptr list = NULL;
    listptr slot;
    wordptr addr;
    N_int   i;

    if (count > 0)
    {
        list = (listptr) malloc(sizeof(wordptr) * count);
        if (list != NULL)
        {
            slot = list;
            for ( i = 0; i < count; i++ )
            {
                addr = Create(bits,clear);
                if (addr == NULL)
                {
                    Destroy_List(list,i);
                    return(NULL);
                }
                *slot++ = addr;
            }
        }
    }
    return(list);
}

wordptr Resize(wordptr oldaddr, N_int bits)       /* realloc       */
{
    N_word  bytes;
    N_word  oldsize;
    N_word  oldmask;
    N_word  newsize;
    N_word  newmask;
    wordptr newaddr;
    wordptr source;
    wordptr target;

    oldsize = size_(oldaddr);
    oldmask = mask_(oldaddr);
    newsize = Size(bits);
    newmask = Mask(bits);
    if (oldsize > 0) *(oldaddr+oldsize-1) &= oldmask;
    if (newsize <= oldsize)
    {
        newaddr = oldaddr;
        bits_(newaddr) = bits;
        size_(newaddr) = newsize;
        mask_(newaddr) = newmask;
        if (newsize > 0) *(newaddr+newsize-1) &= newmask;
    }
    else
    {
        bytes = (newsize + HIDDEN_WORDS) * BYTES;
        newaddr = (wordptr) malloc((size_t) bytes);
        if (newaddr != NULL)
        {
            *newaddr++ = bits;
            *newaddr++ = newsize;
            *newaddr++ = newmask;
            target = newaddr;
            source = oldaddr;
            newsize -= oldsize;
            COPY_WORDS(target,source,oldsize)
            ZERO_WORDS(target,newsize)
        }
        Destroy(oldaddr);
    }
    return(newaddr);
}

wordptr Shadow(wordptr addr)     /* makes new, same size but empty */
{
    return( Create(bits_(addr),true) );
}

wordptr Clone(wordptr addr)               /* makes exact duplicate */
{
    N_word  bits;
    wordptr twin;

    bits = bits_(addr);
    twin = Create(bits,false);
    if ((twin != NULL) && (bits > 0))
        cpy_words(twin,addr,size_(addr));
    return(twin);
}

wordptr Concat(wordptr X, wordptr Y)      /* returns concatenation */
{
    /* BEWARE that X = most significant part, Y = least significant part! */

    N_word  bitsX;
    N_word  bitsY;
    N_word  bitsZ;
    wordptr Z;

    bitsX = bits_(X);
    bitsY = bits_(Y);
    bitsZ = bitsX + bitsY;
    Z = Create(bitsZ,false);
    if ((Z != NULL) && (bitsZ > 0))
    {
        cpy_words(Z,Y,size_(Y));
        Interval_Copy(Z,X,bitsY,0,bitsX);
        *(Z+size_(Z)-1) &= mask_(Z);
    }
    return(Z);
}

void Copy(wordptr X, wordptr Y)                           /* X = Y */
{
    N_word  sizeX = size_(X);
    N_word  sizeY = size_(Y);
    N_word  maskX = mask_(X);
    N_word  maskY = mask_(Y);
    N_word  fill  = 0;
    wordptr lastX;
    wordptr lastY;

    if ((X != Y) && (sizeX > 0))
    {
        lastX = X + sizeX - 1;
        if (sizeY > 0)
        {
            lastY = Y + sizeY - 1;
            if ( (*lastY AND (maskY AND NOT (maskY >> 1))) == 0 ) *lastY &= maskY;
            else
            {
                fill = (N_word) ~0L;
                *lastY |= NOT maskY;
            }
            while ((sizeX > 0) && (sizeY > 0))
            {
                *X++ = *Y++;
                sizeX--;
                sizeY--;
            }
            *lastY &= maskY;
        }
        while (sizeX-- > 0) *X++ = fill;
        *lastX &= maskX;
    }
}

void Empty(wordptr addr)                        /* X = {}  clr all */
{
    N_word size = size_(addr);

    ZERO_WORDS(addr,size)
}

void Fill(wordptr addr)                         /* X = ~{} set all */
{
    N_word size = size_(addr);
    N_word mask = mask_(addr);
    N_word fill = (N_word) ~0L;

    if (size > 0)
    {
        FILL_WORDS(addr,fill,size)
        *(--addr) &= mask;
    }
}

void Flip(wordptr addr)                         /* X = ~X flip all */
{
    N_word size = size_(addr);
    N_word mask = mask_(addr);
    N_word flip = (N_word) ~0L;

    if (size > 0)
    {
        FLIP_WORDS(addr,flip,size)
        *(--addr) &= mask;
    }
}

void Primes(wordptr addr)
{
    N_word  bits = bits_(addr);
    N_word  size = size_(addr);
    wordptr work;
    N_word  temp;
    N_word  i,j;

    if (size > 0)
    {
        temp = 0xAAAA;
        i = BITS >> 4;
        while (--i > 0)
        {
            temp <<= 16;
            temp |= 0xAAAA;
        }
        i = size;
        work = addr;
        *work++ = temp XOR 0x0006;
        while (--i > 0) *work++ = temp;
        for ( i = 3; (j = i * i) < bits; i += 2 )
        {
            for ( ; j < bits; j += i ) CLR_BIT(addr,j)
        }
        *(addr+size-1) &= mask_(addr);
    }
}

void Reverse(wordptr X, wordptr Y)
{
    N_word bits = bits_(X);
    N_word mask;
    N_word bit;
    N_word value;

    if (bits > 0)
    {
        if (X == Y) Interval_Reverse(X,0,bits-1);
        else if (bits == bits_(Y))
        {
/*          mask = mask_(Y);  */
/*          mask &= NOT (mask >> 1);  */
            mask = BITMASKTAB[(bits-1) AND MODMASK];
            Y += size_(Y) - 1;
            value = 0;
            bit = LSBMASK;
            while (bits-- > 0)
            {
                if ((*Y AND mask) != 0)
                {
                    value |= bit;
                }
                if (!(mask >>= 1))
                {
                    Y--;
                    mask = MSBMASK;
                }
                if (!(bit <<= 1))
                {
                    *X++ = value;
                    value = 0;
                    bit = LSBMASK;
                }
            }
            if (bit > LSBMASK) *X = value;
        }
    }
}

void Interval_Empty(wordptr addr, N_int lower, N_int upper)
{                                                  /* X = X \ [lower..upper] */
    N_word  bits = bits_(addr);
    N_word  size = size_(addr);
    wordptr loaddr;
    wordptr hiaddr;
    N_word  lobase;
    N_word  hibase;
    N_word  lomask;
    N_word  himask;
    N_word  diff;

    if ((size > 0) && (lower < bits) && (upper < bits) && (lower <= upper))
    {
        lobase = lower / BITS;
        hibase = upper / BITS;
        diff = hibase - lobase;
        loaddr = addr + lobase;
        hiaddr = addr + hibase;

        lomask = (N_word)   (~0L << (lower AND MODMASK));
        himask = (N_word) ~((~0L << (upper AND MODMASK)) << 1);

        if (diff == 0)
        {
            *loaddr &= NOT (lomask AND himask);
        }
        else
        {
            *loaddr++ &= NOT lomask;
            while (--diff > 0)
            {
                *loaddr++ = 0;
            }
            *hiaddr &= NOT himask;
        }
    }
}

void Interval_Fill(wordptr addr, N_int lower, N_int upper)
{                                                  /* X = X + [lower..upper] */
    N_word  bits = bits_(addr);
    N_word  size = size_(addr);
    N_word  fill = (N_word) ~0L;
    wordptr loaddr;
    wordptr hiaddr;
    N_word  lobase;
    N_word  hibase;
    N_word  lomask;
    N_word  himask;
    N_word  diff;

    if ((size > 0) && (lower < bits) && (upper < bits) && (lower <= upper))
    {
        lobase = lower / BITS;
        hibase = upper / BITS;
        diff = hibase - lobase;
        loaddr = addr + lobase;
        hiaddr = addr + hibase;

        lomask = (N_word)   (~0L << (lower AND MODMASK));
        himask = (N_word) ~((~0L << (upper AND MODMASK)) << 1);

        if (diff == 0)
        {
            *loaddr |= (lomask AND himask);
        }
        else
        {
            *loaddr++ |= lomask;
            while (--diff > 0)
            {
                *loaddr++ = fill;
            }
            *hiaddr |= himask;
        }
        *(addr+size-1) &= mask_(addr);
    }
}

void Interval_Flip(wordptr addr, N_int lower, N_int upper)
{                                                  /* X = X ^ [lower..upper] */
    N_word  bits = bits_(addr);
    N_word  size = size_(addr);
    N_word  flip = (N_word) ~0L;
    wordptr loaddr;
    wordptr hiaddr;
    N_word  lobase;
    N_word  hibase;
    N_word  lomask;
    N_word  himask;
    N_word  diff;

    if ((size > 0) && (lower < bits) && (upper < bits) && (lower <= upper))
    {
        lobase = lower / BITS;
        hibase = upper / BITS;
        diff = hibase - lobase;
        loaddr = addr + lobase;
        hiaddr = addr + hibase;

        lomask = (N_word)   (~0L << (lower AND MODMASK));
        himask = (N_word) ~((~0L << (upper AND MODMASK)) << 1);

        if (diff == 0)
        {
            *loaddr ^= (lomask AND himask);
        }
        else
        {
            *loaddr++ ^= lomask;
            while (--diff > 0)
            {
                *loaddr++ ^= flip;
            }
            *hiaddr ^= himask;
        }
        *(addr+size-1) &= mask_(addr);
    }
}

void Interval_Reverse(wordptr addr, N_int lower, N_int upper)
{
    N_word  bits = bits_(addr);
    wordptr loaddr;
    wordptr hiaddr;
    N_word  lomask;
    N_word  himask;

    if ((bits > 0) && (lower < bits) && (upper < bits) && (lower < upper))
    {
        loaddr = addr + (lower / BITS);
        hiaddr = addr + (upper / BITS);
        lomask = BITMASKTAB[lower AND MODMASK];
        himask = BITMASKTAB[upper AND MODMASK];
        for ( bits = upper - lower + 1; bits > 1; bits -= 2 )
        {
            if (((*loaddr AND lomask) != 0) XOR ((*hiaddr AND himask) != 0))
            {
                *loaddr ^= lomask;  /* swap bits only if they differ! */
                *hiaddr ^= himask;
            }
            if (!(lomask <<= 1))
            {
                lomask = LSBMASK;
                loaddr++;
            }
            if (!(himask >>= 1))
            {
                himask = MSBMASK;
                hiaddr--;
            }
        }
    }
}

bool interval_scan_inc(wordptr addr, N_int start,
                                    N_intptr min, N_intptr max)
{
    N_word  size = size_(addr);
    N_word  mask = mask_(addr);
    N_word  offset;
    N_word  bitmask;
    N_word  value;
    bool empty;

    if ((size == 0) || (start >= bits_(addr))) return(false);

    *min = start;
    *max = start;

    offset = start / BITS;

    *(addr+size-1) &= mask;

    addr += offset;
    size -= offset;

    bitmask = BITMASKTAB[start AND MODMASK];
    mask = NOT (bitmask OR (bitmask - 1));

    value = *addr++;
    if ((value AND bitmask) == 0)
    {
        value &= mask;
        if (value == 0)
        {
            offset++;
            empty = true;
            while (empty && (--size > 0))
            {
                if ((value = *addr++)) empty = false; else offset++;
            }
            if (empty) return(false);
        }
        start = offset * BITS;
        bitmask = LSBMASK;
        mask = value;
        while (!(mask AND LSBMASK))
        {
            bitmask <<= 1;
            mask >>= 1;
            start++;
        }
        mask = NOT (bitmask OR (bitmask - 1));
        *min = start;
        *max = start;
    }
    value = NOT value;
    value &= mask;
    if (value == 0)
    {
        offset++;
        empty = true;
        while (empty && (--size > 0))
        {
            if ((value = NOT *addr++)) empty = false; else offset++;
        }
        if (empty) value = LSBMASK;
    }
    start = offset * BITS;
    while (!(value AND LSBMASK))
    {
        value >>= 1;
        start++;
    }
    *max = --start;
    return(true);
}

bool interval_scan_dec(wordptr addr, N_int start,
                                    N_intptr min, N_intptr max)
{
    N_word  size = size_(addr);
    N_word  mask = mask_(addr);
    N_word offset;
    N_word bitmask;
    N_word value;
    bool empty;

    if ((size == 0) || (start >= bits_(addr))) return(false);

    *min = start;
    *max = start;

    offset = start / BITS;

    if (offset >= size) return(false);

    *(addr+size-1) &= mask;

    addr += offset;
    size = ++offset;

    bitmask = BITMASKTAB[start AND MODMASK];
    mask = (bitmask - 1);

    value = *addr--;
    if ((value AND bitmask) == 0)
    {
        value &= mask;
        if (value == 0)
        {
            offset--;
            empty = true;
            while (empty && (--size > 0))
            {
                if ((value = *addr--)) empty = false; else offset--;
            }
            if (empty) return(false);
        }
        start = offset * BITS;
        bitmask = MSBMASK;
        mask = value;
        while (!(mask AND MSBMASK))
        {
            bitmask >>= 1;
            mask <<= 1;
            start--;
        }
        mask = (bitmask - 1);
        *max = --start;
        *min = start;
    }
    value = NOT value;
    value &= mask;
    if (value == 0)
    {
        offset--;
        empty = true;
        while (empty && (--size > 0))
        {
            if ((value = NOT *addr--)) empty = false; else offset--;
        }
        if (empty) value = MSBMASK;
    }
    start = offset * BITS;
    while (!(value AND MSBMASK))
    {
        value <<= 1;
        start--;
    }
    *min = start;
    return(true);
}

void Interval_Copy(wordptr X, wordptr Y, N_int Xoffset,
                             N_int Yoffset, N_int length)
{
    N_word  bitsX = bits_(X);
    N_word  bitsY = bits_(Y);
    N_word  source = 0;        /* silence compiler warning */
    N_word  target = 0;        /* silence compiler warning */
    N_word  s_lo_base;
    N_word  s_hi_base;
    N_word  s_lo_bit;
    N_word  s_hi_bit;
    N_word  s_base;
    N_word  s_lower = 0;       /* silence compiler warning */
    N_word  s_upper = 0;       /* silence compiler warning */
    N_word  s_bits;
    N_word  s_min;
    N_word  s_max;
    N_word  t_lo_base;
    N_word  t_hi_base;
    N_word  t_lo_bit;
    N_word  t_hi_bit;
    N_word  t_base;
    N_word  t_lower = 0;       /* silence compiler warning */
    N_word  t_upper = 0;       /* silence compiler warning */
    N_word  t_bits;
    N_word  t_min;
    N_word  mask;
    N_word  bits;
    N_word  sel;
    bool ascending;
    bool notfirst;
    wordptr Z = X;

    if ((length > 0) && (Xoffset < bitsX) && (Yoffset < bitsY))
    {
        if ((Xoffset + length) > bitsX) length = bitsX - Xoffset;
        if ((Yoffset + length) > bitsY) length = bitsY - Yoffset;

        ascending = (Xoffset <= Yoffset);

        s_lo_base = Yoffset / BITS;
        s_lo_bit = Yoffset AND MODMASK;
        Yoffset += --length;
        s_hi_base = Yoffset / BITS;
        s_hi_bit = Yoffset AND MODMASK;

        t_lo_base = Xoffset / BITS;
        t_lo_bit = Xoffset AND MODMASK;
        Xoffset += length;
        t_hi_base = Xoffset / BITS;
        t_hi_bit = Xoffset AND MODMASK;

        if (ascending)
        {
            s_base = s_lo_base;
            t_base = t_lo_base;
        }
        else
        {
            s_base = s_hi_base;
            t_base = t_hi_base;
        }
        s_bits = 0;
        t_bits = 0;
        Y += s_base;
        X += t_base;
        notfirst = false;
        while (true)
        {
            if (t_bits == 0)
            {
                if (notfirst)
                {
                    *X = target;
                    if (ascending)
                    {
                        if (t_base == t_hi_base) break;
                        t_base++;
                        X++;
                    }
                    else
                    {
                        if (t_base == t_lo_base) break;
                        t_base--;
                        X--;
                    }
                }
                sel = ((t_base == t_hi_base) << 1) OR (t_base == t_lo_base);
                switch (sel)
                {
                    case 0:
                        t_lower = 0;
                        t_upper = BITS - 1;
                        t_bits = BITS;
                        target = 0;
                        break;
                    case 1:
                        t_lower = t_lo_bit;
                        t_upper = BITS - 1;
                        t_bits = BITS - t_lo_bit;
                        mask = (N_word) (~0L << t_lower);
                        target = *X AND NOT mask;
                        break;
                    case 2:
                        t_lower = 0;
                        t_upper = t_hi_bit;
                        t_bits = t_hi_bit + 1;
                        mask = (N_word) ((~0L << t_upper) << 1);
                        target = *X AND mask;
                        break;
                    case 3:
                        t_lower = t_lo_bit;
                        t_upper = t_hi_bit;
                        t_bits = t_hi_bit - t_lo_bit + 1;
                        mask = (N_word) (~0L << t_lower);
                        mask &= (N_word) ~((~0L << t_upper) << 1);
                        target = *X AND NOT mask;
                        break;
                }
            }
            if (s_bits == 0)
            {
                if (notfirst)
                {
                    if (ascending)
                    {
                        if (s_base == s_hi_base) break;
                        s_base++;
                        Y++;
                    }
                    else
                    {
                        if (s_base == s_lo_base) break;
                        s_base--;
                        Y--;
                    }
                }
                source = *Y;
                sel = ((s_base == s_hi_base) << 1) OR (s_base == s_lo_base);
                switch (sel)
                {
                    case 0:
                        s_lower = 0;
                        s_upper = BITS - 1;
                        s_bits = BITS;
                        break;
                    case 1:
                        s_lower = s_lo_bit;
                        s_upper = BITS - 1;
                        s_bits = BITS - s_lo_bit;
                        break;
                    case 2:
                        s_lower = 0;
                        s_upper = s_hi_bit;
                        s_bits = s_hi_bit + 1;
                        break;
                    case 3:
                        s_lower = s_lo_bit;
                        s_upper = s_hi_bit;
                        s_bits = s_hi_bit - s_lo_bit + 1;
                        break;
                }
            }
            notfirst = true;
            if (s_bits > t_bits)
            {
                bits = t_bits - 1;
                if (ascending)
                {
                    s_min = s_lower;
                    s_max = s_lower + bits;
                }
                else
                {
                    s_max = s_upper;
                    s_min = s_upper - bits;
                }
                t_min = t_lower;
            }
            else
            {
                bits = s_bits - 1;
                if (ascending) t_min = t_lower;
                else           t_min = t_upper - bits;
                s_min = s_lower;
                s_max = s_upper;
            }
            bits++;
            mask = (N_word) (~0L << s_min);
            mask &= (N_word) ~((~0L << s_max) << 1);
            if (s_min == t_min) target |= (source AND mask);
            else
            {
                if (s_min < t_min) target |= (source AND mask) << (t_min-s_min);
                else               target |= (source AND mask) >> (s_min-t_min);
            }
            if (ascending)
            {
                s_lower += bits;
                t_lower += bits;
            }
            else
            {
                s_upper -= bits;
                t_upper -= bits;
            }
            s_bits -= bits;
            t_bits -= bits;
        }
        *(Z+size_(Z)-1) &= mask_(Z);
    }
}


wordptr Interval_Substitute(wordptr X, wordptr Y,
                                      N_int Xoffset, N_int Xlength,
                                      N_int Yoffset, N_int Ylength)
{
    N_word Xbits = bits_(X);
    N_word Ybits = bits_(Y);
    N_word limit;
    N_word diff;

    if ((Xoffset <= Xbits) && (Yoffset <= Ybits))
    {
        limit = Xoffset + Xlength;
        if (limit > Xbits)
        {
            limit = Xbits;
            Xlength = Xbits - Xoffset;
        }
        if ((Yoffset + Ylength) > Ybits)
        {
            Ylength = Ybits - Yoffset;
        }
        if (Xlength == Ylength)
        {
            if ((Ylength > 0) && ((X != Y) || (Xoffset != Yoffset)))
            {
                Interval_Copy(X,Y,Xoffset,Yoffset,Ylength);
            }
        }
        else /* Xlength != Ylength */
        {
            if (Xlength > Ylength)
            {
                diff = Xlength - Ylength;
                if (Ylength > 0) Interval_Copy(X,Y,Xoffset,Yoffset,Ylength);
                if (limit < Xbits) Delete(X,Xoffset+Ylength,diff,false);
                if ((X = Resize(X,Xbits-diff)) == NULL) return(NULL);
            }
            else /* Ylength > Xlength  ==>  Ylength > 0 */
            {
                diff = Ylength - Xlength;
                if (X != Y)
                {
                    if ((X = Resize(X,Xbits+diff)) == NULL) return(NULL);
                    if (limit < Xbits) Insert(X,limit,diff,false);
                    Interval_Copy(X,Y,Xoffset,Yoffset,Ylength);
                }
                else /* in-place */
                {
                    if ((Y = X = Resize(X,Xbits+diff)) == NULL) return(NULL);
                    if (limit >= Xbits)
                    {
                        Interval_Copy(X,Y,Xoffset,Yoffset,Ylength);
                    }
                    else /* limit < Xbits */
                    {
                        Insert(X,limit,diff,false);
                        if ((Yoffset+Ylength) <= limit)
                        {
                            Interval_Copy(X,Y,Xoffset,Yoffset,Ylength);
                        }
                        else /* overlaps or lies above critical area */
                        {
                            if (limit <= Yoffset)
                            {
                                Yoffset += diff;
                                Interval_Copy(X,Y,Xoffset,Yoffset,Ylength);
                            }
                            else /* Yoffset < limit */
                            {
                                Xlength = limit - Yoffset;
                                Interval_Copy(X,Y,Xoffset,Yoffset,Xlength);
                                Yoffset = Xoffset + Ylength; /* = limit + diff */
                                Xoffset += Xlength;
                                Ylength -= Xlength;
                                Interval_Copy(X,Y,Xoffset,Yoffset,Ylength);
                            }
                        }
                    }
                }
            }
        }
    }
    return(X);
}

bool is_empty(wordptr addr)                    /* X == {} ?     */
{
    N_word  size = size_(addr);
    bool r = true;

    if (size > 0)
    {
        *(addr+size-1) &= mask_(addr);
        while (r && (size-- > 0)) r = ( *addr++ == 0 );
    }
    return(r);
}

bool is_full(wordptr addr)                     /* X == ~{} ?    */
{
    N_word  size = size_(addr);
    N_word  mask = mask_(addr);
    bool r = false;
    wordptr last;

    if (size > 0)
    {
        r = true;
        last = addr + size - 1;
        *last |= NOT mask;
        while (r && (size-- > 0)) r = ( NOT *addr++ == 0 );
        *last &= mask;
    }
    return(r);
}

bool equal(wordptr X, wordptr Y)               /* X == Y ?      */
{
    N_word  size = size_(X);
    N_word  mask = mask_(X);
    bool r = false;

    if (bits_(X) == bits_(Y))
    {
        r = true;
        if (size > 0)
        {
            *(X+size-1) &= mask;
            *(Y+size-1) &= mask;
            while (r && (size-- > 0)) r = (*X++ == *Y++);
        }
    }
    return(r);
}

Z_int Lexicompare(wordptr X, wordptr Y)           /* X <,=,> Y ?   */
{                                                           /*  unsigned     */
    N_word  bitsX = bits_(X);
    N_word  bitsY = bits_(Y);
    N_word  size  = size_(X);
    bool r = true;

    if (bitsX == bitsY)
    {
        if (size > 0)
        {
            X += size;
            Y += size;
            while (r && (size-- > 0)) r = (*(--X) == *(--Y));
        }
        if (r) return((Z_int) 0);
        else
        {
            if (*X < *Y) return((Z_int) -1); else return((Z_int) 1);
        }
    }
    else
    {
        if (bitsX < bitsY) return((Z_int) -1); else return((Z_int) 1);
    }
}

Z_int Compare(wordptr X, wordptr Y)               /* X <,=,> Y ?   */
{                                                           /*   signed      */
    N_word  bitsX = bits_(X);
    N_word  bitsY = bits_(Y);
    N_word  size  = size_(X);
    N_word  mask  = mask_(X);
    N_word  sign;
    bool r = true;

    if (bitsX == bitsY)
    {
        if (size > 0)
        {
            X += size;
            Y += size;
            mask &= NOT (mask >> 1);
            if ((sign = (*(X-1) AND mask)) != (*(Y-1) AND mask))
            {
                if (sign) return((Z_int) -1); else return((Z_int) 1);
            }
            while (r && (size-- > 0)) r = (*(--X) == *(--Y));
        }
        if (r) return((Z_int) 0);
        else
        {
            if (*X < *Y) return((Z_int) -1); else return((Z_int) 1);
        }
    }
    else
    {
        if (bitsX < bitsY) return((Z_int) -1); else return((Z_int) 1);
    }
}

charptr to_Hex(wordptr addr)
{
    N_word  bits = bits_(addr);
    N_word  size = size_(addr);
    N_word  value;
    N_word  count;
    N_word  digit;
    N_word  length;
    charptr string;

    length = bits >> 2;
    if (bits AND 0x0003) length++;
    string = (charptr) malloc((size_t) (length+1));
    if (string == NULL) return(NULL);
    string += length;
    *string = (N_char) '\0';
    if (size > 0)
    {
        *(addr+size-1) &= mask_(addr);
        while ((size-- > 0) && (length > 0))
        {
            value = *addr++;
            count = BITS >> 2;
            while ((count-- > 0) && (length > 0))
            {
                digit = value AND 0x000F;
                if (digit > 9) digit += (N_word) 'A' - 10;
                else           digit += (N_word) '0';
                *(--string) = (N_char) digit; length--;
                if ((count > 0) && (length > 0)) value >>= 4;
            }
        }
    }
    return(string);
}

ErrCode from_Hex(wordptr addr, charptr string)
{
    N_word  size = size_(addr);
    N_word  mask = mask_(addr);
    bool ok = true;
    size_t  length;
    N_word  value;
    N_word  count;
    int     digit;

    if (size > 0)
    {
        length = strlen((char *) string);
        string += length;
        while (size-- > 0)
        {
            value = 0;
            for ( count = 0; (ok && (length > 0) && (count < BITS)); count += 4 )
            {
                digit = (int) *(--string); length--;
                /* separate because toupper() is likely a macro! */
                digit = toupper(digit);
                if ((ok = (isxdigit(digit) != 0)))
                {
                    if (digit >= (int) 'A') digit -= (int) 'A' - 10;
                    else                    digit -= (int) '0';
                    value |= (((N_word) digit) << count);
                }
            }
            *addr++ = value;
        }
        *(--addr) &= mask;
    }
    if (ok) return(ErrCode_Ok);
    else    return(ErrCode_Pars);
}

ErrCode from_Oct(wordptr addr, charptr string)
{
    N_word  size = size_(addr);
    N_word  mask = mask_(addr);
    bool ok = true;
    size_t  length;
    N_word  value;
    N_word  value_fill = 0;
    N_word  count;
    Z_word  count_fill = 0;
    int     digit = 0;

    if (size > 0)
    {
        length = strlen((char *) string);
        string += length;
        while (size-- > 0)
        {
            value = value_fill;
            for ( count = count_fill; (ok && (length > 0) && (count < BITS)); count += 3 )
            {
                digit = (int) *(--string); length--;
                if ((ok = (isdigit(digit) && digit != '8' && digit != '9')) != 0)
                {
                    digit -= (int) '0';
                    value |= (((N_word) digit) << count);
                }
            }
            count_fill = (Z_word)count-(Z_word)BITS;
            if (count_fill > 0)
                value_fill = (((N_word) digit) >> (3-count_fill));
            else
                value_fill = 0;
            *addr++ = value;
        }
        *(--addr) &= mask;
    }
    if (ok) return(ErrCode_Ok);
    else    return(ErrCode_Pars);
}

charptr to_Bin(wordptr addr)
{
    N_word  size = size_(addr);
    N_word  value;
    N_word  count;
    N_word  digit;
    N_word  length;
    charptr string;

    length = bits_(addr);
    string = (charptr) malloc((size_t) (length+1));
    if (string == NULL) return(NULL);
    string += length;
    *string = (N_char) '\0';
    if (size > 0)
    {
        *(addr+size-1) &= mask_(addr);
        while (size-- > 0)
        {
            value = *addr++;
            count = BITS;
            if (count > length) count = length;
            while (count-- > 0)
            {
                digit = value AND 0x0001;
                digit += (N_word) '0';
                *(--string) = (N_char) digit; length--;
                if (count > 0) value >>= 1;
            }
        }
    }
    return(string);
}

ErrCode from_Bin(wordptr addr, charptr string)
{
    N_word  size = size_(addr);
    N_word  mask = mask_(addr);
    bool ok = true;
    size_t  length;
    N_word  value;
    N_word  count;
    int     digit;

    if (size > 0)
    {
        length = strlen((char *) string);
        string += length;
        while (size-- > 0)
        {
            value = 0;
            for ( count = 0; (ok && (length > 0) && (count < BITS)); count++ )
            {
                digit = (int) *(--string); length--;
                switch (digit)
                {
                    case (int) '0':
                        break;
                    case (int) '1':
                        value |= BITMASKTAB[count];
                        break;
                    default:
                        ok = false;
                        break;
                }
            }
            *addr++ = value;
        }
        *(--addr) &= mask;
    }
    if (ok) return(ErrCode_Ok);
    else    return(ErrCode_Pars);
}

charptr to_Dec(wordptr addr)
{
    N_word  bits = bits_(addr);
    N_word  length;
    N_word  digits;
    N_word  count;
    N_word  q;
    N_word  r;
    bool loop;
    charptr result;
    charptr string;
    wordptr quot;
    wordptr rest;
    wordptr temp;
    wordptr base;
    Z_int   sign;

    length = (N_word) (bits / 3.3);        /* digits = bits * ln(2) / ln(10) */
    length += 2; /* compensate for truncating & provide space for minus sign */
    result = (charptr) malloc((size_t) (length+1));   /* remember the '\0'! */
    if (result == NULL) return(NULL);
    string = result;
    sign = Sign(addr);
    if ((bits < 4) || (sign == 0))
    {
        if (bits > 0) digits = *addr; else digits = (N_word) 0;
        if (sign < 0) digits = ((N_word)(-((Z_word)digits))) AND mask_(addr);
        *string++ = (N_char) digits + (N_char) '0';
        digits = 1;
    }
    else
    {
        quot = Create(bits,false);
        if (quot == NULL)
        {
            Dispose(result);
            return(NULL);
        }
        rest = Create(bits,false);
        if (rest == NULL)
        {
            Dispose(result);
            Destroy(quot);
            return(NULL);
        }
        temp = Create(bits,false);
        if (temp == NULL)
        {
            Dispose(result);
            Destroy(quot);
            Destroy(rest);
            return(NULL);
        }
        base = Create(bits,true);
        if (base == NULL)
        {
            Dispose(result);
            Destroy(quot);
            Destroy(rest);
            Destroy(temp);
            return(NULL);
        }
        if (sign < 0) Negate(quot,addr);
        else           Copy(quot,addr);
        digits = 0;
        *base = EXP10;
        loop = (bits >= BITS);
        do
        {
            if (loop)
            {
                Copy(temp,quot);
                if (Div_Pos(quot,temp,base,rest))
                {
                    Dispose(result); /* emergency exit */
                    Destroy(quot);
                    Destroy(rest);   /* should never occur */
                    Destroy(temp);   /* under normal operation */
                    Destroy(base);
                    return(NULL);
                }
                loop = !is_empty(quot);
                q = *rest;
            }
            else q = *quot;
            count = LOG10;
            while (((loop && (count-- > 0)) || ((!loop) && (q != 0))) &&
                (digits < length))
            {
                if (q != 0)
                {
                    DIGITIZE(N_word,q,r)
                }
                else r = (N_word) '0';
                *string++ = (N_char) r;
                digits++;
            }
        }
        while (loop && (digits < length));
        Destroy(quot);
        Destroy(rest);
        Destroy(temp);
        Destroy(base);
    }
    if ((sign < 0) && (digits < length))
    {
        *string++ = (N_char) '-';
        digits++;
    }
    *string = (N_char) '\0';
    reverse(result,digits);
    return(result);
}

struct from_Dec_static_data {
    wordptr term;
    wordptr base;
    wordptr prod;
    wordptr rank;
    wordptr temp;
};

from_Dec_static_data *from_Dec_static_Boot(N_word bits)
{
    from_Dec_static_data *data;

    data = (from_Dec_static_data *)malloc(sizeof(from_Dec_static_data));

    if (bits > 0)
    {
        data->term = Create(BITS,false);
        data->base = Create(BITS,false);
        data->prod = Create(bits,false);
        data->rank = Create(bits,false);
        data->temp = Create(bits,false);
    } else {
        data->term = NULL;
        data->base = NULL;
        data->prod = NULL;
        data->rank = NULL;
        data->temp = NULL;
    }
    return data;
}

void from_Dec_static_Shutdown(from_Dec_static_data *data)
{
    if (data) {
        Destroy(data->term);
        Destroy(data->base);
        Destroy(data->prod);
        Destroy(data->rank);
        Destroy(data->temp);
    }
    free(data);
}

ErrCode from_Dec_static(from_Dec_static_data *data,
                                  wordptr addr, charptr string)
{
    ErrCode error = ErrCode_Ok;
    N_word  bits = bits_(addr);
    N_word  mask = mask_(addr);
    bool init = (bits > BITS);
    bool minus;
    bool shift;
    bool carry;
    wordptr term;
    wordptr base;
    wordptr prod;
    wordptr rank;
    wordptr temp;
    N_word  accu;
    N_word  powr;
    N_word  count;
    size_t  length;
    int     digit;

    if (bits > 0)
    {
        term = data->term;
        base = data->base;
        prod = data->prod;
        rank = data->rank;
        temp = data->temp;

        length = strlen((char *) string);
        if (length == 0) return(ErrCode_Pars);
        digit = (int) *string;
        if ((minus = (digit == (int) '-')) ||
                     (digit == (int) '+'))
        {
            string++;
            if (--length == 0) return(ErrCode_Pars);
        }
        string += length;
        if (init)
        {
            Empty(prod);
            Empty(rank);
        }
        Empty(addr);
        *base = EXP10;
        shift = false;
        while ((!error) && (length > 0))
        {
            accu = 0;
            powr = 1;
            count = LOG10;
            while ((!error) && (length > 0) && (count-- > 0))
            {
                digit = (int) *(--string); length--;
                /* separate because isdigit() is likely a macro! */
                if (isdigit(digit) != 0)
                {
                    accu += ((N_word) digit - (N_word) '0') * powr;
                    powr *= 10;
                }
                else error = ErrCode_Pars;
            }
            if (!error)
            {
                if (shift)
                {
                    *term = accu;
                    Copy(temp,rank);
                    error = Mul_Pos(prod,temp,term,false);
                }
                else
                {
                    *prod = accu;
                    if ((!init) && ((accu AND NOT mask) != 0)) error = ErrCode_Ovfl;
                }
                if (!error)
                {
                    carry = false;
                    compute(addr,addr,prod,false,&carry);
                    /* ignores sign change (= overflow) but not */
                    /* numbers too large (= carry) for resulting bit vector */
                    if (carry) error = ErrCode_Ovfl;
                    else
                    {
                        if (length > 0)
                        {
                            if (shift)
                            {
                                Copy(temp,rank);
                                error = Mul_Pos(rank,temp,base,false);
                            }
                            else
                            {
                                *rank = *base;
                                shift = true;
                            }
                        }
                    }
                }
            }
        }
        if (!error && minus)
        {
            Negate(addr,addr);
            if ((*(addr + size_(addr) - 1) AND mask AND NOT (mask >> 1)) == 0)
                error = ErrCode_Ovfl;
        }
    }
    return(error);
}

ErrCode from_Dec(wordptr addr, charptr string)
{
    ErrCode error = ErrCode_Ok;
    N_word  bits = bits_(addr);
    N_word  mask = mask_(addr);
    bool init = (bits > BITS);
    bool minus;
    bool shift;
    bool carry;
    wordptr term;
    wordptr base;
    wordptr prod;
    wordptr rank;
    wordptr temp;
    N_word  accu;
    N_word  powr;
    N_word  count;
    size_t  length;
    int     digit;

    if (bits > 0)
    {
        length = strlen((char *) string);
        if (length == 0) return(ErrCode_Pars);
        digit = (int) *string;
        if ((minus = (digit == (int) '-')) ||
                     (digit == (int) '+'))
        {
            string++;
            if (--length == 0) return(ErrCode_Pars);
        }
        string += length;
        term = Create(BITS,false);
        if (term == NULL)
        {
            return(ErrCode_Null);
        }
        base = Create(BITS,false);
        if (base == NULL)
        {
            Destroy(term);
            return(ErrCode_Null);
        }
        prod = Create(bits,init);
        if (prod == NULL)
        {
            Destroy(term);
            Destroy(base);
            return(ErrCode_Null);
        }
        rank = Create(bits,init);
        if (rank == NULL)
        {
            Destroy(term);
            Destroy(base);
            Destroy(prod);
            return(ErrCode_Null);
        }
        temp = Create(bits,false);
        if (temp == NULL)
        {
            Destroy(term);
            Destroy(base);
            Destroy(prod);
            Destroy(rank);
            return(ErrCode_Null);
        }
        Empty(addr);
        *base = EXP10;
        shift = false;
        while ((!error) && (length > 0))
        {
            accu = 0;
            powr = 1;
            count = LOG10;
            while ((!error) && (length > 0) && (count-- > 0))
            {
                digit = (int) *(--string); length--;
                /* separate because isdigit() is likely a macro! */
                if (isdigit(digit) != 0)
                {
                    accu += ((N_word) digit - (N_word) '0') * powr;
                    powr *= 10;
                }
                else error = ErrCode_Pars;
            }
            if (!error)
            {
                if (shift)
                {
                    *term = accu;
                    Copy(temp,rank);
                    error = Mul_Pos(prod,temp,term,false);
                }
                else
                {
                    *prod = accu;
                    if ((!init) && ((accu AND NOT mask) != 0)) error = ErrCode_Ovfl;
                }
                if (!error)
                {
                    carry = false;
                    compute(addr,addr,prod,false,&carry);
                    /* ignores sign change (= overflow) but not */
                    /* numbers too large (= carry) for resulting bit vector */
                    if (carry) error = ErrCode_Ovfl;
                    else
                    {
                        if (length > 0)
                        {
                            if (shift)
                            {
                                Copy(temp,rank);
                                error = Mul_Pos(rank,temp,base,false);
                            }
                            else
                            {
                                *rank = *base;
                                shift = true;
                            }
                        }
                    }
                }
            }
        }
        Destroy(term);
        Destroy(base);
        Destroy(prod);
        Destroy(rank);
        Destroy(temp);
        if (!error && minus)
        {
            Negate(addr,addr);
            if ((*(addr + size_(addr) - 1) AND mask AND NOT (mask >> 1)) == 0)
                error = ErrCode_Ovfl;
        }
    }
    return(error);
}

charptr to_Enum(wordptr addr)
{
    N_word  bits = bits_(addr);
    N_word  sample;
    N_word  length;
    N_word  digits;
    N_word  factor;
    N_word  power;
    N_word  start;
    N_word  min;
    N_word  max;
    charptr string;
    charptr target;
    bool comma;

    if (bits > 0)
    {
        sample = bits - 1;  /* greatest possible index */
        length = 2;         /* account for index 0 and terminating '\0' */
        digits = 1;         /* account for intervening dashes and commas */
        factor = 1;
        power = 10;
        while (sample >= (power-1))
        {
            length += ++digits * factor * 6;  /* 9,90,900,9000,... (9*2/3 = 6) */
            factor = power;
            power *= 10;
        }
        if (sample > --factor)
        {
            sample -= factor;
            factor = (N_word) ( sample / 3 );
            factor = (factor << 1) + (sample - (factor * 3));
            length += ++digits * factor;
        }
    }
    else length = 1;
    string = (charptr) malloc((size_t) length);
    if (string == NULL) return(NULL);
    start = 0;
    comma = false;
    target = string;
    while ((start < bits) && interval_scan_inc(addr,start,&min,&max))
    {
        start = max + 2;
        if (comma) *target++ = (N_char) ',';
        if (min == max)
        {
            target += int2str(target,min);
        }
        else
        {
            if (min+1 == max)
            {
                target += int2str(target,min);
                *target++ = (N_char) ',';
                target += int2str(target,max);
            }
            else
            {
                target += int2str(target,min);
                *target++ = (N_char) '-';
                target += int2str(target,max);
            }
        }
        comma = true;
    }
    *target = (N_char) '\0';
    return(string);
}

ErrCode from_Enum(wordptr addr, charptr string)
{
    ErrCode error = ErrCode_Ok;
    N_word  bits = bits_(addr);
    N_word  state = 1;
    N_word  token;
    N_word  indx = 0;          /* silence compiler warning */
    N_word  start = 0;         /* silence compiler warning */

    if (bits > 0)
    {
        Empty(addr);
        while ((!error) && (state != 0))
        {
            token = (N_word) *string;
            /* separate because isdigit() is likely a macro! */
            if (isdigit((int)token) != 0)
            {
                string += str2int(string,&indx);
                if (indx < bits) token = (N_word) '0';
                else error = ErrCode_Indx;
            }
            else string++;
            if (!error)
            switch (state)
            {
                case 1:
                    switch (token)
                    {
                        case (N_word) '0':
                            state = 2;
                            break;
                        case (N_word) '\0':
                            state = 0;
                            break;
                        default:
                            error = ErrCode_Pars;
                            break;
                    }
                    break;
                case 2:
                    switch (token)
                    {
                        case (N_word) '-':
                            start = indx;
                            state = 3;
                            break;
                        case (N_word) ',':
                            SET_BIT(addr,indx)
                            state = 5;
                            break;
                        case (N_word) '\0':
                            SET_BIT(addr,indx)
                            state = 0;
                            break;
                        default:
                            error = ErrCode_Pars;
                            break;
                    }
                    break;
                case 3:
                    switch (token)
                    {
                        case (N_word) '0':
                            if (start < indx)
                                Interval_Fill(addr,start,indx);
                            else if (start == indx)
                                SET_BIT(addr,indx)
                            else error = ErrCode_Ordr;
                            state = 4;
                            break;
                        default:
                            error = ErrCode_Pars;
                            break;
                    }
                    break;
                case 4:
                    switch (token)
                    {
                        case (N_word) ',':
                            state = 5;
                            break;
                        case (N_word) '\0':
                            state = 0;
                            break;
                        default:
                            error = ErrCode_Pars;
                            break;
                    }
                    break;
                case 5:
                    switch (token)
                    {
                        case (N_word) '0':
                            state = 2;
                            break;
                        default:
                            error = ErrCode_Pars;
                            break;
                    }
                    break;
            }
        }
    }
    return(error);
}

void Bit_Off(wordptr addr, N_int indx)            /* X = X \ {x}   */
{
    if (indx < bits_(addr)) CLR_BIT(addr,indx)
}

void Bit_On(wordptr addr, N_int indx)             /* X = X + {x}   */
{
    if (indx < bits_(addr)) SET_BIT(addr,indx)
}

bool bit_flip(wordptr addr, N_int indx)    /* X=(X+{x})\(X*{x}) */
{
    N_word mask;

    if (indx < bits_(addr)) return( FLP_BIT(addr,indx,mask) );
    else                    return( false );
}

bool bit_test(wordptr addr, N_int indx)        /* {x} in X ?    */
{
    if (indx < bits_(addr)) return( TST_BIT(addr,indx) );
    else                    return( false );
}

void Bit_Copy(wordptr addr, N_int indx, bool bit)
{
    if (indx < bits_(addr))
    {
        if (bit) SET_BIT(addr,indx)
        else     CLR_BIT(addr,indx)
    }
}

void LSB(wordptr addr, bool bit)
{
    if (bits_(addr) > 0)
    {
        if (bit) *addr |= LSBMASK;
        else     *addr &= NOT LSBMASK;
    }
}

void MSB(wordptr addr, bool bit)
{
    N_word size = size_(addr);
    N_word mask = mask_(addr);

    if (size-- > 0)
    {
        if (bit) *(addr+size) |= mask AND NOT (mask >> 1);
        else     *(addr+size) &= NOT mask OR (mask >> 1);
    }
}

bool lsb_(wordptr addr)
{
    if (size_(addr) > 0) return( (*addr AND LSBMASK) != 0 );
    else                 return( false );
}

bool msb_(wordptr addr)
{
    N_word size = size_(addr);
    N_word mask = mask_(addr);

    if (size-- > 0)
        return( (*(addr+size) AND (mask AND NOT (mask >> 1))) != 0 );
    else
        return( false );
}

bool rotate_left(wordptr addr)
{
    N_word  size = size_(addr);
    N_word  mask = mask_(addr);
    N_word  msb;
    bool carry_in;
    bool carry_out = false;

    if (size > 0)
    {
        msb = mask AND NOT (mask >> 1);
        carry_in = ((*(addr+size-1) AND msb) != 0);
        while (size-- > 1)
        {
            carry_out = ((*addr AND MSBMASK) != 0);
            *addr <<= 1;
            if (carry_in) *addr |= LSBMASK;
            carry_in = carry_out;
            addr++;
        }
        carry_out = ((*addr AND msb) != 0);
        *addr <<= 1;
        if (carry_in) *addr |= LSBMASK;
        *addr &= mask;
    }
    return(carry_out);
}

bool rotate_right(wordptr addr)
{
    N_word  size = size_(addr);
    N_word  mask = mask_(addr);
    N_word  msb;
    bool carry_in;
    bool carry_out = false;

    if (size > 0)
    {
        msb = mask AND NOT (mask >> 1);
        carry_in = ((*addr AND LSBMASK) != 0);
        addr += size-1;
        *addr &= mask;
        carry_out = ((*addr AND LSBMASK) != 0);
        *addr >>= 1;
        if (carry_in) *addr |= msb;
        carry_in = carry_out;
        addr--;
        size--;
        while (size-- > 0)
        {
            carry_out = ((*addr AND LSBMASK) != 0);
            *addr >>= 1;
            if (carry_in) *addr |= MSBMASK;
            carry_in = carry_out;
            addr--;
        }
    }
    return(carry_out);
}

bool shift_left(wordptr addr, bool carry_in)
{
    N_word  size = size_(addr);
    N_word  mask = mask_(addr);
    N_word  msb;
    bool carry_out = carry_in;

    if (size > 0)
    {
        msb = mask AND NOT (mask >> 1);
        while (size-- > 1)
        {
            carry_out = ((*addr AND MSBMASK) != 0);
            *addr <<= 1;
            if (carry_in) *addr |= LSBMASK;
            carry_in = carry_out;
            addr++;
        }
        carry_out = ((*addr AND msb) != 0);
        *addr <<= 1;
        if (carry_in) *addr |= LSBMASK;
        *addr &= mask;
    }
    return(carry_out);
}

bool shift_right(wordptr addr, bool carry_in)
{
    N_word  size = size_(addr);
    N_word  mask = mask_(addr);
    N_word  msb;
    bool carry_out = carry_in;

    if (size > 0)
    {
        msb = mask AND NOT (mask >> 1);
        addr += size-1;
        *addr &= mask;
        carry_out = ((*addr AND LSBMASK) != 0);
        *addr >>= 1;
        if (carry_in) *addr |= msb;
        carry_in = carry_out;
        addr--;
        size--;
        while (size-- > 0)
        {
            carry_out = ((*addr AND LSBMASK) != 0);
            *addr >>= 1;
            if (carry_in) *addr |= MSBMASK;
            carry_in = carry_out;
            addr--;
        }
    }
    return(carry_out);
}

void Move_Left(wordptr addr, N_int bits)
{
    N_word count;
    N_word words;

    if (bits > 0)
    {
        count = bits AND MODMASK;
        words = bits / BITS;
        if (bits >= bits_(addr)) Empty(addr);
        else
        {
            while (count-- > 0) shift_left(addr,0);
            Word_Insert(addr,0,words,true);
        }
    }
}

void Move_Right(wordptr addr, N_int bits)
{
    N_word count;
    N_word words;

    if (bits > 0)
    {
        count = bits AND MODMASK;
        words = bits / BITS;
        if (bits >= bits_(addr)) Empty(addr);
        else
        {
            while (count-- > 0) shift_right(addr,0);
            Word_Delete(addr,0,words,true);
        }
    }
}

void Insert(wordptr addr, N_int offset, N_int count, bool clear)
{
    N_word bits = bits_(addr);
    N_word last;

    if ((count > 0) && (offset < bits))
    {
        last = offset + count;
        if (last < bits)
        {
            Interval_Copy(addr,addr,last,offset,(bits-last));
        }
        else last = bits;
        if (clear) Interval_Empty(addr,offset,(last-1));
    }
}

void Delete(wordptr addr, N_int offset, N_int count, bool clear)
{
    N_word bits = bits_(addr);
    N_word last;

    if ((count > 0) && (offset < bits))
    {
        last = offset + count;
        if (last < bits)
        {
            Interval_Copy(addr,addr,offset,last,(bits-last));
        }
        else count = bits - offset;
        if (clear) Interval_Empty(addr,(bits-count),(bits-1));
    }
}

bool increment(wordptr addr)                   /* X++           */
{
    N_word  size  = size_(addr);
    N_word  mask  = mask_(addr);
    wordptr last  = addr + size - 1;
    bool carry = true;

    if (size > 0)
    {
        *last |= NOT mask;
        while (carry && (size-- > 0))
        {
            carry = (++(*addr++) == 0);
        }
        *last &= mask;
    }
    return(carry);
}

bool decrement(wordptr addr)                   /* X--           */
{
    N_word  size  = size_(addr);
    N_word  mask  = mask_(addr);
    wordptr last  = addr + size - 1;
    bool carry = true;

    if (size > 0)
    {
        *last &= mask;
        while (carry && (size-- > 0))
        {
            carry = (*addr == 0);
            --(*addr++);
        }
        *last &= mask;
    }
    return(carry);
}

bool compute(wordptr X, wordptr Y, wordptr Z, bool minus, bool *carry)
{
    N_word size = size_(X);
    N_word mask = mask_(X);
    N_word vv = 0;
    N_word cc;
    N_word mm;
    N_word yy;
    N_word zz;
    N_word lo;
    N_word hi;

    if (size > 0)
    {
        if (minus) cc = (*carry == 0);
        else       cc = (*carry != 0);
        /* deal with (size-1) least significant full words first: */
        while (--size > 0)
        {
            yy = *Y++;
            if (minus) zz = (N_word) NOT ( Z ? *Z++ : 0 );
            else       zz = (N_word)     ( Z ? *Z++ : 0 );
            lo = (yy AND LSBMASK) + (zz AND LSBMASK) + cc;
            hi = (yy >> 1) + (zz >> 1) + (lo >> 1);
            cc = ((hi AND MSBMASK) != 0);
            *X++ = (hi << 1) OR (lo AND LSBMASK);
        }
        /* deal with most significant word (may be used only partially): */
        yy = *Y AND mask;
        if (minus) zz = (N_word) NOT ( Z ? *Z : 0 );
        else       zz = (N_word)     ( Z ? *Z : 0 );
        zz &= mask;
        if (mask == LSBMASK) /* special case, only one bit used */
        {
            vv = cc;
            lo = yy + zz + cc;
            cc = (lo >> 1);
            vv ^= cc;
            *X = lo AND LSBMASK;
        }
        else
        {
            if (NOT mask) /* not all bits are used, but more than one */
            {
                mm = (mask >> 1);
                vv = (yy AND mm) + (zz AND mm) + cc;
                mm = mask AND NOT mm;
                lo = yy + zz + cc;
                cc = (lo >> 1);
                vv ^= cc;
                vv &= mm;
                cc &= mm;
                *X = lo AND mask;
            }
            else /* other special case, all bits are used */
            {
                mm = NOT MSBMASK;
                lo = (yy AND mm) + (zz AND mm) + cc;
                vv = lo AND MSBMASK;
                hi = ((yy AND MSBMASK) >> 1) + ((zz AND MSBMASK) >> 1) + (vv >> 1);
                cc = hi AND MSBMASK;
                vv ^= cc;
                *X = (hi << 1) OR (lo AND mm);
            }
        }
        if (minus) *carry = (cc == 0);
        else       *carry = (cc != 0);
    }
    return(vv != 0);
}

bool add(wordptr X, wordptr Y, wordptr Z, bool *carry)
{
    return(compute(X,Y,Z,false,carry));
}

bool sub(wordptr X, wordptr Y, wordptr Z, bool *carry)
{
    return(compute(X,Y,Z,true,carry));
}

bool inc(wordptr X, wordptr Y)
{
    bool carry = true;

    return(compute(X,Y,NULL,false,&carry));
}

bool dec(wordptr X, wordptr Y)
{
    bool carry = true;

    return(compute(X,Y,NULL,true,&carry));
}

void Negate(wordptr X, wordptr Y)
{
    N_word  size  = size_(X);
    N_word  mask  = mask_(X);
    bool carry = true;

    if (size > 0)
    {
        while (size-- > 0)
        {
            *X = NOT *Y++;
            if (carry)
            {
                carry = (++(*X) == 0);
            }
            X++;
        }
        *(--X) &= mask;
    }
}

void Absolute(wordptr X, wordptr Y)
{
    N_word size = size_(Y);
    N_word mask = mask_(Y);

    if (size > 0)
    {
        if (*(Y+size-1) AND (mask AND NOT (mask >> 1))) Negate(X,Y);
        else                                             Copy(X,Y);
    }
}

Z_int Sign(wordptr addr)
{
    N_word  size = size_(addr);
    N_word  mask = mask_(addr);
    wordptr last = addr + size - 1;
    bool r    = true;

    if (size > 0)
    {
        *last &= mask;
        while (r && (size-- > 0)) r = ( *addr++ == 0 );
    }
    if (r) return((Z_int) 0);
    else
    {
        if (*last AND (mask AND NOT (mask >> 1))) return((Z_int) -1);
        else                                      return((Z_int)  1);
    }
}

ErrCode Mul_Pos(wordptr X, wordptr Y, wordptr Z, bool strict)
{
    N_word  mask;
    N_word  limit;
    N_word  count;
    Z_long  last;
    wordptr sign;
    bool carry;
    bool overflow;
    bool ok = true;

    /*
       Requirements:
         -  X, Y and Z must be distinct
         -  X and Y must have equal sizes (whereas Z may be any size!)
         -  Z should always contain the SMALLER of the two factors Y and Z
       Constraints:
         -  The contents of Y (and of X, of course) are destroyed
            (only Z is preserved!)
    */

    if ((X == Y) || (X == Z) || (Y == Z)) return(ErrCode_Same);
    if (bits_(X) != bits_(Y)) return(ErrCode_Size);
    Empty(X);
    if (is_empty(Y)) return(ErrCode_Ok); /* exit also taken if bits_(Y)==0 */
    if ((last = Set_Max(Z)) < 0L) return(ErrCode_Ok);
    limit = (N_word) last;
    sign = Y + size_(Y) - 1;
    mask = mask_(Y);
    *sign &= mask;
    mask &= NOT (mask >> 1);
    for ( count = 0; (ok && (count <= limit)); count++ )
    {
        if ( TST_BIT(Z,count) )
        {
            carry = false;
            overflow = compute(X,X,Y,false,&carry);
            if (strict) ok = !(carry || overflow);
            else        ok = ! carry;
        }
        if (ok && (count < limit))
        {
            carry = shift_left(Y,0);
            if (strict)
            {
                overflow = ((*sign AND mask) != 0);
                ok = !(carry || overflow);
            }
            else ok = !carry;
        }
    }
    if (ok) return(ErrCode_Ok); else return(ErrCode_Ovfl);
}

ErrCode Multiply(wordptr X, wordptr Y, wordptr Z)
{
    ErrCode error = ErrCode_Ok;
    N_word  bit_x = bits_(X);
    N_word  bit_y = bits_(Y);
    N_word  bit_z = bits_(Z);
    N_word  size;
    N_word  mask;
    N_word  msb;
    wordptr ptr_y;
    wordptr ptr_z;
    bool sgn_x;
    bool sgn_y;
    bool sgn_z;
    bool zero;
    wordptr A;
    wordptr B;

    /*
       Requirements:
         -  Y and Z must have equal sizes
         -  X must have at least the same size as Y and Z but may be larger (!)
       Features:
         -  The contents of Y and Z are preserved
         -  X may be identical with Y or Z (or both!)
            (in-place multiplication is possible!)
    */

    if ((bit_y != bit_z) || (bit_x < bit_y)) return(ErrCode_Size);
    if (is_empty(Y) || is_empty(Z))
    {
        Empty(X);
    }
    else
    {
        A = Create(bit_y,false);
        if (A == NULL) return(ErrCode_Null);
        B = Create(bit_z,false);
        if (B == NULL) { Destroy(A); return(ErrCode_Null); }
        size  = size_(Y);
        mask  = mask_(Y);
        msb   = (mask AND NOT (mask >> 1));
        sgn_y = (((*(Y+size-1) &= mask) AND msb) != 0);
        sgn_z = (((*(Z+size-1) &= mask) AND msb) != 0);
        sgn_x = sgn_y XOR sgn_z;
        if (sgn_y) Negate(A,Y); else Copy(A,Y);
        if (sgn_z) Negate(B,Z); else Copy(B,Z);
        ptr_y = A + size;
        ptr_z = B + size;
        zero = true;
        while (zero && (size-- > 0))
        {
            zero &= (*(--ptr_y) == 0);
            zero &= (*(--ptr_z) == 0);
        }
        if (*ptr_y > *ptr_z)
        {
            if (bit_x > bit_y)
            {
                A = Resize(A,bit_x);
                if (A == NULL) { Destroy(B); return(ErrCode_Null); }
            }
            error = Mul_Pos(X,A,B,true);
        }
        else
        {
            if (bit_x > bit_z)
            {
                B = Resize(B,bit_x);
                if (B == NULL) { Destroy(A); return(ErrCode_Null); }
            }
            error = Mul_Pos(X,B,A,true);
        }
        if ((!error) && sgn_x) Negate(X,X);
        Destroy(A);
        Destroy(B);
    }
    return(error);
}

ErrCode Div_Pos(wordptr Q, wordptr X, wordptr Y, wordptr R)
{
    N_word  bits = bits_(Q);
    N_word  mask;
    wordptr addr;
    Z_long  last;
    bool flag;
    bool copy = false; /* flags whether valid rest is in R (0) or X (1) */

    /*
       Requirements:
         -  All bit vectors must have equal sizes
         -  Q, X, Y and R must all be distinct bit vectors
         -  Y must be non-zero (of course!)
       Constraints:
         -  The contents of X (and Q and R, of course) are destroyed
            (only Y is preserved!)
    */

    if ((bits != bits_(X)) || (bits != bits_(Y)) || (bits != bits_(R)))
        return(ErrCode_Size);
    if ((Q == X) || (Q == Y) || (Q == R) || (X == Y) || (X == R) || (Y == R))
        return(ErrCode_Same);
    if (is_empty(Y))
        return(ErrCode_Zero);

    Empty(R);
    Copy(Q,X);
    if ((last = Set_Max(Q)) < 0L) return(ErrCode_Ok);
    bits = (N_word) ++last;
    while (bits-- > 0)
    {
        addr = Q + (bits / BITS);
        mask = BITMASKTAB[bits AND MODMASK];
        flag = ((*addr AND mask) != 0);
        if (copy)
        {
            shift_left(X,flag);
            flag = false;
            compute(R,X,Y,true,&flag);
        }
        else
        {
            shift_left(R,flag);
            flag = false;
            compute(X,R,Y,true,&flag);
        }
        if (flag) *addr &= NOT mask;
        else
        {
            *addr |= mask;
            copy = !copy;
        }
    }
    if (copy) Copy(R,X);
    return(ErrCode_Ok);
}

ErrCode Divide(wordptr Q, wordptr X, wordptr Y, wordptr R)
{
    ErrCode error = ErrCode_Ok;
    N_word  bits = bits_(Q);
    N_word  size = size_(Q);
    N_word  mask = mask_(Q);
    N_word  msb = (mask AND NOT (mask >> 1));
    bool sgn_q;
    bool sgn_x;
    bool sgn_y;
    wordptr A;
    wordptr B;

    /*
       Requirements:
         -  All bit vectors must have equal sizes
         -  Q and R must be two distinct bit vectors
         -  Y must be non-zero (of course!)
       Features:
         -  The contents of X and Y are preserved
         -  Q may be identical with X or Y (or both)
            (in-place division is possible!)
         -  R may be identical with X or Y (or both)
            (but not identical with Q!)
    */

    if ((bits != bits_(X)) || (bits != bits_(Y)) || (bits != bits_(R)))
        return(ErrCode_Size);
    if (Q == R)
        return(ErrCode_Same);
    if (is_empty(Y))
        return(ErrCode_Zero);

    if (is_empty(X))
    {
        Empty(Q);
        Empty(R);
    }
    else
    {
        A = Create(bits,false);
        if (A == NULL) return(ErrCode_Null);
        B = Create(bits,false);
        if (B == NULL) { Destroy(A); return(ErrCode_Null); }
        size--;
        sgn_x = (((*(X+size) &= mask) AND msb) != 0);
        sgn_y = (((*(Y+size) &= mask) AND msb) != 0);
        sgn_q = sgn_x XOR sgn_y;
        if (sgn_x) Negate(A,X); else Copy(A,X);
        if (sgn_y) Negate(B,Y); else Copy(B,Y);
        if (!(error = Div_Pos(Q,A,B,R)))
        {
            if (sgn_q) Negate(Q,Q);
            if (sgn_x) Negate(R,R);
        }
        Destroy(A);
        Destroy(B);
    }
    return(error);
}

ErrCode GCD(wordptr X, wordptr Y, wordptr Z)
{
    ErrCode error = ErrCode_Ok;
    N_word  bits = bits_(X);
    N_word  size = size_(X);
    N_word  mask = mask_(X);
    N_word  msb = (mask AND NOT (mask >> 1));
    bool sgn_a;
    bool sgn_b;
    bool sgn_r;
    wordptr Q;
    wordptr R;
    wordptr A;
    wordptr B;
    wordptr T;

    /*
       Requirements:
         -  All bit vectors must have equal sizes
       Features:
         -  The contents of Y and Z are preserved
         -  X may be identical with Y or Z (or both)
            (in-place is possible!)
         -  GCD(0,z) == GCD(z,0) == z
         -  negative values are handled correctly
    */

    if ((bits != bits_(Y)) || (bits != bits_(Z))) return(ErrCode_Size);
    if (is_empty(Y))
    {
        if (X != Z) Copy(X,Z);
        return(ErrCode_Ok);
    }
    if (is_empty(Z))
    {
        if (X != Y) Copy(X,Y);
        return(ErrCode_Ok);
    }
    Q = Create(bits,false);
    if (Q == NULL)
    {
        return(ErrCode_Null);
    }
    R = Create(bits,false);
    if (R == NULL)
    {
        Destroy(Q);
        return(ErrCode_Null);
    }
    A = Create(bits,false);
    if (A == NULL)
    {
        Destroy(Q);
        Destroy(R);
        return(ErrCode_Null);
    }
    B = Create(bits,false);
    if (B == NULL)
    {
        Destroy(Q);
        Destroy(R);
        Destroy(A);
        return(ErrCode_Null);
    }
    size--;
    sgn_a = (((*(Y+size) &= mask) AND msb) != 0);
    sgn_b = (((*(Z+size) &= mask) AND msb) != 0);
    if (sgn_a) Negate(A,Y); else Copy(A,Y);
    if (sgn_b) Negate(B,Z); else Copy(B,Z);
    while (!error)
    {
        if (!(error = Div_Pos(Q,A,B,R)))
        {
            if (is_empty(R)) break;
            T = A; sgn_r = sgn_a;
            A = B; sgn_a = sgn_b;
            B = R; sgn_b = sgn_r;
            R = T;
        }
    }
    if (!error)
    {
        if (sgn_b) Negate(X,B); else Copy(X,B);
    }
    Destroy(Q);
    Destroy(R);
    Destroy(A);
    Destroy(B);
    return(error);
}

ErrCode GCD2(wordptr U, wordptr V, wordptr W, wordptr X, wordptr Y)
{
    ErrCode error = ErrCode_Ok;
    N_word  bits = bits_(U);
    N_word  size = size_(U);
    N_word  mask = mask_(U);
    N_word  msb = (mask AND NOT (mask >> 1));
    bool minus;
    bool carry;
    bool sgn_q;
    bool sgn_r;
    bool sgn_a;
    bool sgn_b;
    bool sgn_x;
    bool sgn_y;
    listptr L;
    wordptr Q;
    wordptr R;
    wordptr A;
    wordptr B;
    wordptr T;
    wordptr X1;
    wordptr X2;
    wordptr X3;
    wordptr Y1;
    wordptr Y2;
    wordptr Y3;
    wordptr Z;

    /*
       Requirements:
         -  All bit vectors must have equal sizes
         -  U, V, and W must all be distinct bit vectors
       Features:
         -  The contents of X and Y are preserved
         -  U, V and W may be identical with X or Y (or both,
            provided that U, V and W are mutually distinct)
            (i.e., in-place is possible!)
         -  GCD(0,z) == GCD(z,0) == z
         -  negative values are handled correctly
    */

    if ((bits != bits_(V)) ||
        (bits != bits_(W)) ||
        (bits != bits_(X)) ||
        (bits != bits_(Y)))
    {
        return(ErrCode_Size);
    }
    if ((U == V) || (U == W) || (V == W))
    {
        return(ErrCode_Same);
    }
    if (is_empty(X))
    {
        if (U != Y) Copy(U,Y);
        Empty(V);
        Empty(W);
        *W = 1;
        return(ErrCode_Ok);
    }
    if (is_empty(Y))
    {
        if (U != X) Copy(U,X);
        Empty(V);
        Empty(W);
        *V = 1;
        return(ErrCode_Ok);
    }
    if ((L = Create_List(bits,false,11)) == NULL)
    {
        return(ErrCode_Null);
    }
    Q  = L[0];
    R  = L[1];
    A  = L[2];
    B  = L[3];
    X1 = L[4];
    X2 = L[5];
    X3 = L[6];
    Y1 = L[7];
    Y2 = L[8];
    Y3 = L[9];
    Z  = L[10];
    size--;
    sgn_a = (((*(X+size) &= mask) AND msb) != 0);
    sgn_b = (((*(Y+size) &= mask) AND msb) != 0);
    if (sgn_a) Negate(A,X); else Copy(A,X);
    if (sgn_b) Negate(B,Y); else Copy(B,Y);
    Empty(X1);
    Empty(X2);
    *X1 = 1;
    Empty(Y1);
    Empty(Y2);
    *Y2 = 1;
    sgn_x = false;
    sgn_y = false;
    while (!error)
    {
        if ((error = Div_Pos(Q,A,B,R)))
        {
            break;
        }
        if (is_empty(R))
        {
            break;
        }
        sgn_q = sgn_a XOR sgn_b;

        if (sgn_x) Negate(Z,X2); else Copy(Z,X2);
        if ((error = Mul_Pos(X3,Z,Q,true)))
        {
            break;
        }
        minus = !(sgn_x XOR sgn_q);
        carry = 0;
        if (compute(X3,X1,X3,minus,&carry))
        {
            error = ErrCode_Ovfl;
            break;
        }
        sgn_x = (((*(X3+size) &= mask) AND msb) != 0);

        if (sgn_y) Negate(Z,Y2); else Copy(Z,Y2);
        if ((error = Mul_Pos(Y3,Z,Q,true)))
        {
            break;
        }
        minus = !(sgn_y XOR sgn_q);
        carry = 0;
        if (compute(Y3,Y1,Y3,minus,&carry))
        {
            error = ErrCode_Ovfl;
            break;
        }
        sgn_y = (((*(Y3+size) &= mask) AND msb) != 0);

        T = A; sgn_r = sgn_a;
        A = B; sgn_a = sgn_b;
        B = R; sgn_b = sgn_r;
        R = T;

        T = X1;
        X1 = X2;
        X2 = X3;
        X3 = T;

        T = Y1;
        Y1 = Y2;
        Y2 = Y3;
        Y3 = T;
    }
    if (!error)
    {
        if (sgn_b) Negate(U,B); else Copy(U,B);
        Copy(V,X2);
        Copy(W,Y2);
    }
    Destroy_List(L,11);
    return(error);
}

ErrCode Power(wordptr X, wordptr Y, wordptr Z)
{
    ErrCode error = ErrCode_Ok;
    N_word  bits  = bits_(X);
    bool first = true;
    Z_long  last;
    N_word  limit;
    N_word  count;
    wordptr T;

    /*
       Requirements:
         -  X must have at least the same size as Y but may be larger (!)
         -  X may not be identical with Z
         -  Z must be positive
       Features:
         -  The contents of Y and Z are preserved
    */

    if (X == Z) return(ErrCode_Same);
    if (bits < bits_(Y)) return(ErrCode_Size);
    if (msb_(Z)) return(ErrCode_Expo);
    if ((last = Set_Max(Z)) < 0L)
    {
        if (bits < 2) return(ErrCode_Ovfl);
        Empty(X);
        *X |= LSBMASK;
        return(ErrCode_Ok);                             /* anything ^ 0 == 1 */
    }
    if (is_empty(Y))
    {
        if (X != Y) Empty(X);
        return(ErrCode_Ok);                    /* 0 ^ anything not zero == 0 */
    }
    T = Create(bits,false);
    if (T == NULL) return(ErrCode_Null);
    limit = (N_word) last;
    for ( count = 0; ((!error) && (count <= limit)); count++ )
    {
        if ( TST_BIT(Z,count) )
        {
            if (first)
            {
                first = false;
                if (count) {             Copy(X,T); }
                else       { if (X != Y) Copy(X,Y); }
            }
            else error = Multiply(X,T,X); /* order important because T > X */
        }
        if ((!error) && (count < limit))
        {
            if (count) error = Multiply(T,T,T);
            else       error = Multiply(T,Y,Y);
        }
    }
    Destroy(T);
    return(error);
}

void Block_Store(wordptr addr, charptr buffer, N_int length)
{
    N_word  size = size_(addr);
    N_word  mask = mask_(addr);
    N_word  value;
    N_word  count;

    /* provide translation for independence of endian-ness: */
    if (size > 0)
    {
        while (size-- > 0)
        {
            value = 0;
            for ( count = 0; (length > 0) && (count < BITS); count += 8 )
            {
                value |= (((N_word) *buffer++) << count); length--;
            }
            *addr++ = value;
        }
        *(--addr) &= mask;
    }
}

charptr Block_Read(wordptr addr, N_intptr length)
{
    N_word  size = size_(addr);
    N_word  value;
    N_word  count;
    charptr buffer;
    charptr target;

    /* provide translation for independence of endian-ness: */
    *length = size * BYTES;
    buffer = (charptr) malloc((size_t) ((*length)+1));
    if (buffer == NULL) return(NULL);
    target = buffer;
    if (size > 0)
    {
        *(addr+size-1) &= mask_(addr);
        while (size-- > 0)
        {
            value = *addr++;
            count = BITS >> 3;
            while (count-- > 0)
            {
                *target++ = (N_char) (value AND 0x00FF);
                if (count > 0) value >>= 8;
            }
        }
    }
    *target = (N_char) '\0';
    return(buffer);
}

void Word_Store(wordptr addr, N_int offset, N_int value)
{
    N_word size = size_(addr);

    if (size > 0)
    {
        if (offset < size) *(addr+offset) = value;
        *(addr+size-1) &= mask_(addr);
    }
}

N_int Word_Read(wordptr addr, N_int offset)
{
    N_word size = size_(addr);

    if (size > 0)
    {
        *(addr+size-1) &= mask_(addr);
        if (offset < size) return( *(addr+offset) );
    }
    return( (N_int) 0 );
}

void Word_Insert(wordptr addr, N_int offset, N_int count,
                           bool clear)
{
    N_word  size = size_(addr);
    N_word  mask = mask_(addr);
    wordptr last = addr+size-1;

    if (size > 0)
    {
        *last &= mask;
        if (offset > size) offset = size;
        ins_words(addr+offset,size-offset,count,clear);
        *last &= mask;
    }
}

void Word_Delete(wordptr addr, N_int offset, N_int count,
                           bool clear)
{
    N_word  size = size_(addr);
    N_word  mask = mask_(addr);
    wordptr last = addr+size-1;

    if (size > 0)
    {
        *last &= mask;
        if (offset > size) offset = size;
        del_words(addr+offset,size-offset,count,clear);
        *last &= mask;
    }
}

void Chunk_Store(wordptr addr, N_int chunksize, N_int offset,
                           N_long value)
{
    N_word bits = bits_(addr);
    N_word mask;
    N_word temp;

    if ((chunksize > 0) && (offset < bits))
    {
        if (chunksize > LONGBITS) chunksize = LONGBITS;
        if ((offset + chunksize) > bits) chunksize = bits - offset;
        addr += offset / BITS;
        offset &= MODMASK;
        while (chunksize > 0)
        {
            mask = (N_word) (~0L << offset);
            bits = offset + chunksize;
            if (bits < BITS)
            {
                mask &= (N_word) ~(~0L << bits);
                bits = chunksize;
            }
            else bits = BITS - offset;
            temp = (N_word) (value << offset);
            temp &= mask;
            *addr &= NOT mask;
            *addr++ |= temp;
            value >>= bits;
            chunksize -= bits;
            offset = 0;
        }
    }
}

N_long Chunk_Read(wordptr addr, N_int chunksize, N_int offset)
{
    N_word bits = bits_(addr);
    N_word chunkbits = 0;
    N_long value = 0L;
    N_long temp;
    N_word mask;

    if ((chunksize > 0) && (offset < bits))
    {
        if (chunksize > LONGBITS) chunksize = LONGBITS;
        if ((offset + chunksize) > bits) chunksize = bits - offset;
        addr += offset / BITS;
        offset &= MODMASK;
        while (chunksize > 0)
        {
            bits = offset + chunksize;
            if (bits < BITS)
            {
                mask = (N_word) ~(~0L << bits);
                bits = chunksize;
            }
            else
            {
                mask = (N_word) ~0L;
                bits = BITS - offset;
            }
            temp = (N_long) ((*addr++ AND mask) >> offset);
            value |= temp << chunkbits;
            chunkbits += bits;
            chunksize -= bits;
            offset = 0;
        }
    }
    return(value);
}

    /*******************/
    /* set operations: */
    /*******************/

void Set_Union(wordptr X, wordptr Y, wordptr Z)             /* X = Y + Z     */
{
    N_word bits = bits_(X);
    N_word size = size_(X);
    N_word mask = mask_(X);

    if ((size > 0) && (bits == bits_(Y)) && (bits == bits_(Z)))
    {
        while (size-- > 0) *X++ = *Y++ OR *Z++;
        *(--X) &= mask;
    }
}

void Set_Intersection(wordptr X, wordptr Y, wordptr Z)      /* X = Y * Z     */
{
    N_word bits = bits_(X);
    N_word size = size_(X);
    N_word mask = mask_(X);

    if ((size > 0) && (bits == bits_(Y)) && (bits == bits_(Z)))
    {
        while (size-- > 0) *X++ = *Y++ AND *Z++;
        *(--X) &= mask;
    }
}

void Set_Difference(wordptr X, wordptr Y, wordptr Z)        /* X = Y \ Z     */
{
    N_word bits = bits_(X);
    N_word size = size_(X);
    N_word mask = mask_(X);

    if ((size > 0) && (bits == bits_(Y)) && (bits == bits_(Z)))
    {
        while (size-- > 0) *X++ = *Y++ AND NOT *Z++;
        *(--X) &= mask;
    }
}

void Set_ExclusiveOr(wordptr X, wordptr Y, wordptr Z)       /* X=(Y+Z)\(Y*Z) */
{
    N_word bits = bits_(X);
    N_word size = size_(X);
    N_word mask = mask_(X);

    if ((size > 0) && (bits == bits_(Y)) && (bits == bits_(Z)))
    {
        while (size-- > 0) *X++ = *Y++ XOR *Z++;
        *(--X) &= mask;
    }
}

void Set_Complement(wordptr X, wordptr Y)                   /* X = ~Y        */
{
    N_word size = size_(X);
    N_word mask = mask_(X);

    if ((size > 0) && (bits_(X) == bits_(Y)))
    {
        while (size-- > 0) *X++ = NOT *Y++;
        *(--X) &= mask;
    }
}

    /******************/
    /* set functions: */
    /******************/

bool Set_subset(wordptr X, wordptr Y)                    /* X subset Y ?  */
{
    N_word size = size_(X);
    bool r = false;

    if ((size > 0) && (bits_(X) == bits_(Y)))
    {
        r = true;
        while (r && (size-- > 0)) r = ((*X++ AND NOT *Y++) == 0);
    }
    return(r);
}

Z_long Set_Min(wordptr addr)                                /* = min(X)      */
{
    bool empty = true;
    N_word  size  = size_(addr);
    N_word  i     = 0;
    N_word  c     = 0;         /* silence compiler warning */

    while (empty && (size-- > 0))
    {
        if ((c = *addr++)) empty = false; else i++;
    }
    if (empty) return((Z_long) LONG_MAX);                  /* plus infinity  */
    i *= BITS;
    while (!(c AND LSBMASK))
    {
        c >>= 1;
        i++;
    }
    return((Z_long) i);
}

Z_long Set_Max(wordptr addr)                                /* = max(X)      */
{
    bool empty = true;
    N_word  size  = size_(addr);
    N_word  i     = size;
    N_word  c     = 0;         /* silence compiler warning */

    addr += size-1;
    while (empty && (size-- > 0))
    {
        if ((c = *addr--)) empty = false; else i--;
    }
    if (empty) return((Z_long) LONG_MIN);                  /* minus infinity */
    i *= BITS;
    while (!(c AND MSBMASK))
    {
        c <<= 1;
        i--;
    }
    return((Z_long) --i);
}

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

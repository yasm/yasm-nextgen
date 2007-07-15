/**
 * \file libyasm/file.h
 * \brief YASM file helpers.
 *
 * \license
 *  Copyright (C) 2001-2007  Peter Johnson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * \endlicense
 */
#ifndef YASM_FILE_H
#define YASM_FILE_H

#include <boost/function.hpp>

namespace yasm {

/** Re2c scanner state. */
class Scanner {
public:
    ~Scanner() { delete bot; }

    /** Fill a scanner state structure with data coming from an input
     * function.
     * \param cursor        Re2c scan cursor
     * \param input_func    Input function to read data; takes buffer and
     *                      maximum number of bytes, returns number of bytes
     *                      read.
     * \return True if this was the first time this function was called on this
     *         scanner state, false otherwise.
     */
    bool fill_helper(unsigned char* &cursor,
        boost::function<size_t (unsigned char* buf, size_t max)> input_func);

    unsigned char* bot;     /**< Bottom of scan buffer */
    unsigned char* tok;     /**< Start of token */
    unsigned char* ptr;     /**< Scan marker */
    unsigned char* cur;     /**< Cursor (1 past end of token) */
    unsigned char* lim;     /**< Limit of good data */
    unsigned char* top;     /**< Top of scan buffer */
    unsigned char* eof;     /**< End of file */
};

/** Unescape a string with C-style escapes.  Handles b, f, n, r, t, and hex
 * and octal escapes.  String is updated in-place.
 * Edge cases:
 * - hex escapes: reads as many hex digits as possible, takes last 2 as value.
 * - oct escapes: takes up to 3 digits 0-9 and scales appropriately, with
 *                warning.
 * \param str           C-style string (updated in place)
 * \param len           length of string (updated with new length)
 */
void unescape_cstring(unsigned char* str, size_t& len);

/** Split a UNIX pathname into head (directory) and tail (base filename)
 * portions.
 * \param path  pathname
 * \param tail  (returned) base filename
 * \return Length of head (directory).
 */
size_t splitpath_unix(const char* path, /*@out@*/ const char* &tail);

/** Split a Windows pathname into head (directory) and tail (base filename)
 * portions.
 * \param path  pathname
 * \param tail  (returned) base filename
 * \return Length of head (directory).
 */
size_t splitpath_win(const char* path, /*@out@*/ const char* &tail);

/** Split a pathname into head (directory) and tail (base filename) portions.
 * Unless otherwise defined, defaults to splitpath_unix().
 * \param path  pathname
 * \param tail  (returned) base filename
 * \return Length of head (directory).
 */
#ifndef splitpath
# if defined (_WIN32) || defined (WIN32) || defined (__MSDOS__) || \
 defined (__DJGPP__) || defined (__OS2__) || defined (__CYGWIN__) || \
 defined (__CYGWIN32__)
#  define splitpath(path, tail)     splitpath_win(path, tail)
# else
#  define splitpath(path, tail)     splitpath_unix(path, tail)
# endif
#endif

/** Convert a UNIX relative or absolute pathname into an absolute pathname.
 * \param path  pathname
 * \return Absolute version of path (newly allocated).
 */
/*@only@*/ char* abspath_unix(const char* path);

/** Convert a Windows relative or absolute pathname into an absolute pathname.
 * \internal
 * \param path  pathname
 * \return Absolute version of path (newly allocated).
 */
/*@only@*/ char* abspath_win(const char* path);

/** Convert a relative or absolute pathname into an absolute pathname.
 * Unless otherwise defined, defaults to abspath_unix().
 * \param path  pathname
 * \return Absolute version of path (newly allocated).
 */
#ifndef HAVE_ABSPATH
#define HAVE_ABSPATH 1
# if defined (_WIN32) || defined (WIN32) || defined (__MSDOS__) || \
 defined (__DJGPP__) || defined (__OS2__) || defined (__CYGWIN__) || \
 defined (__CYGWIN32__)
inline char*
abspath(const char* path)
{
    return abspath_win(path);
}
# else
inline char*
abspath(const char* path)
{
    return abspath_unix(path);
}
# endif
#endif

/** Build a UNIX pathname that is equivalent to accessing the "to" pathname
 * when you're in the directory containing "from".  Result is relative if both
 * from and to are relative.
 * \param from  from pathname
 * \param to    to pathname
 * \return Combined path (newly allocated).
 */
char* combpath_unix(const char* from, const char* to);

/** Build a Windows pathname that is equivalent to accessing the "to" pathname
 * when you're in the directory containing "from".  Result is relative if both
 * from and to are relative.
 * \param from  from pathname
 * \param to    to pathname
 * \return Combined path (newly allocated).
 */
char* combpath_win(const char* from, const char* to);

/** Build a pathname that is equivalent to accessing the "to" pathname
 * when you're in the directory containing "from".  Result is relative if both
 * from and to are relative.
 * Unless otherwise defined, defaults to combpath_unix().
 * \param from  from pathname
 * \param to    to pathname
 * \return Combined path (newly allocated).
 */
#ifndef HAVE_COMBPATH
#define HAVE_COMBPATH 1
# if defined (_WIN32) || defined (WIN32) || defined (__MSDOS__) || \
 defined (__DJGPP__) || defined (__OS2__) || defined (__CYGWIN__) || \
 defined (__CYGWIN32__)
inline char*
combpath(const char* from, const char* to)
{
    return combpath_win(from, to);
}
# else
inline char*
combpath(const char* from, const char* to)
{
    return combpath_unix(from, to);
}
# endif
#endif

#if 0
/** Try to find and open an include file, searching through include paths.
 * First iname is looked for relative to the directory containing "from", then
 * it's looked for relative to each of the include paths.
 *
 * All pathnames may be either absolute or relative; from, oname, and
 * include paths, if relative, are relative from the current working directory.
 *
 * First match wins; the full pathname (newly allocated) to the opened file
 * is saved into oname, and the fopen'ed FILE * is returned.  If not found,
 * NULL is returned.
 *
 * \param iname     file to include
 * \param from      file doing the including
 * \param mode      fopen mode string
 * \param oname     full pathname of included file (may be relative). NULL
 *                  may be passed if this is unwanted.
 * \return fopen'ed include file, or NULL if not found.
 */
/*@null@*/ FILE *yasm_fopen_include
    (const char *iname, const char *from, const char *mode,
     /*@null@*/ /*@out@*/ /*@only@*/ char **oname);

/** Delete any stored include paths added by yasm_add_include_path().
 */
void yasm_delete_include_paths();

/** Add an include path for use by yasm_fopen_include().
 * If path is relative, it is treated by yasm_fopen_include() as relative to
 * the current working directory.
 *
 * \param path      path to add
 */
void yasm_add_include_path(const char *path);
#endif
/** Write an 8-bit value to a buffer, incrementing buffer pointer.
 * \param ptr   buffer
 * \param val   8-bit value
 */
inline void
WRITE_8(unsigned char* &ptr, unsigned int val)
{
    *(ptr++) = (unsigned char)(val & 0xFF);
}

/** Write a 16-bit value to a buffer in little endian, incrementing buffer
 * pointer.
 * \param ptr   buffer
 * \param val   16-bit value
 */
inline void
WRITE_16_L(unsigned char* &ptr, unsigned short val)
{
    *(ptr++) = (unsigned char)(val & 0xFF);
    *(ptr++) = (unsigned char)((val >> 8) & 0xFF);
}

/** Write a 32-bit value to a buffer in little endian, incrementing buffer
 * pointer.
 * \note Only works properly if ptr is an (unsigned char *).
 * \param ptr   buffer
 * \param val   32-bit value
 */
inline void
WRITE_32_L(unsigned char* &ptr, unsigned long val)
{
    *(ptr++) = (unsigned char)(val & 0xFF);
    *(ptr++) = (unsigned char)((val >> 8) & 0xFF);
    *(ptr++) = (unsigned char)((val >> 16) & 0xFF);
    *(ptr++) = (unsigned char)((val >> 24) & 0xFF);
}

/** Write a 16-bit value to a buffer in big endian, incrementing buffer
 * pointer.
 * \param ptr   buffer
 * \param val   16-bit value
 */
inline void
WRITE_16_B(unsigned char* &ptr, unsigned short val)
{
    *(ptr++) = (unsigned char)((val >> 8) & 0xFF);
    *(ptr++) = (unsigned char)(val & 0xFF);
}

/** Write a 32-bit value to a buffer in big endian, incrementing buffer
 * pointer.
 * \param ptr   buffer
 * \param val   32-bit value
 */
inline void
WRITE_32_B(unsigned char* &ptr, unsigned long val)
{
    *(ptr++) = (unsigned char)((val >> 24) & 0xFF);
    *(ptr++) = (unsigned char)((val >> 16) & 0xFF);
    *(ptr++) = (unsigned char)((val >> 8) & 0xFF);
    *(ptr++) = (unsigned char)(val & 0xFF);
}


/** Write an 8-bit value to a buffer.  Does not increment buffer pointer.
 * \param ptr   buffer
 * \param val   8-bit value
 */
inline void
SAVE_8(unsigned char* ptr, unsigned int val)
{
    WRITE_8(ptr, val);
}

/** Write a 16-bit value to a buffer in little endian.  Does not increment
 * buffer pointer.
 * \param ptr   buffer
 * \param val   16-bit value
 */
inline void
SAVE_16_L(unsigned char* ptr, unsigned short val)
{
    WRITE_16_L(ptr, val);
}

/** Write a 32-bit value to a buffer in little endian.  Does not increment
 * buffer pointer.
 * \param ptr   buffer
 * \param val   32-bit value
 */
inline void
SAVE_32_L(unsigned char* ptr, unsigned long val)
{
    WRITE_32_L(ptr, val);
}

/** Write a 16-bit value to a buffer in big endian.  Does not increment buffer
 * pointer.
 * \param ptr   buffer
 * \param val   16-bit value
 */
inline void
SAVE_16_B(unsigned char* ptr, unsigned short val)
{
    WRITE_16_B(ptr, val);
}

/** Write a 32-bit value to a buffer in big endian.  Does not increment buffer
 * pointer.
 * \param ptr   buffer
 * \param val   32-bit value
 */
inline void
SAVE_32_B(unsigned char* ptr, unsigned long val)
{
    WRITE_32_B(ptr, val);
}
#if 0
/** Direct-to-file version of YASM_SAVE_16_L().
 * \note Using the macro multiple times with a single fwrite() call will
 *       probably be faster than calling this function many times.
 * \param val   16-bit value
 * \param f     file
 * \return 1 if the write was successful, 0 if not (just like fwrite()).
 */
size_t yasm_fwrite_16_l(unsigned short val, FILE *f);

/** Direct-to-file version of YASM_SAVE_32_L().
 * \note Using the macro multiple times with a single fwrite() call will
 *       probably be faster than calling this function many times.
 * \param val   32-bit value
 * \param f     file
 * \return 1 if the write was successful, 0 if not (just like fwrite()).
 */
size_t yasm_fwrite_32_l(unsigned long val, FILE *f);

/** Direct-to-file version of YASM_SAVE_16_B().
 * \note Using the macro multiple times with a single fwrite() call will
 *       probably be faster than calling this function many times.
 * \param val   16-bit value
 * \param f     file
 * \return 1 if the write was successful, 0 if not (just like fwrite()).
 */
size_t yasm_fwrite_16_b(unsigned short val, FILE *f);

/** Direct-to-file version of YASM_SAVE_32_B().
 * \note Using the macro multiple times with a single fwrite() call will
 *       probably be faster than calling this function many times.
 * \param val   32-bit value
 * \param f     file
 * \return 1 if the write was successful, 0 if not (just like fwrite()).
 */
size_t yasm_fwrite_32_b(unsigned long val, FILE *f);
#endif
/** Read an 8-bit value from a buffer, incrementing buffer pointer.
 * \param ptr   buffer
 * \param val   8-bit value
 */
inline unsigned int
READ_8(unsigned char* &ptr)
{
    unsigned int val;
    val = *(ptr++) & 0xFF;
    return val;
}

inline void
READ_8(unsigned int& val, unsigned char* &ptr)
{
    val = READ_8(ptr);
}

/** Read a 16-bit value from a buffer in little endian, incrementing buffer
 * pointer.
 * \param ptr   buffer
 * \param val   16-bit value
 */
inline unsigned short
READ_16_L(unsigned char* &ptr)
{
    unsigned short val;
    val = *(ptr++) & 0xFF;
    val |= (*(ptr++) & 0xFF) << 8;
    return val;
}

inline void
READ_16_L(unsigned short& val, unsigned char* &ptr)
{
    val = READ_16_L(ptr);
}

/** Read a 32-bit value from a buffer in little endian, incrementing buffer
 * pointer.
 * \param ptr   buffer
 * \param val   32-bit value
 */
inline unsigned long
READ_32_L(unsigned char* &ptr)
{
    unsigned long val;
    val = *(ptr++) & 0xFF;
    val |= (*(ptr++) & 0xFF) << 8;
    val |= (*(ptr++) & 0xFF) << 16;
    val |= (*(ptr++) & 0xFF) << 24;
    return val;
}

inline void
READ_32_L(unsigned long& val, unsigned char* &ptr)
{
    val = READ_32_L(ptr);
}

/** Read a 16-bit value from a buffer in big endian, incrementing buffer
 * pointer.
 * \param ptr   buffer
 * \param val   16-bit value
 */
inline unsigned short
READ_16_B(unsigned char* &ptr)
{
    unsigned short val;
    val = (*(ptr++) & 0xFF) << 8;
    val |= *(ptr++) & 0xFF;
    return val;
}

inline void
READ_16_B(unsigned short& val, unsigned char* &ptr)
{
    val = READ_16_B(ptr);
}

/** Read a 32-bit value from a buffer in big endian, incrementing buffer
 * pointer.
 * \param ptr   buffer
 * \param val   32-bit value
 */
inline unsigned long
READ_32_B(unsigned char* &ptr)
{
    unsigned long val;
    val = (*(ptr++) & 0xFF) << 24;
    val |= (*(ptr++) & 0xFF) << 16;
    val |= (*(ptr++) & 0xFF) << 8;
    val |= *(ptr++) & 0xFF;
    return val;
}

inline void
READ_32_B(unsigned long& val, unsigned char* &ptr)
{
    val = READ_32_B(ptr);
}

/** Read an 8-bit value from a buffer.  Does not increment buffer pointer.
 * \param ptr   buffer
 * \param val   8-bit value
 */
inline unsigned int
LOAD_8(unsigned char* ptr)
{
    return READ_8(ptr);
}

inline void
LOAD_8(unsigned int& val, unsigned char* ptr)
{
    val = READ_8(ptr);
}

/** Read a 16-bit value from a buffer in little endian.  Does not increment
 * buffer pointer.
 * \param ptr   buffer
 * \param val   16-bit value
 */
inline unsigned short
LOAD_16_L(unsigned char* ptr)
{
    return READ_16_L(ptr);
}

inline void
LOAD_16_L(unsigned short& val, unsigned char* ptr)
{
    val = READ_16_L(ptr);
}

/** Read a 32-bit value from a buffer in little endian.  Does not increment
 * buffer pointer.
 * \param ptr   buffer
 * \param val   32-bit value
 */
inline unsigned long
LOAD_32_L(unsigned char* ptr)
{
    return READ_32_L(ptr);
}

inline void
LOAD_32_L(unsigned long& val, unsigned char* ptr)
{
    val = READ_32_L(ptr);
}

/** Read a 16-bit value from a buffer in big endian.  Does not increment buffer
 * pointer.
 * \param ptr   buffer
 * \param val   16-bit value
 */
inline unsigned short
LOAD_16_B(unsigned char* ptr)
{
    return READ_16_B(ptr);
}

inline void
LOAD_16_B(unsigned short& val, unsigned char* ptr)
{
    val = READ_16_B(ptr);
}

/** Read a 32-bit value from a buffer in big endian.  Does not increment buffer
 * pointer.
 * \param ptr   buffer
 * \param val   32-bit value
 */
inline unsigned long
LOAD_32_B(unsigned char* ptr)
{
    return READ_32_B(ptr);
}

inline void
LOAD_32_B(unsigned long& val, unsigned char* ptr)
{
    val = READ_32_B(ptr);
}

} // namespace yasm

#endif

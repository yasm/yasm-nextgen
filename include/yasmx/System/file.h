#ifndef YASM_FILE_H
#define YASM_FILE_H
///
/// @file
/// @brief File helpers.
///
/// @license
///  Copyright (C) 2001-2007  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
///  - Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
///  - Redistributions in binary form must reproduce the above copyright
///    notice, this list of conditions and the following disclaimer in the
///    documentation and/or other materials provided with the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
/// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
/// @endlicense
///
#include <string>

#include "yasmx/Config/export.h"


namespace yasm
{

/// Unescape a string with C-style escapes.  Handles b, f, n, r, t, and hex
/// and octal escapes.
/// Edge cases:
/// - hex escapes: reads as many hex digits as possible, takes last 2 as value.
/// - oct escapes: takes up to 3 digits 0-9 and scales appropriately, with
///                warning.
/// @param str          C-style string
/// @return Unescaped string
YASM_LIB_EXPORT
std::string unescape(const std::string& str);

/// Split a UNIX pathname into head (directory) and tail (base filename)
/// portions.
/// @param path     pathname
/// @param tail     (returned) base filename
/// @return Head (directory).
YASM_LIB_EXPORT
std::string splitpath_unix(const std::string& path,
                           /*@out@*/ std::string& tail);

/// Split a Windows pathname into head (directory) and tail (base filename)
/// portions.
/// @param path     pathname
/// @param tail     (returned) base filename
/// @return Head (directory).
YASM_LIB_EXPORT
std::string splitpath_win(const std::string& path,
                          /*@out@*/ std::string& tail);

/// Split a pathname into head (directory) and tail (base filename) portions.
/// Unless otherwise defined, defaults to splitpath_unix().
/// @param path     pathname
/// @param tail     (returned) base filename
/// @return Head (directory).
#ifndef HAVE_SPLITPATH
#define HAVE_SPLITPATH 1
# if defined (_WIN32) || defined (WIN32) || defined (__MSDOS__) || \
 defined (__DJGPP__) || defined (__OS2__) || defined (__CYGWIN__) || \
 defined (__CYGWIN32__)
inline std::string
splitpath(const std::string& path, /*@out@*/ std::string& tail)
{
    return splitpath_win(path, tail);
}
# else
inline std::string
splitpath(const std::string& path, /*@out@*/ std::string& tail)
{
    return splitpath_unix(path, tail);
}
# endif
#endif

/// Convert a UNIX relative or absolute pathname into an absolute pathname.
/// @param path     pathname
/// @return Absolute version of path.
YASM_LIB_EXPORT
std::string abspath_unix(const std::string& path);

/// Convert a Windows relative or absolute pathname into an absolute pathname.
/// @param path     pathname
/// @return Absolute version of path.
YASM_LIB_EXPORT
std::string abspath_win(const std::string& path);

/// Convert a relative or absolute pathname into an absolute pathname.
/// Unless otherwise defined, defaults to abspath_unix().
/// @param path  pathname
/// @return Absolute version of path (newly allocated).
#ifndef HAVE_ABSPATH
#define HAVE_ABSPATH 1
# if defined (_WIN32) || defined (WIN32) || defined (__MSDOS__) || \
 defined (__DJGPP__) || defined (__OS2__) || defined (__CYGWIN__) || \
 defined (__CYGWIN32__)
inline std::string
abspath(const std::string& path)
{
    return abspath_win(path);
}
# else
inline std::string
abspath(const std::string& path)
{
    return abspath_unix(path);
}
# endif
#endif

/// Build a UNIX pathname that is equivalent to accessing the "to" pathname
/// when you're in the directory containing "from".  Result is relative if
/// both from and to are relative.
/// @param from     from pathname
/// @param to       to pathname
/// @return Combined path (newly allocated).
YASM_LIB_EXPORT
std::string combpath_unix(const std::string& from, const std::string& to);

/// Build a Windows pathname that is equivalent to accessing the "to" pathname
/// when you're in the directory containing "from".  Result is relative if
/// both from and to are relative.
/// @param from     from pathname
/// @param to       to pathname
/// @return Combined path (newly allocated).
YASM_LIB_EXPORT
std::string combpath_win(const std::string& from, const std::string& to);

/// Build a pathname that is equivalent to accessing the "to" pathname
/// when you're in the directory containing "from".  Result is relative if
/// both from and to are relative.
/// Unless otherwise defined, defaults to combpath_unix().
/// @param from     from pathname
/// @param to       to pathname
/// @return Combined path (newly allocated).
#ifndef HAVE_COMBPATH
#define HAVE_COMBPATH 1
# if defined (_WIN32) || defined (WIN32) || defined (__MSDOS__) || \
 defined (__DJGPP__) || defined (__OS2__) || defined (__CYGWIN__) || \
 defined (__CYGWIN32__)
inline std::string
combpath(const std::string& from, const std::string& to)
{
    return combpath_win(from, to);
}
# else
inline std::string
combpath(const std::string& from, const std::string& to)
{
    return combpath_unix(from, to);
}
# endif
#endif

/// Replace extension on a filename (or append one if none is present).
/// @param orig     original filename
/// @param ext      extension, should include '.'
/// @param def      default output filename if orig == new.
/// @return Filename with new extension, or default filename.
YASM_LIB_EXPORT
std::string replace_extension(const std::string& orig,
                              const std::string& ext,
                              const std::string& def);

} // namespace yasm

#endif

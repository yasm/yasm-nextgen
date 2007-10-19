#ifndef YASM_FILE_H
#define YASM_FILE_H
///
/// @file libyasm/file.h
/// @brief YASM file helpers.
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
#include <fstream>
#include <string>
#include <vector>

#include <boost/function.hpp>


namespace yasm {

/// Re2c scanner state.
class Scanner {
public:
    /// Constructor.
    Scanner();

    /// Destructor.
    ~Scanner();

    /// Fill a scanner state structure with data coming from an input
    /// function.
    /// @param cursor       Re2c scan cursor
    /// @param input_func   Input function to read data; takes buffer and
    ///                     maximum number of bytes, returns number of bytes
    ///                     read.
    /// @return True if this was the first time this function was called on
    ///         this scanner state, false otherwise.
    bool fill_helper(unsigned char* &cursor,
        boost::function<size_t (unsigned char* buf, size_t max)> input_func);

    unsigned char* bot;     ///< Bottom of scan buffer
    unsigned char* tok;     ///< Start of token
    unsigned char* ptr;     ///< Scan marker
    unsigned char* cur;     ///< Cursor (1 past end of token)
    unsigned char* lim;     ///< Limit of good data
    unsigned char* top;     ///< Top of scan buffer
    unsigned char* eof;     ///< End of file
};

/// Unescape a string with C-style escapes.  Handles b, f, n, r, t, and hex
/// and octal escapes.
/// Edge cases:
/// - hex escapes: reads as many hex digits as possible, takes last 2 as value.
/// - oct escapes: takes up to 3 digits 0-9 and scales appropriately, with
///                warning.
/// @param str          C-style string
/// @return Unescaped string
std::string unescape(const std::string& str);

/// Split a UNIX pathname into head (directory) and tail (base filename)
/// portions.
/// @param path     pathname
/// @param tail     (returned) base filename
/// @return Head (directory).
std::string splitpath_unix(const std::string& path,
                           /*@out@*/ std::string& tail);

/// Split a Windows pathname into head (directory) and tail (base filename)
/// portions.
/// @param path     pathname
/// @param tail     (returned) base filename
/// @return Head (directory).
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
std::string abspath_unix(const std::string& path);

/// Convert a Windows relative or absolute pathname into an absolute pathname.
/// @param path     pathname
/// @return Absolute version of path.
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
std::string combpath_unix(const std::string& from, const std::string& to);

/// Build a Windows pathname that is equivalent to accessing the "to" pathname
/// when you're in the directory containing "from".  Result is relative if
/// both from and to are relative.
/// @param from     from pathname
/// @param to       to pathname
/// @return Combined path (newly allocated).
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

/// Include path storage and search.  Paths are stored as std::strings.
/// If a path is relative, it is treated by Includes::open() as relative to
/// the current working directory.
class Includes : public std::vector<std::string> {
public:
    /// Try to find and open an include file, searching through include paths.
    /// First iname is looked for relative to the directory containing "from",
    /// then it's looked for relative to each of the include paths.
    ///
    /// All pathnames may be either absolute or relative; from, oname, and
    /// include paths, if relative, are relative from the current working
    /// directory.
    ///
    /// First match wins; the full pathname (newly allocated) to the opened file
    /// is returned, and the ifstream is opened.  If not found, the ifstream
    /// after the call is not an open ifstream.
    ///
    /// If ifs is currently open, it is closed first.
    ///
    /// @param ifs      input file stream [modified]
    /// @param iname    filename to include
    /// @param from     filename doing the including
    /// @param mode     file open mode
    /// @return full pathname of included file (may be relative) [output]
    std::string open(std::ifstream& ifs,
                     const std::string& iname,
                     const std::string& from,
                     std::ios_base::openmode mode = std::ios_base::in) const;

    /// Add an include path.
    /// @param path     pathname
    void push_back(const std::string& path);
};

} // namespace yasm

#endif

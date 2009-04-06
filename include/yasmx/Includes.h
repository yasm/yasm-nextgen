#ifndef YASM_INCLUDES_H
#define YASM_INCLUDES_H
///
/// @file
/// @brief Include path manager
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

#include "yasmx/Config/export.h"

namespace yasm
{

/// Include path storage and search.  Paths are stored as std::strings.
/// If a path is relative, it is treated by Includes::open() as relative to
/// the current working directory.
class YASM_LIB_EXPORT Includes : public std::vector<std::string>
{
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

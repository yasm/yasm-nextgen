//
// Include path manager.
//
//  Copyright (C) 2001-2007  Peter Johnson
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
#include "yasmx/Includes.h"

#include "yasmx/System/file.h"


namespace yasm
{

std::string
Includes::open(std::ifstream& ifs,
               const std::string& iname,
               const std::string& from,
               std::ios_base::openmode mode) const
{
    // Close the stream if already open
    if (ifs.is_open())
        ifs.close();
    ifs.clear();

    // Try directly relative to from first, then each of the include paths.
    std::string combine = combpath(from, iname);
    ifs.open(combine.c_str(), mode);
    if (ifs)
        return combine;

    for (const_iterator i = begin(), e = end(); i != e; ++i)
    {
        combine = combpath(*i, iname);
        ifs.open(combine.c_str(), mode);
        if (ifs)
            return combine;
    }

    return "";
}

void
Includes::push_back(const std::string& path)
{
    // Add trailing slash if it is missing.
    std::string::size_type len = path.length();
    if (len > 0 && path[len-1] != '\\' && path[len-1] != '/')
        std::vector<std::string>::push_back(path+'/');
    else
        std::vector<std::string>::push_back(path);
}

} // namespace yasm

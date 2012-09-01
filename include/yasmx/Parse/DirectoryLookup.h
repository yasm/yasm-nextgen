#ifndef YASM_PARSE_DIRECTORYLOOKUP_H
#define YASM_PARSE_DIRECTORYLOOKUP_H
//
// Info for searching for headers interface
//
// Based on the LLVM Compiler Infrastructure
// (distributed under the University of Illinois Open Source License.
// See Copying/LLVM.txt for details).
//
// Modifications copyright (C) 2010  Peter Johnson
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
#include "yasmx/Basic/LLVM.h"

namespace yasm
{

class DirectoryEntry;
class FileEntry;
class HeaderSearch;

/// DirectoryLookup - This class represents one entry in the search list that
/// specifies the search order for directories in #include directives.
class DirectoryLookup
{
private:
    /// Dir - This is the actual directory that we're referring to.
    const DirectoryEntry *Dir;

    /// UserSupplied - True if this is a user-supplied directory.
    bool UserSupplied : 1;

public:
    /// DirectoryLookup ctor - Note that this ctor *does not take ownership* of
    /// 'dir'.
    DirectoryLookup(const DirectoryEntry* dir, bool isUser)
        : Dir(dir), UserSupplied(isUser)
    {
    }

    /// getName - Return the directory or filename corresponding to this lookup
    /// object.
    const char* getName() const;

    /// getDir - Return the directory that this entry refers to.
    ///
    const DirectoryEntry* getDir() const { return Dir; }

    /// isUserSupplied - True if this is a user-supplied directory.
    ///
    bool isUserSupplied() const { return UserSupplied; }

    /// LookupFile - Lookup the specified file in this search path, returning it
    /// if it exists or returning null if not.
    const FileEntry* LookupFile(StringRef Filename, HeaderSearch& HS) const;
};

} // namespace yasm

#endif

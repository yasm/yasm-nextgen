#ifndef YASM_STRINGTABLE_H
#define YASM_STRINGTABLE_H
///
/// @file
/// @brief String table interface.
///
/// @license
///  Copyright (C) 2008  Peter Johnson
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
#include <vector>

#include "yasmx/Basic/LLVM.h"
#include "yasmx/Config/export.h"


namespace yasm
{

/// A string table of 0-terminated strings.  Always begins with a 0-length
/// string (a single 0 byte) at offset 0.
class YASM_LIB_EXPORT StringTable
{
public:
    /// Empty table constructor.
    /// @param first_index  Indexes will be returned and interpreted
    ///                     as if the first string starts at this offset.
    ///                     Defaults to 0.
    explicit StringTable(unsigned long first_index=0);

    /// Construct from iterator.
    template <typename InputIterator>
    StringTable(InputIterator first,
                InputIterator last,
                unsigned long first_index=0)
        : m_storage(first, last)
        , m_first_index(first_index)
    {}

    /// Destructor.
    ~StringTable();

    /// Get an index for a string.  If the asked-for string is a tail substring
    /// of an existing string in the table, it may be reused.
    /// @param str      String
    /// @return String index.
    unsigned long getIndex(StringRef str);

    /// Get the string corresponding to a particular index.  Due to legal use
    /// of substrings, no error checking is performed except for trying to read
    /// past the end of the string table.
    /// @param index    String index (offset into string table)
    /// @return String.
    /// Throws out_of_range exception if index is out of range.
    StringRef getString(unsigned long index) const;

    /// Get the size of the string table.
    /// @return Size in bytes.
    unsigned long getSize() const { return m_storage.size(); }

    /// Write the string table to an output stream.
    /// @param os       output stream
    void Write(raw_ostream& os) const;

    /// Read the string table from a string.
    /// Deletes any pre-existing string table data.
    /// @param buf      string
    void Read(StringRef buf);

private:
    std::vector<char> m_storage;
    unsigned long m_first_index;
};

} // namespace yasm

#endif

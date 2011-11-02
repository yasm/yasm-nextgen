#ifndef YASM_BYTES_H
#define YASM_BYTES_H
///
/// @file
/// @brief Bytes interface.
///
/// @license
///  Copyright (C) 2007  Peter Johnson
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

#include "yasmx/Config/export.h"
#include "yasmx/Support/EndianState.h"
#include "yasmx/DebugDumper.h"


namespace llvm { class raw_ostream; }

namespace yasm
{

/// A vector of bytes.
class YASM_LIB_EXPORT Bytes
    : private std::vector<unsigned char>
    , public EndianState
{
    typedef std::vector<unsigned char> base_vector;

public:
    Bytes() {}

    template <class InputIterator>
    Bytes(InputIterator first, InputIterator last) : base_vector(first, last) {}

    typedef base_vector::reference reference;
    typedef base_vector::const_reference const_reference;
    typedef base_vector::iterator iterator;
    typedef base_vector::const_iterator const_iterator;
    typedef base_vector::size_type size_type;
    typedef base_vector::difference_type difference_type;
    typedef base_vector::value_type value_type;
    typedef base_vector::reverse_iterator reverse_iterator;
    typedef base_vector::const_reverse_iterator const_reverse_iterator;

    using base_vector::begin;
    using base_vector::end;
    using base_vector::rbegin;
    using base_vector::rend;
    using base_vector::size;
    using base_vector::max_size;
    using base_vector::resize;
    using base_vector::capacity;
    using base_vector::empty;
    using base_vector::reserve;
    using base_vector::operator[];
    using base_vector::at;
    using base_vector::front;
    using base_vector::back;
    using base_vector::assign;
    using base_vector::push_back;
    using base_vector::pop_back;
    using base_vector::insert;
    using base_vector::erase;
    using base_vector::clear;

    void swap(Bytes& oth);

    /// Copy from a byte array, appending the values to the end.
    /// @param  buf input buffer
    /// @param  n   number of bytes
    void Write(const unsigned char* buf, size_type n);

    /// Append n bytes of value v.
    /// @param  n   number of bytes
    /// @param  v   byte value
    void Write(size_type n, unsigned char v);

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    /// @param out          XML node
    /// @return Root node.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML
};

inline void
Bytes::Write(const unsigned char* buf, size_type n)
{
    insert(end(), buf, buf+n);
}

/// Output the entire contents of a bytes container to an output stream.
/// @param os    output stream
/// @param bytes bytes
/// @return Output stream
YASM_LIB_EXPORT
llvm::raw_ostream& operator<< (llvm::raw_ostream& os, const Bytes& bytes);

/// Specialized swap for algorithms.
inline void
swap(Bytes& left, Bytes& right)
{
    left.swap(right);
}

} // namespace yasm

namespace std
{

// Specialized std::swap.
template <>
inline void
swap(yasm::Bytes& left, yasm::Bytes& right)
{
    left.swap(right);
}

}

#endif

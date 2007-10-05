//
// Bytes implementation.
//
//  Copyright (C) 2007  Peter Johnson
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
#include <algorithm>
#include <istream>
#include <iterator>
#include <ostream>

#include "bytes.h"


namespace yasm {

Bytes::Bytes(bool bigendian=false)
    : m_bigendian(bigendian)
{
}

Bytes::~Bytes()
{
}

std::ostream&
operator<< (std::ostream& os, const Bytes& bytes)
{
    os.write(reinterpret_cast<const char*>(&bytes[0]), bytes.size());
    return os;
}

void
Bytes::write(std::istream& is, size_type n)
{
    size_type sz = size();
    resize(sz+n);
    is.read(reinterpret_cast<char*>(&(at(sz))), n);
    resize(sz+is.gcount());
}

void
Bytes::write(const unsigned char* buf, size_type n)
{
    size_type sz = size();
    reserve(sz+n);
    std::copy(buf, buf+n, std::back_inserter(*this));
}

} // namespace yasm

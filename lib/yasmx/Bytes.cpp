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
#include "yasmx/Bytes.h"

#include <algorithm>
#include <istream>
#include <iterator>
#include <iomanip>
#include <ostream>

#include "llvm/Support/raw_ostream.h"
#include "YAML/emitter.h"


namespace yasm
{

std::ostream&
operator<< (std::ostream& os, const Bytes& bytes)
{
    os.write(reinterpret_cast<const char*>(&bytes[0]), bytes.size());
    return os;
}

void
Bytes::swap(Bytes& oth)
{
    base_vector::swap(*this);
    EndianState::swap(*this);
}

void
Bytes::Write(std::istream& is, size_type n)
{
    if (n == 0)
        return;
    size_type sz = size();
    resize(sz+n);
    is.read(reinterpret_cast<char*>(&(at(sz))), n);
    resize(sz+is.gcount());
}

void
Bytes::Write(size_type n, unsigned char v)
{
    if (n == 0)
        return;
    do {
        push_back(v);
        n--;
    } while (n != 0);
}

void
Bytes::Dump() const
{
    YAML::Emitter out;
    out << *this;
    llvm::errs() << out.c_str() << '\n';
}

YAML::Emitter&
operator<< (YAML::Emitter& out, const Bytes& bytes)
{
    out << YAML::Flow << YAML::BeginSeq;
    for (Bytes::size_type i=0; i<bytes.size(); ++i)
        out << YAML::Hex << static_cast<unsigned int>(bytes[i]);
    out << YAML::EndSeq;
    return out;
}

} // namespace yasm

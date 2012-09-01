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
#include <iterator>

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"


using namespace yasm;

raw_ostream&
yasm::operator<< (raw_ostream& os, const Bytes& bytes)
{
    if (bytes.empty())
        return os;
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
Bytes::Write(size_type n, unsigned char v)
{
    if (n == 0)
        return;
    do {
        push_back(v);
        n--;
    } while (n != 0);
}

#ifdef WITH_XML
pugi::xml_node
Bytes::Write(pugi::xml_node out) const
{
    SmallString<128> ss;
    llvm::raw_svector_ostream oss(ss);
    for (size_type i=0; i<size(); ++i)
    {
        oss.write_hex(static_cast<unsigned int>(operator[](i)));
        if (i+1 < size())
            oss << ' ';
    }
    oss << '\0';
    return append_data(out, oss.str().data());
}
#endif // WITH_XML

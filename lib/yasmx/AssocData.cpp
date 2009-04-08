//
// Associated data implementation.
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
#include "yasmx/AssocData.h"

#include "util.h"

#include "yasmx/Support/marg_ostream.h"


namespace yasm
{

AssocData::~AssocData()
{
}

AssocDataContainer::AssocDataContainer()
{
}

AssocDataContainer::~AssocDataContainer()
{
    for (AssocMap::iterator i=m_assoc_map.begin(), end=m_assoc_map.end();
         i != end; ++i)
        delete i->second;
}

std::auto_ptr<AssocData>
AssocDataContainer::add_assoc_data(const void* key,
                                   std::auto_ptr<AssocData> data)
{
    AssocData*& x = m_assoc_map[key];
    std::auto_ptr<AssocData> rv(x);
    x = data.release();
    return rv;
}

AssocData*
AssocDataContainer::get_assoc_data(const void* key)
{
    AssocMap::iterator i = m_assoc_map.find(key);
    if (i == m_assoc_map.end())
        return 0;
    return i->second;
}

const AssocData*
AssocDataContainer::get_assoc_data(const void* key) const
{
    AssocMap::const_iterator i = m_assoc_map.find(key);
    if (i == m_assoc_map.end())
        return 0;
    return i->second;
}

marg_ostream&
operator<< (marg_ostream& os, const AssocDataContainer& container)
{
    for (AssocDataContainer::AssocMap::const_iterator
         i=container.m_assoc_map.begin(), end=container.m_assoc_map.end();
         i != end; ++i)
        os << *i->second;
    return os;
}

} // namespace yasm

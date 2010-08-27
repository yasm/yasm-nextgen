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

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"
#include "YAML/emitter.h"


using namespace yasm;

AssocData::~AssocData()
{
}

void
AssocData::Dump() const
{
    YAML::Emitter out;
    Write(out);
    llvm::errs() << out.c_str() << '\n';
}

AssocDataContainer::AssocDataContainer()
{
}

AssocDataContainer::~AssocDataContainer()
{
    for (AssocMap::iterator i=m_assoc_map.begin(), end=m_assoc_map.end();
         i != end; ++i)
        delete i->value;
}

std::auto_ptr<AssocData>
AssocDataContainer::AddAssocData(const void* key,
                                 std::auto_ptr<AssocData> data)
{
    // simple linear search
    for (AssocMap::iterator i=m_assoc_map.begin(), end=m_assoc_map.end();
         i != end; ++i)
    {
        if (i->key == key)
        {
            std::auto_ptr<AssocData> rv(i->value);
            i->value = data.release();
            return rv;
        }
    }

    // not found, insert at end
    AssocMapEntry entry = {key, data.release()};
    m_assoc_map.push_back(entry);
    return std::auto_ptr<AssocData>(0);
}

AssocData*
AssocDataContainer::getAssocData(const void* key)
{
    // simple linear search
    for (AssocMap::iterator i=m_assoc_map.begin(), end=m_assoc_map.end();
         i != end; ++i)
    {
        if (i->key == key)
            return i->value;
    }
    return 0;
}

const AssocData*
AssocDataContainer::getAssocData(const void* key) const
{
    // simple linear search
    for (AssocMap::const_iterator i=m_assoc_map.begin(), end=m_assoc_map.end();
         i != end; ++i)
    {
        if (i->key == key)
            return i->value;
    }
    return 0;
}

void
AssocDataContainer::Write(YAML::Emitter& out) const
{
    if (m_assoc_map.empty())
        out << YAML::Flow;
    out << YAML::BeginMap;
    for (AssocMap::const_iterator i=m_assoc_map.begin(), end=m_assoc_map.end();
         i != end; ++i)
    {
        llvm::SmallString<128> ss;
        llvm::raw_svector_ostream oss(ss);
        oss << (uint64_t)(i->key);
        out << YAML::Key << oss.str() << YAML::Value << *i->value;
    }
    out << YAML::EndMap;
}

void
AssocDataContainer::Dump() const
{
    YAML::Emitter out;
    Write(out);
    llvm::errs() << out.c_str() << '\n';
}

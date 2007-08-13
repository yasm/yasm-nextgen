#ifndef YASM_ASSOC_DATA_H
#define YASM_ASSOC_DATA_H
///
/// @file libyasm/assoc_data.h
/// @brief YASM associated data interface.
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
#include <iosfwd>
#include <map>
#include <memory>
#include <string>

#include <boost/noncopyable.hpp>


namespace yasm {

/// Associated data interface (abstract base).
class AssocData : private boost::noncopyable {
public:
    /// Constructor.
    AssocData() {}

    /// Destructor.
    virtual ~AssocData() {}

    virtual void put(std::ostream& os, int indent_level) const = 0;
};

class AssocDataContainer {
    typedef std::map<const void*, AssocData*> AssocMap;
    AssocMap m_assoc_map;

public:
    AssocDataContainer();
    ~AssocDataContainer();

    std::auto_ptr<AssocData> add_assoc_data(const void* key,
                                            std::auto_ptr<AssocData> data)
    {
        AssocData*& x = m_assoc_map[key];
        std::auto_ptr<AssocData> rv(x);
        x = data.release();
        return rv;
    }

    AssocData* get_assoc_data(const void* key)
    {
        AssocMap::iterator i = m_assoc_map.find(key);
        if (i == m_assoc_map.end())
            return 0;
        return i->second;
    }

protected:
    void put_assoc_data(std::ostream& os, int indent_level) const;
};

} // namespace yasm

#endif

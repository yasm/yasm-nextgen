#ifndef YASM_ASSOCDATA_H
#define YASM_ASSOCDATA_H
///
/// @file
/// @brief Associated data interface.
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
#include <map>
#include <memory>
#include <string>

#include "Config/export.h"
#include "Support/marg_ostream_fwd.h"


namespace yasm
{

/// Associated data interface (abstract base).
class YASM_LIB_EXPORT AssocData
{
public:
    AssocData() {}

    /// Destructor.
    virtual ~AssocData();

    virtual void put(marg_ostream& os) const = 0;

private:
    AssocData(const AssocData&);                    // not implemented
    const AssocData& operator=(const AssocData&);   // not implemented
};

inline marg_ostream&
operator<< (marg_ostream& os, const AssocData& data)
{
    data.put(os);
    return os;
}

/// Associated data container.
class YASM_LIB_EXPORT AssocDataContainer
{
    friend marg_ostream& operator<< (marg_ostream& os,
                                     const AssocDataContainer& container);

    typedef std::map<const void*, AssocData*> AssocMap;
    AssocMap m_assoc_map;

public:
    AssocDataContainer();
    virtual ~AssocDataContainer();

    std::auto_ptr<AssocData> add_assoc_data(const void* key,
                                            std::auto_ptr<AssocData> data);

    AssocData* get_assoc_data(const void* key);
    const AssocData* get_assoc_data(const void* key) const;
};

YASM_LIB_EXPORT
marg_ostream& operator<< (marg_ostream& os,
                          const AssocDataContainer& container);

} // namespace yasm

#endif

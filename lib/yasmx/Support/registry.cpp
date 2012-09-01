///
/// @file libyasm/registry.cpp
/// @brief YASM module registry implementation.
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
#include "yasmx/Support/registry.h"

#include <algorithm>

#include "llvm/ADT/IndexedMap.h"
#include "llvm/ADT/StringMap.h"


using namespace yasm;
using namespace yasm::impl;

namespace yasm { namespace impl {
class ModuleFactory::Impl
{
public:
    ~Impl()
    {
        for (unsigned int i=0; i<registry.size(); ++i)
            delete registry[i];
    }

    /// FN_REGISTRY is the registry of all the BASE_CREATE_FN
    /// pointers registered.  Functions are registered using the
    /// regCreateFn member function (see below).
    typedef llvm::StringMap<BASE_CREATE_FN> FN_REGISTRY;
    typedef llvm::IndexedMap<FN_REGISTRY*> MODULE_REGISTRY;
    MODULE_REGISTRY registry;
};
}} // namespace yasm::impl

ModuleFactory::ModuleFactory()
    : m_impl(new Impl)
{
}

ModuleFactory::~ModuleFactory()
{
}

ModuleFactory&
ModuleFactory::Instance()
{
    static ModuleFactory static_instance;
    return static_instance;
}

// Register the creation function.  This simply associates the classIDKey
// with the function used to create the class.  The return value is a dummy
// value, which is used to allow static initialization of the registry.
// See example implementations in base.cc and derived.cc
void
ModuleFactory::AddCreateFn(unsigned int type,
                           StringRef keyword,
                           BASE_CREATE_FN func)
{
    m_impl->registry.grow(type);
    if (!m_impl->registry[type])
        m_impl->registry[type] = new Impl::FN_REGISTRY();
    (*m_impl->registry[type])[keyword] = func;
}

// The create function simple looks up the class ID, and if it's in the list,
// the statement "(*i).second();" calls the function.
ModuleFactory::BASE_CREATE_FN
ModuleFactory::getCreateFn(unsigned int type, StringRef keyword) const
{
    if (type >= m_impl->registry.size())
        return 0;
    Impl::FN_REGISTRY* module_entry = m_impl->registry[type];
    if (!module_entry)
        return 0;

    Impl::FN_REGISTRY::const_iterator func_entry = module_entry->find(keyword);
    if (func_entry == module_entry->end())
        return 0;

    return func_entry->getValue();
}

// Just return a list of the classIDKeys used.
ModuleNames
ModuleFactory::getRegistered(unsigned int type) const
{
    if (type >= m_impl->registry.size())
        return ModuleNames();

    Impl::FN_REGISTRY* module_entry = m_impl->registry[type];
    if (!module_entry)
        return ModuleNames();

    ModuleNames ret(module_entry->size());
    int count = 0;
    for (Impl::FN_REGISTRY::const_iterator func_entry = module_entry->begin(),
         end = module_entry->end(); func_entry != end; ++func_entry, ++count)
    {
        ret[count] = func_entry->getKey();
    }
    std::sort(ret.begin(), ret.end());
    return ret;
}

bool
ModuleFactory::isRegistered(unsigned int type, StringRef keyword) const
{
    if (type >= m_impl->registry.size())
        return false;
    Impl::FN_REGISTRY* module_entry = m_impl->registry[type];
    if (!module_entry)
        return false;
    return module_entry->find(keyword) != module_entry->end();
}

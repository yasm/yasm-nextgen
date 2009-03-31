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

#include <map>


namespace yasm
{
namespace impl
{

class ModuleFactory::Impl
{
public:
    /// FN_REGISTRY is the registry of all the BASE_CREATE_FN
    /// pointers registered.  Functions are registered using the
    /// regCreateFn member function (see below).
    typedef std::map<std::string, BASE_CREATE_FN> FN_REGISTRY;
    typedef std::map<int, FN_REGISTRY> MODULE_REGISTRY;
    MODULE_REGISTRY registry;
};

ModuleFactory::ModuleFactory()
    : m_impl(new Impl)
{
}

ModuleFactory::~ModuleFactory()
{
}

ModuleFactory&
ModuleFactory::instance()
{
    static ModuleFactory static_instance;
    return static_instance;
}

// Register the creation function.  This simply associates the classIDKey
// with the function used to create the class.  The return value is a dummy
// value, which is used to allow static initialization of the registry.
// See example implementations in base.cc and derived.cc
void
ModuleFactory::add_create_fn(int type, const char* keyword, BASE_CREATE_FN func)
{
    m_impl->registry[type][keyword]=func;
}

// The create function simple looks up the class ID, and if it's in the list,
// the statement "(*i).second();" calls the function.
ModuleFactory::BASE_CREATE_FN
ModuleFactory::get_create_fn(int type, const std::string& keyword) const
{
    Impl::MODULE_REGISTRY::const_iterator module_entry =
        m_impl->registry.find(type);
    if (module_entry == m_impl->registry.end())
        return 0;

    Impl::FN_REGISTRY::const_iterator func_entry =
        module_entry->second.find(keyword);
    if (func_entry == module_entry->second.end())
        return 0;

    return func_entry->second;
}

// Just return a list of the classIDKeys used.
std::vector<std::string>
ModuleFactory::get_registered(int type) const
{
    Impl::MODULE_REGISTRY::const_iterator module_entry =
        m_impl->registry.find(type);
    if (module_entry == m_impl->registry.end())
        return std::vector<std::string>();

    std::vector<std::string> ret(module_entry->second.size());
    int count = 0;
    for (Impl::FN_REGISTRY::const_iterator
         func_entry = module_entry->second.begin(),
         end = module_entry->second.end();
         func_entry != end; ++func_entry, ++count)
    {
        ret[count] = func_entry->first;
    }
    return ret;
}

bool
ModuleFactory::is_registered(int type, const std::string& keyword) const
{
    Impl::MODULE_REGISTRY::const_iterator module_entry =
        m_impl->registry.find(type);
    if (module_entry == m_impl->registry.end())
        return false;
    return module_entry->second.find(keyword) != module_entry->second.end();
}

}} // namespace yasm::impl

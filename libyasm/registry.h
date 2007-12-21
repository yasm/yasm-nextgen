#ifndef YASM_REGISTRY_H
#define YASM_REGISTRY_H
///
/// @file libyasm/registry.h
/// @brief YASM module registry interface.
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
#include <string>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>


/// Force inclusion of a module statically.
/// Must be called outside of a namespace or in an anonymous namespace.
/// To include all modules, #include static_modules.h.
#define YASM_STATIC_MODULE_REF(type, keyword) \
    namespace yasm { namespace type { namespace keyword { \
        extern bool static_ref; \
        static bool do_static_ref = static_ref; \
    }}}

namespace yasm {

// standard yasm module types
class Arch;
class DebugFormat;
class ListFormat;
class ObjectFormat;
class Parser;
class Preprocessor;

namespace impl {

// Implemented using the Singleton pattern
class ModuleFactory : private boost::noncopyable {
public:
    /// A BASE_CREATE_FN is a function that takes no parameters
    /// and returns an auto_ptr to a manufactuedObj.  Note that
    /// we use no parameters, but you could add them
    /// easily enough to allow overloaded ctors.
    typedef void* (*BASE_CREATE_FN)();

    /// Singleton access.
    static ModuleFactory& instance();

    /// Derived classes call this function once
    /// per program to register the class ID key, and a pointer to
    /// the function that creates the class.
    void add_create_fn(int type, const char* keyword, BASE_CREATE_FN func);

    /// Get the creation function for a given type and class name.
    /// @return NULL if not found.
    BASE_CREATE_FN get_create_fn(int type, const std::string& keyword) const;

    /// Return a list of classes that are registered.
    std::vector<std::string> get_registered(int type) const;

    /// Return true if the specific class is registered.
    bool is_registered(int type, const std::string& keyword) const;

private:
    /// Singleton implementation - private ctor.
    ModuleFactory();

    /// Pimpl for class internals.
    class Impl;
    boost::scoped_ptr<Impl> m_impl;
};

} // namespace impl

template <typename Ancestor, typename Manufactured>
class RegisterModule
{
private:
    static void* create_instance()
    {
        return new Manufactured;
    }

public:
    RegisterModule(const char* keyword)
    {
        impl::ModuleFactory::instance().add_create_fn(Ancestor::module_type,
                                                      keyword, create_instance);
    }
};

template <typename T>
inline std::auto_ptr<T>
load_module(const std::string& keyword)
{
    impl::ModuleFactory::BASE_CREATE_FN create = 
        impl::ModuleFactory::instance().get_create_fn(T::module_type, keyword);
    return std::auto_ptr<T>(static_cast<T*>(create()));
}

template <typename T>
inline bool
is_module(const std::string& keyword)
{
    return impl::ModuleFactory::instance().is_registered(T::module_type,
                                                         keyword);
}

template <typename T>
inline std::vector<std::string>
get_modules()
{
    return impl::ModuleFactory::instance().get_registered(T::module_type);
}

} // namespace yasm

#endif

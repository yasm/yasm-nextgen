#ifndef YASM_MODULE_H
#define YASM_MODULE_H
///
/// @file libyasm/module.h
/// @brief YASM base module interface.
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

#include <boost/noncopyable.hpp>


/// Force inclusion of a module statically.
/// Must be called outside of a namespace or in an anonymous namespace.
/// To include all modules, #include static_modules.h.
#define YASM_STATIC_MODULE_REF(type, keyword) \
    namespace yasm { namespace type { namespace keyword { \
        extern bool static_ref; \
        static bool do_static_ref = static_ref; \
    }}}

namespace yasm {

class Directives;

/// Module interface (abstract base).
class Module : private boost::noncopyable {
public:
    /// Constructor.
    Module() {}

    /// Destructor.
    virtual ~Module() {}

    /// Get the one-line description of the module.
    /// @return One-line description of module.
    virtual std::string get_name() const = 0;

    /// Get the keyword used to select the module.
    /// @return Module keyword.
    virtual std::string get_keyword() const = 0;

    /// Get the module type name (e.g. "ObjectFormat", "DebugFormat", ...).
    /// @return Module typename.
    virtual std::string get_type() const = 0;

    /// Add directive handlers.
    virtual void add_directives(Directives& dirs, const std::string& parser);
};

} // namespace yasm

#endif

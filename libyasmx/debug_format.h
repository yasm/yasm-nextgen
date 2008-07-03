#ifndef YASM_DEBUG_FORMAT_H
#define YASM_DEBUG_FORMAT_H
///
/// @file
/// @brief YASM debug format interface.
///
/// @license
///  Copyright (C) 2002-2007  Peter Johnson
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

#include "export.h"
#include "module.h"


namespace yasm
{

class Errwarns;
class Linemap;
class Object;

/// Debug format interface.
/// To make debug format truly usable, set_object() needs to be called.
class YASM_LIB_EXPORT DebugFormat : public Module
{
public:
    enum { module_type = 2 };

    /// Destructor.
    virtual ~DebugFormat();

    /// Get the module type.
    /// @return "DebugFormat".
    std::string get_type() const;

    /// Set associated object.
    /// @param object       object
    /// @return False on error (object format cannot handle that object).
    virtual bool set_object(Object* object) = 0;

    /// Generate debugging information bytecodes.
    /// @param linemap      virtual/physical line mapping
    /// @param errwarns     error/warning set
    /// @note Errors and warnings are stored into errwarns.
    virtual void generate(Linemap& linemap, Errwarns& errwarns) = 0;
};

} // namespace yasm

#endif

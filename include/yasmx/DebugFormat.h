#ifndef YASM_DEBUGFORMAT_H
#define YASM_DEBUGFORMAT_H
///
/// @file
/// @brief Debug format interface.
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
#include <memory>

#include "yasmx/Config/export.h"

#include "yasmx/Module.h"


namespace yasm
{

class DebugFormatModule;
class Directives;
class Errwarns;
class Linemap;
class Object;

/// Debug format interface.
class YASM_LIB_EXPORT DebugFormat
{
public:
    /// Constructor.
    DebugFormat(const DebugFormatModule& module) : m_module(module) {}

    /// Destructor.
    virtual ~DebugFormat();

    /// Get module.
    const DebugFormatModule& getModule() const { return m_module; }

    /// Add directive handlers.
    virtual void AddDirectives(Directives& dirs, const char* parser);

    /// Generate debugging information bytecodes.
    /// @param linemap      virtual/physical line mapping
    /// @param errwarns     error/warning set
    /// @note Errors and warnings are stored into errwarns.
    virtual void Generate(Linemap& linemap, Errwarns& errwarns) = 0;

private:
    DebugFormat(const DebugFormat&);                  // not implemented
    const DebugFormat& operator=(const DebugFormat&); // not implemented

    const DebugFormatModule& m_module;
};

/// Debug format module interface.
class YASM_LIB_EXPORT DebugFormatModule : public Module
{
public:
    enum { module_type = 2 };

    /// Destructor.
    virtual ~DebugFormatModule();

    /// Get the module type.
    /// @return "DebugFormat".
    const char* getType() const;

    /// Determine if object is acceptable to debug format.
    /// @param object       object
    /// @return False on error (debug format cannot handle the object).
    virtual bool isOkObject(Object& object) const = 0;

    /// DebugFormat factory function.
    /// @return New debug format.
    virtual std::auto_ptr<DebugFormat> Create(Object& object) const = 0;
};

template <typename DebugFormatImpl>
class DebugFormatModuleImpl : public DebugFormatModule
{
public:
    DebugFormatModuleImpl() {}
    ~DebugFormatModuleImpl() {}

    const char* getName() const { return DebugFormatImpl::getName(); }
    const char* getKeyword() const { return DebugFormatImpl::getKeyword(); }

    bool isOkObject(Object& object) const
    { return DebugFormatImpl::isOkObject(object); }

    std::auto_ptr<DebugFormat> Create(Object& object) const
    {
        return std::auto_ptr<DebugFormat>(new DebugFormatImpl(*this, object));
    }
};

} // namespace yasm

#endif

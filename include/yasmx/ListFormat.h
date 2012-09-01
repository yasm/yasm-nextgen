#ifndef YASM_LISTFORMAT_H
#define YASM_LISTFORMAT_H
///
/// @file
/// @brief List format interface.
///
/// @license
///  Copyright (C) 2004-2007  Peter Johnson
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

#include "llvm/ADT/StringRef.h"
#include "yasmx/Basic/LLVM.h"
#include "yasmx/Config/export.h"

#include "yasmx/Module.h"


namespace yasm
{

class Arch;
class Directives;
class Linemap;
class ListFormatModule;

/// List format interface.
class YASM_LIB_EXPORT ListFormat
{
public:
    /// Constructor.
    ListFormat(const ListFormatModule& module) : m_module(module) {}

    /// Destructor.
    virtual ~ListFormat();

    /// Get module.
    const ListFormatModule& getModule() const { return m_module; }

    /// Add directive handlers.
    virtual void AddDirectives(Directives& dirs, StringRef parser);

    /// Write out list to the list file.
    /// This function may call all read-only yasm:: functions as necessary.
    /// @param os           output stream
    /// @param linemap      line mapping repository
    /// @param arch         architecture
    virtual void Output(raw_ostream& os,
                        Linemap& linemap,
                        Arch& arch) = 0;

private:
    ListFormat(const ListFormat&);                  // not implemented
    const ListFormat& operator=(const ListFormat&); // not implemented

    const ListFormatModule& m_module;
};

/// List format module interface.
class YASM_LIB_EXPORT ListFormatModule : public Module
{
public:
    enum { module_type = 3 };

    /// Destructor.
    virtual ~ListFormatModule();

    /// Get the module type.
    /// @return "ListFormat".
    StringRef getType() const;

    /// ListFormat factory function.
    /// @return New list format.
    virtual std::auto_ptr<ListFormat> Create() const = 0;
};

template <typename ListFormatImpl>
class ListFormatModuleImpl : public ListFormatModule
{
public:
    ListFormatModuleImpl() {}
    ~ListFormatModuleImpl() {}

    StringRef getName() const { return ListFormatImpl::getName(); }
    StringRef getKeyword() const { return ListFormatImpl::getKeyword(); }

    std::auto_ptr<ListFormat> Create() const
    {
        return std::auto_ptr<ListFormat>(new ListFormatImpl(*this));
    }
};

} // namespace yasm

#endif

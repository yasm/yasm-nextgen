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
#include <iosfwd>
#include <memory>

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
    const ListFormatModule& get_module() const { return m_module; }

    /// Add directive handlers.
    virtual void add_directives(Directives& dirs, const char* parser);

    /// Write out list to the list file.
    /// This function may call all read-only yasm:: functions as necessary.
    /// @param os           output stream
    /// @param linemap      line mapping repository
    /// @param arch         architecture
    virtual void output(std::ostream& os, Linemap& linemap, Arch& arch) = 0;

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
    const char* get_type() const;

    /// ListFormat factory function.
    /// @return New list format.
    virtual std::auto_ptr<ListFormat> create() const = 0;
};

template <typename ListFormatImpl>
class ListFormatModuleImpl : public ListFormatModule
{
public:
    ListFormatModuleImpl() {}
    ~ListFormatModuleImpl() {}

    const char* get_name() const { return ListFormatImpl::get_name(); }
    const char* get_keyword() const { return ListFormatImpl::get_keyword(); }

    std::auto_ptr<ListFormat> create() const
    {
        return std::auto_ptr<ListFormat>(new ListFormatImpl(*this));
    }
};

} // namespace yasm

#endif

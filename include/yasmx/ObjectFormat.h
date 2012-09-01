#ifndef YASM_OBJECTFORMAT_H
#define YASM_OBJECTFORMAT_H
///
/// @file
/// @brief Object format module interface.
///
/// @license
///  Copyright (C) 2001-2007  Peter Johnson
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
#include <string>
#include <vector>

#include "llvm/ADT/StringRef.h"
#include "yasmx/Basic/LLVM.h"
#include "yasmx/Config/export.h"

#include "yasmx/Module.h"


namespace yasm
{

class DebugFormat;
class DiagnosticsEngine;
class Directives;
class Object;
class ObjectFormatModule;
class Section;
class SourceLocation;
class SourceManager;

/// Object format interface.
class YASM_LIB_EXPORT ObjectFormat
{
public:
    /// Constructor.
    ObjectFormat(const ObjectFormatModule& module, Object& object)
        : m_module(module), m_object(object)
    {}

    /// Destructor.
    virtual ~ObjectFormat();

    /// Get module.
    const ObjectFormatModule& getModule() const { return m_module; }

    /// Add directive handlers.
    virtual void AddDirectives(Directives& dirs, StringRef parser);

    /// Initialize symbols (default and special symbols).
    /// Called prior to assembly process.
    /// Default implementation does nothing.
    /// @param parser       parser keyword
    virtual void InitSymbols(StringRef parser);

    /// Read object file into associated object.
    /// May create sections, relocations, and bytecodes, as well as modify
    /// any other portion of associated object.
    /// The default implementation returns false (taste before you attempt
    /// reading).
    /// @param sm           source manager (main file is object file to read)
    /// @param diags        diagnostic reporting
    /// @return False if an error occurred.
    virtual bool Read(SourceManager& sm, DiagnosticsEngine& diags);

    /// Write out (post-optimized) sections to the object file.
    /// This function may call #Symbol and #Object functions as necessary
    /// to retrieve symbolic information.
    /// @param os           output object file
    /// @param all_syms     if true, all symbols should be included in
    ///                     the object file
    /// @param dbgfmt       debugging format
    /// @param diags        diagnostic reporting
    /// @note Errors and warnings are reported via diags.
    virtual void Output(raw_fd_ostream& os,
                        bool all_syms,
                        DebugFormat& dbgfmt,
                        DiagnosticsEngine& diags) = 0;

    /// Add a default section to an object.
    /// @return Default section.
    virtual Section* AddDefaultSection() = 0;

    /// Add a section to an object.  Use this function instead of
    /// Object::AppendSection() to ensure the object format is aware of the
    /// section.  As with Object::AppendSection(), this does not check to see
    /// if the section already exists; use Object::FindSection() first.
    /// @param name     section name
    /// @param source   source location
    /// @param diags    diagnostic reporting
    /// @return New section.
    virtual Section* AppendSection(StringRef name,
                                   SourceLocation source,
                                   DiagnosticsEngine& diags) = 0;

private:
    ObjectFormat(const ObjectFormat&);                  // not implemented
    const ObjectFormat& operator=(const ObjectFormat&); // not implemented

    const ObjectFormatModule& m_module;

protected:
    Object& m_object;
};

/// Object format module interface.
class YASM_LIB_EXPORT ObjectFormatModule : public Module
{
public:
    enum { module_type = 4 };

    /// Destructor.
    virtual ~ObjectFormatModule();

    /// Get the module type.
    /// @return "ObjectFormat".
    StringRef getType() const;

    /// Get the default file extension (including the '.').
    /// @return File extension.
    virtual StringRef getExtension() const = 0;

    /// Get default (starting) x86 BITS setting.  This only appies to the
    /// x86 architecture; other architectures ignore this setting.
    /// @return Default x86 BITS setting.
    virtual unsigned int getDefaultX86ModeBits() const = 0;

    /// Get list of debug format (DebugFormat) keywords that are
    /// valid to use with this object format.  The null debug format
    /// ("null") should always be in this list so it's possible to
    /// have no debug output.
    /// @return Vector of debug format keywords.
    virtual std::vector<StringRef> getDebugFormatKeywords() const = 0;

    /// Get default debug format keyword.
    /// @return Default debug format keyword.
    virtual StringRef getDefaultDebugFormatKeyword() const = 0;

    /// Determine if object is acceptable to object format.
    /// @param object       object
    /// @return False on error (object format cannot handle the object).
    virtual bool isOkObject(Object& object) const = 0;

    /// Taste object file to see if it is readable by this object format.
    /// The default implementation always returns false.
    /// @param in           input object file
    /// @param arch_keyword architecture keyword (output)
    /// @param machine      machine (output)
    /// @return True if object file readable, false otherwise.
    virtual bool Taste(const MemoryBuffer& in,
                       /*@out@*/ std::string* arch_keyword,
                       /*@out@*/ std::string* machine) const = 0;

    /// ObjectFormat factory function.
    /// @return New object format.
    virtual std::auto_ptr<ObjectFormat> Create(Object& object) const = 0;
};

template <typename ObjectFormatImpl>
class ObjectFormatModuleImpl : public ObjectFormatModule
{
public:
    ObjectFormatModuleImpl() {}
    ~ObjectFormatModuleImpl() {}

    StringRef getName() const { return ObjectFormatImpl::getName(); }
    StringRef getKeyword() const { return ObjectFormatImpl::getKeyword(); }

    StringRef getExtension() const
    { return ObjectFormatImpl::getExtension(); }
    unsigned int getDefaultX86ModeBits() const
    { return ObjectFormatImpl::getDefaultX86ModeBits(); }

    std::vector<StringRef> getDebugFormatKeywords() const
    { return ObjectFormatImpl::getDebugFormatKeywords(); }
    StringRef getDefaultDebugFormatKeyword() const
    { return ObjectFormatImpl::getDefaultDebugFormatKeyword(); }

    bool isOkObject(Object& object) const
    { return ObjectFormatImpl::isOkObject(object); }
    bool Taste(const MemoryBuffer& in,
               /*@out@*/ std::string* arch_keyword,
               /*@out@*/ std::string* machine) const
    { return ObjectFormatImpl::Taste(in, arch_keyword, machine); }

    std::auto_ptr<ObjectFormat> Create(Object& object) const
    {
        return std::auto_ptr<ObjectFormat>(new ObjectFormatImpl(*this, object));
    }
};

} // namespace yasm

#endif

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
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "yasmx/Config/export.h"

#include "yasmx/Module.h"


namespace yasm
{

class Directives;
class Errwarns;
class NameValues;
class Object;
class ObjectFormatModule;
class Section;

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
    const ObjectFormatModule& get_module() const { return m_module; }

    /// Add directive handlers.
    virtual void add_directives(Directives& dirs, const char* parser);

    /// Initialize symbols (default and special symbols).
    /// Called prior to assembly process.
    /// Default implementation does nothing.
    /// @param parser       parser keyword
    virtual void init_symbols(const char* parser);

    /// Read object file into associated object.
    /// May create sections, relocations, and bytecodes, as well as modify
    /// any other portion of associated object.
    /// The default implementation always throws NotImplementedError.
    /// @param is           input object file
    virtual void read(std::istream& is);

    /// Write out (post-optimized) sections to the object file.
    /// This function may call #Symbol and #Object functions as necessary
    /// to retrieve symbolic information.
    /// @param os           output object file
    /// @param all_syms     if true, all symbols should be included in
    ///                     the object file
    /// @param errwarns     error/warning set
    /// @note Errors and warnings are stored into errwarns.
    virtual void output(std::ostream& os,
                        bool all_syms,
                        Errwarns& errwarns) = 0;

    /// Add a default section to an object.
    /// @return Default section.
    virtual Section* add_default_section() = 0;

    /// Add a section to an object.  Use this function instead of
    /// Object::append_section() to ensure the object format is aware of the
    /// section.  As with Object::append_section(), this does not check to see
    /// if the section already exists; use Object::find_section() first.
    /// @return New section.
    virtual Section* append_section(const std::string& name,
                                    unsigned long line) = 0;

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
    const char* get_type() const;

    /// Get the default file extension (including the '.').
    /// @return File extension.
    virtual const char* get_extension() const = 0;

    /// Get default (starting) x86 BITS setting.  This only appies to the
    /// x86 architecture; other architectures ignore this setting.
    /// @return Default x86 BITS setting.
    virtual unsigned int get_default_x86_mode_bits() const = 0;

    /// Get list of debug format (DebugFormat) keywords that are
    /// valid to use with this object format.  The null debug format
    /// ("null") should always be in this list so it's possible to
    /// have no debug output.
    /// @return Vector of debug format keywords.
    virtual std::vector<const char*> get_dbgfmt_keywords() const = 0;

    /// Get default debug format keyword.
    /// @return Default debug format keyword.
    virtual const char* get_default_dbgfmt_keyword() const = 0;

    /// Determine if object is acceptable to object format.
    /// @param object       object
    /// @return False on error (object format cannot handle the object).
    virtual bool ok_object(Object& object) const = 0;

    /// Taste object file to see if it is readable by this object format.
    /// The default implementation always returns false.
    /// @param is           input object file
    /// @param arch_keyword architecture keyword (output)
    /// @param machine      machine (output)
    /// @return True if object file readable, false otherwise.
    virtual bool taste(std::istream& is,
                       /*@out@*/ std::string* arch_keyword,
                       /*@out@*/ std::string* machine) const = 0;

    /// ObjectFormat factory function.
    /// @return New object format.
    virtual std::auto_ptr<ObjectFormat> create(Object& object) const = 0;
};

template <typename ObjectFormatImpl>
class ObjectFormatModuleImpl : public ObjectFormatModule
{
public:
    ObjectFormatModuleImpl() {}
    ~ObjectFormatModuleImpl() {}

    const char* get_name() const { return ObjectFormatImpl::get_name(); }
    const char* get_keyword() const { return ObjectFormatImpl::get_keyword(); }

    const char* get_extension() const
    { return ObjectFormatImpl::get_extension(); }
    unsigned int get_default_x86_mode_bits() const
    { return ObjectFormatImpl::get_default_x86_mode_bits(); }

    std::vector<const char*> get_dbgfmt_keywords() const
    { return ObjectFormatImpl::get_dbgfmt_keywords(); }
    const char* get_default_dbgfmt_keyword() const
    { return ObjectFormatImpl::get_default_dbgfmt_keyword(); }

    bool ok_object(Object& object) const
    { return ObjectFormatImpl::ok_object(object); }
    bool taste(std::istream& is,
               /*@out@*/ std::string* arch_keyword,
               /*@out@*/ std::string* machine) const
    { return ObjectFormatImpl::taste(is, arch_keyword, machine); }

    std::auto_ptr<ObjectFormat> create(Object& object) const
    {
        return std::auto_ptr<ObjectFormat>(new ObjectFormatImpl(*this, object));
    }
};

} // namespace yasm

#endif

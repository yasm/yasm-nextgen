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
#include <string>
#include <vector>

#include "yasmx/Config/export.h"

#include "yasmx/Module.h"


namespace yasm
{

class Errwarns;
class NameValues;
class Object;
class Section;

/// Object format interface.
class YASM_LIB_EXPORT ObjectFormat : public Module
{
public:
    enum { module_type = 4 };

    /// Constructor.
    /// To make object format truly usable, set_object()
    /// needs to be called.
    ObjectFormat();

    /// Destructor.
    virtual ~ObjectFormat();

    /// Get the module type.
    /// @return "ObjectFormat".
    std::string get_type() const;

    /// Set associated object.
    /// @param object       object
    /// @return False on error (object format cannot handle that object).
    /// @note The default implementation accepts all objects.
    bool set_object(Object* object);

    /// Get the default file extension (including the '.').
    /// @return File extension.
    virtual std::string get_extension() const = 0;

    /// Get default (starting) x86 BITS setting.  This only appies to the
    /// x86 architecture; other architectures ignore this setting.
    /// @return Default x86 BITS setting.
    virtual unsigned int get_default_x86_mode_bits() const = 0;

    /// Get list of debug format (DebugFormat) keywords that are
    /// valid to use with this object format.  The null debug format
    /// ("null") should always be in this list so it's possible to
    /// have no debug output.
    /// @return Vector of debug format keywords.
    virtual std::vector<std::string> get_dbgfmt_keywords() const = 0;

    /// Get default debug format keyword.
    /// @return Default debug format keyword.
    virtual std::string get_default_dbgfmt_keyword() const = 0;

    /// Initialize symbols (default and special symbols).
    /// Called after set_object() but prior to assembly process.
    /// Default implementation does nothing.
    /// @param parser       parser keyword
    virtual void init_symbols(const std::string& parser);

    /// Taste object file to see if it is readable by this object format.
    /// The default implementation always returns false.
    /// @param is           input object file
    /// @param arch_keyword architecture keyword (output)
    /// @param machine      machine (output)
    /// @return True if object file readable, false otherwise.
    virtual bool taste(std::istream& is,
                       /*@out@*/ std::string* arch_keyword,
                       /*@out@*/ std::string* machine);

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

protected:
    /// Determine if object is acceptable to object format.
    /// @param object       object
    /// @return False on error (object format cannot handle the object).
    /// @note The default implementation accepts all objects.
    virtual bool ok_object(Object* object) const;

    /// Initialize object format.  Called by set_object() after m_object
    /// is initialized.  Default implementation does nothing.
    virtual void initialize();

    Object* m_object;
};

} // namespace yasm

#endif

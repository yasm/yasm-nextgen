#ifndef YASM_OBJECT_FORMAT_H
#define YASM_OBJECT_FORMAT_H
///
/// @file libyasm/object_format.h
/// @brief YASM object format module interface.
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
#include <ostream>
#include <string>
#include <vector>

#include <boost/noncopyable.hpp>


namespace yasm {

class Errwarns;
class Object;
class Section;

/// Object format interface.
class ObjectFormat : private boost::noncopyable {
public:
#if 0
    /** NULL-terminated list of directives.  NULL if none. */
    /*@null@*/ const yasm_directive *directives;
#endif
    /// Constructor.
    /// To make object format truly usable, set_object()
    /// needs to be called.
    ObjectFormat();

    /// Destructor.
    virtual ~ObjectFormat() {}

    /// Set associated object.
    /// @param object       object
    /// @return False on error (object format cannot handle that object).
    virtual bool set_object(Object* object) = 0;

    /// Get the one-line description of the object format.
    /// @return One-line description of the object format.
    virtual std::string get_name() const = 0;

    /// Get the keyword used to select the object format.
    /// @return Object format keyword.
    virtual std::string get_keyword() const = 0;

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

    /// Write out (post-optimized) sections to the object file.
    /// This function may call #Symbol and #Object functions as necessary
    /// to retrieve symbolic information.
    /// @param os           output object file
    /// @param all_syms     if true, all symbols should be included in
    ///                     the object file
    /// @param errwarns     error/warning set
    /// @note Errors and warnings are stored into errwarns.
    virtual void output(std::ostream& os, bool all_syms,
                        Errwarns& errwarns) = 0;

    /// Add a default section to an object.
    /// @return Default section.
    virtual Section* add_default_section() = 0;
#if 0
    /// Switch object file sections.  The first val of the valparams should
    /// be the section name.
    /// @param object               object
    /// @param valparams            value/parameters
    /// @param objext_valparams     object format-specific value/parameters
    /// @param line                 virtual line (from yasm_linemap)
    /// @return NULL on error, otherwise new section.
    virtual Section* section_switch(yasm_valparamhead *valparams,
         /*@null@*/ yasm_valparamhead *objext_valparams,
         unsigned long line) = 0;
#endif
};

} // namespace yasm

#endif

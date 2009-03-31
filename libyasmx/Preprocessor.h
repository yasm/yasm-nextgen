#ifndef YASM_PREPROCESSOR_H
#define YASM_PREPROCESSOR_H
///
/// @file
/// @brief Preprocessor interface.
///
/// @license
///  Copyright (C) 2001-2007  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
/// 1. Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
/// 2. Redistributions in binary form must reproduce the above copyright
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

#include "export.h"
#include "Module.h"


namespace yasm
{

class Errwarns;
class Linemap;

/// Preprocesor interface.
/// To make preprocessor usable, init() needs to be called.
class YASM_LIB_EXPORT Preprocessor : public Module
{
public:
    enum { module_type = 6 };

    /// Destructor.
    virtual ~Preprocessor();

    /// Get the module type.
    /// @return "Preprocessor".
    std::string get_type() const;

    /// Initialize preprocessor.
    /// The preprocessor needs access to the object format to find out
    /// any object format specific macros.
    /// @param is           initial starting stream
    /// @param in_filename  initial starting file filename
    /// @param linemap      line mapping repository
    /// @param errwarns     error/warning set
    /// @return New preprocessor.
    /// @note Errors/warnings are stored into errwarns.
    virtual void init(std::istream& is, const std::string& in_filename,
                      Linemap& linemap, Errwarns& errwarns) = 0;

    /// Gets a line of preprocessed source code.
    /// @param line     destination string for line of preprocessed source
    /// @return True if line read; false if no more lines.
    virtual bool get_line(/*@out@*/ std::string& line) = 0;

    /// Get the next filename included by the source code.
    /// @return Filename.
    virtual std::string get_included_file() = 0;

    /// Pre-include a file.
    /// @param filename     filename
    virtual void add_include_file(const std::string& filename) = 0;

    /// Pre-define a macro.
    /// @param macronameval "name=value" string
    virtual void predefine_macro(const std::string& macronameval) = 0;

    /// Un-define a macro.
    /// @param macroname    macro name
    virtual void undefine_macro(const std::string& macroname) = 0;

    /// Define a builtin macro, preprocessed before the "standard" macros.
    /// @param macronameval "name=value" string
    virtual void define_builtin(const std::string& macronameval) = 0;
};

} // namespace yasm

#endif

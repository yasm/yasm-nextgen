#ifndef YASM_PARSER_H
#define YASM_PARSER_H
///
/// @file
/// @brief Parser interface.
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
#include <vector>

#include "Config/export.h"

#include "Module.h"


namespace yasm
{

class Directives;
class Errwarns;
class Linemap;
class Object;
class Preprocessor;

/// Parser interface.  The "front end" of the assembler.
class YASM_LIB_EXPORT Parser : public Module
{
public:
    enum { module_type = 5 };

    /// Destructor.
    virtual ~Parser();

    /// Get the module type.
    /// @return "Parser".
    std::string get_type() const;

    /// Get list of preprocessor (Preprocessor) keywords that are
    /// recommended to use with this parser.  The raw preprocessor
    /// ("raw") should always be in this list.
    /// @return Vector of preprocessor keywords.
    virtual std::vector<std::string> get_preproc_keywords() const = 0;

    /// Get default preprocessor keyword.
    /// @return Default preprocessor keyword.
    virtual std::string get_default_preproc_keyword() const = 0;

    /// Parse an input stream into an object.
    /// @param object       object to parse into
    /// @param preproc      preprocessor
    /// @param save_input   true if the parser should save the original
    ///                     lines of source into the linemap (via
    ///                     Linemap::add_source()).
    /// @param dirs         available directives
    /// @param linemap      line mapping repository
    /// @param errwarns     error/warning set
    /// @note Parse errors and warnings are stored into errwarns.
    virtual void parse(Object& object,
                       Preprocessor& preproc,
                       bool save_input,
                       Directives& dirs,
                       Linemap& linemap,
                       Errwarns& errwarns) = 0;
};

} // namespace yasm

#endif

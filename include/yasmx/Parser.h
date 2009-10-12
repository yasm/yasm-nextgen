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
#include <memory>
#include <vector>

#include "yasmx/Config/export.h"

#include "yasmx/Module.h"


namespace llvm { class StringRef; }

namespace yasm
{

class Directives;
class Errwarns;
class Linemap;
class Object;
class ParserModule;
class Preprocessor;

/// Parser interface.  The "front end" of the assembler.
class YASM_LIB_EXPORT Parser
{
public:
    /// Constructor.
    Parser(const ParserModule& module, Errwarns& errwarns)
        : m_module(module), m_errwarns(errwarns)
    {}

    /// Destructor.
    virtual ~Parser();

    /// Get module.
    const ParserModule& get_module() const { return m_module; }

    /// Add directive handlers.
    virtual void AddDirectives(Directives& dirs, const llvm::StringRef& parser);

    /// Parse an input stream into an object.
    /// @param object       object to parse into
    /// @param preproc      preprocessor
    /// @param dirs         available directives
    /// @note Parse errors and warnings are stored into errwarns.
    virtual void Parse(Object& object,
                       Preprocessor& preproc,
                       Directives& dirs) = 0;

private:
    Parser(const Parser&);                  // not implemented
    const Parser& operator=(const Parser&); // not implemented

    const ParserModule& m_module;

protected:
    Errwarns& m_errwarns;
};

/// Parser module interface.
class YASM_LIB_EXPORT ParserModule : public Module
{
public:
    enum { module_type = 5 };

    /// Destructor.
    virtual ~ParserModule();

    /// Get the module type.
    /// @return "Parser".
    const char* getType() const;

    /// Get list of preprocessor (Preprocessor) keywords that are
    /// recommended to use with this parser.  The raw preprocessor
    /// ("raw") should always be in this list.
    /// @return Vector of preprocessor keywords.
    virtual std::vector<const char*> getPreprocessorKeywords() const = 0;

    /// Get default preprocessor keyword.
    /// @return Default preprocessor keyword.
    virtual const char* getDefaultPreprocessorKeyword() const = 0;

    /// Parser factory function.
    /// @param errwarns     error/warning set
    /// @return New parser.
    /// @note Errors/warnings are stored into errwarns.
    virtual std::auto_ptr<Parser> Create(Errwarns& errwarns) const = 0;
};

template <typename ParserImpl>
class ParserModuleImpl : public ParserModule
{
public:
    ParserModuleImpl() {}
    ~ParserModuleImpl() {}

    const char* getName() const { return ParserImpl::getName(); }
    const char* getKeyword() const { return ParserImpl::getKeyword(); }

    std::vector<const char*> getPreprocessorKeywords() const
    { return ParserImpl::getPreprocessorKeywords(); }
    const char* getDefaultPreprocessorKeyword() const
    { return ParserImpl::getDefaultPreprocessorKeyword(); }

    std::auto_ptr<Parser> Create(Errwarns& errwarns) const
    {
        return std::auto_ptr<Parser>(new ParserImpl(*this, errwarns));
    }
};

} // namespace yasm

#endif

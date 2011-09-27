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
#include <memory>
#include <string>
#include <vector>

#include "llvm/ADT/StringRef.h"
#include "yasmx/Config/export.h"

#include "yasmx/Module.h"


namespace yasm
{

class Diagnostic;
class Directives;
class HeaderSearch;
class Linemap;
class Object;
class ParserModule;
class Preprocessor;
class SourceManager;

/// Parser interface.  The "front end" of the assembler.
class YASM_LIB_EXPORT Parser
{
public:
    /// Constructor.
    Parser(const ParserModule& module) : m_module(module) {}

    /// Destructor.
    virtual ~Parser();

    /// Get module.
    const ParserModule& getModule() const { return m_module; }

    /// Add directive handlers.
    virtual void AddDirectives(Directives& dirs, llvm::StringRef parser);

    virtual Preprocessor& getPreprocessor() const = 0;

    /// Parse an input stream into an object.
    /// @param object       object to parse into
    /// @param preproc      preprocessor
    /// @param dirs         available directives
    /// @param diags        diagnostic reporter
    /// @note Parse errors and warnings are stored into errwarns.
    virtual void Parse(Object& object, Directives& dirs, Diagnostic& diags) = 0;

private:
    Parser(const Parser&);                  // not implemented
    const Parser& operator=(const Parser&); // not implemented

    const ParserModule& m_module;
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
    llvm::StringRef getType() const;

    /// Parser factory function.
    /// @param diags        diagnostic reporting
    /// @param sm           source manager
    /// @return New parser.
    /// @note It is assumed sm is already loaded with a main file.
    virtual std::auto_ptr<Parser> Create(Diagnostic& diags,
                                         SourceManager& sm,
                                         HeaderSearch& headers) const = 0;
};

template <typename ParserImpl>
class ParserModuleImpl : public ParserModule
{
public:
    ParserModuleImpl() {}
    ~ParserModuleImpl() {}

    llvm::StringRef getName() const { return ParserImpl::getName(); }
    llvm::StringRef getKeyword() const { return ParserImpl::getKeyword(); }

    std::auto_ptr<Parser> Create(Diagnostic& diags,
                                 SourceManager& sm,
                                 HeaderSearch& headers) const
    {
        return std::auto_ptr<Parser>(new ParserImpl(*this, diags, sm, headers));
    }
};

} // namespace yasm

#endif

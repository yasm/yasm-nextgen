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
#include <memory>
#include <string>

#include "llvm/ADT/StringRef.h"
#include "yasmx/Config/export.h"

#include "yasmx/Module.h"


namespace clang {
class FileManager; class SourceLocation; class SourceManager;
}
namespace llvm { class MemoryBuffer; }

namespace yasm
{

class Directives;
class Errwarns;
class Linemap;
class PreprocessorModule;

/// Preprocesor interface.
class YASM_LIB_EXPORT Preprocessor
{
public:
    /// Constructor.
    Preprocessor(const PreprocessorModule& module, Errwarns& errwarns)
        : m_module(module), m_errwarns(errwarns)
    {}

    /// Destructor.
    virtual ~Preprocessor();

    /// Get module.
    const PreprocessorModule& getModule() const { return m_module; }

    /// Add directive handlers.
    virtual void AddDirectives(Directives& dirs, llvm::StringRef parser);

    /// Initialize preprocessor.  Must be called prior to first getLine() call.
    /// It is assumed source_mgr is already loaded with a main file.
    /// @param source_mgr   source manager
    /// @param file_mgr     file manager
    virtual void Initialize(clang::SourceManager& source_mgr,
                            clang::FileManager& file_mgr) = 0;

    /// Gets a line of preprocessed source code.
    /// @param line     destination string for line of preprocessed source
    /// @param source   source location of beginning of line
    /// @return True if line read; false if no more lines.
    virtual bool getLine(/*@out@*/ std::string* line,
                         /*@out@*/ clang::SourceLocation* source) = 0;

    /// Gets the source manager.
    virtual clang::SourceManager& getSourceManager() = 0;

    /// Get the next filename included by the source code.
    /// @return Filename.
    virtual std::string getIncludedFile() = 0;

    /// Pre-include a file.
    /// @param filename     filename
    virtual void AddIncludeFile(llvm::StringRef filename) = 0;

    /// Pre-define a macro.
    /// @param macronameval "name=value" string
    virtual void PredefineMacro(llvm::StringRef macronameval) = 0;

    /// Un-define a macro.
    /// @param macroname    macro name
    virtual void UndefineMacro(llvm::StringRef macroname) = 0;

    /// Define a builtin macro, preprocessed before the "standard" macros.
    /// @param macronameval "name=value" string
    virtual void DefineBuiltin(llvm::StringRef macronameval) = 0;

private:
    Preprocessor(const Preprocessor&);                  // not implemented
    const Preprocessor& operator=(const Preprocessor&); // not implemented

    const PreprocessorModule& m_module;

protected:
    Errwarns& m_errwarns;
};

/// Preprocessor module interface.
class YASM_LIB_EXPORT PreprocessorModule : public Module
{
public:
    enum { module_type = 6 };

    /// Destructor.
    virtual ~PreprocessorModule();

    /// Get the module type.
    /// @return "Preprocessor".
    const char* getType() const;

    /// Preprocessor factory function.
    /// The preprocessor needs access to the object format to find out
    /// any object format specific macros.
    /// @param errwarns     error/warning set
    /// @return New preprocessor.
    /// @note Errors/warnings are stored into errwarns.
    virtual std::auto_ptr<Preprocessor> Create(Errwarns& errwarns) const = 0;
};

template <typename PreprocessorImpl>
class PreprocessorModuleImpl : public PreprocessorModule
{
public:
    PreprocessorModuleImpl() {}
    ~PreprocessorModuleImpl() {}

    const char* getName() const { return PreprocessorImpl::getName(); }
    const char* getKeyword() const { return PreprocessorImpl::getKeyword(); }

    std::auto_ptr<Preprocessor> Create(Errwarns& errwarns) const
    {
        return std::auto_ptr<Preprocessor>
            (new PreprocessorImpl(*this, errwarns));
    }
};

} // namespace yasm

#endif

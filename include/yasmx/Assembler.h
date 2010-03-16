#ifndef YASM_ASSEMBLER_H
#define YASM_ASSEMBLER_H
///
/// @file
/// @brief Assembler interface.
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
#include "llvm/ADT/StringRef.h"
#include "yasmx/Config/export.h"
#include "yasmx/Support/scoped_ptr.h"


namespace clang { class FileManager; class SourceManager; }
namespace llvm { class MemoryBuffer; class raw_fd_ostream; }

/// Namespace for classes, functions, and templates related to the Yasm
/// assembler.
namespace yasm
{

class Arch;
class Errwarns;
class Object;
class Preprocessor;

/// An assembler.
class YASM_LIB_EXPORT Assembler
{
public:
    enum ObjectDumpTime
    {
        DUMP_NEVER = 0,
        DUMP_AFTER_PARSE,
        DUMP_AFTER_FINALIZE,
        DUMP_AFTER_OPTIMIZE,
        DUMP_AFTER_OUTPUT
    };

    /// Constructor.  A default section is created as the first
    /// section, and an empty symbol table is created.
    /// The object filename is initially unset (empty string).
    /// @param parser_keyword   parser keyword
    /// @param objfmt_keyword   object format keyword
    /// @param dump_time        when (if ever) to dump object YAML to stderr
    Assembler(llvm::StringRef arch_keyword,
              llvm::StringRef parser_keyword,
              llvm::StringRef objfmt_keyword,
              ObjectDumpTime dump_time = DUMP_NEVER);

    /// Destructor.
    ~Assembler();

    /// Set the object filename; if not set prior to assembly, determined
    /// from source filename according to the object format settings.
    /// @param obj_filename     object filename (e.g. "file.o")
    void setObjectFilename(llvm::StringRef obj_filename);

    /// Set the machine of architecture; if not set prior to assembly,
    /// determined by object format.
    /// @param machine          machine name
    void setMachine(llvm::StringRef machine);

    /// Set the preprocessor; if not set prior to assembly, determined
    /// by parser.
    /// @param preproc_keyword  preprocessor keyword
    void setPreprocessor(llvm::StringRef preproc_keyword);

    /// Set the debug format; if not set prior to assembly, defaults to null
    /// debug format (e.g. no debugging information).
    /// @param dbgfmt_keyword   debug format keyword
    void setDebugFormat(llvm::StringRef dbgfmt_keyword);

    /// Set the list format; if not set prior to assembly, defaults to null
    /// list format (e.g. no list output).
    /// @param listfmt_keyword  list format keyword
    void setListFormat(llvm::StringRef list_keyword);

    /// Actually perform assembly.  Does not write to output file.
    /// It is assumed source_mgr is already loaded with a main file.
    /// @param source_mgr       source manager
    /// @param file_mgr         file manager
    /// @param warning_error    treat warnings as errors if true
    /// @return True on success, false on failure.
    bool Assemble(clang::SourceManager& source_mgr,
                  clang::FileManager& file_mgr,
                  bool warning_error = false);

    /// Write assembly results to output file.  Fails if assembly not
    /// performed first.
    /// @param os               output stream
    /// @param warning_error    treat warnings as errors if true
    /// @return True on success, false on failure.
    bool Output(llvm::raw_fd_ostream& os, bool warning_error = false);

    /// Get the object.  Returns 0 until after assembly is successful.
    /// @return Object.
    Object* getObject();

    /// Get the preprocessor.
    /// @return Preprocessor.
    Preprocessor* getPreprocessor();

    /// Get the architecture.
    /// @return Architecture.
    Arch* getArch();

    /// Get the error/warning set.
    /// @return Errwarns.
    Errwarns* getErrwarns();

    /// Get the object filename.  May return empty string if called prior
    /// to assemble() being called.
    llvm::StringRef getObjectFilename() const;

private:
    Assembler(const Assembler&);                    // not implemented
    const Assembler& operator=(const Assembler&);   // not implemented

    /// Pimpl
    class Impl;
    util::scoped_ptr<Impl> m_impl;
};

} // namespace yasm

#endif

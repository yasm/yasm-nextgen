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
#include "yasmx/Basic/LLVM.h"
#include "yasmx/Config/export.h"
#include "yasmx/Support/scoped_ptr.h"


/// Namespace for classes, functions, and templates related to the Yasm
/// assembler.
namespace yasm
{

class Arch;
class ArchModule;
class DebugFormat;
class DebugFormatModule;
class DiagnosticsEngine;
class FileManager;
class HeaderSearch;
class ListFormat;
class ListFormatModule;
class Object;
class ObjectFormat;
class ObjectFormatModule;
class Parser;
class ParserModule;
class SourceManager;

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
    /// @param arch_keyword     architecture keyword
    /// @param objfmt_keyword   object format keyword
    /// @param diags            diagnostic reporting
    /// @param dump_time        when (if ever) to dump object XML to stderr
    Assembler(StringRef arch_keyword,
              StringRef objfmt_keyword,
              DiagnosticsEngine& diags,
              ObjectDumpTime dump_time = DUMP_NEVER);

    /// Destructor.
    ~Assembler();

    /// Set the object filename; if not set prior to assembly, determined
    /// from source filename according to the object format settings.
    /// @param obj_filename     object filename (e.g. "file.o")
    void setObjectFilename(StringRef obj_filename);

    /// Set the machine of architecture; if not set prior to assembly,
    /// determined by object format.
    /// @param machine          machine name
    /// @param diags            diagnostic reporting
    /// @return False on error.
    bool setMachine(StringRef machine, DiagnosticsEngine& diags);

    /// Set the parser.
    /// @param parser_keyword   parser keyword
    /// @param diags            diagnostic reporting
    /// @return False on error.
    bool setParser(StringRef parser_keyword, DiagnosticsEngine& diags);

    /// Check to see if a particular debug format is legal for the selected
    /// object format.
    /// @return False if not legal.
    bool isOkDebugFormat(StringRef dbgfmt_keyword) const;

    /// Set the debug format; if not set prior to assembly, defaults to null
    /// debug format (e.g. no debugging information).
    /// @param dbgfmt_keyword   debug format keyword
    /// @param diags            diagnostic reporting
    /// @return False on error.
    bool setDebugFormat(StringRef dbgfmt_keyword, DiagnosticsEngine& diags);

    /// Set the list format; if not set prior to assembly, defaults to null
    /// list format (e.g. no list output).
    /// @param listfmt_keyword  list format keyword
    /// @param diags            diagnostic reporting
    /// @return False on error.
    bool setListFormat(StringRef list_keyword, DiagnosticsEngine& diags);

    /// Initialize the object for assembly.  Does not read from input file.
    /// @param source_mgr       source manager
    /// @param diags            diagnostic reporting
    /// @return True on success, false on failure.
    bool InitObject(SourceManager& source_mgr, DiagnosticsEngine& diags);

    /// Initialize parser.  Does not read from input file.
    /// @param source_mgr       source manager
    /// @param diags            diagnostic reporting
    /// @param headers          header search paths
    /// @return Parser.
    Parser& InitParser(SourceManager& source_mgr,
                       DiagnosticsEngine& diags,
                       HeaderSearch& headers);


    /// Actually perform assembly.  Does not write to output file.
    /// It is assumed source_mgr is already loaded with a main file.
    /// @param source_mgr       source manager
    /// @param diags            diagnostic reporting
    /// @return True on success, false on failure.
    bool Assemble(SourceManager& source_mgr, DiagnosticsEngine& diags);

    /// Write assembly results to output file.  Fails if assembly not
    /// performed first.
    /// @param os               output stream
    /// @return True on success, false on failure.
    bool Output(raw_fd_ostream& os, DiagnosticsEngine& diags);

    /// Get the object.  Returns 0 until after InitObject() is called.
    /// @return Object.
    Object* getObject() { return m_object.get(); }

    /// Get the architecture.
    /// @return Architecture.
    Arch* getArch() { return m_arch.get(); }

    /// Get the object filename.  May return empty string if called prior
    /// to assemble() being called.
    StringRef getObjectFilename() const { return m_obj_filename; }

private:
    Assembler(const Assembler&);                    // not implemented
    const Assembler& operator=(const Assembler&);   // not implemented

    util::scoped_ptr<ArchModule> m_arch_module;
    util::scoped_ptr<ParserModule> m_parser_module;
    util::scoped_ptr<ObjectFormatModule> m_objfmt_module;
    util::scoped_ptr<DebugFormatModule> m_dbgfmt_module;
    util::scoped_ptr<ListFormatModule> m_listfmt_module;

    util::scoped_ptr<Arch> m_arch;
    util::scoped_ptr<Parser> m_parser;
    util::scoped_ptr<ObjectFormat> m_objfmt;
    util::scoped_ptr<DebugFormat> m_dbgfmt;
    util::scoped_ptr<ListFormat> m_listfmt;

    util::scoped_ptr<Object> m_object;

    std::string m_obj_filename;
    std::string m_machine;
    Assembler::ObjectDumpTime m_dump_time;
};

} // namespace yasm

#endif

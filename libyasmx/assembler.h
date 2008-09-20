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
#include <iosfwd>
#include <string>

#include "export.h"
#include "scoped_ptr.h"

/// Namespace for classes, functions, and templates related to the Yasm
/// assembler.
namespace yasm
{

class Arch;
class Errwarns;
class Linemap;
class Object;
class Preprocessor;

/// An assembler.
class YASM_LIB_EXPORT Assembler
{
public:
    /// Constructor.  A default section is created as the first
    /// section, and an empty symbol table is created.
    /// The object filename is initially unset (empty string).
    /// @param parser_keyword   parser keyword
    /// @param objfmt_keyword   object format keyword
    Assembler(const std::string& arch_keyword,
              const std::string& parser_keyword,
              const std::string& objfmt_keyword);

    /// Destructor.
    ~Assembler();

    /// Set the object filename; if not set prior to assembly, determined
    /// from source filename according to the object format settings.
    /// @param obj_filename     object filename (e.g. "file.o")
    void set_obj_filename(const std::string& obj_filename);

    /// Set the machine of architecture; if not set prior to assembly,
    /// determined by object format.
    /// @param machine          machine name
    void set_machine(const std::string& machine);

    /// Set the preprocessor; if not set prior to assembly, determined
    /// by parser.
    /// @param preproc_keyword  preprocessor keyword
    void set_preproc(const std::string& preproc_keyword);

    /// Set the debug format; if not set prior to assembly, defaults to null
    /// debug format (e.g. no debugging information).
    /// @param dbgfmt_keyword   debug format keyword
    void set_dbgfmt(const std::string& dbgfmt_keyword);

    /// Set the list format; if not set prior to assembly, defaults to null
    /// list format (e.g. no list output).
    /// @param listfmt_keyword  list format keyword
    void set_listfmt(const std::string& list_keyword);

    /// Actually perform assembly.  Does not write to output file.
    /// @param is               input stream
    /// @param src_filename     source filename (e.g. "file.asm")
    /// @param warning_error    treat warnings as errors if true
    /// @return True on success, false on failure.
    bool assemble(std::istream& is,
                  const std::string& src_filename,
                  bool warning_error = false);

    /// Write assembly results to output file.  Fails if assembly not
    /// performed first.
    /// @param os               output stream
    /// @param warning_error    treat warnings as errors if true
    /// @return True on success, false on failure.
    bool output(std::ostream& os, bool warning_error = false);

    /// Get the object.  Returns 0 until after assembly is successful.
    /// @return Object.
    Object* get_object();

    /// Get the preprocessor.
    /// @return Preprocessor.
    Preprocessor* get_preproc();

    /// Get the architecture.
    /// @return Architecture.
    Arch* get_arch();

    /// Get the error/warning set.
    /// @return Errwarns.
    Errwarns* get_errwarns();

    /// Get the line map.
    /// @return Line map.
    Linemap* get_linemap();

    /// Get the object filename.  May return empty string if called prior
    /// to assemble() being called.
    std::string get_obj_filename() const;

private:
    Assembler(const Assembler&);                    // not implemented
    const Assembler& operator=(const Assembler&);   // not implemented

    /// Pimpl
    class Impl;
    util::scoped_ptr<Impl> m_impl;
};

} // namespace yasm

#endif

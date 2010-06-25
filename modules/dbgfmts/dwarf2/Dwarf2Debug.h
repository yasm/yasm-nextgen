#ifndef YASM_DWARF2DEBUG_H
#define YASM_DWARF2DEBUG_H
//
// DWARF2 debugging format
//
//  Copyright (C) 2006-2007  Peter Johnson
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#include <vector>

#include "clang/Basic/SourceLocation.h"
#include "yasmx/Config/export.h"
#include "yasmx/DebugFormat.h"
#include "yasmx/Location.h"
#include "yasmx/SymbolRef.h"

#define WITH_DWARF3 1
#include "Dwarf2Types.h"


namespace clang { class FileEntry; }

namespace yasm {
class BytecodeContainer;
class DirectiveInfo;
class Section;

namespace dbgfmt {
struct Dwarf2Loc;
struct Dwarf2LineState;

class YASM_STD_EXPORT Dwarf2Debug : public DebugFormat
{
public:
    Dwarf2Debug(const DebugFormatModule& module, Object& object);
    ~Dwarf2Debug();

    static llvm::StringRef getName() { return "DWARF2 debugging format"; }
    static llvm::StringRef getKeyword() { return "dwarf2"; }
    static bool isOkObject(Object& object) { return true; }

    void AddDirectives(Directives& dirs, llvm::StringRef parser);
    void Generate(ObjectFormat& objfmt,
                  clang::SourceManager& smgr,
                  Diagnostic& diags);

    struct Filename
    {
        std::string pathname;       // full filename
        std::string filename;       // basename of full filename
        // index into directories array for relative path;
        // 0 for current directory.
        unsigned long dir;
        unsigned long time;
        unsigned long length;
    };

private:
    ObjectFormat* m_objfmt;
    Diagnostic* m_diags;

    typedef std::vector<std::string> Dirs;
    Dirs m_dirs;

    typedef std::vector<Filename> Filenames;
    Filenames m_filenames;

    enum Format
    {
        FORMAT_32BIT,
        FORMAT_64BIT
    };

    Format m_format;

    unsigned int m_sizeof_address, m_sizeof_offset, m_min_insn_len;

    // Line number directives
    void DirLoc(DirectiveInfo& info, Diagnostic& diags);
    void DirFile(DirectiveInfo& info, Diagnostic& diags);

    /// Generate .debug_line section.
    Section& Generate_line(clang::SourceManager& smgr,
                           bool asm_source,
                           /*@out@*/ Section** main_code,
                           /*@out@*/ size_t* num_line_sections);
    void AppendLineOp(BytecodeContainer& container,
                      unsigned int opcode);
    void AppendLineOp(BytecodeContainer& container,
                      unsigned int opcode,
                      const IntNum& operand);
    void AppendLineExtOp(BytecodeContainer& container,
                         DwarfLineNumberExtOp ext_opcode);
    void AppendLineExtOp(BytecodeContainer& container,
                         DwarfLineNumberExtOp ext_opcode,
                         unsigned long ext_operandsize,
                         SymbolRef ext_operand);
    void GenerateLineSection(Section& sect,
                             Section& debug_line,
                             clang::SourceManager& smgr,
                             bool asm_source,
                             Section** last_code,
                             size_t* num_line_sections);
    void GenerateLineOp(Section& debug_line,
                        Dwarf2LineState* state,
                        const Dwarf2Loc& loc,
                        const Dwarf2Loc* nextloc);
    void GenerateLineBC(Section& debug_line,
                        clang::SourceManager& smgr,
                        Dwarf2LineState* state,
                        Bytecode& bc,
                        Dwarf2Loc* loc,
                        unsigned long* lastfile);
    /// Append statement program prologue
    void AppendSPP(BytecodeContainer& container);

    /// Generate .debug_aranges section (address range table).
    Section& Generate_aranges(Section& debug_info);

#if 0
    /// Generate .debug_pubnames section (name lookup table).
    Section& Generate_pubnames(Section& debug_info);
#endif

    /// Generate .debug_info section.
    Section& Generate_info(Section& debug_line, /*@null@*/ Section* main_code);

    /// Append a debug header.
    /// @return The location of the aranges length field (used by setHeadLength).
    Location AppendHead(Section& sect,
                        /*@null@*/ Section* debug_ptr,
                        bool with_address,
                        bool with_segment);

    /// Set the header length based on a tail location.
    /// @note head and tail must be in the same section.
    /// @param head     location returned by AppendHead().
    /// @param tail     end of data
    void setHeadEnd(Location head, Location tail);

    size_t AddFile(unsigned long filenum, llvm::StringRef pathname);
    size_t AddFile(const clang::FileEntry* file);
    unsigned long AddDir(llvm::StringRef dirname);
};

}} // namespace yasm::dbgfmt

#endif

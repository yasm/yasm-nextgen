#ifndef YASM_DWARFDEBUG_H
#define YASM_DWARFDEBUG_H
//
// DWARF debugging format
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

#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Config/export.h"
#include "yasmx/DebugFormat.h"
#include "yasmx/Location.h"
#include "yasmx/SymbolRef.h"

#define WITH_DWARF3 1
#include "DwarfCfi.h"
#include "DwarfTypes.h"


namespace yasm {
class BytecodeContainer;
class DirectiveInfo;
class FileEntry;
class Section;

namespace dbgfmt {
struct DwarfLoc;
struct DwarfLineState;

class YASM_STD_EXPORT DwarfDebug : public DebugFormat
{
public:
    DwarfDebug(const DebugFormatModule& module, Object& object);
    ~DwarfDebug();

    static StringRef getName() { return "DWARF debugging format"; }
    static StringRef getKeyword() { return "dwarf"; }
    static bool isOkObject(Object& object) { return true; }

    void AddDirectives(Directives& dirs, StringRef parser);
    void Generate(ObjectFormat& objfmt,
                  SourceManager& smgr,
                  DiagnosticsEngine& diags);

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

    bool gotFile() { return !m_filenames.empty(); }

protected:
    void AddCfiDirectives(Directives& dirs, StringRef parser);
    void GenerateCfi(ObjectFormat& objfmt,
                     SourceManager& smgr,
                     DiagnosticsEngine& diags);

    void AddDebugDirectives(Directives& dirs, StringRef parser);
    void GenerateDebug(ObjectFormat& objfmt,
                       SourceManager& smgr,
                       DiagnosticsEngine& diags);

private:
    friend class DwarfCfiCie;
    friend class DwarfCfiFde;
    friend class DwarfCfiInsn;

    ObjectFormat* m_objfmt;
    DiagnosticsEngine* m_diags;

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

    // CFI FDEs
    typedef stdx::ptr_vector<DwarfCfiFde> FDEs;
    FDEs m_fdes;
    stdx::ptr_vector_owner<DwarfCfiFde> m_fdes_owner;

    // CFI directive state
    DwarfCfiFde* m_cur_fde;
    Location m_last_address;
    IntNum m_cfa_cur_offset;
    std::vector<IntNum> m_cfa_stack;

    // CFI sections to emit
    bool m_eh_frame;
    bool m_debug_frame;

    // CFI architecture-specific settings
    int m_cie_data_alignment;
    unsigned int m_default_return_column;
    void (*m_frame_initial_instructions) (DwarfCfiFde& fde);
    unsigned int m_fde_reloc_size;

    // Initialize CFI settings
    void InitCfi(Arch& arch);

    // Line number directives
    void DirLoc(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirFile(DirectiveInfo& info, DiagnosticsEngine& diags);

    // CFI directives
    void DirCfiStartproc(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiEndproc(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiSections(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiPersonality(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiLsda(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiDefCfa(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiDefCfaRegister(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiDefCfaOffset(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiAdjustCfaOffset(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiOffset(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiRelOffset(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiRegister(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiRestore(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiUndefined(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiSameValue(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiRememberState(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiRestoreState(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiReturnColumn(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiSignalFrame(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiWindowSave(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiEscape(DirectiveInfo& info, DiagnosticsEngine& diags);
    void DirCfiValEncodedAddr(DirectiveInfo& info, DiagnosticsEngine& diags);

    // CFI directive helpers
    bool DirCheck(DirectiveInfo& info,
                  DiagnosticsEngine& diags,
                  unsigned int nargs);
    void DirRegNum(NameValue& nv,
                   DiagnosticsEngine& diags,
                   Object* obj,
                   unsigned int* out,
                   bool* out_set);
    void AdvanceCfiAddress(Location loc, SourceLocation source);

    // Generate CFI section.
    void GenerateCfiSection(ObjectFormat& ofmt,
                            DiagnosticsEngine& diags,
                            StringRef sectname,
                            bool eh_frame);

    /// Generate .debug_line section.
    Section& Generate_line(SourceManager& smgr,
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
                         const IntNum& operand);
    void AppendLineExtOp(BytecodeContainer& container,
                         DwarfLineNumberExtOp ext_opcode,
                         unsigned long ext_operandsize,
                         SymbolRef ext_operand);
    void GenerateLineSection(Section& sect,
                             Section& debug_line,
                             SourceManager& smgr,
                             bool asm_source,
                             Section** last_code,
                             size_t* num_line_sections);
    void GenerateLineOp(Section& debug_line,
                        DwarfLineState* state,
                        const DwarfLoc& loc,
                        const DwarfLoc* nextloc);
    void GenerateLineBC(Section& debug_line,
                        SourceManager& smgr,
                        DwarfLineState* state,
                        Bytecode& bc,
                        DwarfLoc* loc,
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

    size_t AddFile(unsigned long filenum, StringRef pathname);
    size_t AddFile(const FileEntry* file);
    unsigned long AddDir(StringRef dirname);
};

class YASM_STD_EXPORT DwarfPassDebug : public DwarfDebug
{
public:
    DwarfPassDebug(const DebugFormatModule& module, Object& object)
        : DwarfDebug(module, object)
    {}
    ~DwarfPassDebug();

    static StringRef getName() { return "DWARF passthrough only"; }
    static StringRef getKeyword() { return "dwarfpass"; }
    static bool isOkObject(Object& object) { return true; }

    void Generate(ObjectFormat& objfmt,
                  SourceManager& smgr,
                  DiagnosticsEngine& diags);
};

class YASM_STD_EXPORT CfiDebug : public DwarfDebug
{
public:
    CfiDebug(const DebugFormatModule& module, Object& object)
        : DwarfDebug(module, object)
    {}
    ~CfiDebug();

    static StringRef getName() { return "CFI information only"; }
    static StringRef getKeyword() { return "cfi"; }
    static bool isOkObject(Object& object) { return true; }

    void AddDirectives(Directives& dirs, StringRef parser);
    void Generate(ObjectFormat& objfmt,
                  SourceManager& smgr,
                  DiagnosticsEngine& diags);
};


}} // namespace yasm::dbgfmt

#endif

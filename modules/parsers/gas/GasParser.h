#ifndef YASM_GASPARSER_H
#define YASM_GASPARSER_H
//
// GAS-compatible parser header file
//
//  Copyright (C) 2005-2007  Peter Johnson
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the author nor the names of other contributors
//    may be used to endorse or promote products derived from this
//    software without specific prior written permission.
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
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/StringMap.h"
#include "yasmx/Basic/SourceLocation.h"
#include "yasmx/Config/export.h"
#include "yasmx/Parse/Parser.h"
#include "yasmx/Parse/ParserImpl.h"
#include "yasmx/Insn.h"
#include "yasmx/IntNum.h"

#include "GasPreproc.h"

namespace yasm
{

class Arch;
class Bytecode;
class Directives;
class Expr;
class FloatNum;
class IntNum;
class NameValues;
class Register;
class RegisterGroup;
class Section;

namespace parser
{

#define YYCTYPE         char

class GasParser;
struct GasDirLookup
{
    const char* name;
    bool (GasParser::*handler) (unsigned int, SourceLocation source);
    unsigned int param;
};

class YASM_STD_EXPORT GasParser : public Parser, public ParserImpl
{
public:
    GasParser(const ParserModule& module,
              DiagnosticsEngine& diags,
              SourceManager& sm,
              HeaderSearch& headers);
    ~GasParser();

    void AddDirectives(Directives& dirs, StringRef parser);

    Preprocessor& getPreprocessor() const;

    static StringRef getName() { return "GNU AS (GAS)-compatible parser"; }
    static StringRef getKeyword() { return "gas"; }

    void Parse(Object& object, Directives& dirs, DiagnosticsEngine& diags);

private:

    /// Get the local label name for the given numeric index + suffix.
    /// @param name     label name (output)
    /// @param num      numeric index string
    /// @param suffix   suffix (e.g. 'f', 'b' for reference, ':' for
    ///                 definition)
    /// @param source   source location of numeric index
    /// @param inc      increment label index (typically used for definition).
    /// @return True on success, false on error.
    bool getLocalLabel(SmallVectorImpl<char>& name,
                       StringRef num,
                       char suffix,
                       SourceLocation source,
                       bool inc = false);

    bool ParseLine();
    void setDebugFile(StringRef filename,
                      SourceRange filename_source,
                      SourceLocation dir_source);
    void setDebugFile(const IntNum& fileno,
                      SourceRange fileno_source,
                      StringRef filename,
                      SourceRange filename_source,
                      SourceLocation dir_source);
    void ParseCppLineMarker();
    void ParseNasmLineMarker();

    bool ParseDirLine(unsigned int, SourceLocation source);
    bool ParseDirInclude(unsigned int, SourceLocation source);
    bool ParseDirMacro(unsigned int, SourceLocation source);
    bool ParseDirEndm(unsigned int, SourceLocation source);
    bool ParseDirRept(unsigned int, SourceLocation source);
    bool ParseDirEndr(unsigned int, SourceLocation source);
    bool ParseDirAlign(unsigned int power2, SourceLocation source);
    bool ParseDirOrg(unsigned int, SourceLocation source);
    bool ParseDirLocal(unsigned int, SourceLocation source);
    bool ParseDirComm(unsigned int is_lcomm, SourceLocation source);
    bool ParseDirAscii(unsigned int withzero, SourceLocation source);
    bool ParseDirFloat(unsigned int size, SourceLocation source);
    bool ParseDirData(unsigned int size, SourceLocation source);
    bool ParseDirLeb128(unsigned int sign, SourceLocation source);
    bool ParseDirZero(unsigned int, SourceLocation source);
    bool ParseDirSkip(unsigned int size, SourceLocation source);
    bool ParseDirFill(unsigned int, SourceLocation source);
    bool ParseDirFloatFill(unsigned int size, SourceLocation source);
    bool ParseDirBssSection(unsigned int, SourceLocation source);
    bool ParseDirDataSection(unsigned int, SourceLocation source);
    bool ParseDirTextSection(unsigned int, SourceLocation source);
    bool ParseDirSection(unsigned int, SourceLocation source);
    bool ParseDirPopSection(unsigned int, SourceLocation source);
    bool ParseDirPrevious(unsigned int, SourceLocation source);
    bool ParseDirEqu(unsigned int, SourceLocation source);
    bool ParseDirFile(unsigned int, SourceLocation source);

    void SkipConditional(SourceLocation begin);
    void HandleIf(bool is_true, SourceLocation begin);
    bool ParseDirElse(unsigned int, SourceLocation source);
    bool ParseDirElseif(unsigned int, SourceLocation source);
    bool ParseDirEndif(unsigned int, SourceLocation source);
    bool ParseDirIf(unsigned int op, SourceLocation source);
    bool ParseDirIfb(unsigned int negate, SourceLocation source);
    bool ParseDirIfdef(unsigned int negate, SourceLocation source);
    bool ParseDirIfeqs(unsigned int negate, SourceLocation source);

    bool ParseDirSyntax(unsigned int intel, SourceLocation source);

    Insn::Ptr ParseInsn();
    bool ParseDirective(NameValues* nvs, const ParseExprTerm* parse_term = 0);
    Operand ParseMemoryAddress();
    Operand ParseRegOperand();
    Operand ParseOperand();

    unsigned int getSizeOverride();
    Operand ParseMemoryAddressIntel();
    Operand ParseOperandIntel();

    bool ParseExpr(Expr& e, const ParseExprTerm* parse_term = 0);
    bool ParseExpr0(Expr& e, const ParseExprTerm* parse_term);
    bool ParseExpr1(Expr& e, const ParseExprTerm* parse_term);
    bool ParseExpr2(Expr& e, const ParseExprTerm* parse_term);
    bool ParseExpr3(Expr& e, const ParseExprTerm* parse_term);

    SymbolRef ParseSymbol(IdentifierInfo* ii);
    bool ParseInteger(IntNum* intn);
    const Register* ParseRegister();

    void DefineLabel(StringRef name, SourceLocation source);
    void DefineLcomm(SymbolRef sym,
                     SourceLocation source,
                     std::auto_ptr<Expr> size,
                     const Expr& align);
    void SwitchSection(StringRef name, bool builtin, SourceRange source);
    Section& getSection(StringRef name, bool builtin, SourceRange source);

    void DoParse();

    Object* m_object;
    Arch* m_arch;
    Directives* m_dirs;

    GasPreproc m_gas_preproc;

    BytecodeContainer* m_container;
    /*@null@*/ Bytecode* m_bc;

    GasDirLookup m_sized_gas_dirs[1];
    typedef llvm::StringMap<const GasDirLookup*, llvm::BumpPtrAllocator,
                            false> GasDirMap;
    GasDirMap m_gas_dirs;

    // last "base" label for local (.) labels
    std::string m_locallabel_base;

    // .line/.file: we have to see both to start setting linemap versions
    enum
    {
        FL_NONE,
        FL_FILE,
        FL_LINE,
        FL_BOTH
    } m_dir_fileline;
    std::string m_dir_file;
    unsigned long m_dir_line;

    // Have we seen a line marker?
    bool m_seen_line_marker;

    // Index of local labels; what's stored here is the /next/ index,
    // so these are all 0 at start.
    SmallVector<unsigned long, 10> m_local;

    // Start of comment.
    SourceLocation m_comment_start;

    // Conditional stack.
    struct CondStatus
    {
        bool done;      // if we've output a section of this conditional
        bool saw_else;  // if we've seen an else case for this conditional
    };
    std::vector<CondStatus> m_cond_stack;

    // Syntax modes.
    bool m_intel;
    bool m_reg_prefix;

    // Section stack and previous section.
    struct SectionState
    {
        Section* cur_sect;
        Section* prev_sect;
    };
    std::vector<SectionState> m_section_stack;
    Section* m_previous_section;
};

}} // namespace yasm::parser

#endif

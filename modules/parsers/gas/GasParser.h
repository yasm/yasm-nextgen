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

#include "clang/Basic/SourceLocation.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/StringMap.h"
#include "yasmx/Config/export.h"
#include "yasmx/Parse/ParserImpl.h"
#include "yasmx/Support/ptr_vector.h"
#include "yasmx/Insn.h"
#include "yasmx/IntNum.h"
#include "yasmx/Parser.h"

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
    bool (GasParser::*handler) (unsigned int, clang::SourceLocation source);
    unsigned int param;
};

class YASM_STD_EXPORT GasParser : public Parser, public ParserImpl
{
public:
    GasParser(const ParserModule& module,
              Diagnostic& diags,
              clang::SourceManager& sm,
              HeaderSearch& headers);
    ~GasParser();

    void AddDirectives(Directives& dirs, llvm::StringRef parser);

    static llvm::StringRef getName() { return "GNU AS (GAS)-compatible parser"; }
    static llvm::StringRef getKeyword() { return "gas"; }

    void Parse(Object& object, Directives& dirs, Diagnostic& diags);

private:

    bool ParseLine();
    void setDebugFile(llvm::StringRef filename,
                      clang::SourceLocation filename_source,
                      clang::SourceLocation dir_source);
    void setDebugFile(const IntNum& fileno,
                      clang::SourceLocation fileno_source,
                      llvm::StringRef filename,
                      clang::SourceLocation filename_source,
                      clang::SourceLocation dir_source);
    void ParseCppLineMarker();
    void ParseNasmLineMarker();

    bool ParseDirLine(unsigned int, clang::SourceLocation source);
#if 0
    bool ParseDirRept(unsigned int);
    bool ParseDirEndr(unsigned int);
#endif
    bool ParseDirAlign(unsigned int power2, clang::SourceLocation source);
    bool ParseDirOrg(unsigned int, clang::SourceLocation source);
    bool ParseDirLocal(unsigned int, clang::SourceLocation source);
    bool ParseDirComm(unsigned int is_lcomm, clang::SourceLocation source);
    bool ParseDirAscii(unsigned int withzero, clang::SourceLocation source);
    bool ParseDirFloat(unsigned int size, clang::SourceLocation source);
    bool ParseDirData(unsigned int size, clang::SourceLocation source);
    bool ParseDirLeb128(unsigned int sign, clang::SourceLocation source);
    bool ParseDirZero(unsigned int, clang::SourceLocation source);
    bool ParseDirSkip(unsigned int, clang::SourceLocation source);
    bool ParseDirFill(unsigned int, clang::SourceLocation source);
    bool ParseDirBssSection(unsigned int, clang::SourceLocation source);
    bool ParseDirDataSection(unsigned int, clang::SourceLocation source);
    bool ParseDirTextSection(unsigned int, clang::SourceLocation source);
    bool ParseDirSection(unsigned int, clang::SourceLocation source);
    bool ParseDirEqu(unsigned int, clang::SourceLocation source);
    bool ParseDirFile(unsigned int, clang::SourceLocation source);

    Insn::Ptr ParseInsn();
    bool ParseDirective(NameValues* nvs);
    Operand ParseMemoryAddress();
    Operand ParseOperand();
    bool ParseExpr(Expr& e, const ParseExprTerm* parse_term = 0);
    bool ParseExpr0(Expr& e, const ParseExprTerm* parse_term);
    bool ParseExpr1(Expr& e, const ParseExprTerm* parse_term);
    bool ParseExpr2(Expr& e, const ParseExprTerm* parse_term);
    bool ParseExpr3(Expr& e, const ParseExprTerm* parse_term);

    SymbolRef ParseSymbol(IdentifierInfo* ii);
    bool ParseInteger(IntNum* intn);
    const Register* ParseRegister();

    void DefineLabel(llvm::StringRef name, clang::SourceLocation source);
    void DefineLcomm(SymbolRef sym,
                     clang::SourceLocation source,
                     std::auto_ptr<Expr> size,
                     const Expr& align);
    void SwitchSection(llvm::StringRef name,
                       bool builtin,
                       clang::SourceLocation source);
    Section& getSection(llvm::StringRef name,
                        bool builtin,
                        clang::SourceLocation source);

    void DoParse();

    Object* m_object;
    Arch* m_arch;
    Directives* m_dirs;

    GasPreproc m_gas_preproc;

    BytecodeContainer* m_container;
    /*@null@*/ Bytecode* m_bc;

    GasDirLookup m_sized_gas_dirs[1];
    typedef llvm::StringMap<const GasDirLookup*> GasDirMap;
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

#if 0
    stdx::ptr_vector<GasRept> m_rept;
    stdx::ptr_vector_owner<GasRept> m_rept_owner;
#endif
    // Index of local labels; what's stored here is the /next/ index,
    // so these are all 0 at start.
    unsigned long m_local[10];

    // Start of comment.
    clang::SourceLocation m_comment_start;
};

}} // namespace yasm::parser

#endif

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

#include <yasmx/Mixin/ParserMixin.h>
#include <yasmx/Support/ptr_vector.h>
#include <yasmx/Insn.h>
#include <yasmx/Linemap.h>
#include <yasmx/Parser.h>

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
namespace gas
{

#define YYCTYPE         char

struct yystype
{
    std::string str;
    std::auto_ptr<IntNum> intn;
    std::auto_ptr<FloatNum> flt;
    Insn::Ptr insn;
    union
    {
        unsigned int int_info;
        const Insn::Prefix* prefix;
        const SegmentRegister* segreg;
        const Register* reg;
        const RegisterGroup* reggroup;
        const Insn::Operand::TargetModifier* targetmod;
    };
};
#define YYSTYPE yystype

struct GasRept
{
    GasRept(unsigned long line, unsigned long n);
    ~GasRept();

    std::vector<std::string> lines;     // repeated lines
    unsigned long startline;    // line number of rept directive
    unsigned long numrept;      // number of repititions to generate
    unsigned long numdone;      // number of repititions executed so far
    int line;                   // next line to repeat
    size_t linepos;             // position to start pulling chars from line
    bool ended;                 // seen endr directive yet?

    YYCTYPE* oldbuf;            // saved previous fill buffer
    size_t oldbuflen;           // previous fill buffer length
    size_t oldbufpos;           // position in previous fill buffer
};

class GasParser;
struct GasDirLookup
{
    const char* name;
    void (GasParser::*handler) (unsigned int);
    unsigned int param;
};

class GasParser
    : public Parser
    , public ParserMixin<GasParser, YYSTYPE, YYCTYPE>
{
public:
    GasParser(const ParserModule& module, Errwarns& errwarns);
    ~GasParser();

    void add_directives(Directives& dirs, const char* parser);

    static const char* get_name() { return "GNU AS (GAS)-compatible parser"; }
    static const char* get_keyword() { return "gas"; }
    static std::vector<const char*> get_preproc_keywords();
    static const char* get_default_preproc_keyword() { return "raw"; }

    void parse(Object& object,
               Preprocessor& preproc,
               bool save_input,
               Directives& dirs,
               Linemap& linemap);

    enum TokenType
    {
        INTNUM = 258,
        FLTNUM,
        STRING,
        REG,
        REGGROUP,
        SEGREG,
        TARGETMOD,
        LEFT_OP,
        RIGHT_OP,
        ID,
        LABEL,
        CPP_LINE_MARKER,
        NASM_LINE_MARKER,
        NONE                // special token for lookahead
    };

    static bool is_eol_tok(int tok)
    {
        return (tok == '\n' || tok == ';' || tok == 0);
    }
    static const char* describe_token(int tok);

    int lex(YYSTYPE* lvalp);

private:

    void parse_line();
    void debug_file(NameValues& nvs);
    void cpp_line_marker();
    void nasm_line_marker();

    void dir_line(unsigned int);
    void dir_rept(unsigned int);
    void dir_endr(unsigned int);
    void dir_align(unsigned int power2);
    void dir_org(unsigned int);
    void dir_local(unsigned int);
    void dir_comm(unsigned int is_lcomm);
    void dir_ascii(unsigned int withzero);
    void dir_data(unsigned int size);
    void dir_leb128(unsigned int sign);
    void dir_zero(unsigned int);
    void dir_skip(unsigned int);
    void dir_fill(unsigned int);
    void dir_bss_section(unsigned int);
    void dir_data_section(unsigned int);
    void dir_text_section(unsigned int);
    void dir_section(unsigned int);
    void dir_equ(unsigned int);
    void dir_file(unsigned int);

    Insn::Ptr parse_instr();
    void parse_dirvals(NameValues* nvs);
    Insn::Operand parse_memaddr();
    Insn::Operand parse_operand();
    bool parse_expr(Expr& e);
    bool parse_expr0(Expr& e);
    bool parse_expr1(Expr& e);
    bool parse_expr2(Expr& e);

    void define_label(const std::string& name, bool local);
    void define_lcomm(const std::string& name,
                      std::auto_ptr<Expr> size,
                      const Expr& align);
    void switch_section(const std::string& name,
                        NameValues& objext_namevals,
                        bool builtin);
    Section& get_section(const std::string& name,
                         NameValues& objext_namevals,
                         bool builtin);

    void do_parse();

    GasDirLookup m_sized_gas_dirs[1];
    typedef std::map<std::string, const GasDirLookup*> GasDirMap;
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

    enum State
    {
        INITIAL,
        COMMENT,
        SECTION_DIRECTIVE,
        NASM_FILENAME
    } m_state;

    stdx::ptr_vector<GasRept> m_rept;
    stdx::ptr_vector_owner<GasRept> m_rept_owner;

    // Index of local labels; what's stored here is the /next/ index,
    // so these are all 0 at start.
    unsigned long m_local[10];

    bool m_is_nasm_preproc;
    bool m_is_cpp_preproc;
};

#define INTNUM_val      (m_tokval.intn)
#define FLTNUM_val      (m_tokval.flt)
#define STRING_val      (m_tokval.str)
#define REG_val         (m_tokval.reg)
#define REGGROUP_val    (m_tokval.reggroup)
#define SEGREG_val      (m_tokval.segreg)
#define TARGETMOD_val   (m_tokval.targetmod)
#define ID_val          (m_tokval.str)
#define LABEL_val       (m_tokval.str)

}}} // namespace yasm::parser::gas

#endif

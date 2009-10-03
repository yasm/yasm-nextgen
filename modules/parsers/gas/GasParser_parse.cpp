//
// GAS-compatible parser
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
#include "util.h"

#include <cctype>
#include <climits>
#include <cmath>

#include "clang/Basic/SourceManager.h"
#include "yasmx/Support/bitcount.h"
#include "yasmx/Support/Compose.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Arch.h"
#include "yasmx/Bytecode.h"
#include "yasmx/BytecodeContainer.h"
#include "yasmx/BytecodeContainer_util.h"
#include "yasmx/Directive.h"
#include "yasmx/Errwarns.h"
#include "yasmx/NameValue.h"
#include "yasmx/Object.h"
#include "yasmx/Preprocessor.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"
#include "yasmx/Symbol_util.h"

#include "GasParser.h"


namespace yasm
{
namespace parser
{
namespace gas
{

llvm::StringRef
GasParser::DescribeToken(int token)
{
    static char strch[] = "` '";
    const char *str;

    switch (token)
    {
        case 0: case '\n':      str = "end of line"; break;
        case INTNUM:            str = "integer"; break;
        case FLTNUM:            str = "floating point value"; break;
        case STRING:            str = "string"; break;
        case REG:               str = "register"; break;
        case REGGROUP:          str = "register group"; break;
        case SEGREG:            str = "segment register"; break;
        case TARGETMOD:         str = "target modifier"; break;
        case LEFT_OP:           str = "<<"; break;
        case RIGHT_OP:          str = ">>"; break;
        case ID:                str = "identifier"; break;
        case LABEL:             str = "label"; break;
        default:
            strch[1] = token;
            str = strch;
            break;
    }

    return str;
}

bool
GasParser::ParseLine()
{
    if (isEol())
        return true;

    m_container = m_object->getCurSection();

    Insn::Ptr insn = ParseInsn();
    if (insn.get() != 0)
    {
        insn->Append(*m_container, m_source);
        return true;
    }

    switch (m_token)
    {
        case ID:
        {
            clang::SourceLocation name_src = getTokenSource();
            std::string name;
            std::swap(name, ID_val);
            getNextToken(); // ID

            // See if it's a gas-specific directive
            GasDirMap::iterator p = m_gas_dirs.find(name);
            if (p != m_gas_dirs.end())
            {
                // call directive handler (function in this class) w/parameter
                return (this->*(p->second->handler))(p->second->param);
            }

            if (m_token == ':')
            {
                // Label
                m_state = INITIAL;
                getNextToken(); // :
                DefineLabel(name, false);
                if (!ParseLine())
                    return false;
                break;
            }
            else if (m_token == '=')
            {
                // EQU
                // TODO: allow redefinition, assigning to . (same as .org)
                m_state = INITIAL;
                getNextToken(); // =
                Expr e;
                if (!ParseExpr(e))
                {
                    Diag(getTokenSource(), diag::err_expected_expression_after)
                        << "=";
                    return false;
                }
                m_object->getSymbol(name)->DefineEqu(e, m_source);
                break;
            }

            // possibly a directive; try to parse it
            DirectiveInfo dirinfo(*m_object, m_source);
            ParseDirective(&dirinfo.getNameValues());
            Directive dir;
            if (m_dirs->get(&dir, name))
            {
                dir(dirinfo);
                break;
            }

            // didn't match, warn/error as appropriate
            if (name[0] == '.')
                Diag(name_src, diag::warn_unrecognized_directive);
            else
            {
                Diag(name_src, diag::err_unrecognized_instruction);
                return false;
            }
            break;
        }
        case LABEL:
            DefineLabel(LABEL_val, false);
            getNextToken(); // LABEL
            if (!ParseLine())
                return false;
            break;
        case CPP_LINE_MARKER:
            getNextToken();
            ParseCppLineMarker();
            break;
        case NASM_LINE_MARKER:
            getNextToken();
            ParseNasmLineMarker();
            break;
        default:
            Diag(getTokenSource(), diag::err_expected_insn_label_after_eol);
            return false;
    }
    return true;
}

void
GasParser::setDebugFile(llvm::StringRef filename)
{
    Directive dir;
    if (!m_dirs->get(&dir, ".file"))
        return;

    DirectiveInfo info(*m_object, m_source);
    info.getNameValues().push_back(new NameValue(filename));
    dir(info);
}

void
GasParser::setDebugFile(const IntNum& fileno, llvm::StringRef filename)
{
    Directive dir;
    if (!m_dirs->get(&dir, ".file"))
        return;

    DirectiveInfo info(*m_object, m_source);
    NameValues& nvs = info.getNameValues();
    nvs.push_back(new NameValue(Expr::Ptr(new Expr(fileno))));
    nvs.push_back(new NameValue(filename));
    dir(info);
}

// Handle line markers generated by cpp.
//
// We expect a positive integer (line) followed by a string (filename). If we
// fail to find either of these, we treat the line as a comment. There is a
// possibility of false positives (mistaking a comment for a line marker, when
// the comment is not intended as a line marker) but this cannot be avoided
// without adding a filter to the input before passing it to cpp.
//
// This function is only called if the preprocessor was 'cpp', since the
// CPP_LINE_MARKER token isn't generated for any other preprocessor. With any
// other preprocessor, anything after a '#' is always treated as a comment.
void
GasParser::ParseCppLineMarker()
{
    // Line number.
    if (m_token != INTNUM)
    {
        // Skip over a comment.
        while (m_token != '\n')
            getNextToken();

        return;
    }

    if (INTNUM_val->getSign() < 0)
    {
        getNextToken(); // INTNUM
        throw SyntaxError(N_("line number is negative"));
    }

    unsigned long line = INTNUM_val->getUInt();

    // Set to (line - 1) since the directive indicates that the *next* line
    // will have the number given.
    //
    // cpp should never produce line=0, but the if keeps us safe just incase.
    if (line != 0)
        line--;

    getNextToken(); // INTNUM

    // File name, in quotes.
    if (m_token != STRING)
    {
        // Skip over a comment.
        while (m_token != '\n')
            getNextToken();

        return;
    }

    std::string filename;
    std::swap(filename, STRING_val);
    getNextToken();

    // Add a line note.
    clang::SourceManager& smgr = m_preproc->getSourceManager();
    smgr.AddLineNote(m_source, line,
                     smgr.getLineTableFilenameID(filename.data(),
                                                 filename.size()));

    // The first line marker in the file (which should be on the first line
    // of the file) will give us the name of the source file. This information
    // needs to be passed on to the debug format module.
    if (!m_seen_line_marker)
    {
        m_seen_line_marker = false;
        setDebugFile(filename);
    }

    // Skip flags.
    for (;;)
    {
        switch (m_token)
        {
            case INTNUM:
                break;
            case '\n':
                return;
            default:
                throw SyntaxError(N_("junk at end of cpp line marker"));
        }
        getNextToken();
    }
}

//  Handle line markers generated by the nasm preproc.
//
//  We expect a positive integer (line) followed by a plus sign, followed by
//  another positive integer, followed by a string (filename).
//
//  This function is only called if the preprocessor was 'nasm', since the
//  NASM_LINE_MARKER token isn't generated for any other preprocessor.
void
GasParser::ParseNasmLineMarker()
{
    // Line number.
    if (m_token != INTNUM) return;

    if (INTNUM_val->getSign() < 0)
    {
        getNextToken(); // INTNUM
        throw SyntaxError(N_("line number is negative"));
    }

    unsigned long line = INTNUM_val->getUInt();

    // Set to (line - 1) since the directive indicates that the *next* line
    // will have the number given.
    //
    // cpp should never produce line=0, but the if keeps us safe just incase.
    if (line != 0)
        line--;

    getNextToken(); // INTNUM

    if (m_token != '+') return;
    getNextToken(); // +

    // Line number increment.
    if (m_token != INTNUM) return;

    if (INTNUM_val->getSign() < 0)
    {
        getNextToken(); // INTNUM
        throw SyntaxError(N_("line increment is negative"));
    }

    unsigned long incr = INTNUM_val->getUInt();
    incr = incr;

    // File name is not in quotes, so need to switch to a different tokenizer
    // state.
    m_state = NASM_FILENAME;
    getNextToken(); // INTNUM
    if (m_token != STRING)
    {
        m_state = INITIAL;
        return;
    }

    std::string filename;
    std::swap(filename, STRING_val);

    // Add a line note.
    // FIXME: handle increment
    clang::SourceManager& smgr = m_preproc->getSourceManager();
    smgr.AddLineNote(m_source, line,
                     smgr.getLineTableFilenameID(filename.data(),
                                                 filename.size()));

    // The first line marker in the file (which should be on the first line
    // of the file) will give us the name of the source file. This information
    // needs to be passed on to the debug format module.
    if (!m_seen_line_marker)
    {
        m_seen_line_marker = true;
        setDebugFile(filename);
    }

    // We need to poke back on the \n that was consumed by the tokenizer
    m_peek_token = '\n';
    getNextToken();
}

// Line directive
bool
GasParser::ParseDirLine(unsigned int param)
{
    if (!Expect(INTNUM, diag::err_expected_integer))
        return false;

    if (INTNUM_val->getSign() < 0)
    {
        Diag(getTokenSource(), m_diags->getCustomDiagID(Diagnostic::Error,
            "line number is negative"));
        getNextToken(); // INTNUM
        return false;
    }

    m_dir_line = INTNUM_val->getUInt();
    getNextToken(); // INTNUM

    if (m_dir_fileline == FL_BOTH)
    {
        // Have both file and line
        m_preproc->getSourceManager().AddLineNote(m_source, m_dir_line, -1);
    }
    else if (m_dir_fileline == FL_FILE)
    {
        // Had previous file directive only
        m_dir_fileline = FL_BOTH;
        clang::SourceManager& smgr = m_preproc->getSourceManager();
        smgr.AddLineNote(m_source, m_dir_line,
                         smgr.getLineTableFilenameID(m_dir_file.data(),
                                                     m_dir_file.size()));
    }
    else
    {
        // Didn't see file yet
        m_dir_fileline = FL_LINE;
    }
    return true;
}

//
// Macro directives
//
#if 0
GasRept::GasRept(unsigned long line, unsigned long n)
    : startline(line)
    , numrept(n)
    , numdone(0)
    , line(-1)
    , linepos(0)
    , ended(false)
    , oldbuf(0)
    , oldbuflen(0)
    , oldbufpos(0)
{
}

GasRept::~GasRept()
{
}

bool
GasParser::ParseDirRept(unsigned int param)
{
    Expr e;
    if (!ParseExpr(e))
    {
        throw SyntaxError(String::Compose(
            N_("expression expected after `%1'"), ".rept"));
    }

    e.Simplify();
    if (!e.isIntNum())
        throw NotAbsoluteError(N_("rept expression not absolute"));

    IntNum intn = e.getIntNum();
    if (intn.getSign() < 0)
        throw ValueError(N_("rept expression is negative"));

    m_rept.push_back(new GasRept(m_source, intn.getUInt()));
    return true;
}

bool
GasParser::ParseDirEndr(unsigned int param)
{
    // Shouldn't ever get here unless we didn't get a DIR_REPT first
    throw SyntaxError(N_("endr without matching rept"));
    return false;
}
#endif
//
// Alignment directives
//

bool
GasParser::ParseDirAlign(unsigned int power2)
{
    Expr bound, fill, maxskip;

    if (!ParseExpr(bound))
    {
        Diag(getTokenSource(), m_diags->getCustomDiagID(Diagnostic::Error,
            ".align directive must specify alignment"));
        return false;
    }

    if (m_token == ',')
    {
        getNextToken(); // ','
        ParseExpr(fill);
        if (m_token == ',')
        {
            getNextToken(); // ','
            ParseExpr(maxskip);
        }
    }

    Section* cur_section = m_container->AsSection();

    // Convert power of two to number of bytes if necessary
    if (power2)
        bound = SHL(1, bound);

    // Largest .align in the section specifies section alignment.
    bound.Simplify();
    if (bound.isIntNum())
    {
        unsigned long boundint = bound.getIntNum().getUInt();

        // Alignments must be a power of two.
        if (isExp2(boundint))
        {
            if (boundint > cur_section->getAlign())
                cur_section->setAlign(boundint);
        }
    }

    AppendAlign(*cur_section, bound, fill, maxskip,
                cur_section->isCode() ?  m_object->getArch()->getFill() : 0,
                m_source);
    return true;
}

bool
GasParser::ParseDirOrg(unsigned int param)
{
    // TODO: support expr instead of intnum
    if (!Expect(INTNUM, diag::err_expected_integer))
        return false;
    unsigned long start = INTNUM_val->getUInt();
    getNextToken(); // INTNUM

    unsigned long value = 0;
    if (m_token == ',')
    {
        getNextToken(); // ','
        // TODO: support expr instead of intnum
        if (!Expect(INTNUM, diag::err_expected_integer))
            return false;
        value = INTNUM_val->getUInt();
        getNextToken(); // INTNUM
    }

    AppendOrg(*m_container, start, value, m_source);
    return true;
}

//
// Data visibility directives
//

bool
GasParser::ParseDirLocal(unsigned int param)
{
    if (!Expect(ID, diag::err_expected_ident))
        return false;
    m_object->getSymbol(ID_val)->Declare(Symbol::DLOCAL, m_source);
    getNextToken(); // ID
    return true;
}

bool
GasParser::ParseDirComm(unsigned int is_lcomm)
{
    if (!Expect(ID, diag::err_expected_ident))
        return false;
    std::string id;
    std::swap(id, ID_val);
    getNextToken(); // ID

    ExpectAndConsume(',', diag::err_expected_comma);

    Expr e, align;
    if (!ParseExpr(e))
    {
        Diag(getTokenSource(), m_diags->getCustomDiagID(Diagnostic::Error,
            "size expected for .COMM"));
        return false;
    }
    if (m_token == ',')
    {
        // Optional alignment expression
        getNextToken(); // ','
        ParseExpr(align);
    }
    // If already explicitly declared local, treat like LCOMM
    SymbolRef oldsym = m_object->FindSymbol(id);
    if (is_lcomm || (oldsym && oldsym->getVisibility() == Symbol::DLOCAL))
    {
        std::auto_ptr<Expr> e_copy(new Expr);
        std::swap(*e_copy, e);
        DefineLcomm(id, e_copy, align);
    }
    else if (!align.isEmpty())
    {
        // Give third parameter as objext valparam
        std::auto_ptr<Expr> align_copy(new Expr);
        std::swap(*align_copy, align);

        NameValues extvps;
        extvps.push_back(new NameValue(align_copy));

        SymbolRef sym = m_object->getSymbol(id);
        sym->Declare(Symbol::COMMON, m_source);
        setCommonSize(*sym, e);
        setObjextNameValues(*sym, extvps);
    }
    else
    {
        SymbolRef sym = m_object->getSymbol(id);
        sym->Declare(Symbol::COMMON, m_source);
        setCommonSize(*sym, e);
    }
    return true;
}

//
// Integer data definition directives
//

bool
GasParser::ParseDirAscii(unsigned int withzero)
{
    for (;;)
    {
        if (!Expect(STRING, diag::err_expected_string))
            return false;
        AppendData(*m_container, STRING_val, withzero);
        getNextToken(); // STRING
        if (m_token != ',')
            break;
        getNextToken(); // ','
    }
    return true;
}

bool
GasParser::ParseDirData(unsigned int size)
{
    for (;;)
    {
        std::auto_ptr<Expr> e(new Expr);
        if (!ParseExpr(*e))
        {
            Diag(getTokenSource(), diag::err_expected_expression_after) << ",";
            return false;
        }
        AppendData(*m_container, e, size, *m_arch, m_source);
        if (m_token != ',')
            break;
        getNextToken(); // ','
    }
    return true;
}

bool
GasParser::ParseDirLeb128(unsigned int sign)
{
    for (;;)
    {
        std::auto_ptr<Expr> e(new Expr);
        if (!ParseExpr(*e))
        {
            Diag(getTokenSource(), diag::err_expected_expression_after) << ",";
            return false;
        }
        AppendLEB128(*m_container, e, sign, m_source);
        if (m_token != ',')
            break;
        getNextToken(); // ','
    }
    return true;
}

//
// Empty space / fill data definition directives
//

bool
GasParser::ParseDirZero(unsigned int param)
{
    std::auto_ptr<Expr> e(new Expr);
    if (!ParseExpr(*e))
    {
        Diag(getTokenSource(), diag::err_expected_expression_after_id)
            << ".ZERO";
        return false;
    }

    BytecodeContainer& inner = AppendMultiple(*m_container, e, m_source);
    AppendByte(inner, 0);
    return true;
}

bool
GasParser::ParseDirSkip(unsigned int param)
{
    std::auto_ptr<Expr> e(new Expr);
    if (!ParseExpr(*e))
    {
        Diag(getTokenSource(), diag::err_expected_expression_after_id)
            << ".SKIP";
        return false;
    }

    BytecodeContainer& inner = AppendMultiple(*m_container, e, m_source);
    if (m_token != ',')
    {
        inner.AppendGap(1, m_source);
        return true;
    }
    getNextToken(); // ','

    // expression after comma forces fill of that value (as a byte)
    std::auto_ptr<Expr> e_val(new Expr);
    if (!ParseExpr(*e_val))
    {
        Diag(getTokenSource(), diag::err_expected_expression_after) << ",";
        return false;
    }
    AppendData(inner, e_val, 1, *m_arch, m_source);
    return true;
}

// fill data definition directive
bool
GasParser::ParseDirFill(unsigned int param)
{
    std::auto_ptr<Expr> repeat(new Expr);
    Expr size, value;
    clang::SourceLocation size_src;
    if (!ParseExpr(*repeat))
    {
        Diag(getTokenSource(), diag::err_expected_expression_after_id)
            << ".FILL";
        return false;
    }
    if (m_token == ',')
    {
        getNextToken(); // ','
        size_src = getTokenSource();
        if (!ParseExpr(size))
        {
            Diag(getTokenSource(), diag::err_expected_expression_after) << ",";
            return false;
        }
        if (m_token == ',')
        {
            getNextToken(); // ','
            if (!ParseExpr(value))
            {
                Diag(getTokenSource(), diag::err_expected_expression_after)
                    << ",";
                return false;
            }
        }
    }

    unsigned int ssize = 1;
    if (!size.isEmpty())
    {
        size.Simplify();
        if (!size.isIntNum())
        {
            Diag(size_src, m_diags->getCustomDiagID(Diagnostic::Error,
                "size must be an absolute expression"));
            return false;
        }
        ssize = size.getIntNum().getUInt();
    }

    BytecodeContainer& inner = AppendMultiple(*m_container, repeat, m_source);
    if (value.isEmpty())
    {
        AppendData(inner, 0, ssize, *m_arch);
    }
    else
    {
        std::auto_ptr<Expr> value_copy(new Expr);
        std::swap(*value_copy, value);
        AppendData(inner, value_copy, ssize, *m_arch, m_source);
    }
    return true;
}

//
// Section directives
//

bool
GasParser::ParseDirBssSection(unsigned int param)
{
    SwitchSection(".bss", true);
    return true;
}

bool
GasParser::ParseDirDataSection(unsigned int param)
{
    SwitchSection(".data", true);
    return true;
}

bool
GasParser::ParseDirTextSection(unsigned int param)
{
    SwitchSection(".text", true);
    return true;
}

bool
GasParser::ParseDirSection(unsigned int param)
{
    // DIR_SECTION ID ',' STRING ',' '@' ID ',' dirvals
    // Really parsed as just a bunch of dirvals; only needs to be unique
    // function to set parser state appropriately.
    m_state = SECTION_DIRECTIVE;
    DirectiveInfo info(*m_object, m_source);
    if (!ParseDirective(&info.getNameValues()))
        return false;
    (*m_dirs)[".section"](info);
    m_state = INITIAL;
    return true;
}

//
// Other directives
//

bool
GasParser::ParseDirEqu(unsigned int param)
{
    // ID ',' expr
    if (!Expect(ID, diag::err_expected_ident))
        return false;
    std::string id;
    std::swap(id, ID_val);
    getNextToken(); // ID

    ExpectAndConsume(',', diag::err_expected_comma);

    Expr e;
    if (!ParseExpr(e))
    {
        Diag(getTokenSource(), diag::err_expected_expression_after) << ",";
        return false;
    }
    m_object->getSymbol(id)->DefineEqu(e, m_source);
    return true;
}

bool
GasParser::ParseDirFile(unsigned int param)
{
    if (m_token == STRING)
    {
        // No file number; this form also sets the assembler's
        // internal line number.
        std::string filename;
        std::swap(filename, STRING_val);
        getNextToken(); // STRING
#if 0
        // FIXME
        if (m_dir_fileline == FL_BOTH)
        {
            // Have both file and line
            std::string old_fn;
            unsigned long old_line;

            m_linemap->Lookup(m_source, &old_fn, &old_line);
            m_linemap->set(filename, old_line, 1);
        }
        else if (m_dir_fileline == FL_LINE)
        {
            // Had previous line directive only
            m_dir_fileline = FL_BOTH;
            m_linemap->set(filename, m_dir_line, 1);
        }
        else
        {
            // Didn't see line yet, save file
            m_dir_fileline = FL_FILE;
            m_dir_file = filename;
        }
#endif
        // Pass change along to debug format
        setDebugFile(filename);
        return true;
    }

    // fileno filename form
    if (m_token != INTNUM)
        return true;
    std::auto_ptr<IntNum> fileno = INTNUM_val;
    getNextToken(); // INTNUM

    if (!Expect(STRING, diag::err_expected_string))
        return false;
    std::string filename;
    std::swap(filename, STRING_val);
    getNextToken(); // STRING

    setDebugFile(*fileno, filename);
    return true;
}

Insn::Ptr
GasParser::ParseInsn()
{
    if (m_token != ID)
        return Insn::Ptr(0);

    // instructions/prefixes must start with a letter
    if (!isalpha(ID_val[0]))
        return Insn::Ptr(0);

    // check to be sure it's not a label or equ
    getPeekToken();
    if (m_peek_token == ':' || m_peek_token == '=')
        return Insn::Ptr(0);

    Arch::InsnPrefix ip = m_arch->ParseCheckInsnPrefix(ID_val);
    switch (ip.getType())
    {
        case Arch::InsnPrefix::INSN:
        {
            // Propagate errors in case we got a warning from the arch
            m_errwarns.Propagate(m_source);

            getNextToken();   // ID

            Insn::Ptr insn = ip.ReleaseInsn();
            if (!isEol())
            {
                // parse operands
                for (;;)
                {
                    insn->AddOperand(ParseOperand());
                    if (isEol())
                        break;
                    ExpectAndConsume(',', diag::err_expected_comma);
                }
            }
            return insn;
        }
        case Arch::InsnPrefix::PREFIX:
        {
            // Propagate errors in case we got a warning from the arch
            m_errwarns.Propagate(m_source);

            getNextToken();   // ID

            Insn::Ptr insn = ParseInsn();
            if (insn.get() == 0)
                insn = m_arch->CreateEmptyInsn();
            insn->AddPrefix(ip.getPrefix());
            return insn;
        }
        default:
            break;
    }

    // Check for segment register used as prefix
    Arch::RegTmod regtmod = m_arch->ParseCheckRegTmod(ID_val);
    switch (regtmod.getType())
    {
        case Arch::RegTmod::SEGREG:
        {
            getNextToken();   // ID
            Insn::Ptr insn = ParseInsn();
            if (insn.get() == 0)
                insn = m_arch->CreateEmptyInsn();
            insn->AddSegPrefix(regtmod.getSegReg());
            return insn;
        }
        default:
            return Insn::Ptr(0);
    }
}

bool
GasParser::ParseDirective(NameValues* nvs)
{
    for (;;)
    {
        switch (m_token)
        {
            case ID:
                getPeekToken();
                switch (m_peek_token)
                {
                    case '+': case '-':
                    case '|': case '^': case '&': case '!':
                    case '*': case '/': case '%': case LEFT_OP: case RIGHT_OP:
                    {
                        Expr::Ptr e(new Expr);
                        if (!ParseExpr(*e))
                            return false;
                        nvs->push_back(new NameValue(e));
                        break;
                    }
                    default:
                        // Just an ID
                        nvs->push_back(new NameValue(ID_val, '\0'));
                        getNextToken(); // ID
                        break;
                }
                break;
            case STRING:
                nvs->push_back(new NameValue(STRING_val));
                getNextToken(); // STRING
                break;
            case REG:
                nvs->push_back(new NameValue(Expr::Ptr(new Expr(*REG_val))));
                getNextToken(); // REG
                break;
            case '@':
                // XXX: is throwing it away *really* the right thing?
                getNextToken(); // @
                continue;
            default:
            {
                Expr::Ptr e(new Expr);
                if (!ParseExpr(*e))
                    return false;
                nvs->push_back(new NameValue(e));
                break;
            }
        }
        if (m_token == ',')
            getNextToken(); // ','
    }
    return true;
}

// instruction operands
// memory addresses
Operand
GasParser::ParseMemoryAddress()
{
    bool strong = false;

    if (m_token == SEGREG)
    {
        const SegmentRegister* segreg = SEGREG_val;
        getNextToken(); // SEGREG
        ExpectAndConsume(':', diag::err_expected_colon_after_segreg);
        Operand op = ParseMemoryAddress();
        op.getMemory()->setSegReg(segreg);
        return op;
    }

    // We want to parse a leading expression, except when it's actually
    // just a memory address (with no preceding expression) such as
    // (REG...) or (,...).
    Expr e1;
    getPeekToken();
    if (m_token != '(' || (m_peek_token != REG && m_peek_token != ','))
        ParseExpr(e1);

    if (m_token == '(')
    {
        bool havereg = false;
        const Register* reg = 0;
        clang::SourceLocation scale_src;
        bool havescale = false;
        IntNum scale;
        Expr e2;

        getNextToken(); // '('

        // base register
        if (m_token == REG)
        {
            e2 = Expr(*REG_val);
            getNextToken(); // REG
        }
        else
            e2 = Expr(IntNum(0));

        if (m_token == ')')
            goto done;

        if (m_token != ',')
            throw SyntaxError(N_("invalid memory expression"));
        getNextToken(); // ','

        if (m_token == ')')
            goto done;

        havescale = true;

        // index register
        if (m_token == REG)
        {
            reg = REG_val;
            havereg = true;
            getNextToken(); // REG
            if (m_token != ',')
            {
                scale = 1;
                goto done;
            }
            getNextToken(); // ','
        }

        // scale
        scale_src = getTokenSource();
        if (m_token != INTNUM)
            throw SyntaxError(N_("non-integer scale"));
        scale = *INTNUM_val;
        getNextToken(); // INTNUM

done:
        if (m_token != ')')
            throw SyntaxError(N_("invalid memory expression"));
        getNextToken(); // ')'

        if (havescale)
        {
            if (!havereg)
            {
                if (scale.getUInt() != 1)
                    Diag(scale_src, diag::warn_scale_without_index);
            }
            else
            {
                e2 += MUL(*reg, scale);
            }
        }

        if (!e1.isEmpty())
        {
            // Ordering is critical here to correctly detecting presence of
            // RIP in RIP-relative expressions.
            e1 = ADD(e2, e1);
        }
        else
            std::swap(e1, e2);
        strong = true;
    }

    if (e1.isEmpty())
        throw SyntaxError(N_("expression syntax error"));

    Expr::Ptr e1_copy(new Expr);
    std::swap(*e1_copy, e1);
    Operand op(m_object->getArch()->CreateEffAddr(e1_copy));

    if (strong)
        op.getMemory()->m_strong = true;
    return op;
}

Operand
GasParser::ParseOperand()
{
    switch (m_token)
    {
        case REG:
        {
            Operand op(REG_val);
            getNextToken(); // REG
            return op;
        }
        case SEGREG:
        {
            // need to see if it's really a memory address
            getPeekToken();
            if (m_peek_token == ':')
                return ParseMemoryAddress();
            Operand op(SEGREG_val);
            getNextToken(); // SEGREG
            return op;
        }
        case REGGROUP:
        {
            const RegisterGroup* reggroup = REGGROUP_val;
            getNextToken(); // REGGROUP

            if (m_token != '(')
                return Operand(reggroup->getReg(0));
            getNextToken(); // '('

            if (m_token != INTNUM)
                throw SyntaxError(N_("integer register index expected"));
            unsigned long regindex = INTNUM_val->getUInt();
            getNextToken(); // INTNUM

            if (m_token != ')')
            {
                throw SyntaxError(
                    N_("missing closing parenthesis for register index"));
            }
            getNextToken(); // ')'

            const Register* reg = reggroup->getReg(regindex);
            if (!reg)
            {
                throw SyntaxError(String::Compose(
                    N_("bad register index `%u'"), regindex));
            }
            return Operand(reg);
        }
        case '$':
        {
            getNextToken(); // '$'
            Expr::Ptr e(new Expr);
            if (!ParseExpr(*e))
            {
                throw SyntaxError(String::Compose(
                    N_("expression missing after `%1'"), "$"));
            }
            return Operand(e);
        }
        case '*':
            getNextToken(); // '*'
            if (m_token == REG)
            {
                Operand op(REG_val);
                getNextToken(); // REG
                op.setDeref();
                return op;
            }
            else
            {
                Operand op = ParseMemoryAddress();
                op.setDeref();
                return op;
            }
        default:
            return ParseMemoryAddress();
    }
}

// Expression grammar parsed is:
//
// expr  : expr0 [ {+,-} expr0...]
// expr0 : expr1 [ {|,^,&,!} expr1...]
// expr1 : expr2 [ {*,/,%,<<,>>} expr2...]
// expr2 : { ~,+,- } expr2
//       | (expr)
//       | symbol
//       | number

bool
GasParser::ParseExpr(Expr& e)
{
    if (!ParseExpr0(e))
        return false;

    while (m_token == '+' || m_token == '-')
    {
        int op = m_token;
        getNextToken();
        Expr f;
        if (!ParseExpr0(f))
            return false;

        switch (op)
        {
            case '+': e.Calc(Op::ADD, f); break;
            case '-': e.Calc(Op::SUB, f); break;
        }
    }
    return true;
}

bool
GasParser::ParseExpr0(Expr& e)
{
    if (!ParseExpr1(e))
        return false;

    while (m_token == '|' || m_token == '^' || m_token == '&' || m_token == '!')
    {
        int op = m_token;
        getNextToken();
        Expr f;
        if (!ParseExpr1(f))
            return false;

        switch (op)
        {
            case '|': e.Calc(Op::OR, f); break;
            case '^': e.Calc(Op::XOR, f); break;
            case '&': e.Calc(Op::AND, f); break;
            case '!': e.Calc(Op::NOR, f); break;
        }
    }
    return true;
}

bool
GasParser::ParseExpr1(Expr& e)
{
    if (!ParseExpr2(e))
        return false;

    while (m_token == '*' || m_token == '/' || m_token == '%'
           || m_token == LEFT_OP || m_token == RIGHT_OP)
    {
        int op = m_token;
        getNextToken();
        Expr f;
        if (!ParseExpr2(f))
            return false;

        switch (op)
        {
            case '*': e.Calc(Op::MUL, f); break;
            case '/': e.Calc(Op::DIV, f); break;
            case '%': e.Calc(Op::MOD, f); break;
            case LEFT_OP: e.Calc(Op::SHL, f); break;
            case RIGHT_OP: e.Calc(Op::SHR, f); break;
        }
    }
    return true;
}

bool
GasParser::ParseExpr2(Expr& e)
{
    switch (m_token)
    {
        case '+':
            getNextToken();
            return ParseExpr2(e);
        case '-':
            getNextToken();
            if (!ParseExpr2(e))
                return false;
            e.Calc(Op::NEG);
            return true;
        case '~':
            getNextToken();
            if (!ParseExpr2(e))
                return false;
            e.Calc(Op::NOT);
            return true;
        case '(':
            getNextToken();
            if (!ParseExpr(e))
                return false;
            if (m_token != ')')
                throw SyntaxError(N_("missing parenthesis"));
            break;
        case INTNUM:
            e = INTNUM_val;
            break;
        case FLTNUM:
            e = FLTNUM_val;
            break;
        case ID:
        {
            // "." references the current assembly position
            if (ID_val == ".")
            {
                SymbolRef sym = m_object->AddNonTableSymbol(".");
                Bytecode& bc = m_container->FreshBytecode();
                Location loc = {&bc, bc.getFixedLen()};
                sym->DefineLabel(loc, m_source);
                e = sym;
            }
            else
            {
                SymbolRef sym = m_object->getSymbol(ID_val);
                sym->Use(m_source);
                e = sym;
            }
            getNextToken(); // ID

            if (m_token == '@')
            {
                // TODO: this is needed for shared objects, e.g. sym@PLT
                getNextToken(); // '@'
                if (m_token != ID)
                {
                    Diag(getTokenSource(), diag::err_expected_ident);
                    return false;
                }
                SymbolRef wrt = m_object->FindSpecialSymbol(ID_val);
                if (wrt)
                    e.Calc(Op::WRT, wrt);
                else
                    Diag(getTokenSource(), diag::warn_unrecognized_ident);
                getNextToken(); // ID
            }
            return true;
        }
        default:
            return false;
    }

    getNextToken();
    return true;
}

void
GasParser::DefineLabel(llvm::StringRef name, bool local)
{
    if (!local)
        m_locallabel_base = name;

    SymbolRef sym = m_object->getSymbol(name);
    Bytecode& bc = m_container->FreshBytecode();
    Location loc = {&bc, bc.getFixedLen()};
    sym->DefineLabel(loc, m_source);
}

void
GasParser::DefineLcomm(llvm::StringRef name,
                       std::auto_ptr<Expr> size,
                       const Expr& align)
{
    // Put into .bss section.
    Section& bss = getSection(".bss", true);

    if (!align.isEmpty())
    {
        // XXX: assume alignment is in bytes, not power-of-two
        AppendAlign(bss, align, Expr(), Expr(), 0, m_source);
    }

    // Create common symbol
    Bytecode *bc = &bss.FreshBytecode();
    Location loc = {bc, bc->getFixedLen()};
    m_object->getSymbol(name)->DefineLabel(loc, m_source);

    // Append gap for symbol storage
    size->Simplify();
    if (size->isIntNum())
        bss.AppendGap(size->getIntNum().getUInt(), m_source);
    else
    {
        BytecodeContainer& multc = AppendMultiple(bss, size, m_source);
        multc.AppendGap(1, m_source);
    }
}

void
GasParser::SwitchSection(llvm::StringRef name, bool builtin)
{
    DirectiveInfo info(*m_object, m_source);
    info.getNameValues().push_back(new NameValue(name, '\0'));
    (*m_dirs)[".section"](info);
}

Section&
GasParser::getSection(llvm::StringRef name, bool builtin)
{
    Section* cur_section = m_object->getCurSection();
    SwitchSection(name, builtin);
    Section& new_section = *m_object->getCurSection();
    m_object->setCurSection(cur_section);
    return new_section;
}

void
GasParser::DoParse()
{
    std::string line;

    while (m_preproc->getLine(&line, &m_source))
    {
        try
        {
            m_bot = m_tok = m_ptr = m_cur = &line[0];
            m_lim = &line[line.length()+1];

            do {
                getNextToken();
                if (!isEol())
                {
                    bool ok = ParseLine();
                    if (!ok)
                    {
                        DemandEolNoThrow();
                        m_state = INITIAL;
                    }
                    else
                        DemandEol();
                }
            } while (m_token != '\0');

            m_errwarns.Propagate(m_source);
        }
        catch (Error& err)
        {
            m_errwarns.Propagate(m_source, err);
            DemandEolNoThrow();
            m_state = INITIAL;
        }
    }
}

}}} // namespace yasm::parser::gas

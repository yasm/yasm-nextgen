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
#include <cstdio>

#include "clang/Basic/SourceManager.h"
#include "llvm/ADT/SmallString.h"
#include "yasmx/Support/bitcount.h"
#include "yasmx/Arch.h"
#include "yasmx/Bytecode.h"
#include "yasmx/BytecodeContainer.h"
#include "yasmx/Diagnostic.h"
#include "yasmx/Directive.h"
#include "yasmx/Errwarns.h"
#include "yasmx/Expr_util.h"
#include "yasmx/NameValue.h"
#include "yasmx/Object.h"
#include "yasmx/Section.h"
#include "yasmx/Symbol.h"
#include "yasmx/Symbol_util.h"

#include "GasParser.h"
#include "GasLexer.h"
#include "GasNumericParser.h"
#include "GasStringParser.h"


using namespace yasm;
using namespace yasm::parser;

bool
GasParser::ParseLine()
{
next:
    if (m_token.is(GasToken::eof))
        return true;
    if (m_token.isEndOfStatement())
    {
        ConsumeToken();
        goto next;
    }

    m_container = m_object->getCurSection();

    clang::SourceLocation exp_source = m_token.getLocation();

    switch (m_token.getKind())
    {
        case GasToken::identifier:
        case GasToken::label:
        {
            IdentifierInfo* ii = m_token.getIdentifierInfo();

            const Token& peek_token = NextToken();
            if (peek_token.is(GasToken::colon))
            {
                // Label
                clang::SourceLocation id_source = ConsumeToken();
                ConsumeToken(); // consume the colon too
                Bytecode& bc = m_container->FreshBytecode();
                Location loc = {&bc, bc.getFixedLen()};
                ParseSymbol(ii)->CheckedDefineLabel(loc, id_source,
                                                    m_preproc.getDiagnostics());
                goto next;
            }
            else if (peek_token.is(GasToken::equal))
            {
                // EQU
                // TODO: allow redefinition, assigning to . (same as .org)
                clang::SourceLocation id_source = ConsumeToken();
                clang::SourceLocation equ_source = ConsumeToken();
                Expr e;
                if (!ParseExpr(e))
                {
                    Diag(equ_source, diag::err_expected_expression_after)
                        << "=";
                    return false;
                }
                ParseSymbol(ii)->CheckedDefineEqu(e, id_source,
                                                  m_preproc.getDiagnostics());
                break;
            }

            // possibly a directive; try to parse it
            llvm::StringRef name = ii->getName();
            if (name[0] == '.')
            {
                clang::SourceLocation id_source = ConsumeToken();

                // See if it's a gas-specific directive
                GasDirMap::iterator p = m_gas_dirs.find(name);
                if (p != m_gas_dirs.end())
                {
                    // call directive handler (function in this class) w/parameter
                    return (this->*(p->second->handler))(p->second->param,
                                                         id_source);
                }

                DirectiveInfo dirinfo(*m_object, id_source);
                ParseDirective(&dirinfo.getNameValues());
                Directive dir;
                if (m_dirs->get(&dir, name))
                {
                    dir(dirinfo, m_preproc.getDiagnostics());
                    break;
                }

                // no match
                Diag(id_source, diag::warn_unrecognized_directive);
                break;
            }

            if (m_arch->hasParseInsn())
                return m_arch->ParseInsn(*m_container, *this);

            Insn::Ptr insn = ParseInsn();
            if (insn.get() != 0)
            {
                insn->Append(*m_container, exp_source,
                             m_preproc.getDiagnostics());
                break;
            }

            // id with nothing after it that wasn't an instruction
            Diag(m_token, diag::err_unrecognized_instruction);
            return false;
        }
        case GasToken::numeric_constant:
        {
            // If it's a simple integer from 0-9 and followed by a colon,
            // it's a local label.
            if (m_token.getLength() != 1 ||
                !isdigit(m_token.getLiteralData()[0]) ||
                NextToken().isNot(Token::colon))
            {
                Diag(m_token, diag::err_expected_insn_or_label_after_eol);
                return false;
            }
            char label = m_token.getLiteralData()[0];
            // increment label index
            m_local[label-'0']++;
            // build local label name and define it
            char labelname[30];
            std::sprintf(labelname, "L%c\001%lu", label, m_local[label-'0']);
            DefineLabel(labelname, m_token.getLocation());
            ConsumeToken();
            ConsumeToken(); // also eat the :
            goto next;
        }
#if 0
        case CPP_LINE_MARKER:
            ConsumeToken();
            ParseCppLineMarker();
            break;
        case NASM_LINE_MARKER:
            ConsumeToken();
            ParseNasmLineMarker();
            break;
#endif
        default:
            Diag(m_token, diag::err_expected_insn_or_label_after_eol);
            return false;
    }
    return true;
}

void
GasParser::setDebugFile(llvm::StringRef filename,
                        clang::SourceRange filename_source,
                        clang::SourceLocation dir_source)
{
    Directive dir;
    if (!m_dirs->get(&dir, ".file"))
        return;

    DirectiveInfo info(*m_object, dir_source);
    NameValues& nvs = info.getNameValues();
    nvs.push_back(new NameValue(filename));
    nvs.back().setValueRange(filename_source);
    dir(info, m_preproc.getDiagnostics());
}

void
GasParser::setDebugFile(const IntNum& fileno,
                        clang::SourceRange fileno_source,
                        llvm::StringRef filename,
                        clang::SourceRange filename_source,
                        clang::SourceLocation dir_source)
{
    Directive dir;
    if (!m_dirs->get(&dir, ".file"))
        return;

    DirectiveInfo info(*m_object, dir_source);
    NameValues& nvs = info.getNameValues();
    nvs.push_back(new NameValue(Expr::Ptr(new Expr(fileno))));
    nvs.back().setValueRange(fileno_source);
    nvs.push_back(new NameValue(filename));
    nvs.back().setValueRange(filename_source);
    dir(info, m_preproc.getDiagnostics());
}
#if 0
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
            ConsumeToken();

        return;
    }

    if (INTNUM_val->getSign() < 0)
    {
        ConsumeToken(); // INTNUM
        throw SyntaxError(N_("line number is negative"));
    }

    unsigned long line = INTNUM_val->getUInt();

    // Set to (line - 1) since the directive indicates that the *next* line
    // will have the number given.
    //
    // cpp should never produce line=0, but the if keeps us safe just incase.
    if (line != 0)
        line--;

    ConsumeToken(); // INTNUM

    // File name, in quotes.
    if (m_token != STRING)
    {
        // Skip over a comment.
        while (m_token != '\n')
            ConsumeToken();

        return;
    }

    std::string filename;
    std::swap(filename, STRING_val);
    ConsumeToken();

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
        ConsumeToken();
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
        ConsumeToken(); // INTNUM
        throw SyntaxError(N_("line number is negative"));
    }

    unsigned long line = INTNUM_val->getUInt();

    // Set to (line - 1) since the directive indicates that the *next* line
    // will have the number given.
    //
    // cpp should never produce line=0, but the if keeps us safe just incase.
    if (line != 0)
        line--;

    ConsumeToken(); // INTNUM

    if (m_token != '+') return;
    ConsumeToken(); // +

    // Line number increment.
    if (m_token != INTNUM) return;

    if (INTNUM_val->getSign() < 0)
    {
        ConsumeToken(); // INTNUM
        throw SyntaxError(N_("line increment is negative"));
    }

    unsigned long incr = INTNUM_val->getUInt();
    incr = incr;

    // File name is not in quotes, so need to switch to a different tokenizer
    // state.
    m_state = NASM_FILENAME;
    ConsumeToken(); // INTNUM
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
    ConsumeToken();
}
#endif
// Line directive
bool
GasParser::ParseDirLine(unsigned int param, clang::SourceLocation source)
{
    if (m_token.isNot(GasToken::numeric_constant))
    {
        Diag(m_token, diag::err_expected_integer);
        return false;
    }

    IntNum intn;
    if (!ParseInteger(&intn))
        return false;
    m_dir_line = intn.getUInt();

    if (m_dir_fileline == FL_BOTH)
    {
        // Have both file and line
        m_preproc.getSourceManager().AddLineNote(source, m_dir_line, -1);
    }
    else if (m_dir_fileline == FL_FILE)
    {
        // Had previous file directive only
        m_dir_fileline = FL_BOTH;
        clang::SourceManager& smgr = m_preproc.getSourceManager();
        smgr.AddLineNote(source, m_dir_line,
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
bool
GasParser::ParseDirInclude(unsigned int param, clang::SourceLocation source)
{
    if (m_token.isNot(GasToken::string_literal))
    {
        Diag(m_token, diag::err_expected_string);
        return false;
    }
    clang::SourceLocation filename_source = m_token.getLocation();
    llvm::SmallString<64> strbuf;
    GasStringParser str(m_token.getLiteral(), filename_source, m_preproc);
    if (str.hadError())
        return false;
    llvm::StringRef filename = str.getString(strbuf);
    ConsumeToken();
    return m_gas_preproc.HandleInclude(filename, filename_source);
}

bool
GasParser::ParseDirRept(unsigned int param, clang::SourceLocation source)
{
    Expr e;
    if (!ParseExpr(e))
    {
        Diag(source, diag::err_expected_expression_after_id) << ".rept";
        return false;
    }

    e.Simplify();
    if (!e.isIntNum())
    {
        Diag(source, diag::err_multiple_not_absolute);
        return false;
    }

    IntNum intn = e.getIntNum();
    if (intn.getSign() < 0)
    {
        Diag(source, diag::err_multiple_negative);
        return false;
    }
    unsigned long count = intn.getUInt();

    // Lex and save tokens until we get an .endr
    llvm::SmallVector<Token, 8> tokens;
    int depth = 1;
    for (;;)
    {
        if (m_token.isAtStartOfLine() && m_token.is(GasToken::label) &&
            m_token.getIdentifierInfo()->isStr(".endr"))
        {
            if (depth == 1)
                break;
            --depth;
        }
        // handle nesting
        if (m_token.isAtStartOfLine() && m_token.is(GasToken::label) &&
            m_token.getIdentifierInfo()->isStr(".rept"))
            ++depth;
        if (m_token.is(GasToken::eof))
        {
            Diag(source, diag::err_rept_without_endr);
            // TODO: Push remaining tokens back on so we can continue working
#if 0
            Token* alloc_tokens = new Token[tokens.size()];
            std::copy(tokens.begin(), tokens.end(), alloc_tokens);
            m_preproc.EnterTokenStream(alloc_tokens, tokens.size(), false,
                                       true);
            ConsumeToken(); // get the first token of stream
#endif
            return false;
        }
        tokens.push_back(m_token);
        ConsumeToken();
    }
    Token* alloc_tokens = new Token[count*tokens.size()];
    for (unsigned long i=0; i<count; ++i)
        std::copy(tokens.begin(), tokens.end(), &alloc_tokens[i*tokens.size()]);
    m_preproc.EnterTokenStream(alloc_tokens, count*tokens.size(), false, true);
    ConsumeToken(); // consume the .endr and get the first repeated token
    return true;
}

bool
GasParser::ParseDirEndr(unsigned int param, clang::SourceLocation source)
{
    // Shouldn't ever get here unless we didn't get a .rept first
    Diag(source, diag::err_endr_without_rept);
    return false;
}

//
// Alignment directives
//

bool
GasParser::ParseDirAlign(unsigned int power2, clang::SourceLocation source)
{
    Expr bound, fill, maxskip;

    if (!ParseExpr(bound))
    {
        Diag(source, diag::err_align_no_alignment);
        return false;
    }

    if (m_token.is(GasToken::comma))
    {
        ConsumeToken();
        ParseExpr(fill);
        if (m_token.is(GasToken::comma))
        {
            ConsumeToken();
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
                source);
    return true;
}

bool
GasParser::ParseDirOrg(unsigned int param, clang::SourceLocation source)
{
    // TODO: support expr instead of intnum
    if (m_token.isNot(GasToken::numeric_constant))
    {
        Diag(m_token, diag::err_expected_integer);
        return false;
    }
    IntNum start;
    if (!ParseInteger(&start))
        return false;
    ConsumeToken();

    IntNum value;
    if (m_token.is(GasToken::comma))
    {
        ConsumeToken();
        // TODO: support expr instead of intnum
        if (m_token.isNot(GasToken::numeric_constant))
        {
            Diag(m_token, diag::err_expected_integer);
            return false;
        }
        if (!ParseInteger(&value))
            return false;
        ConsumeToken();
    }

    AppendOrg(*m_container, start.getUInt(), value.getUInt(), source);
    return true;
}

//
// Data visibility directives
//

bool
GasParser::ParseDirLocal(unsigned int param, clang::SourceLocation source)
{
    if (m_token.isNot(GasToken::identifier) && m_token.isNot(GasToken::label))
    {
        Diag(m_token, diag::err_expected_ident);
        return false;
    }

    IdentifierInfo* ii = m_token.getIdentifierInfo();
    ParseSymbol(ii)->CheckedDeclare(Symbol::DLOCAL, ConsumeToken(),
                                    m_preproc.getDiagnostics());
    return true;
}

bool
GasParser::ParseDirComm(unsigned int is_lcomm, clang::SourceLocation source)
{
    if (m_token.isNot(GasToken::identifier) && m_token.isNot(GasToken::label))
    {
        Diag(m_token, diag::err_expected_ident);
        return false;
    }

    IdentifierInfo* ii = m_token.getIdentifierInfo();
    clang::SourceLocation id_source = ConsumeToken();

    ExpectAndConsume(GasToken::comma, diag::err_expected_comma);

    Expr e, align;
    clang::SourceLocation e_source = m_token.getLocation();
    if (!ParseExpr(e))
    {
        Diag(e_source, diag::err_comm_size_expected);
        return false;
    }
    clang::SourceLocation align_start, align_end;
    if (m_token.is(GasToken::comma))
    {
        // Optional alignment expression
        ConsumeToken();
        align_start = m_token.getLocation();
        ParseExpr(align);
        align_end = m_token.getLocation();
    }
    // If already explicitly declared local, treat like LCOMM
    SymbolRef sym = ParseSymbol(ii);
    if (is_lcomm || sym->getVisibility() == Symbol::DLOCAL)
    {
        std::auto_ptr<Expr> e_copy(new Expr);
        std::swap(*e_copy, e);
        DefineLcomm(sym, id_source, e_copy, align);
    }
    else if (!align.isEmpty())
    {
        // Give third parameter as objext valparam
        std::auto_ptr<Expr> align_copy(new Expr);
        std::swap(*align_copy, align);

        NameValues extvps;
        extvps.push_back(new NameValue(align_copy));
        extvps.back().setValueRange(clang::SourceRange(align_start, align_end));

        sym->CheckedDeclare(Symbol::COMMON, id_source,
                            m_preproc.getDiagnostics());
        setCommonSize(*sym, e);
        setObjextNameValues(*sym, extvps);
    }
    else
    {
        sym->CheckedDeclare(Symbol::COMMON, id_source,
                            m_preproc.getDiagnostics());
        setCommonSize(*sym, e);
    }
    return true;
}

//
// Integer data definition directives
//

bool
GasParser::ParseDirAscii(unsigned int withzero, clang::SourceLocation source)
{
    for (;;)
    {
        // <##> character constant
        if (m_token.is(GasToken::less))
        {
            clang::SourceLocation less_loc = ConsumeToken();
            if (m_token.isNot(GasToken::numeric_constant))
            {
                Diag(less_loc, diag::err_expected_string);
                return false;
            }

            IntNum val;
            if (!ParseInteger(&val))
                return false;
            clang::SourceLocation val_source = ConsumeToken();
            AppendByte(*m_container, val.getUInt() & 0xff);

            MatchRHSPunctuation(GasToken::greater, less_loc);
        }
        else if (m_token.is(GasToken::string_literal))
        {
            llvm::SmallString<64> strbuf;
            GasStringParser str(m_token.getLiteral(), m_token.getLocation(),
                                m_preproc);
            if (!str.hadError())
                AppendData(*m_container, str.getString(strbuf), withzero);
            ConsumeToken();
        }
        else
        {
            Diag(m_token, diag::err_expected_string);
            return false;
        }

        if (m_token.isNot(GasToken::comma))
            break;
        ConsumeToken();
    }
    return true;
}

bool
GasParser::ParseDirFloat(unsigned int size, clang::SourceLocation source)
{
    for (;;)
    {
        llvm::StringRef num_str;

        switch (m_token.getKind())
        {
            case GasToken::numeric_constant:
            {
                num_str = m_token.getLiteral();
                break;
            }
            case GasToken::label:
            {
                // Try to parse identifiers starting with . as floating point
                // numbers; this is to allow e.g. ".float .1" to work.
                IdentifierInfo* ii = m_token.getIdentifierInfo();
                num_str = ii->getName();
                if (num_str[0] == '.')
                    break;
                // fallthrough
            }
            default:
                Diag(m_token, diag::err_expected_float);
                return false;
        }

        GasNumericParser num(num_str, m_token.getLocation(), m_preproc, true);
        clang::SourceLocation num_source = ConsumeToken();
        if (num.hadError())
            ;
        else if (num.isInteger())
        {
            Diag(num_source, diag::err_expected_float);
        }
        else if (num.isFloat())
        {
            // FIXME: Make arch-dependent
            Expr::Ptr e(new Expr(std::auto_ptr<llvm::APFloat>(new llvm::APFloat(
                num.getFloatValue(llvm::APFloat::x87DoubleExtended)))));
            AppendData(*m_container, e, size, *m_arch, num_source);
        }
        if (m_token.isNot(GasToken::comma))
            break;
        ConsumeToken();
    }
    return true;
}

bool
GasParser::ParseDirData(unsigned int size, clang::SourceLocation source)
{
    for (;;)
    {
        clang::SourceLocation cur_source = m_token.getLocation();
        std::auto_ptr<Expr> e(new Expr);
        if (!ParseExpr(*e))
        {
            Diag(cur_source, diag::err_expected_expression_after) << ",";
            return false;
        }
        AppendData(*m_container, e, size, *m_arch, cur_source);
        if (m_token.isNot(GasToken::comma))
            break;
        ConsumeToken();
    }
    return true;
}

bool
GasParser::ParseDirLeb128(unsigned int sign, clang::SourceLocation source)
{
    for (;;)
    {
        clang::SourceLocation cur_source = m_token.getLocation();
        std::auto_ptr<Expr> e(new Expr);
        if (!ParseExpr(*e))
        {
            Diag(cur_source, diag::err_expected_expression_after) << ",";
            return false;
        }
        AppendLEB128(*m_container, e, sign, cur_source,
                     m_preproc.getDiagnostics());
        if (m_token.isNot(GasToken::comma))
            break;
        ConsumeToken();
    }
    return true;
}

//
// Empty space / fill data definition directives
//

bool
GasParser::ParseDirZero(unsigned int param, clang::SourceLocation source)
{
    clang::SourceLocation cur_source = m_token.getLocation();
    std::auto_ptr<Expr> e(new Expr);
    if (!ParseExpr(*e))
    {
        Diag(cur_source, diag::err_expected_expression_after_id) << ".ZERO";
        return false;
    }

    BytecodeContainer& inner = AppendMultiple(*m_container, e, source);
    AppendByte(inner, 0);
    return true;
}

bool
GasParser::ParseDirSkip(unsigned int param, clang::SourceLocation source)
{
    clang::SourceLocation cur_source = m_token.getLocation();
    std::auto_ptr<Expr> e(new Expr);
    if (!ParseExpr(*e))
    {
        Diag(cur_source, diag::err_expected_expression_after_id) << ".SKIP";
        return false;
    }

    BytecodeContainer& inner = AppendMultiple(*m_container, e, source);
    if (m_token.isNot(GasToken::comma))
    {
        inner.AppendGap(1, source);
        return true;
    }
    ConsumeToken();

    // expression after comma forces fill of that value (as a byte)
    cur_source = m_token.getLocation();
    std::auto_ptr<Expr> e_val(new Expr);
    if (!ParseExpr(*e_val))
    {
        Diag(cur_source, diag::err_expected_expression_after) << ",";
        return false;
    }
    AppendData(inner, e_val, 1, *m_arch, source);
    return true;
}

// fill data definition directive
bool
GasParser::ParseDirFill(unsigned int param, clang::SourceLocation source)
{
    std::auto_ptr<Expr> repeat(new Expr);
    Expr size, value;
    clang::SourceLocation size_src;
    if (!ParseExpr(*repeat))
    {
        Diag(m_token, diag::err_expected_expression_after_id) << ".FILL";
        return false;
    }
    if (m_token.is(GasToken::comma))
    {
        ConsumeToken();
        size_src = m_token.getLocation();
        if (!ParseExpr(size))
        {
            Diag(size_src, diag::err_expected_expression_after) << ",";
            return false;
        }
        if (m_token.is(GasToken::comma))
        {
            ConsumeToken();
            clang::SourceLocation value_src = m_token.getLocation();
            if (!ParseExpr(value))
            {
                Diag(value_src, diag::err_expected_expression_after) << ",";
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
            Diag(size_src, diag::err_fill_size_not_absolute);
            return false;
        }
        ssize = size.getIntNum().getUInt();
    }

    BytecodeContainer& inner = AppendMultiple(*m_container, repeat, source);
    if (value.isEmpty())
    {
        AppendData(inner, 0, ssize, *m_arch);
    }
    else
    {
        std::auto_ptr<Expr> value_copy(new Expr);
        std::swap(*value_copy, value);
        AppendData(inner, value_copy, ssize, *m_arch, source);
    }
    return true;
}

//
// Section directives
//

bool
GasParser::ParseDirBssSection(unsigned int param, clang::SourceLocation source)
{
    SwitchSection(".bss", true,
                  clang::SourceRange(source, source.getFileLocWithOffset(4)));
    return true;
}

bool
GasParser::ParseDirDataSection(unsigned int param, clang::SourceLocation source)
{
    SwitchSection(".data", true,
                  clang::SourceRange(source, source.getFileLocWithOffset(5)));
    return true;
}

bool
GasParser::ParseDirTextSection(unsigned int param, clang::SourceLocation source)
{
    SwitchSection(".text", true,
                  clang::SourceRange(source, source.getFileLocWithOffset(5)));
    return true;
}

bool
GasParser::ParseDirSection(unsigned int param, clang::SourceLocation source)
{
    // DIR_SECTION ID ',' STRING ',' '@' ID ',' dirvals
    // Really parsed as just a bunch of dirvals; only needs to be unique
    // function to handle section name as a special case.
    DirectiveInfo info(*m_object, source);

    if (m_token.is(GasToken::comma) || m_token.isEndOfStatement())
    {
        Diag(m_token, diag::err_directive_no_args);
        return false;
    }

    // Due to section names with special characters, concatenate tokens until
    // we either get a comma or a token with preceding space.
    unsigned int endtok = GasToken::comma;
    clang::SourceLocation start, end;
    llvm::SmallString<128> sectname_buf;
    llvm::StringRef sectname =
        MergeTokensUntil(&endtok, 1, &start, &end, sectname_buf);

    NameValues& nvs = info.getNameValues();
    nvs.push_back(new NameValue(sectname.str()));
    nvs.back().setValueRange(clang::SourceRange(start, end));

    if (!m_token.isEndOfStatement())
    {
        if (ExpectAndConsume(GasToken::comma, diag::err_expected_comma))
            return false;

        if (!ParseDirective(&nvs))
            return false;
    }

    Directive handler;
    if (m_dirs->get(&handler, ".section"))
        handler(info, m_preproc.getDiagnostics());
    else
        Diag(info.getSource(), diag::err_unrecognized_directive);

    return true;
}

//
// Other directives
//

bool
GasParser::ParseDirEqu(unsigned int param, clang::SourceLocation source)
{
    // ID ',' expr
    if (m_token.isNot(GasToken::identifier) && m_token.isNot(GasToken::label))
    {
        Diag(m_token, diag::err_expected_ident);
        return false;
    }
    IdentifierInfo* ii = m_token.getIdentifierInfo();
    clang::SourceLocation id_source = ConsumeToken();

    if (ExpectAndConsume(GasToken::comma, diag::err_expected_comma))
        return false;

    clang::SourceLocation expr_source = m_token.getLocation();
    Expr e;
    if (!ParseExpr(e))
    {
        Diag(expr_source, diag::err_expected_expression_after) << ",";
        return false;
    }
    ParseSymbol(ii)->CheckedDefineEqu(e, id_source, m_preproc.getDiagnostics());
    return true;
}

bool
GasParser::ParseDirFile(unsigned int param, clang::SourceLocation source)
{
    llvm::SmallString<64> filename_buf;

    if (m_token.is(GasToken::string_literal))
    {
        // No file number; this form also sets the assembler's
        // internal line number.
        GasStringParser filename(m_token.getLiteral(), m_token.getLocation(),
                                 m_preproc);
        if (filename.hadError())
            return false;
        clang::SourceRange filename_source = m_token.getSourceRange();
        ConsumeToken();

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
        setDebugFile(filename.getString(filename_buf), filename_source, source);
        return true;
    }

    //
    // fileno filename form
    //

    // file number
    if (m_token.isNot(GasToken::numeric_constant))
        return true;
    IntNum fileno;
    if (!ParseInteger(&fileno))
        return false;
    clang::SourceRange fileno_source = m_token.getSourceRange();
    ConsumeToken();

    // filename
    if (m_token.isNot(GasToken::string_literal))
    {
        Diag(m_token, diag::err_expected_string);
        return false;
    }
    GasStringParser filename(m_token.getLiteral(), m_token.getLocation(),
                             m_preproc);
    if (filename.hadError())
        return false;
    clang::SourceRange filename_source = m_token.getSourceRange();
    ConsumeToken();

    // Pass along to debug format
    setDebugFile(fileno, fileno_source, filename.getString(filename_buf),
                 filename_source, source);
    return true;
}

//
// Conditional compilation directives
//

void
GasParser::SkipConditional(clang::SourceLocation begin)
{
    Token prev_token = m_token;
    int skip_depth = 1;
    while (skip_depth > 0)
    {
        if (m_token.is(GasToken::eof))
        {
            Diag(begin, diag::err_pp_if_without_endif);
            return;
        }

        // handle nesting
        if (!m_token.isAtStartOfLine() || m_token.isNot(GasToken::label))
        {
            prev_token = m_token;
            ConsumeToken();
            continue;
        }
        IdentifierInfo* ii = m_token.getIdentifierInfo();
        if (ii->getName().startswith(".if"))
            ++skip_depth;
        else if (ii->isStr(".endif") || ii->isStr(".endc"))
        {
            if (skip_depth == 1)
            {
                // insert current token, and make EOL the current token
                m_preproc.EnterToken(m_token);
                m_token = prev_token;
                return;
            }
            --skip_depth;
        }
        else if (ii->isStr(".else") || ii->isStr(".elsec") ||
                 ii->isStr(".elseif"))
        {
            if (skip_depth == 1)
            {
                // insert current token, and make EOL the current token
                m_preproc.EnterToken(m_token);
                m_token = prev_token;
                return;
            }
        }
        prev_token = m_token;
        ConsumeToken();
    }
}

void
GasParser::HandleIf(bool is_true, clang::SourceLocation begin)
{
    if (!is_true)
        SkipConditional(begin);
    CondStatus status = { is_true, false };
    m_cond_stack.push_back(status);
}

bool
GasParser::ParseDirElse(unsigned int param, clang::SourceLocation source)
{
    if (m_cond_stack.empty())
    {
        Diag(source, diag::err_pp_else_without_if);
        return false;
    }

    if (m_cond_stack.back().saw_else)
    {
        Diag(source, diag::err_pp_else_after_else);
        return false;
    }
    m_cond_stack.back().saw_else = true;

    // If we should be skipping, do so.
    if (m_cond_stack.back().done)
    {
        SkipConditional(source);
        return true;
    }

    // Otherwise we should be outputting.
    m_cond_stack.back().done = true;
    return true;
}

bool
GasParser::ParseDirElseif(unsigned int param, clang::SourceLocation source)
{
    if (m_cond_stack.empty())
    {
        Diag(source, diag::err_pp_elseif_without_if);
        return false;
    }

    if (m_cond_stack.back().saw_else)
    {
        Diag(source, diag::err_pp_elseif_after_else);
        return false;
    }

    // if we've already output, don't even bother looking at the expression
    if (m_cond_stack.back().done)
    {
        SkipConditional(source);
        return true;
    }

    m_cond_stack.pop_back();
    return ParseDirIf(Op::NE, source);
}

bool
GasParser::ParseDirEndif(unsigned int param, clang::SourceLocation source)
{
    if (m_cond_stack.empty())
    {
        Diag(source, diag::err_pp_endif_without_if);
        return false;
    }

    m_cond_stack.pop_back();
    return true;
}

bool
GasParser::ParseDirIf(unsigned int op, clang::SourceLocation source)
{
    Expr e;
    if (!ParseExpr(e))
    {
        Diag(source, diag::err_expected_expression);
        return false;
    }

    if (!ExpandEqu(e))
    {
        Diag(source, diag::err_equ_circular_reference);
        return false;
    }

    e.Simplify();
    if (!e.isIntNum())
    {
        Diag(source, diag::err_pp_cond_not_constant);
        return false;
    }

    IntNum equal = e.getIntNum();
    equal.Calc(static_cast<Op::Op>(op), IntNum(0));
    HandleIf(equal.getUInt(), source);
    return true;
}

bool
GasParser::ParseDirIfb(unsigned int negate, clang::SourceLocation source)
{
    bool blank = m_token.isEndOfStatement();
    if (!blank)
        SkipUntil(GasToken::eol, GasToken::semi, true, false);
    HandleIf(negate ? !blank : blank, source);
    return true;
}

bool
GasParser::ParseDirIfdef(unsigned int negate, clang::SourceLocation source)
{
    if (m_token.isNot(GasToken::identifier) && m_token.isNot(GasToken::label))
    {
        Diag(m_token, diag::err_expected_ident);
        return false;
    }
    IdentifierInfo* ii = m_token.getIdentifierInfo();
    clang::SourceLocation id_source = ConsumeToken();

    bool defined = ii->isSymbol() && ii->getSymbol()->isDefined();
    HandleIf(negate ? !defined : defined, source);
    return true;
}

bool
GasParser::ParseDirIfeqs(unsigned int negate, clang::SourceLocation source)
{
    // first string
    if (m_token.isNot(GasToken::string_literal))
    {
        Diag(m_token, diag::err_expected_string);
        return false;
    }
    llvm::SmallString<64> s1_buf;
    GasStringParser s1(m_token.getLiteral(), m_token.getLocation(), m_preproc);
    if (s1.hadError())
        return false;
    ConsumeToken();

    if (ExpectAndConsume(GasToken::comma, diag::err_expected_comma))
        return false;

    // second string
    if (m_token.isNot(GasToken::string_literal))
    {
        Diag(m_token, diag::err_expected_string);
        return false;
    }
    llvm::SmallString<64> s2_buf;
    GasStringParser s2(m_token.getLiteral(), m_token.getLocation(), m_preproc);
    if (s2.hadError())
        return false;
    ConsumeToken();

    if (!m_token.isEndOfStatement())
    {
        Diag(m_token, diag::err_eol_junk);
        return false;
    }

    bool equal = s1.getString(s1_buf) == s2.getString(s2_buf);
    HandleIf(negate ? !equal : equal, source);
    return true;
}

Insn::Ptr
GasParser::ParseInsn()
{
    if (m_token.isNot(GasToken::identifier))
        return Insn::Ptr(0);

    IdentifierInfo* ii = m_token.getIdentifierInfo();
    ii->DoInsnLookup(*m_arch, m_token.getLocation(),
                     m_preproc.getDiagnostics());
    if (const Arch::InsnInfo* insninfo = ii->getInsn())
    {
        ConsumeToken();

        Insn::Ptr insn(m_arch->CreateInsn(insninfo));
        if (m_token.isEndOfStatement())
            return insn; // no operands

        // parse operands
        for (;;)
        {
            clang::SourceLocation start = m_token.getLocation();
            Operand op = ParseOperand();
            op.setSource(start);
            insn->AddOperand(op);

            if (m_token.isEndOfStatement())
                break;
            if (ExpectAndConsume(GasToken::comma, diag::err_expected_comma))
                break;
        }
        return insn;
    }
    if (const Prefix* prefix = ii->getPrefix())
    {
        clang::SourceLocation prefix_source = ConsumeToken();
        Insn::Ptr insn = ParseInsn();
        if (insn.get() == 0)
            insn = m_arch->CreateEmptyInsn();
        insn->AddPrefix(prefix, prefix_source);
        return insn;
    }

    // Check for segment register used as prefix
    ii->DoRegLookup(*m_arch, m_token.getLocation(), m_preproc.getDiagnostics());
    if (const SegmentRegister* segreg = ii->getSegReg())
    {
        clang::SourceLocation segreg_source = ConsumeToken();
        Insn::Ptr insn = ParseInsn();
        if (insn.get() == 0)
            insn = m_arch->CreateEmptyInsn();
        else if (insn->hasSegPrefix())
            Diag(segreg_source, diag::warn_multiple_seg_override);
        insn->setSegPrefix(segreg, segreg_source);
        return insn;
    }

    return Insn::Ptr(0);
}

bool
GasParser::ParseDirective(NameValues* nvs)
{
    for (;;)
    {
        if (m_token.isEndOfStatement())
            break;
        switch (m_token.getKind())
        {
            case GasToken::identifier:
            case GasToken::label:
            {
                const Token& peek_token = NextToken();
                switch (peek_token.getKind())
                {
                    case GasToken::plus: case GasToken::minus:
                    case GasToken::pipe: case GasToken::caret:
                    case GasToken::amp: case GasToken::exclaim:
                    case GasToken::star: case GasToken::slash:
                    case GasToken::percent: case GasToken::lessless:
                    case GasToken::greatergreater:
                    {
                        clang::SourceLocation e_src = m_token.getLocation();
                        Expr::Ptr e(new Expr);
                        if (!ParseExpr(*e))
                            return false;
                        nvs->push_back(new NameValue(e));
                        nvs->back().setValueRange(
                            clang::SourceRange(e_src, m_token.getLocation()));
                        break;
                    }
                    default:
                        // Just an ID
                        nvs->push_back(new NameValue(
                            m_token.getIdentifierInfo()->getName(),
                            '\0'));
                        nvs->back().setValueRange(m_token.getSourceRange());
                        ConsumeToken();
                        break;
                }
                break;
            }
            case GasToken::string_literal:
            {
                llvm::SmallString<64> strbuf;
                GasStringParser str(m_token.getLiteral(), m_token.getLocation(),
                                    m_preproc);
                clang::SourceRange str_source = m_token.getSourceRange();
                ConsumeToken();

                if (!str.hadError())
                {
                    nvs->push_back(new NameValue(str.getString(strbuf)));
                    nvs->back().setValueRange(str_source);
                }
                break;
            }
            case GasToken::at:
                // XXX: is throwing it away *really* the right thing?
                ConsumeToken();
                continue;
            default:
            {
                clang::SourceLocation e_src = m_token.getLocation();
                Expr::Ptr e(new Expr);
                if (!ParseExpr(*e))
                    return false;
                nvs->push_back(new NameValue(e));
                nvs->back().setValueRange(
                    clang::SourceRange(e_src, m_token.getLocation()));
                break;
            }
        }
        if (m_token.is(GasToken::comma))
            ConsumeToken();
    }
    return true;
}

// instruction operands
// memory addresses
Operand
GasParser::ParseMemoryAddress()
{
    bool strong = false;

    // We want to parse a leading expression, except when it's actually
    // just a memory address (with no preceding expression) such as
    // (REG...) or (,...).
    Expr e1;
    const Token& next_token = NextToken();
    if (m_token.isNot(GasToken::l_paren) ||
        (next_token.isNot(GasToken::percent) &&
         next_token.isNot(GasToken::comma)))
        ParseExpr(e1);

    if (m_token.is(GasToken::l_paren))
    {
        bool havereg = false;
        const Register* reg = 0;
        clang::SourceLocation scale_src;
        bool havescale = false;
        IntNum scale;
        Expr e2;

        clang::SourceLocation lparen_loc = ConsumeParen();

        // base register
        if (m_token.is(GasToken::percent))
        {
            ConsumeToken();
            const Register* basereg = ParseRegister();
            if (!basereg)
            {
                Diag(m_token, diag::err_bad_register_name);
                return Operand(m_object->getArch()->CreateEffAddr(Expr::Ptr(new Expr(0))));
            }
            ConsumeToken();
            e2 = Expr(*basereg);
        }
        else
            e2 = Expr(IntNum(0));

        if (m_token.is(GasToken::r_paren))
            goto done;

        if (ExpectAndConsume(GasToken::comma, diag::err_expected_comma))
            return Operand(m_object->getArch()->CreateEffAddr(Expr::Ptr(new Expr(0))));

        if (m_token.is(GasToken::r_paren))
            goto done;

        havescale = true;

        // index register
        if (m_token.is(GasToken::percent))
        {
            ConsumeToken();
            reg = ParseRegister();
            if (!reg)
            {
                Diag(m_token, diag::err_bad_register_name);
                return Operand(m_object->getArch()->CreateEffAddr(Expr::Ptr(new Expr(0))));
            }
            ConsumeToken();
            havereg = true;
            if (m_token.isNot(GasToken::comma))
            {
                scale = 1;
                goto done;
            }
            ConsumeToken();
        }

        // scale
        if (m_token.isNot(GasToken::numeric_constant))
        {
            Diag(m_token, diag::err_expected_integer);
            return Operand(m_object->getArch()->CreateEffAddr(Expr::Ptr(new Expr(0))));
        }
        if (!ParseInteger(&scale))
            return Operand(m_object->getArch()->CreateEffAddr(Expr::Ptr(new Expr(0))));
        scale_src = ConsumeToken();

done:
        MatchRHSPunctuation(GasToken::r_paren, lparen_loc);

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
/*
    if (e1.isEmpty())
    {
        throw SyntaxError(N_("expression syntax error"));
    }
*/
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
    switch (m_token.getKind())
    {
        case GasToken::percent:
        {
            // some kind of register operand
            // may also be a memory address (%segreg:memory)
            ConsumeToken();
            if (m_token.isNot(GasToken::identifier))
            {
                Diag(m_token, diag::err_bad_register_name);
                return Operand(Expr::Ptr(new Expr));
            }
            IdentifierInfo* ii = m_token.getIdentifierInfo();
            ii->DoRegLookup(*m_arch, m_token.getLocation(),
                            m_preproc.getDiagnostics());
            if (const SegmentRegister* segreg = ii->getSegReg())
            {
                clang::SourceLocation segreg_source = ConsumeToken();

                // if followed by ':', it's a memory address
                if (m_token.is(GasToken::colon))
                {
                    ConsumeToken();
                    Operand op = ParseMemoryAddress();
                    if (EffAddr* ea = op.getMemory())
                    {
                        if (ea->m_segreg != 0)
                            Diag(segreg_source,
                                 diag::warn_multiple_seg_override);
                        ea->m_segreg = segreg;
                    }
                    return op;
                }
                return Operand(segreg);
            }
            if (const Register* reg = ii->getRegister())
            {
                ConsumeToken();
                return Operand(reg);
            }
            if (const RegisterGroup* reggroup = ii->getRegGroup())
            {
                clang::SourceLocation reggroup_source = ConsumeToken();

                if (m_token.isNot(GasToken::l_paren))
                    return Operand(reggroup->getReg(0));
                clang::SourceLocation lparen_loc = ConsumeParen();

                if (m_token.isNot(GasToken::numeric_constant))
                {
                    Diag(m_token, diag::err_expected_integer);
                    return Operand(reggroup->getReg(0));
                }
                IntNum regindex;
                ParseInteger(&regindex);    // OK to ignore return value
                clang::SourceLocation regindex_source = ConsumeToken();

                MatchRHSPunctuation(GasToken::r_paren, lparen_loc);

                const Register* reg = reggroup->getReg(regindex.getUInt());
                if (!reg)
                {
                    Diag(regindex_source, diag::err_bad_register_index);
                    return Operand(reggroup->getReg(0));
                }
                return Operand(reg);
            }
            // didn't recognize it?
            Diag(m_token, diag::err_bad_register_name);
            ConsumeToken();
            return Operand(Expr::Ptr(new Expr));
        }
        case GasToken::dollar:
        {
            ConsumeToken();
            clang::SourceLocation e_source = m_token.getLocation();
            Expr::Ptr e(new Expr);
            if (!ParseExpr(*e))
                Diag(e_source, diag::err_missing_or_invalid_immediate);
            return Operand(e);
        }
        case GasToken::star:
            ConsumeToken();
            if (m_token.is(GasToken::percent))
            {
                // register
                ConsumeToken();
                const Register* reg = ParseRegister();
                if (!reg)
                {
                    Diag(m_token, diag::err_bad_register_name);
                    return Operand(Expr::Ptr(new Expr));
                }
                ConsumeToken();
                Operand op(reg);
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
// expr  : expr0 [ {&&,||} expr0...]
// expr0 : expr1 [ {+,-,==,<>,<,>,>=,<=} expr1...]
// expr1 : expr2 [ {|,^,&,!} expr2...]
// expr2 : expr3 [ {*,/,%,<<,>>} expr3...]
// expr3 : { ~,+,- } expr3
//       | (expr)
//       | symbol
//       | number

bool
GasParser::ParseExpr(Expr& e, const ParseExprTerm* parse_term)
{
    if (!ParseExpr0(e, parse_term))
        return false;

    for (;;)
    {
        Op::Op op;
        switch (m_token.getKind())
        {
            case GasToken::ampamp:      op = Op::LAND; break;
            case GasToken::pipepipe:    op = Op::LOR; break;
            default: return true;
        }
        ConsumeToken();

        Expr f;
        if (!ParseExpr0(f, parse_term))
            return false;
        e.Calc(op, f);
    }
}

bool
GasParser::ParseExpr0(Expr& e, const ParseExprTerm* parse_term)
{
    if (!ParseExpr1(e, parse_term))
        return false;

    for (;;)
    {
        Op::Op op;
        switch (m_token.getKind())
        {
            case GasToken::plus:            op = Op::ADD; break;
            case GasToken::minus:           op = Op::SUB; break;
            case GasToken::equalequal:      op = Op::EQ; break;
            case GasToken::lessgreater:     op = Op::NE; break;
            case GasToken::less:            op = Op::LT; break;
            case GasToken::greater:         op = Op::GT; break;
            case GasToken::lessequal:       op = Op::LE; break;
            case GasToken::greaterequal:    op = Op::GE; break;
            default: return true;
        }
        ConsumeToken();

        Expr f;
        if (!ParseExpr1(f, parse_term))
            return false;
        e.Calc(op, f);
    }
}

bool
GasParser::ParseExpr1(Expr& e, const ParseExprTerm* parse_term)
{
    if (!ParseExpr2(e, parse_term))
        return false;

    for (;;)
    {
        Op::Op op;
        switch (m_token.getKind())
        {
            case GasToken::pipe:    op = Op::OR; break;
            case GasToken::caret:   op = Op::XOR; break;
            case GasToken::amp:     op = Op::AND; break;
            case GasToken::exclaim: op = Op::NOR; break;
            default: return true;
        }
        ConsumeToken();

        Expr f;
        if (!ParseExpr2(f, parse_term))
            return false;
        e.Calc(op, f);
    }
}

bool
GasParser::ParseExpr2(Expr& e, const ParseExprTerm* parse_term)
{
    if (!ParseExpr3(e, parse_term))
        return false;

    for (;;)
    {
        Op::Op op;
        switch (m_token.getKind())
        {
            case GasToken::star:            op = Op::MUL; break;
            case GasToken::slash:           op = Op::DIV; break;
            case GasToken::percent:         op = Op::MOD; break;
            case GasToken::lessless:        op = Op::SHL; break;
            case GasToken::greatergreater:  op = Op::SHR; break;
            default: return true;
        }
        ConsumeToken();

        Expr f;
        if (!ParseExpr3(f, parse_term))
            return false;
        e.Calc(op, f);
    }
}

bool
GasParser::ParseExpr3(Expr& e, const ParseExprTerm* parse_term)
{
    if (parse_term)
    {
        bool handled = false;
        if (!(*parse_term)(e, *this, &handled))
            return false;
        if (handled)
            return true;
    }

    switch (m_token.getKind())
    {
        case GasToken::plus:
            ConsumeToken();
            return ParseExpr3(e, parse_term);
        case GasToken::minus:
            ConsumeToken();
            if (!ParseExpr3(e, parse_term))
                return false;
            e.Calc(Op::NEG);
            return true;
        case GasToken::tilde:
            ConsumeToken();
            if (!ParseExpr3(e, parse_term))
                return false;
            e.Calc(Op::NOT);
            return true;
        case GasToken::l_paren:
        {
            clang::SourceLocation lparen_loc = ConsumeParen();
            if (!ParseExpr(e, parse_term))
                return false;
            MatchRHSPunctuation(GasToken::r_paren, lparen_loc);
            return true;
        }
        case GasToken::numeric_constant:
        {
            GasNumericParser num(m_token.getLiteral(), m_token.getLocation(),
                                 m_preproc);
            if (num.hadError())
                e = IntNum(0);
            else if (num.isInteger())
            {
                IntNum val;
                num.getIntegerValue(&val);
                e = val;
            }
            else if (num.isFloat())
            {
                // FIXME: Make arch-dependent
                e = Expr(std::auto_ptr<llvm::APFloat>(new llvm::APFloat(
                    num.getFloatValue(llvm::APFloat::x87DoubleExtended))));
            }
            break;
        }
        case GasToken::char_constant:
        {
            GasStringParser str(m_token.getLiteral(), m_token.getLocation(),
                                m_preproc);
            if (str.hadError())
                e = IntNum(0);
            else
            {
                IntNum val;
                str.getIntegerValue(&val);
                val = val.Extract(8, 0);
                e = val;
            }
            break;
        }
        case GasToken::identifier:
        case GasToken::label:
        {
            IdentifierInfo* ii = m_token.getIdentifierInfo();
            clang::SourceLocation id_source = ConsumeToken();
            // "." references the current assembly position
            if (ii->isStr("."))
            {
                SymbolRef sym = m_object->AddNonTableSymbol(".");
                Bytecode& bc = m_container->FreshBytecode();
                Location loc = {&bc, bc.getFixedLen()};
                sym->CheckedDefineLabel(loc, id_source,
                                        m_preproc.getDiagnostics());
                e = sym;
            }
            else
            {
                SymbolRef sym = ParseSymbol(ii);
                sym->Use(id_source);
                e = sym;
            }

            if (m_token.is(GasToken::at))
            {
                // This is needed for shared objects, e.g. sym@PLT
                ConsumeToken();
                if (m_token.isNot(GasToken::identifier) &&
                    m_token.isNot(GasToken::label))
                {
                    Diag(m_token, diag::err_expected_ident);
                    return false;
                }
                SymbolRef wrt = m_object->FindSpecialSymbol(
                    m_token.getIdentifierInfo()->getName());
                if (wrt)
                    e.Calc(Op::WRT, wrt);
                else
                    Diag(m_token, diag::warn_unrecognized_ident);
                ConsumeToken();
            }
            return true;
        }
        default:
            return false;
    }
    ConsumeToken();
    return true;
}

SymbolRef
GasParser::ParseSymbol(IdentifierInfo* ii)
{
    // see if there's a cached version
    if (ii->isSymbol())
        return ii->getSymbol();

    // otherwise, get it from object
    SymbolRef sym = m_object->getSymbol(ii->getName());
    ii->setSymbol(sym);    // cache it
    return sym;
}

bool
GasParser::ParseInteger(IntNum* intn)
{
    assert(m_token.is(GasToken::numeric_constant));

    GasNumericParser num(m_token.getLiteral(), m_token.getLocation(),
                         m_preproc);
    if (num.hadError())
    {
        intn->Zero();
        return false;
    }
    else if (num.isInteger())
    {
        num.getIntegerValue(intn);
        return true;
    }
    else
    {
        Diag(m_token, diag::err_expected_integer);
        intn->Zero();
        return false;
    }
}

const Register*
GasParser::ParseRegister()
{
    if (m_token.isNot(GasToken::identifier))
        return 0;
    IdentifierInfo* ii = m_token.getIdentifierInfo();
    ii->DoRegLookup(*m_arch, m_token.getLocation(), m_preproc.getDiagnostics());
    return ii->getRegister();
}

void
GasParser::DefineLabel(llvm::StringRef name, clang::SourceLocation source)
{
    SymbolRef sym = m_object->getSymbol(name);
    Bytecode& bc = m_container->FreshBytecode();
    Location loc = {&bc, bc.getFixedLen()};
    sym->CheckedDefineLabel(loc, source, m_preproc.getDiagnostics());
}

void
GasParser::DefineLcomm(SymbolRef sym,
                       clang::SourceLocation source,
                       std::auto_ptr<Expr> size,
                       const Expr& align)
{
    // Put into .bss section.
    Section& bss = getSection(".bss", true, source);

    if (!align.isEmpty())
    {
        // XXX: assume alignment is in bytes, not power-of-two
        AppendAlign(bss, align, Expr(), Expr(), 0, source);
    }

    // Create common symbol
    Bytecode *bc = &bss.FreshBytecode();
    Location loc = {bc, bc->getFixedLen()};
    sym->CheckedDefineLabel(loc, source, m_preproc.getDiagnostics());

    // Append gap for symbol storage
    size->Simplify();
    if (size->isIntNum())
        bss.AppendGap(size->getIntNum().getUInt(), source);
    else
    {
        BytecodeContainer& multc = AppendMultiple(bss, size, source);
        multc.AppendGap(1, source);
    }
}

void
GasParser::SwitchSection(llvm::StringRef name,
                         bool builtin,
                         clang::SourceRange source)
{
    DirectiveInfo info(*m_object, source.getBegin());
    NameValues& nvs = info.getNameValues();
    nvs.push_back(new NameValue(name, '\0'));
    nvs.back().setValueRange(source);

    Directive handler;
    if (m_dirs->get(&handler, ".section"))
        handler(info, m_preproc.getDiagnostics());
    else
        Diag(info.getSource(), diag::err_unrecognized_directive);
}

Section&
GasParser::getSection(llvm::StringRef name,
                      bool builtin,
                      clang::SourceRange source)
{
    Section* cur_section = m_object->getCurSection();
    SwitchSection(name, builtin, source);
    Section& new_section = *m_object->getCurSection();
    m_object->setCurSection(cur_section);
    return new_section;
}

void
GasParser::DoParse()
{
    while (m_token.isNot(GasToken::eof))
    {
        if (m_token.isEndOfStatement())
            ConsumeToken();
        else
        {
            bool result = ParseLine();
            if (result && !m_token.isEndOfStatement())
                Diag(m_token, diag::err_eol_junk);
            SkipUntil(GasToken::eol, GasToken::semi, true, false);
        }
    }
}

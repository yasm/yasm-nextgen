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
#include <util.h>

#include <cctype>
#include <climits>
#include <cmath>

#include <yasmx/Support/bitcount.h>
#include <yasmx/Support/Compose.h>
#include <yasmx/Support/errwarn.h>
#include <yasmx/Arch.h>
#include <yasmx/Bytecode.h>
#include <yasmx/BytecodeContainer.h>
#include <yasmx/BytecodeContainer_util.h>
#include <yasmx/Directive.h>
#include <yasmx/Errwarns.h>
#include <yasmx/FloatNum.h>
#include <yasmx/NameValue.h>
#include <yasmx/Object.h>
#include <yasmx/Preprocessor.h>
#include <yasmx/Section.h>
#include <yasmx/Symbol.h>
#include <yasmx/Symbol_util.h>

#include "GasParser.h"


namespace yasm
{
namespace parser
{
namespace gas
{

const char*
GasParser::describe_token(int token)
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

void
GasParser::parse_line()
{
    if (is_eol())
        return;

    m_container = m_object->get_cur_section();

    Insn::Ptr insn = parse_instr();
    if (insn.get() != 0)
    {
        insn->append(*m_container, get_cur_line());
        return;
    }

    switch (m_token)
    {
        case ID:
        {
            std::string name;
            std::swap(name, ID_val);
            get_next_token(); // ID

            // See if it's a gas-specific directive
            GasDirMap::iterator p = m_gas_dirs.find(name);
            if (p != m_gas_dirs.end())
            {
                // call directive handler (function in this class) w/parameter
                (this->*(p->second->handler))(p->second->param);
                return;
            }

            if (m_token == ':')
            {
                // Label
                m_state = INITIAL;
                get_next_token(); // :
                define_label(name, false);
                parse_line();
                break;
            }
            else if (m_token == '=')
            {
                // EQU
                // TODO: allow redefinition, assigning to . (same as .org)
                m_state = INITIAL;
                get_next_token(); // =
                Expr e;
                if (!parse_expr(e))
                {
                    throw SyntaxError(String::compose(
                        N_("expression expected after `%1'"), "="));
                }
                m_object->get_symbol(name)->define_equ(e, get_cur_line());
                break;
            }

            // possibly a directive; try to parse it
            NameValues nvs, objext_nvs;
            parse_dirvals(&nvs);
            Directive dir;
            if (m_dirs->get(&dir, name))
            {
                dir(*m_object, nvs, objext_nvs, get_cur_line());
                break;
            }

            // didn't match, warn/error as appropriate
            if (name[0] == '.')
                warn_set(WARN_GENERAL, String::compose(
                    N_("directive `%1' not recognized"), name));
            else
                throw SyntaxError(String::compose(
                    N_("instruction not recognized: `%1'"), name));
            break;
        }
        case LABEL:
            define_label(LABEL_val, false);
            get_next_token(); // LABEL
            parse_line();
            break;
        case CPP_LINE_MARKER:
            get_next_token();
            cpp_line_marker();
            break;
        case NASM_LINE_MARKER:
            get_next_token();
            nasm_line_marker();
            break;
        default:
            throw SyntaxError(
                N_("label or instruction expected at start of line"));
    }
}

void
GasParser::debug_file(NameValues& nvs)
{
    Directive dir;
    if (!m_dirs->get(&dir, ".file"))
        return;

    NameValues objext_nvs;
    dir(*m_object, nvs, objext_nvs, get_cur_line());
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
GasParser::cpp_line_marker()
{
    // Line number.
    if (m_token != INTNUM)
    {
        // Skip over a comment.
        while (m_token != '\n')
            get_next_token();

        return;
    }

    if (INTNUM_val->sign() < 0)
    {
        get_next_token(); // INTNUM
        throw SyntaxError(N_("line number is negative"));
    }

    unsigned long line = INTNUM_val->get_uint();

    // Set to (line - 1) since the directive indicates that the *next* line
    // will have the number given.
    //
    // cpp should never produce line=0, but the if keeps us safe just incase.
    if (line != 0)
        line--;

    get_next_token(); // INTNUM

    // File name, in quotes.
    if (m_token != STRING)
    {
        // Skip over a comment.
        while (m_token != '\n')
            get_next_token();

        return;
    }

    std::string filename;
    std::swap(filename, STRING_val);
    get_next_token();

    // Set linemap.
    m_linemap->set(filename, line, 1);

    // The first line marker in the file (which should be on the first line
    // of the file) will give us the name of the source file. This information
    // needs to be passed on to the debug format module.
    if (!m_seen_line_marker)
    {
        m_seen_line_marker = false;

        NameValues nvs;
        nvs.push_back(new NameValue(filename));
        debug_file(nvs);
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
        get_next_token();
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
GasParser::nasm_line_marker()
{
    // Line number.
    if (m_token != INTNUM) return;

    if (INTNUM_val->sign() < 0)
    {
        get_next_token(); // INTNUM
        throw SyntaxError(N_("line number is negative"));
    }

    unsigned long line = INTNUM_val->get_uint();

    // Set to (line - 1) since the directive indicates that the *next* line
    // will have the number given.
    //
    // cpp should never produce line=0, but the if keeps us safe just incase.
    if (line != 0)
        line--;

    get_next_token(); // INTNUM

    if (m_token != '+') return;
    get_next_token(); // +

    // Line number increment.
    if (m_token != INTNUM) return;

    if (INTNUM_val->sign() < 0)
    {
        get_next_token(); // INTNUM
        throw SyntaxError(N_("line increment is negative"));
    }

    unsigned long incr = INTNUM_val->get_uint();

    // File name is not in quotes, so need to switch to a different tokenizer
    // state.
    m_state = NASM_FILENAME;
    get_next_token(); // INTNUM
    if (m_token != STRING)
    {
        m_state = INITIAL;
        return;
    }

    std::string filename;
    std::swap(filename, STRING_val);

    // Set linemap.
    m_linemap->set(filename, line, incr);

    // The first line marker in the file (which should be on the first line
    // of the file) will give us the name of the source file. This information
    // needs to be passed on to the debug format module.
    if (!m_seen_line_marker)
    {
        m_seen_line_marker = true;

        NameValues nvs;
        nvs.push_back(new NameValue(filename));
        debug_file(nvs);
    }

    // We need to poke back on the \n that was consumed by the tokenizer
    m_peek_token = '\n';
    get_next_token();
}

// Line directive
void
GasParser::dir_line(unsigned int param)
{
    expect(INTNUM);
    if (INTNUM_val->sign() < 0)
    {
        get_next_token(); // INTNUM
        throw SyntaxError(N_("line number is negative"));
    }

    m_dir_line = INTNUM_val->get_uint();
    get_next_token(); // INTNUM

    if (m_dir_fileline == FL_BOTH)
    {
        // Have both file and line
        m_linemap->set(m_dir_line, 1);
    }
    else if (m_dir_fileline == FL_FILE)
    {
        // Had previous file directive only
        m_dir_fileline = FL_BOTH;
        m_linemap->set(m_dir_file, m_dir_line, 1);
    }
    else
    {
        // Didn't see file yet
        m_dir_fileline = FL_LINE;
    }
}

//
// Macro directives
//

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

void
GasParser::dir_rept(unsigned int param)
{
    Expr e;
    if (!parse_expr(e))
    {
        throw SyntaxError(String::compose(
            N_("expression expected after `%1'"), ".rept"));
    }

    e.simplify();
    if (!e.is_intnum())
        throw NotAbsoluteError(N_("rept expression not absolute"));

    IntNum intn = e.get_intnum();
    if (intn.sign() < 0)
        throw ValueError(N_("rept expression is negative"));

    m_rept.push_back(new GasRept(get_cur_line(), intn.get_uint()));
}

void
GasParser::dir_endr(unsigned int param)
{
    // Shouldn't ever get here unless we didn't get a DIR_REPT first
    throw SyntaxError(N_("endr without matching rept"));
}

//
// Alignment directives
//

void
GasParser::dir_align(unsigned int power2)
{
    Expr bound, fill, maxskip;

    if (!parse_expr(bound))
        throw SyntaxError(N_(".align directive must specify alignment"));

    if (m_token == ',')
    {
        get_next_token(); // ','
        parse_expr(fill);
        if (m_token == ',')
        {
            get_next_token(); // ','
            parse_expr(maxskip);
        }
    }

    Section* cur_section = m_container->as_section();

    // Convert power of two to number of bytes if necessary
    if (power2)
        bound = SHL(1, bound);

    // Largest .align in the section specifies section alignment.
    bound.simplify();
    if (bound.is_intnum())
    {
        unsigned long boundint = bound.get_intnum().get_uint();

        // Alignments must be a power of two.
        if (is_exp2(boundint))
        {
            if (boundint > cur_section->get_align())
                cur_section->set_align(boundint);
        }
    }

    append_align(*cur_section, bound, fill, maxskip,
                 cur_section->is_code() ?  m_object->get_arch()->get_fill() : 0,
                 get_cur_line());
}

void
GasParser::dir_org(unsigned int param)
{
    // TODO: support expr instead of intnum
    expect(INTNUM);
    unsigned long start = INTNUM_val->get_uint();
    get_next_token(); // INTNUM

    unsigned long value = 0;
    if (m_token == ',')
    {
        get_next_token(); // ','
        // TODO: support expr instead of intnum
        expect(INTNUM);
        value = INTNUM_val->get_uint();
        get_next_token(); // INTNUM
    }

    append_org(*m_container, start, value, get_cur_line());
}

//
// Data visibility directives
//

void
GasParser::dir_local(unsigned int param)
{
    expect(ID);
    m_object->get_symbol(ID_val)->declare(Symbol::DLOCAL, get_cur_line());
    get_next_token(); // ID
}

void
GasParser::dir_comm(unsigned int is_lcomm)
{
    expect(ID);
    std::string id;
    std::swap(id, ID_val);
    get_next_token(); // ID

    expect(',');
    get_next_token(); // ,

    Expr e, align;
    if (!parse_expr(e))
    {
        throw SyntaxError(String::compose(N_("size expected for `%1'"),
                                          ".COMM"));
    }
    if (m_token == ',')
    {
        // Optional alignment expression
        get_next_token(); // ','
        parse_expr(align);
    }
    // If already explicitly declared local, treat like LCOMM
    SymbolRef oldsym = m_object->find_symbol(id);
    if (is_lcomm || (oldsym && oldsym->get_visibility() == Symbol::DLOCAL))
    {
        std::auto_ptr<Expr> e_copy(new Expr);
        std::swap(*e_copy, e);
        define_lcomm(id, e_copy, align);
    }
    else if (!align.is_empty())
    {
        // Give third parameter as objext valparam
        std::auto_ptr<Expr> align_copy(new Expr);
        std::swap(*align_copy, align);

        NameValues extvps;
        extvps.push_back(new NameValue(align_copy));

        SymbolRef sym = m_object->get_symbol(id);
        sym->declare(Symbol::COMMON, get_cur_line());
        set_common_size(*sym, e);
        set_objext_namevals(*sym, extvps);
    }
    else
    {
        SymbolRef sym = m_object->get_symbol(id);
        sym->declare(Symbol::COMMON, get_cur_line());
        set_common_size(*sym, e);
    }
}

//
// Integer data definition directives
//

void
GasParser::dir_ascii(unsigned int withzero)
{
    for (;;)
    {
        expect(STRING);
        append_data(*m_container, STRING_val, withzero);
        get_next_token(); // STRING
        if (m_token != ',')
            break;
        get_next_token(); // ','
    }
}

void
GasParser::dir_data(unsigned int size)
{
    for (;;)
    {
        std::auto_ptr<Expr> e(new Expr);
        if (!parse_expr(*e))
            throw SyntaxError(N_("expression expected after `,'"));
        append_data(*m_container, e, size, *m_arch, get_cur_line());
        if (m_token != ',')
            break;
        get_next_token(); // ','
    }
}

void
GasParser::dir_leb128(unsigned int sign)
{
    for (;;)
    {
        std::auto_ptr<Expr> e(new Expr);
        if (!parse_expr(*e))
            throw SyntaxError(N_("expression expected after `,'"));
        append_leb128(*m_container, e, sign, get_cur_line());
        if (m_token != ',')
            break;
        get_next_token(); // ','
    }
}

//
// Empty space / fill data definition directives
//

void
GasParser::dir_zero(unsigned int param)
{
    std::auto_ptr<Expr> e(new Expr);
    if (!parse_expr(*e))
    {
        throw SyntaxError(String::compose(N_("expression expected after `%1'"),
                                          ".ZERO"));
    }

    BytecodeContainer& inner = append_multiple(*m_container, e, get_cur_line());
    append_byte(inner, 0);
}

void
GasParser::dir_skip(unsigned int param)
{
    std::auto_ptr<Expr> e(new Expr);
    if (!parse_expr(*e))
    {
        throw SyntaxError(String::compose(N_("expression expected after `%1'"),
                                          ".SKIP"));
    }

    BytecodeContainer& inner = append_multiple(*m_container, e, get_cur_line());
    if (m_token != ',')
    {
        inner.append_gap(1, get_cur_line());
        return;
    }
    get_next_token(); // ','

    // expression after comma forces fill of that value (as a byte)
    std::auto_ptr<Expr> e_val(new Expr);
    if (!parse_expr(*e_val))
        throw SyntaxError(N_("expression expected after `,'"));
    append_data(inner, e_val, 1, *m_arch, get_cur_line());
}

// fill data definition directive
void
GasParser::dir_fill(unsigned int param)
{
    std::auto_ptr<Expr> repeat(new Expr);
    Expr size, value;
    if (!parse_expr(*repeat))
    {
        throw SyntaxError(String::compose(N_("expression expected after `%1'"),
                                          ".FILL"));
    }
    if (m_token == ',')
    {
        get_next_token(); // ','
        if (!parse_expr(size))
            throw SyntaxError(N_("expression expected after `,'"));
        if (m_token == ',')
        {
            get_next_token(); // ','
            if (!parse_expr(value))
                throw SyntaxError(N_("expression expected after `,'"));
        }
    }

    unsigned int ssize = 1;
    if (!size.is_empty())
    {
        size.simplify();
        if (!size.is_intnum())
            throw NotAbsoluteError(N_("size must be an absolute expression"));
        ssize = size.get_intnum().get_uint();
    }

    BytecodeContainer& inner =
        append_multiple(*m_container, repeat, get_cur_line());
    if (value.is_empty())
    {
        append_data(inner, 0, ssize, *m_arch);
    }
    else
    {
        std::auto_ptr<Expr> value_copy(new Expr);
        std::swap(*value_copy, value);
        append_data(inner, value_copy, ssize, *m_arch, get_cur_line());
    }
}

//
// Section directives
//

void
GasParser::dir_bss_section(unsigned int param)
{
    NameValues nvs;
    switch_section(".bss", nvs, true);
}

void
GasParser::dir_data_section(unsigned int param)
{
    NameValues nvs;
    switch_section(".data", nvs, true);
}

void
GasParser::dir_text_section(unsigned int param)
{
    NameValues nvs;
    switch_section(".text", nvs, true);
}

void
GasParser::dir_section(unsigned int param)
{
    // DIR_SECTION ID ',' STRING ',' '@' ID ',' dirvals
    // Really parsed as just a bunch of dirvals; only needs to be unique
    // function to set parser state appropriately.
    m_state = SECTION_DIRECTIVE;
    NameValues nvs, objext_nvs;
    parse_dirvals(&nvs);
    (*m_dirs)[".section"](*m_object, nvs, objext_nvs, get_cur_line());
    m_state = INITIAL;
}

//
// Other directives
//

void
GasParser::dir_equ(unsigned int param)
{
    // ID ',' expr
    expect(ID);
    std::string id;
    std::swap(id, ID_val);
    get_next_token(); // ID

    expect(',');
    get_next_token(); // ','

    Expr e;
    if (!parse_expr(e))
        throw SyntaxError(N_("expression expected after `,'"));
    m_object->get_symbol(id)->define_equ(e, get_cur_line());
}

void
GasParser::dir_file(unsigned int param)
{
    NameValues nvs;

    if (m_token == STRING)
    {
        // No file number; this form also sets the assembler's
        // internal line number.
        std::string filename;
        std::swap(filename, STRING_val);
        get_next_token(); // STRING

        if (m_dir_fileline == FL_BOTH)
        {
            // Have both file and line
            std::string old_fn;
            unsigned long old_line;

            m_linemap->lookup(get_cur_line(), &old_fn, &old_line);
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

        // Pass change along to debug format
        nvs.push_back(new NameValue(filename));
        debug_file(nvs);
        return;
    }

    // fileno filename form
    if (m_token != INTNUM)
        return;
    nvs.push_back(new NameValue(Expr::Ptr(new Expr(INTNUM_val))));
    get_next_token(); // INTNUM

    expect(STRING);
    nvs.push_back(new NameValue(STRING_val));
    get_next_token(); // STRING

    debug_file(nvs);
}

Insn::Ptr
GasParser::parse_instr()
{
    if (m_token != ID)
        return Insn::Ptr(0);

    // instructions/prefixes must start with a letter
    if (!isalpha(ID_val[0]))
        return Insn::Ptr(0);

    // check to be sure it's not a label or equ
    get_peek_token();
    if (m_peek_token == ':' || m_peek_token == '=')
        return Insn::Ptr(0);

    Arch::InsnPrefix ip =
        m_arch->parse_check_insnprefix(ID_val.c_str(), ID_val.length(),
                                       get_cur_line());
    switch (ip.get_type())
    {
        case Arch::InsnPrefix::INSN:
        {
            // Propagate errors in case we got a warning from the arch
            m_errwarns->propagate(get_cur_line());

            get_next_token();   // ID

            Insn::Ptr insn = ip.release_insn();
            if (!is_eol())
            {
                // parse operands
                for (;;)
                {
                    insn->add_operand(parse_operand());
                    if (is_eol())
                        break;
                    expect(',');
                    get_next_token();
                }
            }
            return insn;
        }
        case Arch::InsnPrefix::PREFIX:
        {
            // Propagate errors in case we got a warning from the arch
            m_errwarns->propagate(get_cur_line());

            get_next_token();   // ID

            Insn::Ptr insn = parse_instr();
            if (insn.get() == 0)
                insn = m_arch->create_empty_insn();
            insn->add_prefix(ip.get_prefix());
            return insn;
        }
        default:
            break;
    }

    // Check for segment register used as prefix
    Arch::RegTmod regtmod =
        m_arch->parse_check_regtmod(ID_val.c_str(), ID_val.length());
    switch (regtmod.get_type())
    {
        case Arch::RegTmod::SEGREG:
        {
            get_next_token();   // ID
            Insn::Ptr insn = parse_instr();
            if (insn.get() == 0)
                insn = m_arch->create_empty_insn();
            insn->add_seg_prefix(regtmod.get_segreg());
            return insn;
        }
        default:
            return Insn::Ptr(0);
    }
}

void
GasParser::parse_dirvals(NameValues* nvs)
{
    for (;;)
    {
        switch (m_token)
        {
            case ID:
                get_peek_token();
                switch (m_peek_token)
                {
                    case '+': case '-':
                    case '|': case '^': case '&': case '!':
                    case '*': case '/': case '%': case LEFT_OP: case RIGHT_OP:
                    {
                        Expr::Ptr e(new Expr);
                        if (!parse_expr(*e))
                            return;
                        nvs->push_back(new NameValue(e));
                        break;
                    }
                    default:
                        // Just an ID
                        nvs->push_back(new NameValue(ID_val, '\0'));
                        get_next_token(); // ID
                        break;
                }
                break;
            case STRING:
                nvs->push_back(new NameValue(STRING_val));
                get_next_token(); // STRING
                break;
            case REG:
                nvs->push_back(new NameValue(Expr::Ptr(new Expr(*REG_val))));
                get_next_token(); // REG
                break;
            case '@':
                // XXX: is throwing it away *really* the right thing?
                get_next_token(); // @
                continue;
            default:
            {
                Expr::Ptr e(new Expr);
                if (!parse_expr(*e))
                    return;
                nvs->push_back(new NameValue(e));
                break;
            }
        }
        if (m_token == ',')
            get_next_token(); // ','
    }
}

// instruction operands
// memory addresses
Insn::Operand
GasParser::parse_memaddr()
{
    bool strong = false;

    if (m_token == SEGREG)
    {
        const SegmentRegister* segreg = SEGREG_val;
        get_next_token(); // SEGREG
        expect(':');
        get_next_token(); // ':'
        Insn::Operand op = parse_memaddr();
        op.get_memory()->set_segreg(segreg);
        return op;
    }

    // We want to parse a leading expression, except when it's actually
    // just a memory address (with no preceding expression) such as
    // (REG...) or (,...).
    Expr e1;
    get_peek_token();
    if (m_token != '(' || (m_peek_token != REG && m_peek_token != ','))
        parse_expr(e1);

    if (m_token == '(')
    {
        bool havereg = false;
        const Register* reg = 0;
        bool havescale = false;
        IntNum scale;
        Expr e2;

        get_next_token(); // '('

        // base register
        if (m_token == REG)
        {
            e2 = Expr(*REG_val);
            get_next_token(); // REG
        }
        else
            e2 = Expr(IntNum(0));

        if (m_token == ')')
            goto done;

        if (m_token != ',')
            throw SyntaxError(N_("invalid memory expression"));
        get_next_token(); // ','

        if (m_token == ')')
            goto done;

        havescale = true;

        // index register
        if (m_token == REG)
        {
            reg = REG_val;
            havereg = true;
            get_next_token(); // REG
            if (m_token != ',')
            {
                scale = 1;
                goto done;
            }
            get_next_token(); // ','
        }

        // scale
        if (m_token != INTNUM)
            throw SyntaxError(N_("non-integer scale"));
        scale = *INTNUM_val;
        get_next_token(); // INTNUM

done:
        if (m_token != ')')
            throw SyntaxError(N_("invalid memory expression"));
        get_next_token(); // ')'

        if (havescale)
        {
            if (!havereg)
            {
                if (scale.get_uint() != 1)
                    warn_set(WARN_GENERAL, String::compose(
                        N_("scale factor of %u without an index register"),
                        scale));
            }
            else
            {
                e2 += MUL(*reg, scale);
            }
        }

        if (!e1.is_empty())
        {
            // Ordering is critical here to correctly detecting presence of
            // RIP in RIP-relative expressions.
            e1 = ADD(e2, e1);
        }
        else
            std::swap(e1, e2);
        strong = true;
    }

    if (e1.is_empty())
        throw SyntaxError(N_("expression syntax error"));

    Expr::Ptr e1_copy(new Expr);
    std::swap(*e1_copy, e1);
    Insn::Operand op(m_object->get_arch()->ea_create(e1_copy));

    if (strong)
        op.get_memory()->m_strong = true;
    return op;
}

Insn::Operand
GasParser::parse_operand()
{
    switch (m_token)
    {
        case REG:
        {
            Insn::Operand op(REG_val);
            get_next_token(); // REG
            return op;
        }
        case SEGREG:
        {
            // need to see if it's really a memory address
            get_peek_token();
            if (m_peek_token == ':')
                return parse_memaddr();
            Insn::Operand op(SEGREG_val);
            get_next_token(); // SEGREG
            return op;
        }
        case REGGROUP:
        {
            const RegisterGroup* reggroup = REGGROUP_val;
            get_next_token(); // REGGROUP

            if (m_token != '(')
                return Insn::Operand(reggroup->get_reg(0));
            get_next_token(); // '('

            if (m_token != INTNUM)
                throw SyntaxError(N_("integer register index expected"));
            unsigned long regindex = INTNUM_val->get_uint();
            get_next_token(); // INTNUM

            if (m_token != ')')
            {
                throw SyntaxError(
                    N_("missing closing parenthesis for register index"));
            }
            get_next_token(); // ')'

            const Register* reg = reggroup->get_reg(regindex);
            if (!reg)
            {
                throw SyntaxError(String::compose(
                    N_("bad register index `%u'"), regindex));
            }
            return Insn::Operand(reg);
        }
        case '$':
        {
            get_next_token(); // '$'
            Expr::Ptr e(new Expr);
            if (!parse_expr(*e))
            {
                throw SyntaxError(String::compose(
                    N_("expression missing after `%1'"), "$"));
            }
            return Insn::Operand(e);
        }
        case '*':
            get_next_token(); // '*'
            if (m_token == REG)
            {
                Insn::Operand op(REG_val);
                get_next_token(); // REG
                op.make_deref();
                return op;
            }
            else
            {
                Insn::Operand op = parse_memaddr();
                op.make_deref();
                return op;
            }
        default:
            return parse_memaddr();
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
GasParser::parse_expr(Expr& e)
{
    if (!parse_expr0(e))
        return false;

    while (m_token == '+' || m_token == '-')
    {
        int op = m_token;
        get_next_token();
        Expr f;
        if (!parse_expr0(f))
            return false;

        switch (op)
        {
            case '+': e.calc(Op::ADD, f); break;
            case '-': e.calc(Op::SUB, f); break;
        }
    }
    return true;
}

bool
GasParser::parse_expr0(Expr& e)
{
    if (!parse_expr1(e))
        return false;

    while (m_token == '|' || m_token == '^' || m_token == '&' || m_token == '!')
    {
        int op = m_token;
        get_next_token();
        Expr f;
        if (!parse_expr1(f))
            return false;

        switch (op)
        {
            case '|': e.calc(Op::OR, f); break;
            case '^': e.calc(Op::XOR, f); break;
            case '&': e.calc(Op::AND, f); break;
            case '!': e.calc(Op::NOR, f); break;
        }
    }
    return true;
}

bool
GasParser::parse_expr1(Expr& e)
{
    if (!parse_expr2(e))
        return false;

    while (m_token == '*' || m_token == '/' || m_token == '%'
           || m_token == LEFT_OP || m_token == RIGHT_OP)
    {
        int op = m_token;
        get_next_token();
        Expr f;
        if (!parse_expr2(f))
            return false;

        switch (op)
        {
            case '*': e.calc(Op::MUL, f); break;
            case '/': e.calc(Op::DIV, f); break;
            case '%': e.calc(Op::MOD, f); break;
            case LEFT_OP: e.calc(Op::SHL, f); break;
            case RIGHT_OP: e.calc(Op::SHR, f); break;
        }
    }
    return true;
}

bool
GasParser::parse_expr2(Expr& e)
{
    switch (m_token)
    {
        case '+':
            get_next_token();
            return parse_expr2(e);
        case '-':
            get_next_token();
            if (!parse_expr2(e))
                return false;
            e.calc(Op::NEG);
            return true;
        case '~':
            get_next_token();
            if (!parse_expr2(e))
                return false;
            e.calc(Op::NOT);
            return true;
        case '(':
            get_next_token();
            if (!parse_expr(e))
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
                SymbolRef sym = m_object->add_non_table_symbol(".");
                Bytecode& bc = m_container->fresh_bytecode();
                Location loc = {&bc, bc.get_fixed_len()};
                sym->define_label(loc, get_cur_line());
                e = sym;
            }
            else
            {
                SymbolRef sym = m_object->get_symbol(ID_val);
                sym->use(get_cur_line());
                e = sym;
            }
            get_next_token(); // ID

            if (m_token == '@')
            {
                // TODO: this is needed for shared objects, e.g. sym@PLT
                get_next_token(); // '@'
                if (m_token != ID)
                    throw SyntaxError(N_("expected identifier after `@'"));
                SymbolRef wrt = m_object->find_special_symbol(ID_val);
                get_next_token(); // ID
                if (wrt)
                    e.calc(Op::WRT, wrt);
                else
                {
                    warn_set(WARN_GENERAL,
                             N_("unrecognized identifier after `@'"));
                }
            }
            return true;
        }
        default:
            return false;
    }

    get_next_token();
    return true;
}

void
GasParser::define_label(const std::string& name, bool local)
{
    if (!local)
        m_locallabel_base = name;

    SymbolRef sym = m_object->get_symbol(name);
    Bytecode& bc = m_container->fresh_bytecode();
    Location loc = {&bc, bc.get_fixed_len()};
    sym->define_label(loc, get_cur_line());
}

void
GasParser::define_lcomm(const std::string& name,
                        std::auto_ptr<Expr> size,
                        const Expr& align)
{
    // Put into .bss section.
    NameValues objext_namevals;
    Section& bss = get_section(".bss", objext_namevals, true);

    if (!align.is_empty())
    {
        // XXX: assume alignment is in bytes, not power-of-two
        append_align(bss, align, Expr(), Expr(), 0, get_cur_line());
    }

    // Create common symbol
    Bytecode *bc = &bss.fresh_bytecode();
    Location loc = {bc, bc->get_fixed_len()};
    m_object->get_symbol(name)->define_label(loc, get_cur_line());

    // Append gap for symbol storage
    size->simplify();
    if (size->is_intnum())
        bss.append_gap(size->get_intnum().get_uint(), get_cur_line());
    else
    {
        BytecodeContainer& multc = append_multiple(bss, size, get_cur_line());
        multc.append_gap(1, get_cur_line());
    }
}

void
GasParser::switch_section(const std::string& name,
                          NameValues& objext_nvs,
                          bool builtin)
{
    NameValues nvs;
    nvs.push_back(new NameValue(name, '\0'));
    (*m_dirs)[".section"](*m_object, nvs, objext_nvs, get_cur_line());
}

Section&
GasParser::get_section(const std::string& name,
                       NameValues& objext_nvs,
                       bool builtin)
{
    Section* cur_section = m_object->get_cur_section();
    switch_section(name, objext_nvs, builtin);
    Section& new_section = *m_object->get_cur_section();
    m_object->set_cur_section(cur_section);
    return new_section;
}

void
GasParser::do_parse()
{
    unsigned long cur_line = get_cur_line();
    std::string line;

    while (m_preproc->get_line(line))
    {
        Bytecode* bc = &m_object->get_cur_section()->fresh_bytecode();
        Location loc = {bc, bc->get_fixed_len()};

        try
        {
            m_bot = m_tok = m_ptr = m_cur = &line[0];
            m_lim = &line[line.length()+1];

            do {
                get_next_token();
                if (!is_eol())
                {
                    parse_line();
                    demand_eol();
                }
            } while (m_token != '\0');

            if (m_save_input)
                m_linemap->add_source(loc, line);
            m_errwarns->propagate(cur_line);
        }
        catch (Error& err)
        {
            m_errwarns->propagate(cur_line, err);
            demand_eol_nothrow();
            m_state = INITIAL;
        }

        cur_line = m_linemap->goto_next();
    }
}

}}} // namespace yasm::parser::gas

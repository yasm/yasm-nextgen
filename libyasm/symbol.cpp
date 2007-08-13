//
// Symbol implementation.
//
//  Copyright (C) 2001-2007  Michael Urman, Peter Johnson
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
#include "util.h"

#include <iomanip>
#include <ostream>
#include <sstream>

#include "bytecode.h"
#include "errwarn.h"
#include "expr.h"
#include "section.h"
#include "symbol.h"


namespace yasm {
#if 0
static void
objext_valparams_destroy(void *data)
{
    yasm_vps_destroy((yasm_valparamhead *)data);
}

static void
objext_valparams_print(void *data, FILE *f, int indent_level)
{
    yasm_vps_print((yasm_valparamhead *)data, f);
}

static yasm_assoc_data_callback objext_valparams_cb = {
    objext_valparams_destroy,
    objext_valparams_print
};

static void
common_size_destroy(void *data)
{
    yasm_expr **e = (yasm_expr **)data;
    yasm_expr_destroy(*e);
    yasm_xfree(data);
}

static void
common_size_print(void *data, FILE *f, int indent_level)
{
    yasm_expr **e = (yasm_expr **)data;
    yasm_expr_print(*e, f);
}

static yasm_assoc_data_callback common_size_cb = {
    common_size_destroy,
    common_size_print
};
#endif
Symbol::Symbol(const std::string& name)
    : m_name(name),
      m_type(UNKNOWN),
      m_status(NOSTATUS),
      m_visibility(LOCAL),
      m_def_line(0),
      m_decl_line(0),
      m_use_line(0),
      m_equ(0),
      m_precbc(0)
{
}

Symbol::~Symbol()
{
}

Symbol*
Symbol::use(unsigned long line)
{
    if (m_use_line == 0)
        m_use_line = line;      // set line number of first use
    m_status |= USED;
    return this;
}

void
Symbol::define(Type type, unsigned long line)
{
    // Has it been defined before (either by DEFINED or COMMON/EXTERN)?
    if (m_status & DEFINED) {
        std::ostringstream emsg, xref;
        emsg << N_("redefinition of") << " `" << m_name << "'";
        Error err(emsg.str());
        xref << "`" << m_name << "' " << N_("previously defined here");
        err.set_xref(m_def_line != 0 ? m_def_line : m_decl_line, xref.str());
        throw err;
    } else {
        if (m_visibility & EXTERN) {
            std::ostringstream wmsg;
            wmsg << "`" << m_name << "' "
                 << N_("both defined and declared extern");
            warn_set(WARN_GENERAL, wmsg.str());
        }
        m_def_line = line;      // set line number of definition
        m_type = type;
        m_status |= DEFINED;
    }
}

Symbol*
Symbol::define_equ(std::auto_ptr<Expr> e, unsigned long line)
{
    define(EQU, line);
    m_equ.reset(e.release());
    m_status |= VALUED;
    return this;
}

Symbol*
Symbol::define_label(Bytecode& precbc, unsigned long line)
{
    define(LABEL, line);
    m_precbc = &precbc;
    precbc.add_symbol(this);   /// XXX: should we add if not in table?
    return this;
}

Symbol*
Symbol::define_curpos(Bytecode& precbc, unsigned long line)
{
    define(CURPOS, line);
    m_precbc = &precbc;
    // NOT added to bytecode cross-reference table
    return this;
}

Symbol*
Symbol::define_special(Visibility vis, unsigned long line)
{
    define(SPECIAL, line);
    m_status |= VALUED;
    m_visibility = vis;
    return this;
}

Symbol*
Symbol::declare(Visibility vis, unsigned long line)
{
    // Allowable combinations:
    //  Existing State--------------  vis  New State-------------------
    //  DEFINED GLOBAL COMMON EXTERN  GCE  DEFINED GLOBAL COMMON EXTERN
    //     0      -      0      0     GCE     0      G      C      E
    //     0      -      0      1     GE      0      G      0      E
    //     0      -      1      0     GC      0      G      C      0
    // X   0      -      1      1
    //     1      -      0      0      G      1      G      0      0
    // X   1      -      -      1
    // X   1      -      1      -
    //
    if ((vis == GLOBAL) ||
        (!(m_status & DEFINED) &&
         (!(m_visibility & (COMMON | EXTERN)) ||
          ((m_visibility & COMMON) && (vis == COMMON)) ||
          ((m_visibility & EXTERN) && (vis == EXTERN))))) {
        m_decl_line = line;
        m_visibility |= vis;
    } else {
        std::ostringstream emsg, xref;
        emsg << N_("redefinition of") << " `" << m_name << "'";
        Error err(emsg.str());
        xref << "`" << m_name << "' " << N_("previously defined here");
        err.set_xref(m_def_line != 0 ? m_def_line : m_decl_line, xref.str());
        throw err;
    }
    return this;
}

void
Symbol::finalize(bool undef_extern)
{
    // error if a symbol is used but never defined or extern/common declared
    if ((m_status & USED) && !(m_status & DEFINED) &&
        !(m_visibility & (EXTERN | COMMON))) {
        if (undef_extern) {
            m_visibility |= EXTERN;
        } else {
            std::ostringstream emsg;
            emsg << N_("undefined symbol") << " `" << m_name << "' "
                 << N_("(first use)");
            throw Error(emsg.str());
        }
    }
}

const Expr*
Symbol::get_equ() const
{
    if (m_type == EQU && (m_status & VALUED))
        return m_equ.get();
    return 0;
}

bool
Symbol::get_label(Bytecode* & precbc) const
{
    if (!(m_type == LABEL || m_type == CURPOS) || !m_precbc) {
        precbc = 0;
        return false;
    }
    precbc = m_precbc;
    return true;
}

bool
Symbol::is_abs() const
{
    return (m_def_line == 0 && m_type == EQU && m_name.length() == 0);
}

bool
Symbol::is_special() const
{
    return (m_type == SPECIAL);
}

bool
Symbol::is_curpos() const
{
    return (m_type == CURPOS);
}
#if 0
void
yasm_symrec_set_objext_valparams(yasm_symrec *sym,
                                 /*@only@*/ yasm_valparamhead *objext_valparams)
{
    yasm_symrec_add_data(sym, &objext_valparams_cb, objext_valparams);
}

yasm_valparamhead *
yasm_symrec_get_objext_valparams(yasm_symrec *sym)
{
    return yasm_symrec_get_data(sym, &objext_valparams_cb);
}

void
yasm_symrec_set_common_size(yasm_symrec *sym,
                            /*@only@*/ yasm_expr *common_size)
{
    yasm_expr **ep = yasm_xmalloc(sizeof(yasm_expr *));
    *ep = common_size;
    yasm_symrec_add_data(sym, &common_size_cb, ep);
}

yasm_expr **
yasm_symrec_get_common_size(yasm_symrec *sym)
{
    return (yasm_expr **)yasm_symrec_get_data(sym, &common_size_cb);
}
#endif
void
Symbol::put(std::ostream& os, int indent_level) const
{
    os << std::setw(indent_level) << "";
    switch (m_type) {
        case UNKNOWN:
            os << "-Unknown (Common/Extern)-\n";
            break;
        case EQU:
            os << "_EQU_\n";
            os << std::setw(indent_level) << "" << "Expn=";
            if (m_status & VALUED)
                os << *m_equ;
            else
                os << "***UNVALUED***";
            os << '\n';
            break;
        case LABEL:
        case CURPOS:
            if (m_type == LABEL)
                os << "_Label_\n";
            else
                os << "_CurPos_\n";
            os << std::setw(indent_level) << "" << "Section:\n";
            m_precbc->get_section()->put(os, indent_level+1, false);
            os << std::setw(indent_level) << "" << "Preceding bytecode:\n";
            m_precbc->put(os, indent_level+1);
            break;
        case SPECIAL:
            os << "-Special-\n";
            break;
    }

    os << std::setw(indent_level) << "" << "Status=";
    if (m_status == NOSTATUS)
        os << "None\n";
    else {
        if (m_status & USED)
            os << "Used,";
        if (m_status & DEFINED)
            os << "Defined,";
        if (m_status & VALUED)
            os << "Valued,";
        os << '\n';
    }

    os << std::setw(indent_level) << "" << "Visibility=";
    if (m_visibility == LOCAL)
        os << "Local\n";
    else {
        if (m_visibility & GLOBAL)
            os << "Global,";
        if (m_visibility & COMMON)
            os << "Common,";
        if (m_visibility & EXTERN)
            os << "Extern,";
        os << '\n';
    }
    os << std::setw(indent_level) << "" << "Associated data:\n";
    put_assoc_data(os, indent_level+1);
    os << std::setw(indent_level) << "" << "Line Index (Defined)="
       << m_def_line << '\n';
    os << std::setw(indent_level) << "" << "Line Index (Declared)="
       << m_decl_line << '\n';
    os << std::setw(indent_level) << "" << "Line Index (Used)="
       << m_use_line << '\n';
}

} // namespace yasm

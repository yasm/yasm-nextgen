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
#include "symbol.h"

#include "util.h"

#include "bytecode.h"
#include "compose.h"
#include "errwarn.h"
#include "expr.h"
#include "marg_ostream.h"


namespace yasm
{

Symbol::Symbol(const std::string& name)
    : m_name(name),
      m_type(UNKNOWN),
      m_status(NOSTATUS),
      m_visibility(LOCAL),
      m_def_line(0),
      m_decl_line(0),
      m_use_line(0),
      m_equ(0)
{
}

Symbol::~Symbol()
{
}

void
Symbol::define(Type type, unsigned long line)
{
    // Has it been defined before (either by DEFINED or COMMON/EXTERN)?
    if (m_status & DEFINED)
    {
        Error err(String::compose(N_("redefinition of `%1'"), m_name));
        err.set_xref(m_def_line != 0 ? m_def_line : m_decl_line,
                     String::compose(N_("`%1' previously defined here"),
                                     m_name));
        throw err;
    }
    else
    {
        if (m_visibility & EXTERN)
            warn_set(WARN_GENERAL, String::compose(
                N_("`%1' both defined and declared extern"), m_name));
        m_def_line = line;      // set line number of definition
        m_type = type;
        m_status |= DEFINED;
    }
}

Symbol&
Symbol::define_equ(std::auto_ptr<Expr> e, unsigned long line)
{
    define(EQU, line);
    m_equ.reset(e.release());
    m_status |= VALUED;
    return *this;
}

Symbol&
Symbol::define_label(Location loc, unsigned long line)
{
    define(LABEL, line);
    m_loc = loc;
    loc.bc->add_symbol(SymbolRef(this)); /// XXX: should we add if not in table?
    return *this;
}

Symbol&
Symbol::define_special(Visibility vis, unsigned long line)
{
    define(SPECIAL, line);
    m_status |= VALUED;
    m_visibility = vis;
    return *this;
}

Symbol&
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
          ((m_visibility & EXTERN) && (vis == EXTERN)))))
    {
        m_decl_line = line;
        m_visibility |= vis;
    }
    else
    {
        Error err(String::compose(N_("redefinition of `%1'"), m_name));
        err.set_xref(m_def_line != 0 ? m_def_line : m_decl_line,
                     String::compose(N_("`%1' previously defined here"),
                                     m_name));
        throw err;
    }
    return *this;
}

void
Symbol::finalize(bool undef_extern)
{
    // error if a symbol is used but never defined or extern/common declared
    if ((m_status & USED) && !(m_status & DEFINED) &&
        !(m_visibility & (EXTERN | COMMON)))
    {
        if (undef_extern)
            m_visibility |= EXTERN;
        else
            throw Error(String::compose(N_("undefined symbol `%1' (first use)"),
                                        m_name));
    }
}

bool
Symbol::get_label(Location* loc) const
{
    if (m_type != LABEL)
        return false;
    *loc = m_loc;
    return true;
}

marg_ostream&
operator<< (marg_ostream& os, const Symbol& sym)
{
    switch (sym.m_type)
    {
        case Symbol::UNKNOWN:
            os << "-Unknown (Common/Extern)-\n";
            break;
        case Symbol::EQU:
            os << "_EQU_\n";
            os << "Expn=";
            if (sym.m_status & Symbol::VALUED)
                os << *sym.m_equ;
            else
                os << "***UNVALUED***";
            os << '\n';
            break;
        case Symbol::LABEL:
            os << "_Label_\n";
            //os << std::setw(indent_level) << "" << "Section:\n";
            //m_precbc->get_section()->put(os, indent_level+1, false);
            //os << std::setw(indent_level) << "" << "Preceding bytecode:\n";
            //m_precbc->put(os, indent_level+1);
            break;
        case Symbol::SPECIAL:
            os << "-Special-\n";
            break;
    }

    os << "Status=";
    if (sym.m_status == Symbol::NOSTATUS)
        os << "None\n";
    else
    {
        if (sym.m_status & Symbol::USED)
            os << "Used,";
        if (sym.m_status & Symbol::DEFINED)
            os << "Defined,";
        if (sym.m_status & Symbol::VALUED)
            os << "Valued,";
        os << '\n';
    }

    os << "Visibility=";
    if (sym.m_visibility == Symbol::LOCAL)
        os << "Local\n";
    else
    {
        if (sym.m_visibility & Symbol::GLOBAL)
            os << "Global,";
        if (sym.m_visibility & Symbol::COMMON)
            os << "Common,";
        if (sym.m_visibility & Symbol::EXTERN)
            os << "Extern,";
        os << '\n';
    }
    os << "Associated data:\n";
    ++os;
    os << static_cast<const AssocDataContainer&>(sym);
    --os;
    os << "Line Index (Defined)=" << sym.m_def_line << '\n';
    os << "Line Index (Declared)=" << sym.m_decl_line << '\n';
    os << "Line Index (Used)=" << sym.m_use_line << '\n';
    return os;
}

} // namespace yasm

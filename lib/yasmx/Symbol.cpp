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
#include "yasmx/Symbol.h"

#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Expr.h"


using namespace yasm;

Symbol::Symbol(StringRef name)
    : m_name(name),
      m_type(UNKNOWN),
      m_status(NOSTATUS),
      m_visibility(LOCAL),
      m_equ(0)
{
}

Symbol::~Symbol()
{
}

bool
Symbol::DefineCheck(SourceLocation source, DiagnosticsEngine& diags) const
{
    assert(source.isValid() && "invalid source location");
    // Has it been defined before (either by DEFINED or COMMON/EXTERN)?
    if (isDefined())
    {
        diags.Report(source, diag::err_symbol_redefined) << m_name;
        diags.Report(m_def_source.isValid() ? m_def_source : m_decl_source,
                     diag::note_previous_definition);
        return false;
    }

    if (m_visibility & EXTERN)
    {
        diags.Report(source, diag::warn_extern_defined) << m_name;
        if (m_decl_source.isValid())
        {
            diags.Report(m_decl_source, diag::note_extern_declaration)
                << m_name;
        }
    }
    return true;
}

void
Symbol::DefineEqu(const Expr& e)
{
    assert(!isDefined() && "symbol already defined");
    m_type = EQU;
    m_status |= DEFINED | VALUED;
    m_equ.reset(new Expr(e));
}

void
Symbol::CheckedDefineEqu(const Expr& e,
                         SourceLocation source,
                         DiagnosticsEngine& diags)
{
    if (!DefineCheck(source, diags))
        return;
    DefineEqu(e);
    m_def_source = source;
}

void
Symbol::DefineLabel(Location loc)
{
    assert(!isDefined() && "symbol already defined");
    m_type = LABEL;
    m_status |= DEFINED;
    m_loc = loc;
}

void
Symbol::CheckedDefineLabel(Location loc,
                           SourceLocation source,
                           DiagnosticsEngine& diags)
{
    if (!DefineCheck(source, diags))
        return;
    DefineLabel(loc);
    m_def_source = source;
}

void
Symbol::DefineSpecial(Visibility vis)
{
    assert(!isDefined() && "symbol already defined");
    m_type = SPECIAL;
    m_status |= VALUED | DEFINED;
    m_visibility = vis;
}

bool
Symbol::okToDeclare(Visibility vis) const
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

    // It's always okay to declare global.
    if (vis == GLOBAL)
        return true;

    // Can't declare a defined symbol anything but global.
    if (isDefined())
        return false;

    // Allow redundant common and extern declarations
    if (m_visibility & COMMON)
        return vis == COMMON;
    if (m_visibility & EXTERN)
        return vis == EXTERN;
    return true;
}

void
Symbol::Declare(Visibility vis)
{
    assert(okToDeclare(vis) && "symbol already defined");
    m_visibility |= vis;
}

void
Symbol::CheckedDeclare(Visibility vis,
                       SourceLocation source,
                       DiagnosticsEngine& diags)
{
    assert(source.isValid() && "invalid source location");
    if (okToDeclare(vis))
    {
        m_decl_source = source;
        m_visibility |= vis;
    }
    else
    {
        diags.Report(source, diag::err_symbol_redefined) << m_name;
        diags.Report(m_def_source.isValid() ? m_def_source : m_decl_source,
                     diag::note_previous_definition);
    }
}

bool
Symbol::getLabel(Location* loc) const
{
    if (m_type != LABEL)
        return false;
    *loc = m_loc;
    return true;
}

#ifdef WITH_XML
pugi::xml_node
Symbol::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("Symbol");
    root.append_attribute("id") = m_name.c_str();
    append_child(root, "Name", m_name);
    pugi::xml_attribute type = root.append_attribute("type");
    switch (m_type)
    {
        case EQU:
        {
            type = "EQU";
            if (m_status & VALUED)
                append_data(root, *m_equ);
            break;
        }
        case LABEL:
            type = "Label";
            append_data(root, m_loc);
            break;
        case SPECIAL:
            type = "Special";
            break;
        case UNKNOWN:
            type = "Unknown (Common/Extern)";
            break;
        default:
            break;
    }

    if (m_status & USED)
        root.append_child("Used").append_attribute("source") =
            m_use_source.getRawEncoding();
    if (m_status & DEFINED)
        root.append_child("Defined").append_attribute("source") =
            m_def_source.getRawEncoding();
    if (m_status & VALUED)
        root.append_attribute("Valued") = true;

    if (m_visibility & GLOBAL)
        root.append_child("Global").append_attribute("source") =
            m_decl_source.getRawEncoding();
    if (m_visibility & COMMON)
        root.append_child("Common").append_attribute("source") =
            m_decl_source.getRawEncoding();
    if (m_visibility & EXTERN)
        root.append_child("Extern").append_attribute("source") =
            m_decl_source.getRawEncoding();
    if (m_visibility & DLOCAL)
        root.append_child("DLocal").append_attribute("source") =
            m_decl_source.getRawEncoding();

    AssocDataContainer::Write(root);
    return root;
}
#endif // WITH_XML

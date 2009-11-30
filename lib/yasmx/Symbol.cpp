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

#include "util.h"

#include "llvm/Support/raw_ostream.h"
#include "YAML/emitter.h"
#include "yasmx/Support/Compose.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Expr.h"


namespace yasm
{

Symbol::Symbol(llvm::StringRef name)
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

void
Symbol::Define(Type type, clang::SourceLocation source)
{
    // Has it been defined before (either by DEFINED or COMMON/EXTERN)?
    if (m_status & DEFINED)
    {
        Error err(String::Compose(N_("redefinition of `%1'"), m_name));
        err.setXRef(m_def_source.isValid() ? m_def_source : m_decl_source,
                    String::Compose(N_("`%1' previously defined here"),
                                    m_name));
        throw err;
    }
    else
    {
        if (m_visibility & EXTERN)
            setWarn(WARN_GENERAL, String::Compose(
                N_("`%1' both defined and declared extern"), m_name));
        m_def_source = source;  // set source location of definition
        m_type = type;
        m_status |= DEFINED;
    }
}

void
Symbol::DefineEqu(const Expr& e, clang::SourceLocation source)
{
    Define(EQU, source);
    m_equ.reset(new Expr(e));
    m_status |= VALUED;
}

void
Symbol::DefineLabel(Location loc, clang::SourceLocation source)
{
    Define(LABEL, source);
    m_loc = loc;
    loc.bc->AddSymbol(SymbolRef(this)); /// XXX: should we add if not in table?
}

void
Symbol::DefineSpecial(Visibility vis, clang::SourceLocation source)
{
    Define(SPECIAL, source);
    m_status |= VALUED;
    m_visibility = vis;
}

void
Symbol::Declare(Visibility vis, clang::SourceLocation source)
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
        m_decl_source = source;
        m_visibility |= vis;
    }
    else
    {
        Error err(String::Compose(N_("redefinition of `%1'"), m_name));
        err.setXRef(m_def_source.isValid() ? m_def_source : m_decl_source,
                    String::Compose(N_("`%1' previously defined here"),
                                    m_name));
        throw err;
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

void
Symbol::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << m_name;
    out << YAML::Key << "type" << YAML::Value;
    switch (m_type)
    {
        case EQU:
            out << "EQU";
            out << YAML::Key << "value" << YAML::Value;
            if (m_status & VALUED)
                out << *m_equ;
            else
                out << YAML::Null;
            break;
        case LABEL:
            out << "Label";
            out << YAML::Key << "loc" << YAML::Value << m_loc;
            break;
        case SPECIAL:
            out << "Special";
            break;
        case UNKNOWN:
            out << "Unknown (Common/Extern)";
            break;
        default:
            out << YAML::Null;
            break;
    }

    out << YAML::Key << "status" << YAML::Value << YAML::Flow << YAML::BeginSeq;
    if (m_status & USED)
        out << "Used";
    if (m_status & DEFINED)
        out << "Defined";
    if (m_status & VALUED)
        out << "Valued";
    out << YAML::EndSeq;

    out << YAML::Key << "visibility";
    out << YAML::Value << YAML::Flow << YAML::BeginSeq;
    if (m_visibility & GLOBAL)
        out << "Global";
    if (m_visibility & COMMON)
        out << "Common";
    if (m_visibility & EXTERN)
        out << "Extern";
    if (m_visibility & DLOCAL)
        out << "DLocal";
    out << YAML::EndSeq;

    out << YAML::Key << "define source"
        << YAML::Value << m_def_source.getRawEncoding();
    out << YAML::Key << "declare source"
        << YAML::Value << m_decl_source.getRawEncoding();
    out << YAML::Key << "use source"
        << YAML::Value << m_use_source.getRawEncoding();

    out << YAML::Key << "assoc data" << YAML::Value;
    AssocDataContainer::Write(out);
    out << YAML::EndMap;
}

void
Symbol::Dump() const
{
    YAML::Emitter out;
    Write(out);
    llvm::errs() << out.c_str() << '\n';
}

} // namespace yasm

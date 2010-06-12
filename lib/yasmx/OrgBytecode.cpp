///
/// ORG bytecode
///
///  Copyright (C) 2005-2007  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
/// 1. Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
/// 2. Redistributions in binary form must reproduce the above copyright
///    notice, this list of conditions and the following disclaimer in the
///    documentation and/or other materials provided with the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
/// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
///
#include "yasmx/BytecodeContainer.h"

#include "util.h"

#include "YAML/emitter.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Diagnostic.h"


using namespace yasm;

namespace {
class OrgBytecode : public Bytecode::Contents
{
public:
    OrgBytecode(unsigned long start, unsigned long fill);
    ~OrgBytecode();

    /// Finalizes the bytecode after parsing.
    bool Finalize(Bytecode& bc, Diagnostic& diags);

    /// Calculates the minimum size of a bytecode.
    bool CalcLen(Bytecode& bc,
                 /*@out@*/ unsigned long* len,
                 const Bytecode::AddSpanFunc& add_span,
                 Diagnostic& diags);

    /// Recalculates the bytecode's length based on an expanded span
    /// length.
    bool Expand(Bytecode& bc,
                unsigned long* len,
                int span,
                long old_val,
                long new_val,
                bool* keep,
                /*@out@*/ long* neg_thres,
                /*@out@*/ long* pos_thres,
                Diagnostic& diags);

    /// Convert a bytecode into its byte representation.
    bool Output(Bytecode& bc, BytecodeOutput& bc_out);

    llvm::StringRef getType() const;

    SpecialType getSpecial() const;

    OrgBytecode* clone() const;

    /// Write a YAML representation.  For debugging purposes.
    void Write(YAML::Emitter& out) const;

private:
    unsigned long m_start;      ///< target starting offset within section
    unsigned long m_fill;       ///< fill value
};
} // anonymous namespace

OrgBytecode::OrgBytecode(unsigned long start, unsigned long fill)
    : m_start(start),
      m_fill(fill)
{
}

OrgBytecode::~OrgBytecode()
{
}

bool
OrgBytecode::Finalize(Bytecode& bc, Diagnostic& diags)
{
    return true;
}

bool
OrgBytecode::CalcLen(Bytecode& bc,
                     /*@out@*/ unsigned long* len,
                     const Bytecode::AddSpanFunc& add_span,
                     Diagnostic& diags)
{
    bool keep = false;
    long neg_thres = 0;
    long pos_thres = m_start;

    *len = 0;
    return Expand(bc, len, 0, 0, static_cast<long>(bc.getTailOffset()), &keep,
                  &neg_thres, &pos_thres, diags);
}

bool
OrgBytecode::Expand(Bytecode& bc,
                    unsigned long* len,
                    int span,
                    long old_val,
                    long new_val,
                    bool* keep,
                    /*@out@*/ long* neg_thres,
                    /*@out@*/ long* pos_thres,
                    Diagnostic& diags)
{
    // Check for overrun
    if (static_cast<unsigned long>(new_val) > m_start)
    {
        diags.Report(bc.getSource(), diag::err_org_overlap);
        return false;
    }

    // Generate space to start offset
    *len = m_start - new_val;
    *keep = true;
    return true;
}

bool
OrgBytecode::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    // Sanity check for overrun
    if (bc.getTailOffset() > m_start)
    {
        bc_out.Diag(bc.getSource(), diag::err_org_overlap);
        return false;
    }

    unsigned long len = m_start - bc.getTailOffset();
    if (!bc_out.isBits())
    {
        bc_out.OutputGap(len, bc.getSource());
        return true;
    }

    Bytes& bytes = bc_out.getScratch();
    // XXX: handle more than 8 bit?
    bytes.insert(bytes.end(), len, static_cast<unsigned char>(m_fill));
    bc_out.OutputBytes(bytes, bc.getSource());
    return true;
}

llvm::StringRef
OrgBytecode::getType() const
{
    return "yasm::OrgBytecode";
}

OrgBytecode::SpecialType
OrgBytecode::getSpecial() const
{
    return SPECIAL_OFFSET;
}

OrgBytecode*
OrgBytecode::clone() const
{
    return new OrgBytecode(m_start, m_fill);
}

void
OrgBytecode::Write(YAML::Emitter& out) const
{
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << "Org";
    out << YAML::Key << "start" << YAML::Value << m_start;
    out << YAML::Key << "fill" << YAML::Value << m_fill;
    out << YAML::EndMap;
}

void
yasm::AppendOrg(BytecodeContainer& container,
                unsigned long start,
                unsigned long fill,
                clang::SourceLocation source)
{
    Bytecode& bc = container.FreshBytecode();
    bc.Transform(Bytecode::Contents::Ptr(new OrgBytecode(start, fill)));
    bc.setSource(source);
}

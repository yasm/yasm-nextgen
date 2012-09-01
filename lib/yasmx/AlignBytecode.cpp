///
/// Align bytecode
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

#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Support/scoped_ptr.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Expr.h"
#include "yasmx/Expr_util.h"
#include "yasmx/IntNum.h"


using namespace yasm;

namespace {
class AlignBytecode : public Bytecode::Contents
{
public:
    AlignBytecode(const Expr& boundary,
                  const Expr& fill,
                  const Expr& maxskip,
                  /*@null@*/ const unsigned char** code_fill);
    ~AlignBytecode();

    /// Finalizes the bytecode after parsing.
    bool Finalize(Bytecode& bc, DiagnosticsEngine& diags);

    /// Calculates the minimum size of a bytecode.
    bool CalcLen(Bytecode& bc,
                 /*@out@*/ unsigned long* len,
                 const Bytecode::AddSpanFunc& add_span,
                 DiagnosticsEngine& diags);

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
                DiagnosticsEngine& diags);

    /// Convert a bytecode into its byte representation.
    bool Output(Bytecode& bc, BytecodeOutput& bc_out);

    StringRef getType() const;

    SpecialType getSpecial() const;

    AlignBytecode* clone() const;

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

private:
    Expr m_boundary;    ///< alignment boundary

    /// What to fill intervening locations with, empty if using code_fill
    Expr m_fill;

    /// Maximum number of bytes to skip, empty if no maximum.
    Expr m_maxskip;

    /// Code fill, NULL if using 0 fill
    /*@null@*/ const unsigned char** m_code_fill;
};
} // anonymous namespace

AlignBytecode::AlignBytecode(const Expr& boundary,
                             const Expr& fill,
                             const Expr& maxskip,
                             /*@null@*/ const unsigned char** code_fill)
    : m_boundary(boundary),
      m_fill(fill),
      m_maxskip(maxskip),
      m_code_fill(code_fill)
{
}

AlignBytecode::~AlignBytecode()
{
}

bool
AlignBytecode::Finalize(Bytecode& bc, DiagnosticsEngine& diags)
{
    if (!ExpandEqu(m_boundary))
    {
        diags.Report(bc.getSource(), diag::err_equ_circular_reference);
        return false;
    }
    m_boundary.Simplify(diags, false);
    if (!m_boundary.isIntNum())
    {
        diags.Report(bc.getSource(), diag::err_align_boundary_not_const);
        return false;
    }

    if (!m_fill.isEmpty())
    {
        if (!ExpandEqu(m_fill))
        {
            diags.Report(bc.getSource(), diag::err_equ_circular_reference);
            return false;
        }
        m_fill.Simplify(diags, false);
        if (!m_fill.isIntNum())
        {
            diags.Report(bc.getSource(), diag::err_align_fill_not_const);
            return false;
        }
    }

    if (!m_maxskip.isEmpty())
    {
        if (!ExpandEqu(m_maxskip))
        {
            diags.Report(bc.getSource(), diag::err_equ_circular_reference);
            return false;
        }
        m_maxskip.Simplify(diags, false);
        if (!m_maxskip.isIntNum())
        {
            diags.Report(bc.getSource(), diag::err_align_skip_not_const);
            return false;
        }
    }
    return true;
}

bool
AlignBytecode::CalcLen(Bytecode& bc,
                       /*@out@*/ unsigned long* len,
                       const Bytecode::AddSpanFunc& add_span,
                       DiagnosticsEngine& diags)
{
    bool keep = false;
    long neg_thres = 0;
    long pos_thres = 0;

    *len = 0;
    return Expand(bc, len, 0, 0, static_cast<long>(bc.getTailOffset()),
                  &keep, &neg_thres, &pos_thres, diags);
}

bool
AlignBytecode::Expand(Bytecode& bc,
                      unsigned long* len,
                      int span,
                      long old_val,
                      long new_val,
                      bool* keep,
                      /*@out@*/ long* neg_thres,
                      /*@out@*/ long* pos_thres,
                      DiagnosticsEngine& diags)
{
    unsigned long boundary = m_boundary.getIntNum().getUInt();

    if (boundary == 0)
    {
        *len = 0;
        *pos_thres = new_val;
        *keep = false;
        return true;
    }

    unsigned long end = static_cast<unsigned long>(new_val);
    if (end & (boundary-1))
        end = (end & ~(boundary-1)) + boundary;

    *pos_thres = static_cast<long>(end);
    *len = end - static_cast<unsigned long>(new_val);

    if (!m_maxskip.isEmpty())
    {
        unsigned long maxskip = m_maxskip.getIntNum().getUInt();
        if (*len > maxskip)
        {
            *pos_thres = static_cast<long>(end-maxskip)-1;
            *len = 0;
        }
    }
    *keep = true;
    return true;
}

bool
AlignBytecode::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    unsigned long len;
    unsigned long boundary = m_boundary.getIntNum().getUInt();
    Bytes& bytes = bc_out.getScratch();

    if (boundary == 0)
        return true;
    else
    {
        unsigned long tail = bc.getTailOffset();
        unsigned long end = tail;
        if (tail & (boundary-1))
            end = (tail & ~(boundary-1)) + boundary;
        len = end - tail;
        if (len == 0)
            return true;
        if (!m_maxskip.isEmpty())
        {
            unsigned long maxskip = m_maxskip.getIntNum().getUInt();
            if (len > maxskip)
                return true;
        }
    }

    if (!bc_out.isBits())
    {
        // Output as a gap.
        bc_out.OutputGap(len, bc.getSource());
        return true;
    }
    if (!m_fill.isEmpty())
    {
        unsigned long v = m_fill.getIntNum().getUInt();
        bytes.insert(bytes.end(), len, static_cast<unsigned char>(v));
    }
    else if (m_code_fill)
    {
        unsigned long maxlen = 15;
        while (!m_code_fill[maxlen] && maxlen>0)
            maxlen--;
        if (maxlen == 0)
        {
            bc_out.Diag(bc.getSource(), diag::err_align_code_not_found);
            return false;
        }

        // Fill with maximum code fill as much as possible
        while (len > maxlen)
        {
            bytes.insert(bytes.end(),
                         &m_code_fill[maxlen][0],
                         &m_code_fill[maxlen][maxlen]);
            len -= maxlen;
        }

        if (!m_code_fill[len])
        {
            bc_out.Diag(bc.getSource(), diag::err_align_invalid_code_size)
                << static_cast<unsigned int>(len);
            return false;
        }
        // Handle rest of code fill
        bytes.insert(bytes.end(),
                     &m_code_fill[len][0],
                     &m_code_fill[len][len]);
    }
    else
    {
        // Just fill with 0
        bytes.insert(bytes.end(), len, 0);
    }
    bc_out.OutputBytes(bytes, bc.getSource());
    return true;
}

StringRef
AlignBytecode::getType() const
{
    return "yasm::AlignBytecode";
}

AlignBytecode::SpecialType
AlignBytecode::getSpecial() const
{
    return SPECIAL_OFFSET;
}

AlignBytecode*
AlignBytecode::clone() const
{
    return new AlignBytecode(m_boundary, m_fill, m_maxskip, m_code_fill);
}

#ifdef WITH_XML
pugi::xml_node
AlignBytecode::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("Align");
    append_child(root, "Boundary", m_boundary);
    append_child(root, "Fill", m_fill);
    append_child(root, "MaxSkip", m_maxskip);
    if (m_code_fill != 0)
        root.append_attribute("code") = true;
    return root;
}
#endif // WITH_XML

void
yasm::AppendAlign(BytecodeContainer& container,
                  const Expr& boundary,
                  const Expr& fill,
                  const Expr& maxskip,
                  /*@null@*/ const unsigned char** code_fill,
                  SourceLocation source)
{
    Bytecode& bc = container.FreshBytecode();
    bc.Transform(Bytecode::Contents::Ptr(
        new AlignBytecode(boundary, fill, maxskip, code_fill)));
    bc.setSource(source);
}

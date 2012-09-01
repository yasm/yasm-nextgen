///
/// Incbin bytecode implementation
///
///  Copyright (C) 2001-2007  Peter Johnson
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

#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/system_error.h"
#include "yasmx/Basic/Diagnostic.h"
#include "yasmx/Support/scoped_ptr.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/Value.h"


using namespace yasm;

namespace {
class IncbinBytecode : public Bytecode::Contents
{
public:
    IncbinBytecode(StringRef filename,
                   std::auto_ptr<Expr> start,
                   std::auto_ptr<Expr> maxlen);
    ~IncbinBytecode();

    /// Finalizes the bytecode after parsing.
    bool Finalize(Bytecode& bc, DiagnosticsEngine& diags);

    /// Calculates the minimum size of a bytecode.
    bool CalcLen(Bytecode& bc,
                 /*@out@*/ unsigned long* len,
                 const Bytecode::AddSpanFunc& add_span,
                 DiagnosticsEngine& diags);

    /// Convert a bytecode into its byte representation.
    bool Output(Bytecode& bc, BytecodeOutput& bc_out);

    StringRef getType() const;

    IncbinBytecode* clone() const;

#ifdef WITH_XML
    /// Write an XML representation.  For debugging purposes.
    pugi::xml_node Write(pugi::xml_node out) const;
#endif // WITH_XML

private:
    std::string m_filename;     ///< file to include data from

    OwningPtr<MemoryBuffer> m_buf;  ///< Buffer for file data

    /// starting offset to read from (NULL=0)
    /*@null@*/ util::scoped_ptr<Expr> m_start;

    /// maximum number of bytes to read (NULL=no limit)
    /*@null@*/ util::scoped_ptr<Expr> m_maxlen;
};
} // anonymous namespace

IncbinBytecode::IncbinBytecode(StringRef filename,
                               std::auto_ptr<Expr> start,
                               std::auto_ptr<Expr> maxlen)
    : m_filename(filename),
      m_start(start.release()),
      m_maxlen(maxlen.release())
{
}

IncbinBytecode::~IncbinBytecode()
{
}

bool
IncbinBytecode::Finalize(Bytecode& bc, DiagnosticsEngine& diags)
{
    if (llvm::error_code err = MemoryBuffer::getFile(m_filename, m_buf))
    {
        diags.Report(bc.getSource(), diag::err_file_read) << m_filename
            << err.message();
        return false;
    }

    if (m_start)
    {
        Value val(0, Expr::Ptr(m_start->clone()));
        if (!val.Finalize(diags, diag::err_incbin_start_too_complex))
            return false;
        if (val.isRelative())
        {
            diags.Report(bc.getSource(), diag::err_incbin_start_not_absolute);
            return false;
        }
        m_start.reset(val.getAbs()->clone());
    }

    if (m_maxlen)
    {
        Value val(0, Expr::Ptr(m_maxlen->clone()));
        if (!val.Finalize(diags, diag::err_incbin_maxlen_too_complex))
            return false;
        if (val.isRelative())
        {
            diags.Report(bc.getSource(), diag::err_incbin_maxlen_not_absolute);
            return false;
        }
        m_maxlen.reset(val.getAbs()->clone());
    }
    return true;
}

bool
IncbinBytecode::CalcLen(Bytecode& bc,
                        /*@out@*/ unsigned long* len,
                        const Bytecode::AddSpanFunc& add_span,
                        DiagnosticsEngine& diags)
{
    unsigned long start = 0, maxlen = 0xFFFFFFFFUL;

    // Try to convert start to integer value
    if (m_start)
    {
        if (m_start->isIntNum())
            start = m_start->getIntNum().getUInt();
        else
        {
            // FIXME
            diags.Report(bc.getSource(), diag::err_incbin_start_not_const);
            return false;
        }
    }

    // Try to convert maxlen to integer value
    if (m_maxlen)
    {
        if (m_maxlen->isIntNum())
            maxlen = m_maxlen->getIntNum().getUInt();
        else
        {
            // FIXME
            diags.Report(bc.getSource(), diag::err_incbin_maxlen_not_const);
            return false;
        }
    }

    // Compute length of incbin from start, maxlen, and len
    unsigned long flen = m_buf->getBufferSize();
    if (start > flen)
    {
        diags.Report(bc.getSource(), diag::warn_incbin_start_after_eof);
        start = flen;
    }
    flen -= start;
    if (m_maxlen)
    {
        if (maxlen < flen)
            flen = maxlen;
    }
    *len = flen;
    return true;
}

bool
IncbinBytecode::Output(Bytecode& bc, BytecodeOutput& bc_out)
{
    unsigned long start = 0;

    // Convert start to integer value
    if (m_start)
    {
        assert(m_start->isIntNum()
               && "could not determine start in incbin::output");
        start = m_start->getIntNum().getUInt();
    }

    // Copy len bytes
    Bytes& bytes = bc_out.getScratch();
    bytes.WriteString(m_buf->getBuffer().substr(start, bc.getTailLen()));
    bc_out.OutputBytes(bytes, bc.getSource());
    return true;
}

StringRef
IncbinBytecode::getType() const
{
    return "yasm::IncbinBytecode";
}

IncbinBytecode*
IncbinBytecode::clone() const
{
    return new IncbinBytecode(m_filename,
                              std::auto_ptr<Expr>(m_start->clone()),
                              std::auto_ptr<Expr>(m_maxlen->clone()));
}

#ifdef WITH_XML
pugi::xml_node
IncbinBytecode::Write(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("Incbin");
    append_child(root, "Filename", m_filename);
    if (m_start)
        append_child(root, "Start", *m_start);
    if (m_maxlen)
        append_child(root, "MaxLen", *m_maxlen);
    return root;
}
#endif // WITH_XML

void
yasm::AppendIncbin(BytecodeContainer& container,
                   StringRef filename,
                   /*@null@*/ std::auto_ptr<Expr> start,
                   /*@null@*/ std::auto_ptr<Expr> maxlen,
                   SourceLocation source)
{
    Bytecode& bc = container.FreshBytecode();
    bc.Transform(Bytecode::Contents::Ptr(
        new IncbinBytecode(filename, start, maxlen)));
    bc.setSource(source);
}

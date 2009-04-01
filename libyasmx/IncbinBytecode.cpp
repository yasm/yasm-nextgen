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
#include "util.h"

#include <fstream>

#include "yasmx/Support/Compose.h"
#include "yasmx/Support/errwarn.h"
#include "yasmx/Support/marg_ostream.h"
#include "yasmx/Support/scoped_ptr.h"
#include "yasmx/System/file.h"

#include "yasmx/BytecodeContainer.h"
#include "yasmx/BytecodeContainer_util.h"
#include "yasmx/BytecodeOutput.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"
#include "yasmx/Linemap.h"
#include "yasmx/Value.h"


namespace
{

using namespace yasm;

class IncbinBytecode : public Bytecode::Contents
{
public:
    IncbinBytecode(const std::string& filename,
                   std::auto_ptr<Expr> start,
                   std::auto_ptr<Expr> maxlen);
    ~IncbinBytecode();

    /// Prints the implementation-specific data (for debugging purposes).
    void put(marg_ostream& os) const;

    /// Finalizes the bytecode after parsing.
    void finalize(Bytecode& bc);

    /// Calculates the minimum size of a bytecode.
    unsigned long calc_len(Bytecode& bc, const Bytecode::AddSpanFunc& add_span);

    /// Convert a bytecode into its byte representation.
    void output(Bytecode& bc, BytecodeOutput& bc_out);

    IncbinBytecode* clone() const;

private:
    std::string m_filename;     ///< file to include data from

    /// starting offset to read from (NULL=0)
    /*@null@*/ util::scoped_ptr<Expr> m_start;

    /// maximum number of bytes to read (NULL=no limit)
    /*@null@*/ util::scoped_ptr<Expr> m_maxlen;
};

IncbinBytecode::IncbinBytecode(const std::string& filename,
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

void
IncbinBytecode::put(marg_ostream& os) const
{
    os << "_IncBin_\n";
    os << "Filename=`" << m_filename << "'\n";
    os << "Start=";
    if (!m_start)
        os << "nil (0)";
    else
        os << *m_start;
    os << "\nMax Len=";
    if (!m_maxlen)
        os << "nil (unlimited)";
    else
        os << *m_maxlen;
    os << '\n';
}

void
IncbinBytecode::finalize(Bytecode& bc)
{
    if (m_start)
    {
        Value val(0, Expr::Ptr(m_start->clone()));
        if (!val.finalize())
            throw TooComplexError(N_("start expression too complex"));
        else if (val.is_relative())
            throw NotAbsoluteError(N_("start expression not absolute"));
        m_start.reset(val.get_abs()->clone());
    }

    if (m_maxlen)
    {
        Value val(0, Expr::Ptr(m_maxlen->clone()));
        if (!val.finalize())
            throw TooComplexError(N_("maximum length expression too complex"));
        else if (val.is_relative())
            throw NotAbsoluteError(
                N_("maximum length expression not absolute"));
        m_maxlen.reset(val.get_abs()->clone());
    }
}

unsigned long
IncbinBytecode::calc_len(Bytecode& bc, const Bytecode::AddSpanFunc& add_span)
{
    unsigned long start = 0, maxlen = 0xFFFFFFFFUL, flen;

    // Try to convert start to integer value
    if (m_start)
    {
        const IntNum* num = m_start->get_intnum();
        if (num)
            start = num->get_uint();
        else
        {
            // FIXME
            throw NotImplementedError(
                N_("incbin does not yet understand non-constant"));
        }
    }

    // Try to convert maxlen to integer value
    if (m_maxlen)
    {
        const IntNum* num = m_maxlen->get_intnum();
        if (num)
            maxlen = num->get_uint();
        else
        {
            // FIXME
            throw NotImplementedError(
                N_("incbin does not yet understand non-constant"));
        }
    }

    // Open file and determine its length
    std::ifstream ifs(m_filename.c_str(),
                      std::ifstream::in | std::ifstream::binary);
    if (!ifs)
        throw IOError(String::compose(N_("`%1': unable to open file `%2'"),
                                      "incbin", m_filename));
    ifs.seekg(0, std::ios_base::end);
    if (!ifs)
        throw IOError(String::compose(N_("`%1': unable to seek on file `%2'"),
                                      "incbin", m_filename));
    flen = static_cast<unsigned long>(ifs.tellg());

    // Compute length of incbin from start, maxlen, and len
    if (start > flen)
    {
        warn_set(WARN_GENERAL,
                 String::compose(N_("`%1': start past end of file `%2'"),
                                 "incbin", m_filename));
        start = flen;
    }
    flen -= start;
    if (m_maxlen)
    {
        if (maxlen < flen)
            flen = maxlen;
    }
    return flen;
}

void
IncbinBytecode::output(Bytecode& bc, BytecodeOutput& bc_out)
{
    unsigned long start = 0;

    // Convert start to integer value
    if (m_start)
    {
        const IntNum* num = m_start->get_intnum();
        assert(num && "could not determine start in incbin::output");
        start = num->get_uint();
    }

    // Open file
    std::ifstream ifs(m_filename.c_str(),
                      std::ifstream::in | std::ifstream::binary);
    if (!ifs)
        throw IOError(String::compose(N_("`%1': unable to open file `%2'"),
                                      "incbin", m_filename));

    // Seek to start of data
    ifs.seekg(start, std::ios_base::beg);
    if (!ifs)
        throw IOError(String::compose(N_("`%1': unable to seek on file `%2'"),
                                      "incbin", m_filename));

    // Read len bytes
    Bytes& bytes = bc_out.get_scratch();
    bytes.write(ifs, bc.get_tail_len());
    if (!ifs)
        throw IOError(String::compose(
            N_("`%1': unable to read %2 bytes from file `%3'"),
            "incbin", bc.get_tail_len(), m_filename));
    bc_out.output(bytes);
}

IncbinBytecode*
IncbinBytecode::clone() const
{
    return new IncbinBytecode(m_filename,
                              std::auto_ptr<Expr>(m_start->clone()),
                              std::auto_ptr<Expr>(m_maxlen->clone()));
}

} // anonymous namespace

namespace yasm
{

void
append_incbin(BytecodeContainer& container,
              const std::string& filename,
              /*@null@*/ std::auto_ptr<Expr> start,
              /*@null@*/ std::auto_ptr<Expr> maxlen,
              unsigned long line)
{
    Bytecode& bc = container.fresh_bytecode();
    bc.transform(Bytecode::Contents::Ptr(
        new IncbinBytecode(filename, start, maxlen)));
    bc.set_line(line);
}

} // namespace yasm

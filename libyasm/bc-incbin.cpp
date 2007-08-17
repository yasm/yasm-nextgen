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

#include <iomanip>
#include <fstream>
#include <ostream>
#include <sstream>

#include <boost/scoped_ptr.hpp>

#include "bytecode.h"
#include "bytes.h"
#include "errwarn.h"
#include "expr.h"
#include "file.h"
#include "intnum.h"
#include "linemap.h"
#include "value.h"


namespace {

using namespace yasm;

class IncbinBytecode : public Bytecode::Contents {
public:
    IncbinBytecode(const std::string& filename, const std::string& from,
                   std::auto_ptr<Expr> start, std::auto_ptr<Expr> maxlen);
    ~IncbinBytecode();

    /// Prints the implementation-specific data (for debugging purposes).
    void put(std::ostream& os, int indent_level) const;

    /// Finalizes the bytecode after parsing.
    void finalize(Bytecode& bc, Bytecode& prev_bc);

    /// Calculates the minimum size of a bytecode.
    unsigned long calc_len(Bytecode& bc, Bytecode::AddSpanFunc add_span);

    /// Recalculates the bytecode's length based on an expanded span
    /// length.
    bool expand(Bytecode& bc, unsigned long& len, int span,
                long old_val, long new_val,
                /*@out@*/ long& neg_thres,
                /*@out@*/ long& pos_thres);

    /// Convert a bytecode into its byte representation.
    void to_bytes(Bytecode& bc, Bytes& bytes, OutputValueFunc output_value,
                  OutputRelocFunc output_reloc = 0);

    IncbinBytecode* clone() const;

private:
    std::string m_filename;     ///< file to include data from
    std::string m_from;         ///< filename of what contained incbin

    /// starting offset to read from (NULL=0)
    /*@null@*/ boost::scoped_ptr<Expr> m_start;

    /// maximum number of bytes to read (NULL=no limit)
    /*@null@*/ boost::scoped_ptr<Expr> m_maxlen;
};

IncbinBytecode::IncbinBytecode(const std::string& filename,
                               const std::string& from,
                               std::auto_ptr<Expr> start,
                               std::auto_ptr<Expr> maxlen)
    : m_filename(filename),
      m_from(from),
      m_start(start.release()),
      m_maxlen(maxlen.release())
{
}

IncbinBytecode::~IncbinBytecode()
{
}

void
IncbinBytecode::put(std::ostream& os, int indent_level) const
{
    os << std::setw(indent_level) << "" << "_IncBin_\n";
    os << std::setw(indent_level) << "" << "Filename=`" << m_filename << "'\n";
    os << std::setw(indent_level) << "" << "Start=";
    if (!m_start)
        os << "nil (0)";
    else
        os << *m_start;
    os << '\n' << std::setw(indent_level) << "" << "Max Len=";
    if (!m_maxlen)
        os << "nil (unlimited)";
    else
        os << *m_maxlen;
    os << '\n';
}

void
IncbinBytecode::finalize(Bytecode& bc, Bytecode& prev_bc)
{
    if (m_start) {
        Value val(0, Expr::Ptr(m_start->clone()));
        if (val.finalize(&prev_bc))
            throw TooComplexError(N_("start expression too complex"));
        else if (val.is_relative())
            throw NotAbsoluteError(N_("start expression not absolute"));
        m_start.reset(val.get_abs()->clone());
    }

    if (m_maxlen) {
        Value val(0, Expr::Ptr(m_maxlen->clone()));
        if (val.finalize(&prev_bc))
            throw TooComplexError(N_("maximum length expression too complex"));
        else if (val.is_relative())
            throw NotAbsoluteError(
                N_("maximum length expression not absolute"));
        m_maxlen.reset(val.get_abs()->clone());
    }
}

unsigned long
IncbinBytecode::calc_len(Bytecode& bc, Bytecode::AddSpanFunc add_span)
{
    unsigned long start = 0, maxlen = 0xFFFFFFFFUL, flen;

    // Try to convert start to integer value
    if (m_start) {
        const IntNum* num = m_start->get_intnum();
        if (num)
            start = num->get_uint();
        else {
            // FIXME
            throw NotImplementedError(
                N_("incbin does not yet understand non-constant"));
        }
    }

    // Try to convert maxlen to integer value
    if (m_maxlen) {
        const IntNum* num = m_maxlen->get_intnum();
        if (num)
            maxlen = num->get_uint();
        else {
            // FIXME
            throw NotImplementedError(
                N_("incbin does not yet understand non-constant"));
        }
    }

    // Open file and determine its length
    std::ifstream ifs;
    open_include(ifs, m_filename, m_from,
                 std::ifstream::in | std::ifstream::binary);
    if (!ifs) {
        std::ostringstream emsg;
        emsg << N_("`incbin': unable to open file");
        emsg << " `" << m_filename << "'";
        throw IOError(emsg.str());
    }
    ifs.seekg(0, std::ios_base::end);
    if (!ifs) {
        std::ostringstream emsg;
        emsg << N_("`incbin': unable to seek on file");
        emsg << " `" << m_filename << "'";
        throw IOError(emsg.str());
    }
    flen = (unsigned long)ifs.tellg();

    // Compute length of incbin from start, maxlen, and len
    if (start > flen) {
        std::ostringstream wmsg;
        wmsg << N_("`incbin': start past end of file");
        wmsg << " `" << m_filename << "'";
        warn_set(WARN_GENERAL, wmsg.str());
        start = flen;
    }
    flen -= start;
    if (m_maxlen)
        if (maxlen < flen)
            flen = maxlen;
    return flen;
}

void
IncbinBytecode::to_bytes(Bytecode& bc, Bytes& bytes,
                         OutputValueFunc output_value,
                         OutputRelocFunc output_reloc)
{
    unsigned long start = 0;

    // Convert start to integer value
    if (m_start) {
        const IntNum* num = m_start->get_intnum();
        if (!num)
            throw InternalError(
                N_("could not determine start in incbin::to_bytes"));
        start = num->get_uint();
    }

    // Open file
    std::ifstream ifs;
    open_include(ifs, m_filename, m_from,
                 std::ifstream::in | std::ifstream::binary);
    if (!ifs) {
        std::ostringstream emsg;
        emsg << N_("`incbin': unable to open file");
        emsg << " `" << m_filename << "'";
        throw IOError(emsg.str());
    }

    // Seek to start of data
    ifs.seekg(start, std::ios_base::beg);
    if (!ifs) {
        std::ostringstream emsg;
        emsg << N_("`incbin': unable to seek on file");
        emsg << " `" << m_filename << "'";
        throw IOError(emsg.str());
    }

    // Read len bytes
    bytes.read(ifs, bc.get_len());
    if (!ifs) {
        std::ostringstream emsg;
        emsg << N_("`incbin': unable to read");
        emsg << ' ' << bc.get_len() << ' ';
        emsg << N_("bytes from file");
        emsg << " `" << m_filename << "'";
        throw IOError(emsg.str());
    }
}

} // anonymous namespace

namespace yasm {

std::auto_ptr<Bytecode>
create_incbin(const std::string& filename,
              /*@null@*/ std::auto_ptr<Expr> start,
              /*@null@*/ std::auto_ptr<Expr> maxlen,
              const Linemap& linemap,
              unsigned long line)
{
    std::string from;
    unsigned long xline;

    // Find from filename based on line number
    linemap.lookup(line, from, xline);

    Bytecode::Contents::Ptr
        contents(new IncbinBytecode(filename, from, start, maxlen));
    return std::auto_ptr<Bytecode>(new Bytecode(contents, line));
}

} // namespace yasm

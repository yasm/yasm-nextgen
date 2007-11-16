#ifndef YASM_PREPROC_H
#define YASM_PREPROC_H
///
/// @file libyasm/preproc.h
/// @brief YASM preprocessor interface.
///
/// @license
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
/// @endlicense
///
#include <cstddef>
#include <cstring>
#include <istream>
#include <memory>
#include <streambuf>
#include <string>

#include <boost/scoped_ptr.hpp>

#include "module.h"


namespace yasm {

class Errwarns;
class Linemap;

class PreprocStreamBuf;

/// Preprocesor interface.
class Preprocessor : public Module {
public:
    /// Constructor.
    /// To make preprocessor usable, init() needs to be called.
    Preprocessor() : m_downstreambuf(0), m_downstream(0) {}

    /// Destructor.
    virtual ~Preprocessor() {}

    /// Get the module type.
    /// @return "Preprocessor".
    std::string get_type() const { return "Preprocessor"; }

    /// Get an input stream for the preprocessed source code.
    /// @return Input stream.
    virtual std::istream& get_stream()
    {
        if (m_downstream.get() != 0)
            return *m_downstream;

        m_downstreambuf.reset(create_streambuf().release());
        m_downstream.reset(new std::istream(m_downstreambuf.get()));
        return *m_downstream;
    }

    /// Initialize preprocessor.
    /// The preprocessor needs access to the object format to find out
    /// any object format specific macros.
    /// @param is           initial starting stream
    /// @param in_filename  initial starting file filename
    /// @param linemap      line mapping repository
    /// @param errwarns     error/warning set
    /// @return New preprocessor.
    /// @note Errors/warnings are stored into errwarns.
    virtual void init(std::istream& is, const std::string& in_filename,
                      Linemap& linemap, Errwarns& errwarns) = 0;

    /// Gets more preprocessed source code (up to max_size bytes) into buf.
    /// More than a single line may be returned in buf.
    /// @param buf      destination buffer for preprocessed source
    /// @param max_size maximum number of bytes that can be returned in buf
    /// @return Actual number of bytes returned in buf.
    virtual std::size_t input(/*@out@*/ char* buf, std::size_t max_size) = 0;

    /// Get the next filename included by the source code.
    /// @return Filename.
    virtual std::string get_included_file() = 0;

    /// Pre-include a file.
    /// @param filename     filename
    virtual void add_include_file(const std::string& filename) = 0;

    /// Pre-define a macro.
    /// @param macronameval "name=value" string
    virtual void predefine_macro(const std::string& macronameval) = 0;

    /// Un-define a macro.
    /// @param macroname    macro name
    virtual void undefine_macro(const std::string& macroname) = 0;

    /// Define a builtin macro, preprocessed before the "standard" macros.
    /// @param macronameval "name=value" string
    virtual void define_builtin(const std::string& macronameval) = 0;

protected:
    /// Create a streambuf for preprocessed source.
    /// @return New streambuf.
    virtual std::auto_ptr<std::streambuf> create_streambuf();

private:
    /// streambuf for prepreprocessed source
    boost::scoped_ptr<std::streambuf> m_downstreambuf;
    /// istream for preprocessed source
    boost::scoped_ptr<std::istream> m_downstream;
};


/// Preprocessor streambuf adapter
class PreprocStreamBuf : public std::streambuf {
public:
    /// Constructor.
    PreprocStreamBuf(Preprocessor& preproc)
        : m_preproc(preproc)
    {
        // force underflow()
        setg(m_buffer+4,    // beginning of putback area
             m_buffer+4,    // read position
             m_buffer+4);   // end position
    }

    /// Destructor.
    ~PreprocStreamBuf() {}

protected:
    /// Underflow handler.  Inserts new characters into the buffer.
    virtual int_type underflow()
    {
        // is read position before end of buffer?
        if (gptr() < egptr())
            return *gptr();

        // process size of putback area
        // - use number of characters read
        // - but at most four
        std::size_t numPutback = gptr() - eback();
        if (numPutback > 4)
            numPutback = 4;

        // copy up to four characters previously read into
        // the putback buffer (area of first four characters)
        std::memcpy(m_buffer+(4-numPutback), gptr()-numPutback, numPutback);

        // read new characters
        std::size_t num = m_preproc.input(m_buffer+4, bufferSize-4);
        if (num <= 0)
            return EOF;     // ERROR or EOF

        // reset buffer pointers
        setg (m_buffer+(4-numPutback),  // beginning of putback area
              m_buffer+4,               // read position
              m_buffer+4+num);          // end of buffer

        // return next character
        return *gptr();
    }

private:
    Preprocessor& m_preproc;

    /* data buffer:
     * - at most, four characters in putback area plus
     * - at most, six characters in ordinary read buffer
     */
    static const size_t bufferSize = 8192+4;    // size of the data buffer
    char m_buffer[bufferSize];                  // data buffer
};

inline std::auto_ptr<std::streambuf>
Preprocessor::create_streambuf()
{
    return std::auto_ptr<std::streambuf>(new PreprocStreamBuf(*this));
}

} // namespace yasm

#endif

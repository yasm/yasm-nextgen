#ifndef YASM_ERRWARNS_H
#define YASM_ERRWARNS_H
///
/// @file libyasm/errwarns.h
/// @brief YASM error and warning set interface.
///
/// @license
///  Copyright (C) 2001-2007  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
///  - Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
///  - Redistributions in binary form must reproduce the above copyright
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
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include "export.h"


namespace yasm
{

class Error;
class Linemap;

class YASM_LIB_EXPORT Errwarns : private boost::noncopyable
{
public:
    /// Create an error/warning set for collection of multiple error/warnings.
    Errwarns();

    /// Destructor.
    ~Errwarns();

    /// Propagate error exception to an error/warning set.
    /// Does not print immediately; output_all() outputs
    /// accumulated errors and warnings.
    /// Generally multiple errors on the same line will be reported, but
    /// errors of class #ERROR_PARSE will get overwritten by any other class
    /// on the same line.
    /// @param line     virtual line
    /// @param err      error exception
    void propagate(unsigned long line, const Error& err);

    /// Propagate warning indicator(s) to an error/warning set.
    /// Has no effect if no warnings have occurred.
    /// Does not print immediately; output_all() outputs
    /// accumulated errors and warnings.
    /// @param line     virtual line
    void propagate(unsigned long line);

    /// Get total number of errors logged.
    /// @param errwarns         error/warning set
    /// @param warning_as_error if true, warnings are treated as errors.
    /// @return Number of errors.
    unsigned int num_errors(bool warning_as_error = false) const;

    /// Print out an error.
    /// @param fn           filename of source file
    /// @param line         line number
    /// @param msg          error message
    /// @param xref_fn      cross-referenced source filename
    /// @param xref_line    cross-referenced line number
    /// @param xref_msg     cross-referenced error message
    typedef void (*yasm_print_error_func)
        (const std::string& fn,
         unsigned long line,
         const std::string& msg,
         const std::string& xref_fn,
         unsigned long xref_line,
         const std::string& xref_msg);

    /// Print out a warning.
    /// @param fn   filename of source file
    /// @param line line number
    /// @param msg  warning message
    typedef void (*yasm_print_warning_func)
        (const std::string& fn, unsigned long line, const std::string& msg);

    /// Outputs error/warning set in sorted order (sorted by line number).
    /// @param lm               line map (to convert virtual lines into
    ///                         filename/line pairs)
    /// @param warning_as_error if non-zero, treat warnings as errors.
    ///                         if 1, prints a "warnings being treated as errors"
    ///                         error first.
    /// @param print_error      function called to print out errors
    /// @param print_warning    function called to print out warnings
    void output_all(const Linemap& lm,
                    int warning_as_error,
                    yasm_print_error_func print_error,
                    yasm_print_warning_func print_warning);

private:
    /// Pimpl
    class Impl;
    boost::scoped_ptr<Impl> m_impl;
};

} // namespace yasm

#endif
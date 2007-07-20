///
/// @file libyasm/errwarn.h
/// @brief YASM error and warning reporting interface.
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
#ifndef YASM_ERRWARN_H
#define YASM_ERRWARN_H

#include <vector>
#include <cstdarg>
#include <string>
#include <stdexcept>

namespace yasm {

class LineMap;

/// Warning classes (that may be enabled/disabled).
enum WarnClass {
    WARN_NONE = 0,          ///< No warning
    WARN_GENERAL,           ///< Non-specific warnings
    WARN_UNREC_CHAR,        ///< Unrecognized characters (while tokenizing)
    WARN_PREPROC,           ///< Preprocessor warnings
    WARN_ORPHAN_LABEL,      ///< Label alone on a line without a colon
    WARN_UNINIT_CONTENTS,   ///< Uninitialized space in code/data section
    WARN_SIZE_OVERRIDE      ///< Double size override
};

/// Exception for internal errors.  These are usually due to sanity
/// check failures in the code.
class InternalError : public std::runtime_error
{
public:
    /// Constructor.
    /// @param message  internal error message
    InternalError(const std::string& message);
    virtual ~InternalError() throw() {}

    virtual const char* what() const throw()
    { return m_message.c_str(); }

private:
    std::string m_message;
};

/// Not implemented error.
class NotImplementedError : public InternalError {
public:
    NotImplementedError(const std::string& message) : InternalError(message) {}
};

/// Exception for fatal errors.
class Fatal : public std::exception {
public:
    /// Constructor.
    /// @param message  fatal error message
    Fatal(const std::string& message);
    virtual ~Fatal() throw() {}

    virtual const char* what() const throw()
    { return m_message.c_str(); }

private:
    std::string m_message;
};

/// Error exception base class / non-specific error.
class Error : public std::exception {
public:
    Error(const std::string& message);
    virtual ~Error() throw() {}

    /// Set a cross-reference for the error
    /// @param xrefline virtual line to cross-reference to (should not be 0)
    /// @param message  cross-reference message
    void set_xref(unsigned long xrefline, const std::string& message);

    virtual const char* what() const throw()
    { return m_message.c_str(); }

    /// Get the cross-reference for the error.
    /// @param xrefstr  cross-reference message (output)
    /// @return Cross-reference line number, or 0 if no cross reference.
    virtual unsigned long what_xref(std::string& xrefstr) const
    {
        xrefstr = m_xrefstr;
        return m_xrefline;
    }
private:
    std::string m_message;
    std::string m_xrefstr;
    unsigned long m_xrefline;
};

// Error classes.

/// Arithmetic error (general).
class ArithmeticError : public Error {
public:
    ArithmeticError(const std::string& message)
        : Error(message) {}
};
/// Arithmetic overflow.
class OverflowError : public ArithmeticError {
public:
    OverflowError(const std::string& message)
        : ArithmeticError(message) {}
};
/// Floating point error.
class FloatingPointError : public ArithmeticError {
public:
    FloatingPointError(const std::string& message)
        : ArithmeticError(message) {}
};
/// Divide-by-zero.
class ZeroDivisionError : public ArithmeticError {
public:
    ZeroDivisionError(const std::string& message) : ArithmeticError(message) {}
};

/// Assertion error.
class AssertionError : public Error {
public:
    AssertionError(const std::string& message) : Error(message) {}
};

/// Value inappropriate (e.g. not in range).
class ValueError : public Error {
public:
    ValueError(const std::string& message) : Error(message) {}
};
/// Absolute expression required.
class NotAbsoluteError : public ValueError {
public:
    NotAbsoluteError(const std::string& message) : ValueError(message) {}
};
/// Expression too complex.
class TooComplexError : public ValueError {
public:
    TooComplexError(const std::string& message) : ValueError(message) {}
};
/// Constant expression required.
class NotConstantError : public ValueError {
public:
    NotConstantError(const std::string& message) : ValueError(message) {}
};

/// I/O error.
class IOError : public Error {
public:
    IOError(const std::string& message) : Error(message) {}
};

/// Type error.
class TypeError : public Error {
public:
    TypeError(const std::string& message) : Error(message) {}
};

/// Syntax error.
class SyntaxError : public Error {
public:
    SyntaxError(const std::string& message) : Error(message) {}
};
/// Parser error.
class ParseError : public Error {
public:
    ParseError(const std::string& message) : Error(message) {}
};

/// Unconditionally clear all warning indicators, freeing any associated data.
/// Has no effect if no warning indicators have been set.
void warn_clear();

/// Get the first warning indicator.  YASM_WARN_NONE is returned if no warning
/// has been set.  Note that as YASM_WARN_NONE is 0, the return value can also
/// be treated as a boolean value.
/// @return First warning indicator.
WarnClass warn_occurred();

/// Add a warning indicator (va_list version).
/// @param wclass   warning class
/// @param format   printf format string
/// @param va       argument list for format
void warn_set_va(WarnClass wclass, const char *format, va_list va);

/// Add a warning indicator.
/// @param wclass   warning class
/// @param format   printf format string
/// @param ...      argument list for format
void warn_set(WarnClass wclass, const char *format, ...)
    /*@printflike@*/;

/// Fetch the first warning indicator and all associated data.  If there
/// is at least one warning indicator, the output pointers are set to the
/// first warning indicator values, and first warning indicator is removed.
/// The code using this function is then responsible for yasm_xfree()'ing
/// str and xrefstr (if non-NULL).  If there is no warning indicator set,
/// all output values are set to 0 (including wclass, which is set to
/// #WARN_NONE).
/// @param wclass   warning class (output)
/// @param str      warning message (output)
void warn_fetch(/*@out@*/ WarnClass &wclass,
                /*@out@*/ /*@only@*/ char * &str);

/// Enable a class of warnings.
/// @param wclass   warning class
void warn_enable(WarnClass wclass);

/// Disable a class of warnings.
/// @param wclass   warning class
void warn_disable(WarnClass wclass);

/// Disable all classes of warnings.
void warn_disable_all();

class Errwarns {
public:
    /// Create an error/warning set for collection of multiple error/warnings.
    Errwarns() : m_ecount(0), m_wcount(0), previous_ew(0) {}

    /// Destructor.
    ~Errwarns();

    /// Propagate error indicator and warning indicator(s) to an error/warning
    /// set.  Has no effect if the error indicator and warning indicator are
    /// not set.  Does not print immediately; output_all() outputs
    /// accumulated errors and warnings.
    /// Generally multiple errors on the same line will be reported, but
    /// errors of class #ERROR_PARSE will get overwritten by any other class
    /// on the same line.
    /// @param line     virtual line
    void propagate(unsigned long line);

    /// Get total number of errors logged.
    /// @param errwarns         error/warning set
    /// @param warning_as_error if nonzero, warnings are treated as errors.
    /// @return Number of errors.
    unsigned int num_errors(int warning_as_error) const;

    /// Print out an error.
    /// @param fn           filename of source file
    /// @param line         line number
    /// @param msg          error message
    /// @param xref_fn      cross-referenced source filename
    /// @param xref_line    cross-referenced line number
    /// @param xref_msg     cross-referenced error message
    typedef void (*yasm_print_error_func)
        (const char *fn, unsigned long line, const char *msg,
         /*@null@*/ const char *xref_fn, unsigned long xref_line,
         /*@null@*/ const char *xref_msg);

    /// Print out a warning.
    /// @param fn   filename of source file
    /// @param line line number
    /// @param msg  warning message
    typedef void (*yasm_print_warning_func)
        (const char *fn, unsigned long line, const char *msg);

    /// Outputs error/warning set in sorted order (sorted by line number).
    /// @param lm               line map (to convert virtual lines into
    ///                         filename/line pairs)
    /// @param warning_as_error if true, treat warnings as errors.
    /// @param print_error      function called to print out errors
    /// @param print_warning    function called to print out warnings
    void output_all(LineMap *lm, bool warning_as_error,
                    yasm_print_error_func print_error,
                    yasm_print_warning_func print_warning);

private:
    class ErrwarnData;
    std::vector<ErrwarnData *> m_errwarns;
    int m_ecount, m_wcount;
    ErrwarnData *previous_ew;
};

/// Convert a possibly unprintable character into a printable string.
/// Uses standard cat(1) convention for unprintable characters.
/// @param ch   possibly unprintable character
/// @return Printable string representation.
std::string conv_unprint(int ch);

/// Hook for library users to map to gettext() if GNU gettext is being used.
/// @param msgid    message catalog identifier
/// @return Translated message.
extern const char * (*gettext_hook) (const char *msgid);

} // namespace yasm

#endif

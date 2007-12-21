#ifndef YASM_ERRWARN_H
#define YASM_ERRWARN_H
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
#include <stdexcept>
#include <string>
#include <vector>


namespace yasm {

class Linemap;

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
    explicit InternalError(const std::string& message);
    virtual ~InternalError() throw();

    virtual const char* what() const throw();

private:
    std::string m_message;
};

/// Not implemented error.
class NotImplementedError : public InternalError {
public:
    explicit NotImplementedError(const std::string& message)
        : InternalError(message) {}
};

/// Exception for fatal errors.
class Fatal : public std::exception {
public:
    /// Constructor.
    /// @param message  fatal error message
    explicit Fatal(const std::string& message);
    virtual ~Fatal() throw();

    virtual const char* what() const throw();

private:
    std::string m_message;
};

/// Error exception base class / non-specific error.
class Error : public virtual std::exception {
public:
    explicit Error(const std::string& message);
    virtual ~Error() throw();

    /// Set a cross-reference for the error
    /// @param xrefline virtual line to cross-reference to (should not be 0)
    /// @param message  cross-reference message
    void set_xref(unsigned long xrefline, const std::string& message);

    virtual const char* what() const throw();

    std::string m_message;
    std::string m_xrefmsg;
    unsigned long m_xrefline;
    bool m_parse_error;
};

// Error classes.

/// Arithmetic error (general).
class ArithmeticError : public Error {
public:
    explicit ArithmeticError(const std::string& message)
        : Error(message) {}
    ~ArithmeticError() throw();
};
/// Arithmetic overflow.
class OverflowError : public ArithmeticError {
public:
    explicit OverflowError(const std::string& message)
        : ArithmeticError(message) {}
    ~OverflowError() throw();
};
/// Floating point error.
class FloatingPointError : public ArithmeticError {
public:
    explicit FloatingPointError(const std::string& message)
        : ArithmeticError(message) {}
    ~FloatingPointError() throw();
};
/// Divide-by-zero.
class ZeroDivisionError : public ArithmeticError {
public:
    explicit ZeroDivisionError(const std::string& message)
        : ArithmeticError(message) {}
    ~ZeroDivisionError() throw();
};

/// Assertion error.
class AssertionError : public Error {
public:
    explicit AssertionError(const std::string& message) : Error(message) {}
    ~AssertionError() throw();
};

/// Value inappropriate (e.g. not in range).
class ValueError : public Error {
public:
    explicit ValueError(const std::string& message) : Error(message) {}
    ~ValueError() throw();
};
/// Absolute expression required.
class NotAbsoluteError : public ValueError {
public:
    explicit NotAbsoluteError(const std::string& message)
        : ValueError(message) {}
    ~NotAbsoluteError() throw();
};
/// Expression too complex.
class TooComplexError : public ValueError {
public:
    explicit TooComplexError(const std::string& message)
        : ValueError(message) {}
    ~TooComplexError() throw();
};
/// Constant expression required.
class NotConstantError : public ValueError {
public:
    explicit NotConstantError(const std::string& message)
        : ValueError(message) {}
    ~NotConstantError() throw();
};

/// I/O error.
class IOError : public Error {
public:
    explicit IOError(const std::string& message) : Error(message) {}
    ~IOError() throw();
};

/// Type error.
class TypeError : public Error {
public:
    explicit TypeError(const std::string& message) : Error(message) {}
    ~TypeError() throw();
};

/// Syntax error.
class SyntaxError : public Error {
public:
    explicit SyntaxError(const std::string& message) : Error(message) {}
    ~SyntaxError() throw();
};
/// Parser error.
class ParseError : public Error {
public:
    explicit ParseError(const std::string& message)
        : Error(message)
    { m_parse_error = true; }
    ~ParseError() throw();
};

/// Unconditionally clear all warning indicators, freeing any associated data.
/// Has no effect if no warning indicators have been set.
void warn_clear();

/// Get the first warning indicator.  YASM_WARN_NONE is returned if no warning
/// has been set.  Note that as YASM_WARN_NONE is 0, the return value can also
/// be treated as a boolean value.
/// @return First warning indicator.
WarnClass warn_occurred();

/// Add a warning indicator.
/// @param wclass   warning class
/// @param wstr     warning message
void warn_set(WarnClass wclass, const std::string& wstr);

/// Fetch the first warning indicator.  If there is at least one warning
/// indicator, the output string is set to the first warning indicator
/// value, the first warning indicator is removed, and the warning
/// indicator is returned.
/// @param wstr     warning message (output)
/// @return If there is no warning indicator set, wstr is unchanged, and
///         #WARN_NONE is returned; otherwise, the first warning indicator
///         is returned.
WarnClass warn_fetch(/*@out@*/ std::string& wmsg);

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
    void output_all(const Linemap& lm, int warning_as_error,
                    yasm_print_error_func print_error,
                    yasm_print_warning_func print_warning);

private:
    class Data {
    public:
        Data(unsigned long line, const Error& err);
        Data(unsigned long line, const std::string& wmsg);
        ~Data();

        bool operator< (const Data& other) const
        { return m_line < other.m_line; }

        enum { ERROR, WARNING, PARSERERROR } m_type;

        unsigned long m_line;
        unsigned long m_xrefline;
        std::string m_message;
        std::string m_xrefmsg;
    };

    std::vector<Data> m_errwarns;
    int m_ecount, m_wcount;
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

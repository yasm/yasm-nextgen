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

#include "export.h"


namespace yasm
{

/// Warning classes (that may be enabled/disabled).
enum WarnClass
{
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
class YASM_LIB_EXPORT InternalError : public std::runtime_error
{
public:
    /// Constructor.
    /// @param message  internal error message
    explicit InternalError(const std::string& message);
    virtual ~InternalError() throw();
};

/// Not implemented error.
class YASM_LIB_EXPORT NotImplementedError : public InternalError
{
public:
    explicit NotImplementedError(const std::string& message)
        : InternalError(message)
    {}
};

/// Exception for fatal errors.
class YASM_LIB_EXPORT Fatal : public std::exception
{
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
class YASM_LIB_EXPORT Error : public virtual std::exception
{
public:
    explicit Error(const std::string& message);
    Error(unsigned long line, const std::string& message);
    virtual ~Error() throw();

    /// Set a cross-reference for the error
    /// @param xrefline virtual line to cross-reference to (should not be 0)
    /// @param message  cross-reference message
    void set_xref(unsigned long xrefline, const std::string& message);

    virtual const char* what() const throw();

    unsigned long m_line;
    std::string m_message;
    std::string m_xrefmsg;
    unsigned long m_xrefline;
    bool m_parse_error;
};

// Error classes.

/// Arithmetic error (general).
class YASM_LIB_EXPORT ArithmeticError : public Error
{
public:
    explicit ArithmeticError(const std::string& message)
        : Error(message)
    {}
    ArithmeticError(unsigned long line, const std::string& message)
        : Error(line, message)
    {}
    ~ArithmeticError() throw();
};
/// Arithmetic overflow.
class YASM_LIB_EXPORT OverflowError : public ArithmeticError
{
public:
    explicit OverflowError(const std::string& message)
        : ArithmeticError(message)
    {}
    OverflowError(unsigned long line, const std::string& message)
        : ArithmeticError(line, message)
    {}
    ~OverflowError() throw();
};
/// Floating point error.
class YASM_LIB_EXPORT FloatingPointError : public ArithmeticError
{
public:
    explicit FloatingPointError(const std::string& message)
        : ArithmeticError(message)
    {}
    FloatingPointError(unsigned long line, const std::string& message)
        : ArithmeticError(line, message)
    {}
    ~FloatingPointError() throw();
};
/// Divide-by-zero.
class YASM_LIB_EXPORT ZeroDivisionError : public ArithmeticError
{
public:
    explicit ZeroDivisionError(const std::string& message)
        : ArithmeticError(message)
    {}
    ZeroDivisionError(unsigned long line, const std::string& message)
        : ArithmeticError(line, message)
    {}
    ~ZeroDivisionError() throw();
};

/// Assertion error.
class YASM_LIB_EXPORT AssertionError : public Error
{
public:
    explicit AssertionError(const std::string& message) : Error(message)
    {}
    AssertionError(unsigned long line, const std::string& message)
        : Error(line, message)
    {}
    ~AssertionError() throw();
};

/// Value inappropriate (e.g. not in range).
class YASM_LIB_EXPORT ValueError : public Error
{
public:
    explicit ValueError(const std::string& message) : Error(message)
    {}
    ValueError(unsigned long line, const std::string& message)
        : Error(line, message)
    {}
    ~ValueError() throw();
};
/// Absolute expression required.
class YASM_LIB_EXPORT NotAbsoluteError : public ValueError
{
public:
    explicit NotAbsoluteError(const std::string& message)
        : ValueError(message)
    {}
    NotAbsoluteError(unsigned long line, const std::string& message)
        : ValueError(line, message)
    {}
    ~NotAbsoluteError() throw();
};
/// Expression too complex.
class YASM_LIB_EXPORT TooComplexError : public ValueError
{
public:
    explicit TooComplexError(const std::string& message)
        : ValueError(message)
    {}
    TooComplexError(unsigned long line, const std::string& message)
        : ValueError(line, message)
    {}
    ~TooComplexError() throw();
};
/// Constant expression required.
class YASM_LIB_EXPORT NotConstantError : public ValueError
{
public:
    explicit NotConstantError(const std::string& message)
        : ValueError(message)
    {}
    NotConstantError(unsigned long line, const std::string& message)
        : ValueError(line, message)
    {}
    ~NotConstantError() throw();
};

/// I/O error.
class YASM_LIB_EXPORT IOError : public Error
{
public:
    explicit IOError(const std::string& message) : Error(message)
    {}
    IOError(unsigned long line, const std::string& message)
        : Error(line, message)
    {}
    ~IOError() throw();
};

/// Type error.
class YASM_LIB_EXPORT TypeError : public Error
{
public:
    explicit TypeError(const std::string& message) : Error(message)
    {}
    TypeError(unsigned long line, const std::string& message)
        : Error(line, message)
    {}
    ~TypeError() throw();
};

/// Syntax error.
class YASM_LIB_EXPORT SyntaxError : public Error
{
public:
    explicit SyntaxError(const std::string& message) : Error(message)
    {}
    SyntaxError(unsigned long line, const std::string& message)
        : Error(line, message)
    {}
    ~SyntaxError() throw();
};
/// Parser error.
class YASM_LIB_EXPORT ParseError : public Error
{
public:
    explicit ParseError(const std::string& message)
        : Error(message)
    {
        m_parse_error = true;
    }
    ParseError(unsigned long line, const std::string& message)
        : Error(line, message)
    {
        m_parse_error = true;
    }
    ~ParseError() throw();
};

/// Unconditionally clear all warning indicators, freeing any associated data.
/// Has no effect if no warning indicators have been set.
YASM_LIB_EXPORT
void warn_clear();

/// Get the first warning indicator.  YASM_WARN_NONE is returned if no warning
/// has been set.  Note that as YASM_WARN_NONE is 0, the return value can also
/// be treated as a boolean value.
/// @return First warning indicator.
YASM_LIB_EXPORT
WarnClass warn_occurred();

/// Add a warning indicator.
/// @param line     line
/// @param wclass   warning class
/// @param wstr     warning message
YASM_LIB_EXPORT
void warn_set(unsigned long line, WarnClass wclass, const std::string& wstr);

/// Add a warning indicator.
/// @param wclass   warning class
/// @param wstr     warning message
inline void
warn_set(WarnClass wclass, const std::string& wstr)
{
    warn_set(0, wclass, wstr);
}

/// Update all warning indicators that do not have a line number set with
/// a line number.
/// @param line     line number
YASM_LIB_EXPORT
void warn_update_line(unsigned long line);

/// Fetch the first warning indicator.  If there is at least one warning
/// indicator, the output string is set to the first warning indicator
/// value, the first warning indicator is removed, and the warning
/// indicator is returned.
/// @param wstr     warning message (output)
/// @param wline    warning line (output)
/// @return If there is no warning indicator set, wstr is unchanged, and
///         #WARN_NONE is returned; otherwise, the first warning indicator
///         is returned.
YASM_LIB_EXPORT
WarnClass warn_fetch(/*@out@*/ std::string* wmsg,
                     /*@out@*/ unsigned long *wline = 0);

/// Enable a class of warnings.
/// @param wclass   warning class
YASM_LIB_EXPORT
void warn_enable(WarnClass wclass);

/// Disable a class of warnings.
/// @param wclass   warning class
YASM_LIB_EXPORT
void warn_disable(WarnClass wclass);

/// Disable all classes of warnings.
YASM_LIB_EXPORT
void warn_disable_all();

/// Convert a possibly unprintable character into a printable string.
/// Uses standard cat(1) convention for unprintable characters.
/// @param ch   possibly unprintable character
/// @return Printable string representation.
YASM_LIB_EXPORT
std::string conv_unprint(int ch);

/// Hook for library users to map to gettext() if GNU gettext is being used.
/// @param msgid    message catalog identifier
/// @return Translated message.
YASM_LIB_EXPORT
extern const char * (*gettext_hook) (const char *msgid);

} // namespace yasm

#endif

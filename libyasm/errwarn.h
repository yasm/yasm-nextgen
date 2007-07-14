/**
 * \file libyasm/errwarn.h
 * \brief YASM error and warning reporting interface.
 *
 * \license
 *  Copyright (C) 2001-2007  Peter Johnson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * \endlicense
 */
#ifndef YASM_ERRWARN_H
#define YASM_ERRWARN_H

#include <vector>
#include <cstdarg>

namespace yasm {

/** Warning classes (that may be enabled/disabled). */
enum WarnClass {
    WARN_NONE = 0,          /**< No warning */
    WARN_GENERAL,           /**< Non-specific warnings */
    WARN_UNREC_CHAR,        /**< Unrecognized characters (while tokenizing) */
    WARN_PREPROC,           /**< Preprocessor warnings */
    WARN_ORPHAN_LABEL,      /**< Label alone on a line without a colon */
    WARN_UNINIT_CONTENTS,   /**< Uninitialized space in code/data section */
    WARN_SIZE_OVERRIDE      /**< Double size override */
};

/** Error classes.  Bitmask-based to support limited subclassing. */
enum ErrorClass {
    ERROR_NONE              = 0x0000, /**< No error */
    ERROR_GENERAL           = 0xFFFF, /**< Non-specific */
    ERROR_ARITHMETIC        = 0x0001, /**< Arithmetic error (general) */
    ERROR_OVERFLOW          = 0x8001, /**< Arithmetic overflow */
    ERROR_FLOATING_POINT    = 0x4001, /**< Floating point error */
    ERROR_ZERO_DIVISION     = 0x2001, /**< Divide-by-zero */
    ERROR_ASSERTION         = 0x0002, /**< Assertion error */
    ERROR_VALUE             = 0x0004, /**< Value inappropriate
                                       *   (e.g. not in range) */
    ERROR_NOT_ABSOLUTE      = 0x8004, /**< Absolute expression required */
    ERROR_TOO_COMPLEX       = 0x4004, /**< Expression too complex */
    ERROR_NOT_CONSTANT      = 0x2004, /**< Constant expression required */
    ERROR_IO                = 0x0008, /**< I/O error */
    ERROR_NOT_IMPLEMENTED   = 0x0010, /**< Not implemented error */
    ERROR_TYPE              = 0x0020, /**< Type error */
    ERROR_SYNTAX            = 0x0040, /**< Syntax error */
    ERROR_PARSE             = 0x8040  /**< Parser error */
};

/** Reporting point of internal errors.  These are usually due to sanity
 * check failures in the code.
 * \warning This function must NOT return to calling code; exit or longjmp
 *          instead.
 * \param file      source file (ala __FILE__)
 * \param line      source line (ala __LINE__)
 * \param message   internal error message
 */
extern /*@exits@*/ void (*internal_error_)
    (const char *file, unsigned int line, const char *message);

/** Easily-callable version of yasm_internal_error_().  Automatically uses
 * __FILE__ and __LINE__ as the file and line.
 * \param message   internal error message
 */
#define internal_error(message) \
    internal_error_(__FILE__, __LINE__, message)

/** Reporting point of fatal errors.
 * \warning This function must NOT return to calling code; exit or longjmp
 *          instead.
 * \param message   fatal error message
 * \param va        va_list argument list for message
 */
extern /*@exits@*/ void (*fatal_) (const char *message, va_list va);

/** Reporting point of fatal errors, with variable arguments (internal only).
 * \warning This function calls #yasm_fatal, and thus does not return to the
 *          calling code.
 * \param message   fatal error message
 * \param ...       argument list for message
 */
/*@exits@*/ void fatal(const char *message, ...);

/** Unconditionally clear the error indicator, freeing any associated data.
 * Has no effect if the error indicator is not set.
 */
void error_clear();

/** Get the error indicator.  YASM_ERROR_NONE is returned if no error has
 * been set.  Note that as YASM_ERROR_NONE is 0, the return value can also
 * be treated as a boolean value.
 * \return Current error indicator.
 */
ErrorClass error_occurred();

/** Check the error indicator against an error class.  To check if any error
 * has been set, check against the YASM_ERROR_GENERAL class.  This function
 * properly checks error subclasses.
 * \param eclass    base error class to check against
 * \return True if error indicator is set and a subclass of eclass.
 */
bool error_matches(ErrorClass eclass);

/** Set the error indicator (va_list version).  Has no effect if the error
 * indicator is already set.
 * \param eclass    error class
 * \param format    printf format string
 * \param va        argument list for format
 */
void error_set_va(ErrorClass eclass, const char *format, va_list va);

/** Set the error indicator.  Has no effect if the error indicator is already
 * set.
 * \param eclass    error class
 * \param format    printf format string
 * \param ...       argument list for format
 */
void error_set(ErrorClass eclass, const char *format, ...)
    /*@printflike@*/;

/** Set a cross-reference for a new error (va_list version).  Has no effect
 * if the error indicator is already set (e.g. with yasm_error_set()).  This
 * function must be called prior to its corresponding yasm_error_set() call.
 * \param xrefline  virtual line to cross-reference to (should not be 0)
 * \param format    printf format string
 * \param va        argument list for format
 */
void error_set_xref_va(unsigned long xrefline, const char *format, va_list va);

/** Set a cross-reference for a new error.  Has no effect if the error
 * indicator is already set (e.g. with yasm_error_set()).  This function
 * must be called prior to its corresponding yasm_error_set() call.
 * \param xrefline  virtual line to cross-reference to (should not be 0)
 * \param format    printf format string
 * \param ...       argument list for format
 */
void error_set_xref(unsigned long xrefline, const char *format, ...)
    /*@printflike@*/;

/** Fetch the error indicator and all associated data.  If the error
 * indicator is set, the output pointers are set to the current error
 * indicator values, and the error indicator is cleared.
 * The code using this function is then responsible for yasm_xfree()'ing
 * str and xrefstr (if non-NULL).  If the error indicator is not set,
 * all output values are set to 0 (including eclass, which is set to
 * YASM_ERROR_NONE).
 * \param eclass    error class (output)
 * \param str       error message
 * \param xrefline  virtual line used for cross-referencing (0 if no xref)
 * \param xrefstr   cross-reference error message (NULL if no xref)
 */
void error_fetch(/*@out@*/ ErrorClass &eclass,
                 /*@out@*/ /*@only@*/ /*@null@*/ char * &str,
                 /*@out@*/ unsigned long &xrefline,
                 /*@out@*/ /*@only@*/ /*@null@*/ char * &xrefstr);

/** Unconditionally clear all warning indicators, freeing any associated data.
 * Has no effect if no warning indicators have been set.
 */
void warn_clear();

/** Get the first warning indicator.  YASM_WARN_NONE is returned if no warning
 * has been set.  Note that as YASM_WARN_NONE is 0, the return value can also
 * be treated as a boolean value.
 * \return First warning indicator.
 */
WarnClass warn_occurred();

/** Add a warning indicator (va_list version).
 * \param wclass    warning class
 * \param format    printf format string
 * \param va        argument list for format
 */
void warn_set_va(WarnClass wclass, const char *format, va_list va);

/** Add a warning indicator.
 * \param wclass    warning class
 * \param format    printf format string
 * \param ...       argument list for format
 */
void warn_set(WarnClass wclass, const char *format, ...)
    /*@printflike@*/;

/** Fetch the first warning indicator and all associated data.  If there
 * is at least one warning indicator, the output pointers are set to the
 * first warning indicator values, and first warning indicator is removed.
 * The code using this function is then responsible for yasm_xfree()'ing
 * str and xrefstr (if non-NULL).  If there is no warning indicator set,
 * all output values are set to 0 (including wclass, which is set to
 * YASM_WARN_NONE).
 * \param wclass    warning class (output)
 * \param str       warning message (output)
 */
void warn_fetch(/*@out@*/ WarnClass &wclass,
                /*@out@*/ /*@only@*/ char * &str);

/** Enable a class of warnings.
 * \param wclass    warning class
 */
void warn_enable(WarnClass wclass);

/** Disable a class of warnings.
 * \param wclass    warning class
 */
void warn_disable(WarnClass wclass);

/** Disable all classes of warnings. */
void warn_disable_all();

class Errwarns {
public:
    /** Create an error/warning set for collection of multiple error/warnings.
     */
    Errwarns() : m_ecount(0), m_wcount(0), previous_ew(0) {}

    /** Destructor. */
    ~Errwarns();

    /** Propagate error indicator and warning indicator(s) to an error/warning
     * set.  Has no effect if the error indicator and warning indicator are
     * not set.  Does not print immediately; output_all() outputs
     * accumulated errors and warnings.
     * Generally multiple errors on the same line will be reported, but errors
     * of class YASM_ERROR_PARSE will get overwritten by any other class on the
     * same line.
     * \param line      virtual line
     */
    void propagate(unsigned long line);

    /** Get total number of errors logged.
     * \param errwarns          error/warning set
     * \param warning_as_error  if nonzero, warnings are treated as errors.
     * \return Number of errors.
     */
    unsigned int num_errors(int warning_as_error) const;

    /** Print out an error.
     * \param fn            filename of source file
     * \param line          line number
     * \param msg           error message
     * \param xref_fn       cross-referenced source filename
     * \param xref_line     cross-referenced line number
     * \param xref_msg      cross-referenced error message
     */
    typedef void (*yasm_print_error_func)
        (const char *fn, unsigned long line, const char *msg,
         /*@null@*/ const char *xref_fn, unsigned long xref_line,
         /*@null@*/ const char *xref_msg);

    /** Print out a warning.
     * \param fn    filename of source file
     * \param line  line number
     * \param msg   warning message
     */
    typedef void (*yasm_print_warning_func)
        (const char *fn, unsigned long line, const char *msg);

    /** Outputs error/warning set in sorted order (sorted by line number).
     * \param lm    line map (to convert virtual lines into filename/line pairs)
     * \param warning_as_error  if true, treat warnings as errors.
     * \param print_error       function called to print out errors
     * \param print_warning     function called to print out warnings
     */
    void output_all(LineMap *lm, bool warning_as_error,
                    yasm_print_error_func print_error,
                    yasm_print_warning_func print_warning);

private:
    class ErrwarnData;
    std::vector<ErrwarnData *> m_errwarns;
    int m_ecount, m_wcount;
    ErrwarnData *previous_ew;
};

/** Convert a possibly unprintable character into a printable string.
 * \internal
 * \param ch    possibly unprintable character
 * \return Printable string representation (static buffer).
 */
char *conv_unprint(int ch);

/** Hook for library users to map to gettext() if GNU gettext is being used.
 * \param msgid     message catalog identifier
 * \return Translated message.
 */
extern const char * (*gettext_hook) (const char *msgid);

} // namespace yasm

#endif

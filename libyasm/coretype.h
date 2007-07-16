/**
 * \file libyasm/coretype.h
 * \brief YASM core types and utility functions.
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
#ifndef YASM_CORETYPE_H
#define YASM_CORETYPE_H

#include <boost/function.hpp>

namespace yasm {

/** Architecture.  \see arch.h for details. */
class Arch;
class Register;
/** Preprocessor.  \see preproc.h for details. */
class Preproc;
/** Parser.  \see parser.h for details. */
class Parser;
/** Object format.  \see objfmt.h for details. */
class ObjectFormat;
/** Debug format.  \see dbgfmt.h for details. */
class DebugFormat;
/** List format.  \see listfmt.h for details. */
class ListFormat;

/** Object format module.  \see objfmt.h for details. */
struct ObjectFormatModule;
/** Debug format module.  \see dbgfmt.h for details. */
struct DebugFormatModule;
#if 0
/** YASM associated data callback structure.  Many data structures can have
 * arbitrary data associated with them.
 */
typedef struct yasm_assoc_data_callback {
    /** Free memory allocated for associated data.
     * \param data      associated data
     */
    void (*destroy) (/*@only@*/ void *data);

    /** Print a description of allocated data.  For debugging purposes.
     * \param data              associated data
     * \param f                 output file
     * \param indent_level      indentation level
     */
    void (*print) (void *data, FILE *f, int indent_level);
} yasm_assoc_data_callback;
#endif
/** Set of collected error/warnings.  \see errwarn.h for details. */
class Errwarns;

/** Bytecode.  \see bytecode.h for related functions. */
class Bytecode;

/** Object.  \see section.h for details and related functions. */
class Object;

/** Section.  \see section.h for related functions. */
class Section;

/** Symbol table.  \see symrec.h for related functions. */
class SymbolTable;

/** Symbol record.  \see symrec.h for related functions. */
class Symbol;

/** Expression.  \see expr.h for related functions. */
class Expr;
/** Integer value.  \see intnum.h for related functions. */
class IntNum;
/** Floating point value.  \see floatnum.h for related functions. */
class FloatNum;

/** A value.  May be absolute or relative.  Outside the parser, yasm_expr
 * should only be used for absolute exprs.  Anything that could contain
 * a relocatable value should use this structure instead.
 * \see value.h for related functions.
 */
class Value;

/** Maximum value of #yasm_value.rshift */
#define YASM_VALUE_RSHIFT_MAX   127

/** Line number mapping repository.  \see linemap.h for related functions. */
class LineMap;

/** Name/value pair.  \see directive.h for related functions. */
class NameVal;

/** Directive list entry.
 * \see directive.h for details and related functions.
 */
struct Directive;

/** An effective address.
 * \see insn.h for related functions.
 */
class EffAddr;

/** An instruction.
 * \see insn.h for related functions.
 */
class Insn;

/** Expression operators usable in #Expr expressions. */
enum ExprOp {
    EXPR_IDENT,     /**< No operation, just a value. */
    EXPR_ADD,       /**< Arithmetic addition (+). */
    EXPR_SUB,       /**< Arithmetic subtraction (-). */
    EXPR_MUL,       /**< Arithmetic multiplication (*). */
    EXPR_DIV,       /**< Arithmetic unsigned division. */
    EXPR_SIGNDIV,   /**< Arithmetic signed division. */
    EXPR_MOD,       /**< Arithmetic unsigned modulus. */
    EXPR_SIGNMOD,   /**< Arithmetic signed modulus. */
    EXPR_NEG,       /**< Arithmetic negation (-). */
    EXPR_NOT,       /**< Bitwise negation. */
    EXPR_OR,        /**< Bitwise OR. */
    EXPR_AND,       /**< Bitwise AND. */
    EXPR_XOR,       /**< Bitwise XOR. */
    EXPR_XNOR,      /**< Bitwise XNOR. */
    EXPR_NOR,       /**< Bitwise NOR. */
    EXPR_SHL,       /**< Shift left (logical). */
    EXPR_SHR,       /**< Shift right (logical). */
    EXPR_LOR,       /**< Logical OR. */
    EXPR_LAND,      /**< Logical AND. */
    EXPR_LNOT,      /**< Logical negation. */
    EXPR_LXOR,      /**< Logical XOR. */
    EXPR_LXNOR,     /**< Logical XNOR. */
    EXPR_LNOR,      /**< Logical NOR. */
    EXPR_LT,        /**< Less than comparison. */
    EXPR_GT,        /**< Greater than comparison. */
    EXPR_EQ,        /**< Equality comparison. */
    EXPR_LE,        /**< Less than or equal to comparison. */
    EXPR_GE,        /**< Greater than or equal to comparison. */
    EXPR_NE,        /**< Not equal comparison. */
    EXPR_NONNUM,    /**< Start of non-numeric operations (not an op). */
    EXPR_SEG,       /**< SEG operator (gets segment portion of address). */
    EXPR_WRT,       /**< WRT operator (gets offset of address relative to
                     *   some other segment). */
    EXPR_SEGOFF     /**< The ':' in segment:offset. */
};

/** Convert yasm_value to its byte representation.  Usually implemented by
 * object formats to keep track of relocations and verify legal expressions.
 * Must put the value into the least significant bits of the destination,
 * unless shifted into more significant bits by the shift parameter.  The
 * destination bits must be cleared before being set.
 * \param value         value
 * \param buf           buffer for byte representation
 * \param destsize      destination size (in bytes)
 * \param offset        offset (in bytes) of the expr contents from the start
 *                      of the bytecode (needed for relative)
 * \param bc            current bytecode (usually passed into higher-level
 *                      calling function)
 * \param warn          enables standard warnings: zero for none;
 *                      nonzero for overflow/underflow floating point warnings;
 *                      negative for signed integer warnings,
 *                      positive for unsigned integer warnings
 * \return True if an error occurred, false otherwise.
 */
typedef
    boost::function<bool (Value* value, /*@out@*/ unsigned char* buf,
                          unsigned int destsize, unsigned long offset,
                          Bytecode *bc, int warn)>
    OutputValueFunc;

/** Convert a symbol reference to its byte representation.  Usually implemented
 * by object formats and debug formats to keep track of relocations generated
 * by themselves.
 * \param sym           symbol
 * \param bc            current bytecode (usually passed into higher-level
 *                      calling function)
 * \param buf           buffer for byte representation
 * \param destsize      destination size (in bytes)
 * \param valsize       size (in bits)
 * \param warn          enables standard warnings: zero for none;
 *                      nonzero for overflow/underflow floating point warnings;
 *                      negative for signed integer warnings,
 *                      positive for unsigned integer warnings
 * \return True if an error occurred, false otherwise.
 */
typedef
    boost::function<bool (Symbol* sym, Bytecode* bc, unsigned char* buf,
                          unsigned int destsize, unsigned int valsize,
                          int warn)>
    OutputRelocFunc;
#if 0
/** Compare two strings, ignoring case differences.
 * \internal
 * \param s1    string 1
 * \param s2    string 2
 * \return 0 if strings are equal, -1 if s1<s2, 1 if s1>s2.
 */
int strcasecmp(const char *s1, const char *s2);

/** Compare portion of two strings, ignoring case differences.
 * \internal
 * \param s1    string 1
 * \param s2    string 2
 * \param n     maximum number of characters to compare
 * \return 0 if strings are equal, -1 if s1<s2, 1 if s1>s2.
 */
int strncasecmp(const char *s1, const char *s2, size_t n);
#endif

} // namespace yasm

#endif

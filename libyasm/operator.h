#ifndef YASM_OPERATOR_H
#define YASM_OPERATOR_H
///
/// @file libyasm/operator.h
/// @brief YASM expression operators.
///
/// @license
///  Copyright (C) 2007  Peter Johnson
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
namespace yasm {

namespace Op {
    /// Operators usable in expressions.
    enum Op {
        IDENT,      ///< No operation, just a value.
        ADD,        ///< Arithmetic addition (+).
        SUB,        ///< Arithmetic subtraction (-).
        MUL,        ///< Arithmetic multiplication (*).
        DIV,        ///< Arithmetic unsigned division.
        SIGNDIV,    ///< Arithmetic signed division.
        MOD,        ///< Arithmetic unsigned modulus.
        SIGNMOD,    ///< Arithmetic signed modulus.
        NEG,        ///< Arithmetic negation (-).
        NOT,        ///< Bitwise negation.
        OR,         ///< Bitwise OR.
        AND,        ///< Bitwise AND.
        XOR,        ///< Bitwise XOR.
        XNOR,       ///< Bitwise XNOR.
        NOR,        ///< Bitwise NOR.
        SHL,        ///< Shift left (logical).
        SHR,        ///< Shift right (logical).
        LOR,        ///< Logical OR.
        LAND,       ///< Logical AND.
        LNOT,       ///< Logical negation.
        LXOR,       ///< Logical XOR.
        LXNOR,      ///< Logical XNOR.
        LNOR,       ///< Logical NOR.
        LT,         ///< Less than comparison.
        GT,         ///< Greater than comparison.
        EQ,         ///< Equality comparison.
        LE,         ///< Less than or equal to comparison.
        GE,         ///< Greater than or equal to comparison.
        NE,         ///< Not equal comparison.
        NONNUM,     ///< Start of non-numeric operations (not an op).
        SEG,        ///< SEG operator (gets segment portion of address).
        /// WRT operator (gets offset of address relative to some other
        /// segment).
        WRT,
        SEGOFF      ///< The ':' in segment:offset.
    };
}

} // namespace yasm

#endif

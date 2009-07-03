///
/// @file
/// @brief Expression operators implementation.
///
/// @license
///  Copyright (C) 2008  Peter Johnson
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
#include "yasmx/Op.h"


namespace yasm
{

bool
isUnary(Op::Op op)
{
    switch (op)
    {
        case Op::IDENT:
        case Op::NEG:
        case Op::NOT:
        case Op::LNOT:
        case Op::SEG:
            return true;
        default:
            return false;
    }
}

bool
isCommutative(Op::Op op)
{
    switch (op)
    {
        case Op::ADD:
        case Op::MUL:
        case Op::OR:
        case Op::AND:
        case Op::XOR:
        case Op::XNOR:
        case Op::NOR:
        case Op::LOR:
        case Op::LAND:
        case Op::LXOR:
        case Op::LXNOR:
        case Op::LNOR:
            return true;
        default:
            return false;
    }
}

bool
isAssociative(Op::Op op)
{
    switch (op)
    {
        case Op::ADD:
        case Op::MUL:
        case Op::OR:
        case Op::AND:
        case Op::XOR:
        case Op::XNOR:
        case Op::LOR:
        case Op::LAND:
        case Op::LXOR:
        case Op::LXNOR:
            return true;
        default:
            return false;
    }
}

} // namespace yasm

///
/// Data (and LEB128) bytecode
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
#include "BytecodeContainer_util.h"

#include "util.h"

#include "Arch.h"
#include "BytecodeContainer.h"
#include "Bytecode.h"
#include "Bytes_util.h"
#include "Expr.h"


namespace yasm
{

void
append_byte(BytecodeContainer& container, unsigned char val)
{
    Bytecode& bc = container.fresh_bytecode();
    write_8(bc.get_fixed(), val);
}

void
append_data(BytecodeContainer& container,
            const IntNum& val,
            unsigned int size,
            const Arch& arch)
{
    Bytecode& bc = container.fresh_bytecode();
    Bytes zero;
    zero.resize(size);
    arch.tobytes(val, zero, size*8, 0, 1);
    bc.get_fixed().insert(bc.get_fixed().end(), zero.begin(), zero.end());
}

void
append_data(BytecodeContainer& container,
            std::auto_ptr<Expr> expr,
            unsigned int size,
            const Arch& arch,
            unsigned long line)
{
    expr->simplify();
    if (IntNum* intn = expr->get_intnum())
    {
        append_data(container, *intn, size, arch);
        return;
    }
    Bytecode& bc = container.fresh_bytecode();
    bc.append_fixed(size, expr, line);
}

void
append_data(BytecodeContainer& container,
            const std::string& str,
            bool append_zero)
{
    Bytes& fixed = container.fresh_bytecode().get_fixed();
    fixed.write(reinterpret_cast<const unsigned char *>(str.data()),
                str.length());
    if (append_zero)
        write_8(fixed, 0);
}

void
append_data(BytecodeContainer& container,
            const std::string& str,
            unsigned int size,
            bool append_zero)
{
    Bytes& fixed = container.fresh_bytecode().get_fixed();
    std::string::size_type len = str.length();
    fixed.write(reinterpret_cast<const unsigned char *>(str.data()), len);
    if ((len % size) != 0)
        fixed.write(size-(len % size), 0);
    if (append_zero)
        write_8(fixed, 0);
}

} // namespace yasm

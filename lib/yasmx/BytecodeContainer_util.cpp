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
#include "yasmx/BytecodeContainer_util.h"

#include "util.h"

#include "yasmx/Arch.h"
#include "yasmx/BytecodeContainer.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/Expr.h"


namespace yasm
{

void
AppendByte(BytecodeContainer& container, unsigned char val)
{
    Bytecode& bc = container.FreshBytecode();
    Write8(bc.getFixed(), val);
}

void
AppendData(BytecodeContainer& container,
           const IntNum& val,
           unsigned int size,
           const Arch& arch)
{
    Bytecode& bc = container.FreshBytecode();
    Bytes zero;
    zero.resize(size);
    arch.ToBytes(val, zero, size*8, 0, 1);
    bc.getFixed().insert(bc.getFixed().end(), zero.begin(), zero.end());
}

void
AppendData(BytecodeContainer& container,
           std::auto_ptr<Expr> expr,
           unsigned int size,
           const Arch& arch,
           unsigned long line)
{
    expr->Simplify();
    if (expr->isIntNum())
    {
        AppendData(container, expr->getIntNum(), size, arch);
        return;
    }
    Bytecode& bc = container.FreshBytecode();
    bc.AppendFixed(size, expr, line);
}

void
AppendData(BytecodeContainer& container,
           const std::string& str,
           bool append_zero)
{
    Bytes& fixed = container.FreshBytecode().getFixed();
    fixed.Write(reinterpret_cast<const unsigned char *>(str.data()),
                str.length());
    if (append_zero)
        Write8(fixed, 0);
}

void
AppendData(BytecodeContainer& container,
           const std::string& str,
           unsigned int size,
           bool append_zero)
{
    Bytes& fixed = container.FreshBytecode().getFixed();
    std::string::size_type len = str.length();
    fixed.Write(reinterpret_cast<const unsigned char *>(str.data()), len);
    if ((len % size) != 0)
        fixed.Write(size-(len % size), 0);
    if (append_zero)
        Write8(fixed, 0);
}

} // namespace yasm

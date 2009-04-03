//
// Extended Dynamic Object format relocation
//
//  Copyright (C) 2004-2007  Peter Johnson
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
#include "XdfReloc.h"

#include <yasmx/Bytes.h>
#include <yasmx/Bytes_util.h>
#include <yasmx/IntNum.h>
#include <yasmx/Value.h>

#include "XdfSymbol.h"


namespace yasm
{
namespace objfmt
{
namespace xdf
{

XdfReloc::XdfReloc(const IntNum& addr,
                   SymbolRef sym,
                   SymbolRef base,
                   Type type,
                   Size size,
                   unsigned int shift)
    : Reloc(addr, sym)
    , m_base(base)
    , m_type(type)
    , m_size(size)
    , m_shift(shift)
{
}

XdfReloc::XdfReloc(const IntNum& addr, const Value& value, bool ip_rel)
    : Reloc(addr, value.get_rel())
    , m_base(0)
    , m_size(static_cast<Size>(value.get_size()/8))
    , m_shift(value.get_rshift())
{
    if (value.is_seg_of())
        m_type = XDF_SEG;
    else if (value.is_wrt())
    {
        m_base = value.get_wrt();
        m_type = XDF_WRT;
    }
    else if (ip_rel)
        m_type = XDF_RIP;
    else
        m_type = XDF_REL;
}

XdfReloc::~XdfReloc()
{
}

Expr
XdfReloc::get_value() const
{
    Expr e(m_sym);
    if (m_type == XDF_WRT)
        e.calc(Op::WRT, m_base);

    if (m_shift > 0)
        e.calc(Op::SHR, m_shift);

    return e;
}

std::string
XdfReloc::get_type_name() const
{
    std::string s;

    switch (m_type)
    {
        case XDF_REL: s += "REL_"; break;
        case XDF_WRT: s += "WRT_"; break;
        case XDF_RIP: s += "RIP_"; break;
        case XDF_SEG: s += "SEG_"; break;
    }

    switch (m_size)
    {
        case XDF_8: s += "8"; break;
        case XDF_16: s += "16"; break;
        case XDF_32: s += "32"; break;
        case XDF_64: s += "64"; break;
    }

    return s;
}

void
XdfReloc::write(Bytes& bytes) const
{
    const XdfSymbol* xsym = get_xdf(m_sym);
    assert(xsym != 0);      // need symbol data for relocated symbol

    bytes << little_endian;

    write_32(bytes, m_addr);
    write_32(bytes, xsym->index);       // relocated symbol

    if (m_base)
    {
        xsym = get_xdf(m_base);
        assert(xsym != 0);  // need symbol data for relocated base symbol
        write_32(bytes, xsym->index);   // base symbol
    }
    else
    {
        // need base symbol for WRT relocation
        assert(m_type != XDF_WRT);
        write_32(bytes, 0);             // no base symbol
    }

    write_8(bytes, m_type);             // type of relocation
    write_8(bytes, m_size);             // size of relocation
    write_8(bytes, m_shift);            // relocation shift
    write_8(bytes, 0);                  // flags
}

}}} // namespace yasm::objfmt::xdf

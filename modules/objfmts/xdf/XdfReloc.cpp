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

#include "yasmx/Bytes.h"
#include "yasmx/Bytes_util.h"
#include "yasmx/IntNum.h"
#include "yasmx/Value.h"

#include "XdfSymbol.h"


using namespace yasm;
using namespace yasm::objfmt;

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
    : Reloc(addr, value.getRelative())
    , m_base(0)
    , m_size(static_cast<Size>(value.getSize()/8))
    , m_shift(value.getRShift())
{
    if (value.isSegOf())
        m_type = XDF_SEG;
    else if (value.isWRT())
    {
        m_base = value.getWRT();
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
XdfReloc::getValue() const
{
    Expr e(m_sym);
    if (m_type == XDF_WRT)
        e.Calc(Op::WRT, m_base);

    if (m_shift > 0)
        e.Calc(Op::SHR, m_shift);

    return e;
}

std::string
XdfReloc::getTypeName() const
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
XdfReloc::Write(Bytes& bytes) const
{
    const XdfSymbol* xsym = m_sym->getAssocData<XdfSymbol>();
    assert(xsym != 0);      // need symbol data for relocated symbol

    bytes.setLittleEndian();

    Write32(bytes, m_addr);
    Write32(bytes, xsym->index);        // relocated symbol

    if (m_base)
    {
        xsym = m_base->getAssocData<XdfSymbol>();
        assert(xsym != 0);  // need symbol data for relocated base symbol
        Write32(bytes, xsym->index);    // base symbol
    }
    else
    {
        // need base symbol for WRT relocation
        assert(m_type != XDF_WRT);
        Write32(bytes, 0);              // no base symbol
    }

    Write8(bytes, m_type);              // type of relocation
    Write8(bytes, m_size);              // size of relocation
    Write8(bytes, m_shift);             // relocation shift
    Write8(bytes, 0);                   // flags
}

pugi::xml_node
XdfReloc::DoWrite(pugi::xml_node out) const
{
    pugi::xml_node root = out.append_child("XdfReloc");
    append_child(root, "Base", m_base);
    pugi::xml_attribute type = root.append_attribute("type");
    switch (m_type)
    {
        case XDF_REL:   type = "REL"; break;
        case XDF_WRT:   type = "WRT"; break;
        case XDF_RIP:   type = "RIP"; break;
        case XDF_SEG:   type = "SEG"; break;
    }
    pugi::xml_attribute size = root.append_attribute("size");
    switch (m_size)
    {
        case XDF_8:     size = 8; break;
        case XDF_16:    size = 16; break;
        case XDF_32:    size = 32; break;
        case XDF_64:    size = 64; break;
    }
    append_child(root, "Shift", m_shift);
    return root;
}

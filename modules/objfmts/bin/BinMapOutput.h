#ifndef YASM_BINMAPOUTPUT_H
#define YASM_BINMAPOUTPUT_H
//
// Flat-format binary object format map file output
//
//  Copyright (C) 2002-2008  Peter Johnson
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
#include <iosfwd>

#include "BinLink.h"


namespace yasm
{

class IntNum;
class Object;

namespace objfmt
{
namespace bin
{

class BinMapOutput
{
public:
    BinMapOutput(std::ostream& os,
                 const Object& object,
                 const IntNum& origin,
                 const BinGroups& groups);
    ~BinMapOutput();

    void output_header();
    void output_origin();
    void output_sections_summary();
    void output_sections_detail();
    void output_sections_symbols();

private:
    void output_intnum(const IntNum& intn);
    void inner_sections_summary(const BinGroups& groups);
    void inner_sections_detail(const BinGroups& groups);
    void output_symbols(const Section* sect);
    void inner_sections_symbols(const BinGroups& groups);

    // address width
    int m_bytes;

    std::ostream& m_os;         // map output file
    const Object& m_object;     // object
    const IntNum& m_origin;     // origin
    const BinGroups& m_groups;  // section groups
};

}}} // namespace yasm::objfmt::bin

#endif

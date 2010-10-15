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
#include "BinLink.h"


namespace llvm { class raw_ostream; }

class Diagnostic;
class IntNum;
class Object;

class BinMapOutput
{
public:
    BinMapOutput(llvm::raw_ostream& os,
                 const Object& object,
                 const IntNum& origin,
                 const BinGroups& groups,
                 Diagnostic& diags);
    ~BinMapOutput();

    void OutputHeader();
    void OutputOrigin();
    void OutputSectionsSummary();
    void OutputSectionsDetail();
    void OutputSectionsSymbols();

private:
    void OutputIntNum(const IntNum& intn);
    void InnerSectionsSummary(const BinGroups& groups);
    void InnerSectionsDetail(const BinGroups& groups);
    void OutputSymbols(const Section* sect);
    void InnerSectionsSymbols(const BinGroups& groups);

    // address width
    int m_bytes;

    llvm::raw_ostream& m_os;        // map output file
    const Object& m_object;     // object
    const IntNum& m_origin;     // origin
    const BinGroups& m_groups;  // section groups
    Diagnostic& m_diags;        // diagnostic reporting
};

#endif

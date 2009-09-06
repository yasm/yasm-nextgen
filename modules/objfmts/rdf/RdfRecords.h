#ifndef YASM_RDFRECORDS_H
#define YASM_RDFRECORDS_H
//
// Relocatable Dynamic Object (RDOFF) record types
//
//  Copyright (C) 2006-2007  Peter Johnson
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
namespace yasm
{
namespace objfmt
{
namespace rdf
{

// Record types that may be present in the RDOFF header
enum RdfRecordType
{
    RDFREC_GENERIC = 0,
    RDFREC_RELOC = 1,
    RDFREC_IMPORT = 2,
    RDFREC_GLOBAL = 3,
    RDFREC_DLL = 4,
    RDFREC_BSS = 5,
    RDFREC_SEGRELOC = 6,
    RDFREC_FARIMPORT = 7,
    RDFREC_MODNAME = 8,
    RDFREC_COMMON = 10
};

}}} // namespace yasm::objfmt::rdf

#endif

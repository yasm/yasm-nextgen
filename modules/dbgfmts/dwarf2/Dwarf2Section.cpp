//
// DWARF2 debugging format - per-section information
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
// 3. Neither the name of the author nor the names of other contributors
//    may be used to endorse or promote products derived from this
//    software without specific prior written permission.
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
#include "Dwarf2Section.h"


using namespace yasm;
using namespace yasm::dbgfmt;

Dwarf2Loc::Dwarf2Loc(Location loc_,
                     clang::SourceLocation source_,
                     unsigned long file_,
                     unsigned long line_)
    : source(source_)
    , file(file_)
    , line(line_)
    , column(0)
    , isa_change(false)
    , isa(0)
    , is_stmt(IS_STMT_NOCHANGE)
    , basic_block(false)
    , prologue_end(false)
    , epilogue_begin(false)
    , loc(loc_)
{
}

const char* Dwarf2Section::key = "yasm::dbgfmt::Dwarf2Section";

Dwarf2Section::Dwarf2Section()
{
}

Dwarf2Section::~Dwarf2Section()
{
}

void
Dwarf2Section::Write(YAML::Emitter& out) const
{
    // TODO
}

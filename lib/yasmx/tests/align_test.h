// Align bytecode unit test
//
//  Copyright (C) 2008  Peter Johnson
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
#include <cxxtest/TestSuite.h>

#include "yasmx/BytecodeContainer.h"
#include "yasmx/BytecodeContainer_util.h"
#include "yasmx/Bytecode.h"
#include "yasmx/Expr.h"
#include "yasmx/IntNum.h"

class AlignTestSuite : public CxxTest::TestSuite
{
public:
    void testAppendAlign()
    {
        yasm::BytecodeContainer container;
        yasm::AppendAlign(container,
                          yasm::Expr(4),
                          yasm::Expr(),    // fill
                          yasm::Expr(),    // maxskip
                          0,               // code fill
                          clang::SourceLocation::getFromRawEncoding(5));
        yasm::Bytecode& align = container.bytecodes_first();

        // align always results in contents
        TS_ASSERT(align.hasContents());
        TS_ASSERT_EQUALS(align.getSpecial(),
                         yasm::Bytecode::Contents::SPECIAL_OFFSET);
        TS_ASSERT_EQUALS(align.getSource().getRawEncoding(), 5U);
        TS_ASSERT(align.getFixed().empty());
    }
};

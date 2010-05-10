#ifndef YASM_NASMSTRINGPARSER_H
#define YASM_NASMSTRINGPARSER_H
//
// NASM-compatible string literal parser
//
//  Copyright (C) 2010  Peter Johnson
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
#include <cctype>

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"


namespace clang
{
class SourceLocation;
} // namespace clang

namespace yasm
{

class IntNum;
class Preprocessor;

namespace parser
{
namespace nasm
{

/// This performs strict semantic analysis of the content of a string token,
/// performs unescaping if necessary, and can convert it to a useful value.
class NasmStringParser
{
    const char* m_chars_begin;
    const char* m_chars_end;

    bool m_needs_unescape;
    bool m_had_error;

public:
    NasmStringParser(const char* begin, const char* end,
                     clang::SourceLocation loc, Preprocessor& pp);

    bool hadError() const { return m_had_error; }

    /// Return the string data.  Unescaping is performed as necessary to
    /// obtain the actual data.
    std::string getString() const;

    /// Get the string data into a SmallVector.
    /// @note The returned StringRef may not point to the supplied buffer
    ///       if a copy can be avoided.
    llvm::StringRef getString(llvm::SmallVectorImpl<char>& buffer) const;

    /// Convert this string literal value to an IntNum.
    /// This follows the NASM "character constant" conversion rules.
    void getIntegerValue(IntNum* val);
};

}}} // namespace yasm::parser::nasm

#endif

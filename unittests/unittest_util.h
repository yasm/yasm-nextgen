#ifndef UNITTEST_UTIL_H
#define	UNITTEST_UTIL_H
///
/// @file
/// @brief Unit test utilities.
///
/// @license
///  Copyright (C) 2009  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
///  - Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
///  - Redistributions in binary form must reproduce the above copyright
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
/// @endlicense
///
#include <iosfwd>

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"
#include "yasmx/Config/export.h"


namespace llvm { class format_object_base; }

namespace yasmunit
{

// allow llvm::format to be used with std::ostream.
YASM_UNIT_EXPORT
std::ostream& operator<< (std::ostream& os, const llvm::format_object_base& fmt);

} // namespace yasmunit

namespace String
{

template<typename T>
std::string
Format(const T& x)
{
    llvm::SmallString<64> ss;
    llvm::raw_svector_ostream oss(ss);
    oss << x;
    return oss.str();
}

} // namespace String

#endif

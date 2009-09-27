#ifndef YASM_SUPPORT_STRINGEXTRAS_H
#define YASM_SUPPORT_STRINGEXTRAS_H
///
/// @file
/// @brief Extra string functions.
///
/// @license
///  Copyright (C) 2001-2007  Peter Johnson
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
#include <string>

#include "yasmx/Config/export.h"


namespace yasm
{

/// Convert a possibly unprintable character into a printable string.
/// Uses standard cat(1) convention for unprintable characters.
/// @param ch   possibly unprintable character
/// @return Printable string representation.
YASM_LIB_EXPORT
std::string ConvUnprint(int ch);

/// Unescape a string with C-style escapes.  Handles b, f, n, r, t, and hex
/// and octal escapes.
/// Edge cases:
/// - hex escapes: reads as many hex digits as possible, takes last 2 as value.
/// - oct escapes: takes up to 3 digits 0-9 and scales appropriately, with
///                warning.
/// @param str      C-style string (modified in place)
/// @return False if octal warning should be issued (octal value out of range).
YASM_LIB_EXPORT
bool Unescape(std::string* str);

} // namespace yasm

#endif

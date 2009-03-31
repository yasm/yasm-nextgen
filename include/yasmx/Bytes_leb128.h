#ifndef YASM_LEB128_H
#define YASM_LEB128_H
///
/// @file
/// @brief LEB128 interface.
///
/// @license
///  Copyright (C) 2008  Peter Johnson
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
#include "yasmx/Config/export.h"


namespace yasm
{

class Bytes;
class IntNum;

/// Output intnum to bytes in signed LEB128-encoded form.
/// @param bytes    output bytes buffer
/// @param intn     intnum
/// @param sign     true if signed LEB128, false if unsigned LEB128
/// @return Number of bytes generated.
YASM_LIB_EXPORT
unsigned long write_leb128(Bytes& bytes, const IntNum& intn, bool sign);

/// Calculate number of bytes signed LEB128-encoded form of intnum will take.
/// @param intn     intnum
/// @return Number of bytes.
YASM_LIB_EXPORT
unsigned long size_leb128(const IntNum& intn, bool sign);

/// Output intnum to bytes in signed LEB128-encoded form.
/// @param bytes    output bytes buffer
/// @param intn     intnum
/// @return Number of bytes generated.
inline unsigned long
write_sleb128(Bytes& bytes, const IntNum& intn)
{
    return write_leb128(bytes, intn, true);
}

/// Calculate number of bytes signed LEB128-encoded form of intnum will take.
/// @param intn     intnum
/// @return Number of bytes.
inline unsigned long
size_sleb128(const IntNum& intn)
{
    return size_leb128(intn, true);
}

/// Output intnum to bytes in unsigned LEB128-encoded form.
/// @param bytes    output bytes buffer
/// @param intn     intnum
/// @return Number of bytes generated.
inline unsigned long
write_uleb128(Bytes& bytes, const IntNum& intn)
{
    return write_leb128(bytes, intn, false);
}

/// Calculate number of bytes unsigned LEB128-encoded form of intnum will take.
/// @param intn     intnum
/// @return Number of bytes.
inline unsigned long
size_uleb128(const IntNum& intn)
{
    return size_leb128(intn, false);
}

/// Output integer to bytes in signed LEB128-encoded form.
/// @param bytes    output bytes buffer
/// @param v        integer
/// @return Number of bytes generated.
YASM_LIB_EXPORT
unsigned long write_sleb128(Bytes& bytes, long v);

/// Calculate number of bytes signed LEB128-encoded form of integer will take.
/// @param v        integer
/// @return Number of bytes.
YASM_LIB_EXPORT
unsigned long size_sleb128(long v);

/// Output integer to bytes in unsigned LEB128-encoded form.
/// @param bytes    output bytes buffer
/// @param v        integer
/// @return Number of bytes generated.
YASM_LIB_EXPORT
unsigned long write_uleb128(Bytes& bytes, unsigned long v);

/// Calculate number of bytes unsigned LEB128-encoded form of integer will
/// take.
/// @param v        integer
/// @return Number of bytes.
YASM_LIB_EXPORT
unsigned long size_uleb128(unsigned long v);

} // namespace yasm

#endif

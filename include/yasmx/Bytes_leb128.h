#ifndef YASM_BYTES_LEB128_H
#define YASM_BYTES_LEB128_H
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
#include "yasmx/IntNum.h"


namespace yasm
{

class Bytes;
class InputBuffer;

/// Output intnum to bytes in LEB128-encoded form.
/// @param bytes    output bytes buffer
/// @param intn     intnum
/// @param sign     true if signed LEB128, false if unsigned LEB128
/// @return Number of bytes generated.
unsigned long WriteLEB128(Bytes& bytes, const IntNum& intn, bool sign);

/// Calculate number of bytes LEB128-encoded form of intnum will take.
/// @param intn     intnum
/// @param sign     true if signed LEB128, false if unsigned LEB128
/// @return Number of bytes.
unsigned long SizeLEB128(const IntNum& intn, bool sign);

/// Read a LEB128-encoded value from an input buffer.
/// @param input    input buffer
/// @param sign     true if signed LEB128, false if unsigned LEB128
/// @param size     number of bytes read (returned)
/// @return IntNum value; number of bytes read returned in size.
IntNum ReadLEB128(InputBuffer& input,
                  bool sign,
                  /*@out@*/ unsigned long* size = 0);

/// Output intnum to bytes in signed LEB128-encoded form.
/// @param bytes    output bytes buffer
/// @param intn     intnum
/// @return Number of bytes generated.
inline unsigned long
WriteSLEB128(Bytes& bytes, const IntNum& intn)
{
    return WriteLEB128(bytes, intn, true);
}

/// Calculate number of bytes signed LEB128-encoded form of intnum will take.
/// @param intn     intnum
/// @return Number of bytes.
inline unsigned long
SizeSLEB128(const IntNum& intn)
{
    return SizeLEB128(intn, true);
}

/// Read a signed LEB128-encoded value from an input buffer.
/// @param input    input buffer
/// @param size     number of bytes read (returned)
/// @return IntNum value; number of bytes read returned in size.
inline IntNum
ReadSLEB128(InputBuffer& input, /*@out@*/ unsigned long* size = 0)
{
    return ReadLEB128(input, true, size);
}

/// Output intnum to bytes in unsigned LEB128-encoded form.
/// @param bytes    output bytes buffer
/// @param intn     intnum
/// @return Number of bytes generated.
inline unsigned long
WriteULEB128(Bytes& bytes, const IntNum& intn)
{
    return WriteLEB128(bytes, intn, false);
}

/// Calculate number of bytes unsigned LEB128-encoded form of intnum will take.
/// @param intn     intnum
/// @return Number of bytes.
inline unsigned long
SizeULEB128(const IntNum& intn)
{
    return SizeLEB128(intn, false);
}

/// Read a unsigned LEB128-encoded value from an input buffer.
/// @param input    input buffer
/// @param size     number of bytes read (returned)
/// @return IntNum value; number of bytes read returned in size.
inline IntNum
ReadULEB128(InputBuffer& input, /*@out@*/ unsigned long* size = 0)
{
    return ReadLEB128(input, false, size);
}

} // namespace yasm

#endif

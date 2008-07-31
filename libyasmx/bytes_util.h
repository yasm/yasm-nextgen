#ifndef YASM_BYTES_UTIL_H
#define YASM_BYTES_UTIL_H
///
/// @file
/// @brief Bytes utility functions interface.
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
#include "bytes.h"
#include "export.h"


namespace yasm
{

class IntNum;

/// Write an intnum as an 8-bit value to a bytes buffer.
/// @param bytes    output bytes buffer
/// @param intn     intnum
/// @warning Intnum is silently truncated to fit into 8 bits.
YASM_LIB_EXPORT
void write_8(Bytes& bytes, const IntNum& intn);

/// Write an intnum as a 16-bit value to a bytes buffer.
/// @param bytes    output bytes buffer
/// @param intn     intnum
/// @warning Intnum is silently truncated to fit into 16 bits.
YASM_LIB_EXPORT
void write_16(Bytes& bytes, const IntNum& intn);

/// Write an intnum as a 32-bit value to a bytes buffer.
/// @param bytes    output bytes buffer
/// @param intn     intnum
/// @warning Intnum is silently truncated to fit into 32 bits.
YASM_LIB_EXPORT
void write_32(Bytes& bytes, const IntNum& intn);

/// Write an intnum as a 64-bit value to a bytes buffer.
/// @param bytes    output bytes buffer
/// @param intn     intnum
/// @warning Intnum is silently truncated to fit into 64 bits.
YASM_LIB_EXPORT
void write_64(Bytes& bytes, const IntNum& intn);

/// Write an 8-bit value to a bytes buffer.
/// @param bytes    output bytes buffer
/// @param val      8-bit value
inline void
write_8(Bytes& bytes, unsigned char val)
{
    bytes.push_back(val & 0xFF);
}

/// Write a 16-bit value to a bytes buffer.
/// @param bytes    output bytes buffer
/// @param val      16-bit value
inline void
write_16(Bytes& bytes, unsigned short val)
{
    if (bytes.is_bigendian())
    {
        bytes.push_back(static_cast<unsigned char>((val >> 8) & 0xFF));
        bytes.push_back(static_cast<unsigned char>(val & 0xFF));
    }
    else
    {
        bytes.push_back(static_cast<unsigned char>(val & 0xFF));
        bytes.push_back(static_cast<unsigned char>((val >> 8) & 0xFF));
    }
}

/// Write a 32-bit value to a bytes buffer.
/// @param bytes    output bytes buffer
/// @param val      32-bit value
inline void
write_32(Bytes& bytes, unsigned long val)
{
    if (bytes.is_bigendian())
    {
        bytes.push_back(static_cast<unsigned char>((val >> 24) & 0xFF));
        bytes.push_back(static_cast<unsigned char>((val >> 16) & 0xFF));
        bytes.push_back(static_cast<unsigned char>((val >> 8) & 0xFF));
        bytes.push_back(static_cast<unsigned char>(val & 0xFF));
    }
    else
    {
        bytes.push_back(static_cast<unsigned char>(val & 0xFF));
        bytes.push_back(static_cast<unsigned char>((val >> 8) & 0xFF));
        bytes.push_back(static_cast<unsigned char>((val >> 16) & 0xFF));
        bytes.push_back(static_cast<unsigned char>((val >> 24) & 0xFF));
    }
}

} // namespace yasm

#endif

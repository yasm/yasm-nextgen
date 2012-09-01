#ifndef YASM_INPUTBUFFER_H
#define YASM_INPUTBUFFER_H
///
/// @file
/// @brief Input buffer interface.
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
#include <stdexcept>

#include "llvm/Support/MemoryBuffer.h"
#include "yasmx/Basic/LLVM.h"
#include "yasmx/Config/export.h"
#include "yasmx/Support/EndianState.h"


namespace yasm
{

class IntNum;

/// An input buffer.
class InputBuffer : public EndianState
{
public:
    InputBuffer(const MemoryBuffer& input, size_t startpos=0);
    InputBuffer(const unsigned char* input, size_t len);

    /// Get buffer size.
    /// @return Buffer size.
    size_t getSize() const { return m_end-m_start; }

    /// Set read position.
    /// @param pos  new read position
    void setPosition(size_t pos) { m_ptr = m_start + pos; }

    /// Get read position.
    /// @return Current read position.
    size_t getPosition() const { return m_ptr - m_start; }

    /// Get remaining bytes after read position.
    /// @return Number of bytes.
    size_t getReadableSize() const;

    /// Perform a "read" by returning a pointer to the current read position
    /// and then advancing the read position.
    /// @param n    number of bytes to advance read position by
    /// @return Pointer to current read position.
    /// Throws std::out_of_range if not enough bytes left to read n bytes.
    const unsigned char* Read(size_t n);

private:
    const unsigned char* m_start;
    const unsigned char* m_end;
    const unsigned char* m_ptr;
};

inline
InputBuffer::InputBuffer(const MemoryBuffer& input, size_t startpos)
    : m_start(reinterpret_cast<const unsigned char*>(input.getBufferStart()))
    , m_end(reinterpret_cast<const unsigned char*>(input.getBufferEnd()))
{
    m_ptr = m_start + startpos;
}

inline
InputBuffer::InputBuffer(const unsigned char* input, size_t len)
    : m_start(input), m_end(input+len), m_ptr(input)
{
}

inline size_t
InputBuffer::getReadableSize() const
{
    if (m_ptr > m_end)
        return 0;
    return m_end-m_ptr;
}

inline const unsigned char*
InputBuffer::Read(size_t n)
{
    const unsigned char* oldptr = m_ptr;
    m_ptr += n;
    if (m_ptr > m_end)
        throw std::out_of_range("read past end of buffer");
    return oldptr;
}

/// Read an unsigned 8-bit value from an input buffer.
/// @param input    input buffer
/// @return 8-bit value.
inline unsigned char
ReadU8(InputBuffer& input)
{
    const unsigned char* ptr = input.Read(1);
    return ptr[0] & 0xFF;
}

/// Read an signed 8-bit value from an input buffer.
/// @param input    input buffer
/// @return 8-bit value.
inline signed char
ReadS8(InputBuffer& input)
{
    unsigned char val = ReadU8(input);
    if (val & 0x80)
        return -static_cast<signed char>((~val)+1);
    else
        return static_cast<signed char>(val);
}

/// Read an unsigned 16-bit value from an input buffer.
/// @param input    input buffer
/// @return 16-bit value.
inline unsigned short
ReadU16(InputBuffer& input)
{
    const unsigned char* ptr = input.Read(2);
    unsigned short val = 0;
    if (input.isBigEndian())
    {
        val |= ptr[0] & 0xFF;
        val <<= 8;
        val |= ptr[1] & 0xFF;
    }
    else
    {
        val |= ptr[1] & 0xFF;
        val <<= 8;
        val |= ptr[0] & 0xFF;
    }
    return val;
}

/// Read an signed 16-bit value from an input buffer.
/// @param input    input buffer
/// @return 16-bit value.
inline short
ReadS16(InputBuffer& input)
{
    unsigned short val = ReadU16(input);
    if (val & 0x8000)
        return -static_cast<short>((~val)+1);
    else
        return static_cast<short>(val);
}

/// Read an unsigned 32-bit value from an input buffer.
/// @param input    input buffer
/// @return 32-bit value.
inline unsigned long
ReadU32(InputBuffer& input)
{
    const unsigned char* ptr = input.Read(4);
    unsigned long val = 0;
    if (input.isBigEndian())
    {
        val |= ptr[0] & 0xFF;
        val <<= 8;
        val |= ptr[1] & 0xFF;
        val <<= 8;
        val |= ptr[2] & 0xFF;
        val <<= 8;
        val |= ptr[3] & 0xFF;
    }
    else
    {
        val |= ptr[3] & 0xFF;
        val <<= 8;
        val |= ptr[2] & 0xFF;
        val <<= 8;
        val |= ptr[1] & 0xFF;
        val <<= 8;
        val |= ptr[0] & 0xFF;
    }
    return val;
}

/// Read an signed 32-bit value from an input buffer.
/// @param input    input buffer
/// @return 32-bit value.
inline long
ReadS32(InputBuffer& input)
{
    unsigned long val = ReadU32(input);
    if (val & 0x80000000UL)
        return -static_cast<long>((~val)+1);
    else
        return static_cast<long>(val);
}

/// Read an unsigned N-bit value from an input buffer.
/// @param input    input buffer
/// @param n        number of bits (must be multiple of 8)
/// @return N-bit value (as an IntNum).
YASM_LIB_EXPORT
IntNum ReadUnsigned(InputBuffer& input, int n);

/// Read an signed N-bit value from an input buffer.
/// @param input    input buffer
/// @param n        number of bits (must be multiple of 8)
/// @return N-bit value (as an IntNum).
YASM_LIB_EXPORT
IntNum ReadSigned(InputBuffer& input, int n);

/// Read an unsigned 64-bit value from an input buffer.
/// @param input    input buffer
/// @return 64-bit value (as an IntNum).
YASM_LIB_EXPORT
IntNum ReadU64(InputBuffer& input);

/// Read an signed 64-bit value from an input buffer.
/// @param input    input buffer
/// @return 64-bit value (as an IntNum).
YASM_LIB_EXPORT
IntNum ReadS64(InputBuffer& input);

} // namespace yasm

#endif

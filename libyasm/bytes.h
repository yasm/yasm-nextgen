#ifndef YASM_BYTES_H
#define YASM_BYTES_H
///
/// @file libyasm/bytes.h
/// @brief YASM bytes interface.
///
/// @license
///  Copyright (C) 2007  Peter Johnson
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
#include <vector>


namespace yasm {

class Bytes : public std::vector<unsigned char> {
public:
    Bytes(bool bigendian=false) : m_bigendian(bigendian) {}
    ~Bytes() {}

    void set_bigendian(bool bigendian) { m_bigendian = bigendian; }
    bool is_bigendian() const { return m_bigendian; }

private:
    bool m_bigendian;
};

struct SetEndian {
    bool m_bigendian;
};

inline SetEndian
set_endian(bool bigendian)
{
    SetEndian x;
    x.m_bigendian = bigendian;
    return x;
}

inline Bytes&
operator<< (Bytes& bytes, const SetEndian& sete)
{
    bytes.set_bigendian(sete.m_bigendian);
    return bytes;
}

inline Bytes&
big_endian(Bytes& bytes)
{
    bytes.set_bigendian(true);
    return bytes;
}

inline Bytes&
little_endian(Bytes& bytes)
{
    bytes.set_bigendian(false);
    return bytes;
}

/// Write an 8-bit value, incrementing buffer pointer.
/// @param bytes    bytes
/// @param val      8-bit value
inline Bytes&
operator<< (Bytes& bytes, unsigned char val)
{
    bytes.push_back(val & 0xFF);
    return bytes;
}

/// Write a 16-bit value, incrementing buffer pointer.
/// @param ptr  buffer
/// @param val  16-bit value
inline Bytes&
operator<< (Bytes& bytes, unsigned short val)
{
    if (bytes.is_bigendian()) {
        bytes.push_back((unsigned char)((val >> 8) & 0xFF));
        bytes.push_back((unsigned char)(val & 0xFF));
    } else {
        bytes.push_back((unsigned char)(val & 0xFF));
        bytes.push_back((unsigned char)((val >> 8) & 0xFF));
    }
    return bytes;
}

/// Write a 32-bit value to a buffer, incrementing buffer pointer.
/// @param ptr  buffer
/// @param val  32-bit value
inline Bytes&
operator<< (Bytes& bytes, unsigned long val)
{
    if (bytes.is_bigendian()) {
        bytes.push_back((unsigned char)((val >> 24) & 0xFF));
        bytes.push_back((unsigned char)((val >> 16) & 0xFF));
        bytes.push_back((unsigned char)((val >> 8) & 0xFF));
        bytes.push_back((unsigned char)(val & 0xFF));
    } else {
        bytes.push_back((unsigned char)(val & 0xFF));
        bytes.push_back((unsigned char)((val >> 8) & 0xFF));
        bytes.push_back((unsigned char)((val >> 16) & 0xFF));
        bytes.push_back((unsigned char)((val >> 24) & 0xFF));
    }
    return bytes;
}

// Output the entire contents of a bytes container to an output stream.
// @param os    output stream
// @param bytes bytes
// @return Output stream
inline std::ostream&
operator<< (std::ostream& os, const Bytes& bytes)
{
    os.write(reinterpret_cast<const char*>(&bytes[0]), bytes.size());
    return os;
}

} // namespace yasm

#endif

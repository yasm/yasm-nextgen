#ifndef YASM_COMPOSE_H
#define YASM_COMPOSE_H
///
/// @file
/// @brief String composition.
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
#include <sstream>
#include <string>

#include "yasmx/Config/export.h"


namespace String
{

class YASM_LIB_EXPORT Compose
{
public:
    // initialize and prepare format string on the form "text %1 text %2 etc."
    explicit Compose(const std::string& fmt);
    ~Compose();

    // copy and assignment
    Compose(const Compose& rhs);
    Compose& operator=(const Compose& rhs);

    // supply an replacement argument starting from %1
    template <typename T>
    Compose& auto_arg(const T& obj);

    template <typename T>
    Compose& operator%(const T& obj) { return auto_arg(obj); }

    // compose and return string
    std::string str() const;
    operator std::string () const { return str(); }

private:
    std::string m_fmt;
    std::ostringstream m_os;
    int m_arg;

    // Starting positions of each of the args in m_os string.
    // can handle a maximum of 9 args
    std::string::size_type m_arg_pos[10];
};

template <typename T>
inline Compose&
Compose::auto_arg(const T& obj)
{
    m_arg_pos[m_arg] = m_os.tellp();
    m_os << obj;
    ++m_arg;
    return *this;
}

inline std::ostream&
operator<< (std::ostream& os, const Compose& c)
{
    os << c.str();
    return os;
}

inline std::string
compose(const std::string& fmt)
{
    Compose c(fmt);
    return c.str();
}

template <typename T1>
inline std::string
compose(const std::string& fmt, const T1& a1)
{
    Compose c(fmt);
    c.auto_arg(a1);
    return c.str();
}

template <typename T1, typename T2>
inline std::string
compose(const std::string& fmt, const T1& a1, const T2& a2)
{
    Compose c(fmt);
    c.auto_arg(a1);
    c.auto_arg(a2);
    return c.str();
}

template <typename T1, typename T2, typename T3>
inline std::string
compose(const std::string& fmt, const T1& a1, const T2& a2, const T3& a3)
{
    Compose c(fmt);
    c.auto_arg(a1);
    c.auto_arg(a2);
    c.auto_arg(a3);
    return c.str();
}

template <typename T1, typename T2, typename T3, typename T4>
inline std::string
compose(const std::string& fmt, const T1& a1, const T2& a2, const T3& a3,
        const T4& a4)
{
    Compose c(fmt);
    c.auto_arg(a1);
    c.auto_arg(a2);
    c.auto_arg(a3);
    c.auto_arg(a4);
    return c.str();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
inline std::string
compose(const std::string& fmt, const T1& a1, const T2& a2, const T3& a3,
        const T4& a4, const T5& a5)
{
    Compose c(fmt);
    c.auto_arg(a1);
    c.auto_arg(a2);
    c.auto_arg(a3);
    c.auto_arg(a4);
    c.auto_arg(a5);
    return c.str();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6>
inline std::string
compose(const std::string& fmt, const T1& a1, const T2& a2, const T3& a3,
        const T4& a4, const T5& a5, const T6& a6)
{
    Compose c(fmt);
    c.auto_arg(a1);
    c.auto_arg(a2);
    c.auto_arg(a3);
    c.auto_arg(a4);
    c.auto_arg(a5);
    c.auto_arg(a6);
    return c.str();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7>
inline std::string
compose(const std::string& fmt, const T1& a1, const T2& a2, const T3& a3,
        const T4& a4, const T5& a5, const T6& a6, const T7& a7)
{
    Compose c(fmt);
    c.auto_arg(a1);
    c.auto_arg(a2);
    c.auto_arg(a3);
    c.auto_arg(a4);
    c.auto_arg(a5);
    c.auto_arg(a6);
    c.auto_arg(a7);
    return c.str();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8>
inline std::string
compose(const std::string& fmt, const T1& a1, const T2& a2, const T3& a3,
        const T4& a4, const T5& a5, const T6& a6, const T7& a7, const T8& a8)
{
    Compose c(fmt);
    c.auto_arg(a1);
    c.auto_arg(a2);
    c.auto_arg(a3);
    c.auto_arg(a4);
    c.auto_arg(a5);
    c.auto_arg(a6);
    c.auto_arg(a7);
    c.auto_arg(a8);
    return c.str();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9>
inline std::string
compose(const std::string& fmt, const T1& a1, const T2& a2, const T3& a3,
        const T4& a4, const T5& a5, const T6& a6, const T7& a7, const T8& a8,
        const T9& a9)
{
    Compose c(fmt);
    c.auto_arg(a1);
    c.auto_arg(a2);
    c.auto_arg(a3);
    c.auto_arg(a4);
    c.auto_arg(a5);
    c.auto_arg(a6);
    c.auto_arg(a7);
    c.auto_arg(a8);
    c.auto_arg(a9);
    return c.str();
}

template <typename T1>
inline std::string
format(const T1& a1)
{
    std::ostringstream os;
    os << a1;
    return os.str();
}

template <typename T1, typename T2>
inline std::string
format(const T1& a1, const T2& a2)
{
    std::ostringstream os;
    os << a1 << a2;
    return os.str();
}

template <typename T1, typename T2, typename T3>
inline std::string
format(const T1& a1, const T2& a2, const T3& a3)
{
    std::ostringstream os;
    os << a1 << a2 << a3;
    return os.str();
}

template <typename T1, typename T2, typename T3, typename T4>
inline std::string
format(const T1& a1, const T2& a2, const T3& a3, const T4& a4)
{
    std::ostringstream os;
    os << a1 << a2 << a3 << a4;
    return os.str();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
inline std::string
format(const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5)
{
    std::ostringstream os;
    os << a1 << a2 << a3 << a4 << a5;
    return os.str();
}

} // namespace String

#endif

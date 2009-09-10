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
#include <string>

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include "yasmx/Config/export.h"


namespace String
{

class YASM_LIB_EXPORT Composer
{
public:
    // initialize and prepare format string on the form "text %1 text %2 etc."
    explicit Composer(const llvm::StringRef& fmt)
        : m_fmt(fmt), m_os(m_ss), m_arg(0)
    {
        m_arg_pos[0] = 0;
    }
    ~Composer();

    // copy and assignment
    Composer(const Composer& rhs);
    Composer& operator=(const Composer& rhs);

    // supply an replacement argument starting from %1
    template <typename T>
    Composer& AutoArg(const T& obj);

    template <typename T>
    Composer& operator%(const T& obj) { return AutoArg(obj); }

    // compose and return string
    std::string getStr() const;
    operator std::string () const { return getStr(); }

private:
    llvm::StringRef m_fmt;
    llvm::SmallString<128> m_ss;
    mutable llvm::raw_svector_ostream m_os;
    int m_arg;

    // Starting positions of each of the args in m_os string.
    // can handle a maximum of 9 args
    std::string::size_type m_arg_pos[10];
};

template <typename T>
inline Composer&
Composer::AutoArg(const T& obj)
{
    m_arg_pos[m_arg] = m_os.tell();
    m_os << obj;
    ++m_arg;
    return *this;
}

inline std::ostream&
operator<< (std::ostream& os, const Composer& c)
{
    os << c.getStr();
    return os;
}

inline std::string
Compose(const llvm::StringRef& fmt)
{
    Composer c(fmt);
    return c.getStr();
}

template <typename T1>
inline std::string
Compose(const llvm::StringRef& fmt, const T1& a1)
{
    Composer c(fmt);
    c.AutoArg(a1);
    return c.getStr();
}

template <typename T1, typename T2>
inline std::string
Compose(const llvm::StringRef& fmt, const T1& a1, const T2& a2)
{
    Composer c(fmt);
    c.AutoArg(a1);
    c.AutoArg(a2);
    return c.getStr();
}

template <typename T1, typename T2, typename T3>
inline std::string
Compose(const llvm::StringRef& fmt, const T1& a1, const T2& a2, const T3& a3)
{
    Composer c(fmt);
    c.AutoArg(a1);
    c.AutoArg(a2);
    c.AutoArg(a3);
    return c.getStr();
}

template <typename T1, typename T2, typename T3, typename T4>
inline std::string
Compose(const llvm::StringRef& fmt, const T1& a1, const T2& a2, const T3& a3,
        const T4& a4)
{
    Composer c(fmt);
    c.AutoArg(a1);
    c.AutoArg(a2);
    c.AutoArg(a3);
    c.AutoArg(a4);
    return c.getStr();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
inline std::string
Compose(const llvm::StringRef& fmt, const T1& a1, const T2& a2, const T3& a3,
        const T4& a4, const T5& a5)
{
    Composer c(fmt);
    c.AutoArg(a1);
    c.AutoArg(a2);
    c.AutoArg(a3);
    c.AutoArg(a4);
    c.AutoArg(a5);
    return c.getStr();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6>
inline std::string
Compose(const llvm::StringRef& fmt, const T1& a1, const T2& a2, const T3& a3,
        const T4& a4, const T5& a5, const T6& a6)
{
    Composer c(fmt);
    c.AutoArg(a1);
    c.AutoArg(a2);
    c.AutoArg(a3);
    c.AutoArg(a4);
    c.AutoArg(a5);
    c.AutoArg(a6);
    return c.getStr();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7>
inline std::string
Compose(const llvm::StringRef& fmt, const T1& a1, const T2& a2, const T3& a3,
        const T4& a4, const T5& a5, const T6& a6, const T7& a7)
{
    Composer c(fmt);
    c.AutoArg(a1);
    c.AutoArg(a2);
    c.AutoArg(a3);
    c.AutoArg(a4);
    c.AutoArg(a5);
    c.AutoArg(a6);
    c.AutoArg(a7);
    return c.getStr();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8>
inline std::string
Compose(const llvm::StringRef& fmt, const T1& a1, const T2& a2, const T3& a3,
        const T4& a4, const T5& a5, const T6& a6, const T7& a7, const T8& a8)
{
    Composer c(fmt);
    c.AutoArg(a1);
    c.AutoArg(a2);
    c.AutoArg(a3);
    c.AutoArg(a4);
    c.AutoArg(a5);
    c.AutoArg(a6);
    c.AutoArg(a7);
    c.AutoArg(a8);
    return c.getStr();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9>
inline std::string
Compose(const llvm::StringRef& fmt, const T1& a1, const T2& a2, const T3& a3,
        const T4& a4, const T5& a5, const T6& a6, const T7& a7, const T8& a8,
        const T9& a9)
{
    Composer c(fmt);
    c.AutoArg(a1);
    c.AutoArg(a2);
    c.AutoArg(a3);
    c.AutoArg(a4);
    c.AutoArg(a5);
    c.AutoArg(a6);
    c.AutoArg(a7);
    c.AutoArg(a8);
    c.AutoArg(a9);
    return c.getStr();
}

template <typename T1>
inline std::string
Format(const T1& a1)
{
    std::string s;
    llvm::raw_string_ostream os(s);
    os << a1;
    os.flush();
    return s;
}

template <typename T1, typename T2>
inline std::string
Format(const T1& a1, const T2& a2)
{
    std::string s;
    llvm::raw_string_ostream os(s);
    os << a1 << a2;
    os.flush();
    return s;
}

template <typename T1, typename T2, typename T3>
inline std::string
Format(const T1& a1, const T2& a2, const T3& a3)
{
    std::string s;
    llvm::raw_string_ostream os(s);
    os << a1 << a2 << a3;
    os.flush();
    return s;
}

template <typename T1, typename T2, typename T3, typename T4>
inline std::string
Format(const T1& a1, const T2& a2, const T3& a3, const T4& a4)
{
    std::string s;
    llvm::raw_string_ostream os(s);
    os << a1 << a2 << a3 << a4;
    os.flush();
    return s;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
inline std::string
Format(const T1& a1, const T2& a2, const T3& a3, const T4& a4, const T5& a5)
{
    std::string s;
    llvm::raw_string_ostream os(s);
    os << a1 << a2 << a3 << a4 << a5;
    os.flush();
    return s;
}

} // namespace String

#endif

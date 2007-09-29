#ifndef YASM_COMPOSE_H
#define YASM_COMPOSE_H
///
/// @file libyasm/compose.h
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
#include <cctype>
#include <sstream>
#include <string>

namespace String {

class compose {
public:
    // initialize and prepare format string on the form "text %1 text %2 etc."
    compose(const std::string& fmt)
        : m_fmt(fmt), m_arg(0)
    {
        m_arg_pos[0] = 0;
    }

    // supply an replacement argument starting from %1
    template <typename T>
    compose& operator%(const T &obj);

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

// implementation of class Composition
template <typename T>
inline compose&
compose::operator%(const T &obj)
{
    m_arg_pos[m_arg] = m_os.tellp();
    m_os << obj;
    std::string::size_type newpos = m_os.tellp();

    // manipulators don't produce output
    // this is imperfect as empty string arguments won't advance arg
    if (newpos != m_arg_pos[m_arg])
        ++m_arg;

    return *this;
}

inline std::ostream&
operator<< (std::ostream& os, const compose& c)
{
    os << c.str();
    return os;
}

} // namespace String

#endif

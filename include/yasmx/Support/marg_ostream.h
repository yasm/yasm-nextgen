#ifndef YASM_MARG_OSTREAM_H
#define YASM_MARG_OSTREAM_H
///
/// @file
/// @brief Output stream with indentation support.
///
/// @license
///  Copyright (C) 2008  Peter Johnson
///  Copyright (C) 2004  Larry Evans
///
/// Based on sandbox/boost/io/filters/marg_ostream.hpp and kin
/// (http://svn.boost.org/trac/boost/browser/sandbox/boost/io/filters).
/// This code was dramatically simplified and shortened from that much
/// more generic heavily templated implementation by Peter Johnson.
/// Any bugs are mine.
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
#include "yasmx/Support/marg_ostream_fwd.h"

#include <ostream>

namespace yasm
{

namespace detail
{

// Adapt a basic_marg_ostream to ostreambuf interface.
template <typename CharType, typename TraitsType=std::char_traits<CharType> >
class marg_ostreambuf : public std::basic_streambuf<CharType, TraitsType>
{
public:
    typedef typename TraitsType::int_type int_type;
    typedef basic_marg_ostream<CharType, TraitsType> upstream_type;

    marg_ostreambuf(upstream_type* a_upstream)
      : m_upstream(a_upstream)
    {
        //so all xsputn char's passed to overflow immediately.
        std::basic_streambuf<CharType, TraitsType>::setbuf(0,0);
    }

    int_type
    overflow(int_type i=TraitsType::eof())
    {
        return m_upstream->pumpout_fluid(CharType(i));
    }

private:
    upstream_type* m_upstream;
};

} // namespace detail

// Purpose:
//  Adapt OutputPipeline to appear as a ostream
template <typename CharType, typename TraitsType>
class basic_marg_ostream
  : private detail::marg_ostreambuf<CharType, TraitsType>,  // adapt ostreambuf
    public std::basic_ostream<CharType, TraitsType>         // and ostream
{
public:
    typedef typename TraitsType::int_type int_type;
    typedef std::basic_streambuf<CharType, TraitsType> sink_type;
    typedef std::basic_string<CharType, TraitsType> string_type;
    typedef std::basic_ostream<CharType, TraitsType> ostream_type;
    typedef detail::marg_ostreambuf<CharType, TraitsType> ostreambuf_type;
    typedef basic_marg_ostream<CharType, TraitsType> my_type;

    basic_marg_ostream(sink_type* a_sink)
      : ostreambuf_type(this),
        ostream_type(static_cast<ostreambuf_type*>(this)),
        m_sink(a_sink),
        m_bol(false),
        m_width(0),
        m_delta(2)
    {
    }

    ostream_type&
    as_super_ostream()
    {
        return *this;
    }

    void
    attach_sink(std::streambuf* a_sink=0)
    {
        m_sink = a_sink;
    }

    bool
    attached(void) const
    {
        return m_sink != 0;
    }

    unsigned int
    get_width(void) const
    {
        return m_width;
    }

    unsigned int
    get_delta(void) const
    {
        return m_delta;
    }

    // set margin delta
    const my_type&
    put_delta(unsigned int a_delta)
    {
        m_delta = a_delta;
        return *this;
    }

    // set margin width
    const my_type&
    put_width(unsigned int a_width)
    {
        m_width = a_width;
        return *this;
    }

    // increment width
    const my_type&
    operator+=(int w)
    {
        if (w > 0)
            m_width += w;
        else if (static_cast<unsigned int>(-w) < m_width)
            m_width -= static_cast<unsigned int>(-w);
        else
            m_width = 0;
        return *this;
    }

    // decrement width
    const my_type&
    operator-=(int w)
    {
        return operator+=(-w);
    }

    const my_type&
    operator++(void)
    {
        return operator+=(m_delta);
    }

    const my_type&
    operator--(void)
    {
        return operator-=(m_delta);
    }

    // Indicates whether at the "Beginning-Of-Line"
    bool
    bol(void) const
    {
        return m_bol;
    }

    int_type
    pumpout_fluid(CharType c)
    {
        if(m_bol)
        {
            for (unsigned int i=0; i<m_width; ++i)
                m_sink->sputc(' ');
        }
        m_bol = (c == '\n');
        int_type l_result = m_sink->sputc(c);
        return l_result;
    }

    //  Make super_insert available to allow user coding of any
    //  operator<<(my_type&, T) for which there's a corresponding
    //  operator<<(ostream_type&, T) or
    //  ostream_type::operator<<(T):
    template<typename T>
    my_type&
    super_insert(T n)
    {
        as_super_ostream()<<n;
        return *this;
    }

    my_type& operator<<(my_type& (*n)(my_type&))
    {
        return n(*this);
    }

    //  Provide my_type::operator<<(T) for each
    //  ostream_type::operator<<(T) or
    //  operator<<(ostream_type&, T):

    typedef ostream_type& (*super_io_manip_type)(ostream_type&);

    my_type& operator<<(super_io_manip_type n){return super_insert(n);}
    my_type& operator<<(bool n){return super_insert(n);}
    my_type& operator<<(short n){return super_insert(n);}
    my_type& operator<<(unsigned short n){return super_insert(n);}
    my_type& operator<<(int n){return super_insert(n);}
    my_type& operator<<(unsigned int n){return super_insert(n);}
    my_type& operator<<(long n){return super_insert(n);}
    my_type& operator<<(unsigned long n){return super_insert(n);}
    my_type& operator<<(float n){return super_insert(n);}
    my_type& operator<<(double n){return super_insert(n);}
    my_type& operator<<(long double n){return super_insert(n);}
    my_type& operator<<(void* n){return super_insert(n);}
    my_type& operator<<(const char* n){return super_insert(n);}
    my_type& operator<<(const unsigned char* n){return super_insert(n);}
    my_type& operator<<(char n){return super_insert(n);}
    my_type& operator<<(const string_type& n){return super_insert(n);}

private:
    sink_type* m_sink;
    // records whether last character forwarded was an end-of-line.
    bool m_bol;
    // margin width
    unsigned int m_width;
    // amount m_width changed by operator{++,--}
    unsigned int m_delta;
};

} // namespace yasm

#endif

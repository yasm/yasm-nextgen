#ifndef YASM_SCOPED_ARRAY_H
#define YASM_SCOPED_ARRAY_H

//  (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
//  Copyright (c) 2001, 2002 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  http://www.boost.org/libs/smart_ptr/scoped_array.htm
//

#include <assert.h>
#include <cstddef>            // for std::ptrdiff_t

namespace util
{

template<class T> inline void checked_array_delete(T * x)
{
    typedef char type_must_be_complete[ sizeof(T)? 1: -1 ];
    (void) sizeof(type_must_be_complete);
    delete [] x;
}

//  scoped_array extends scoped_ptr to arrays. Deletion of the array pointed to
//  is guaranteed, either on destruction of the scoped_array or via an explicit
//  reset(). Use shared_array or std::vector if your needs are more complex.

template<class T> class scoped_array // noncopyable
{
private:

    T * ptr;

    scoped_array(scoped_array const &);
    scoped_array & operator=(scoped_array const &);

    typedef scoped_array<T> this_type;

public:

    typedef T element_type;

    explicit scoped_array(T * p = 0) : ptr(p) // never throws
    {
    }

    ~scoped_array() // never throws
    {
        checked_array_delete(ptr);
    }

    void reset(T * p = 0) // never throws
    {
        assert(p == 0 || p != ptr); // catch self-reset errors
        this_type(p).swap(*this);
    }

    T & operator[](std::ptrdiff_t i) const // never throws
    {
        assert(ptr != 0);
        assert(i >= 0);
        return ptr[i];
    }

    T * get() const // never throws
    {
        return ptr;
    }

    // implicit conversion to "bool"
    typedef T * this_type::*unspecified_bool_type;

    operator unspecified_bool_type() const // never throws
    {
        return ptr == 0? 0: &this_type::ptr;
    }

    bool operator! () const // never throws
    {
        return ptr == 0;
    }

    void swap(scoped_array & b) // never throws
    {
        T * tmp = b.ptr;
        b.ptr = ptr;
        ptr = tmp;
    }

};

template<class T> inline void swap(scoped_array<T> & a, scoped_array<T> & b) // never throws
{
    a.swap(b);
}

} // namespace util

#endif

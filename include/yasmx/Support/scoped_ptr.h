#ifndef YASM_SCOPED_PTR_H
#define YASM_SCOPED_PTR_H

//  (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
//  Copyright (c) 2001, 2002 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  http://www.boost.org/libs/smart_ptr/scoped_ptr.htm
//

#include <assert.h>
#include <memory>           // for std::auto_ptr

namespace util
{

template<class T> inline void checked_delete(T * x)
{
    // intentionally complex - simplification causes regressions
    typedef char type_must_be_complete[ sizeof(T)? 1: -1 ];
    (void) sizeof(type_must_be_complete);
    delete x;
}

//  scoped_ptr mimics a built-in pointer except that it guarantees deletion
//  of the object pointed to, either on destruction of the scoped_ptr or via
//  an explicit reset(). scoped_ptr is a simple solution for simple needs;
//  use shared_ptr or std::auto_ptr if your needs are more complex.

template<class T> class scoped_ptr // noncopyable
{
private:

    T * ptr;

    scoped_ptr(scoped_ptr const &);
    scoped_ptr & operator=(scoped_ptr const &);

    typedef scoped_ptr<T> this_type;

public:

    typedef T element_type;

    explicit scoped_ptr(T * p = 0): ptr(p) // never throws
    {
    }

    explicit scoped_ptr(std::auto_ptr<T> p): ptr(p.release()) // never throws
    {
    }

    ~scoped_ptr() // never throws
    {
        checked_delete(ptr);
    }

    void reset(T * p = 0) // never throws
    {
        assert(p == 0 || p != ptr); // catch self-reset errors
        this_type(p).swap(*this);
    }

    T & operator*() const // never throws
    {
        assert(ptr != 0);
        return *ptr;
    }

    T * operator->() const // never throws
    {
        assert(ptr != 0);
        return ptr;
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

    void swap(scoped_ptr & b) // never throws
    {
        T * tmp = b.ptr;
        b.ptr = ptr;
        ptr = tmp;
    }
};

template<class T> inline void swap(scoped_ptr<T> & a, scoped_ptr<T> & b) // never throws
{
    a.swap(b);
}

} // namespace util

#endif

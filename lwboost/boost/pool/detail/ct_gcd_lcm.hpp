// Copyright (C) 2000 Stephen Cleary
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org for updates, documentation, and revision history.

#ifndef BOOST_POOL_CT_GCD_LCM_HPP
#define BOOST_POOL_CT_GCD_LCM_HPP

#include <boost/static_assert.hpp>

namespace boost {

namespace details {
namespace pool {

// Compile-time calculation of greatest common divisor and least common multiple

//
// ct_gcd is a compile-time algorithm that calculates the greatest common
//  divisor of two unsigned integers, using Euclid's algorithm.
//
// assumes: A != 0 && B != 0
//

namespace details {

template <int b1, int b2>
struct ice_eq
{
    static const bool value = (b1 == b2);
};

template <int b1, int b2>
struct ice_ne
{
    static const bool value = (b1 != b2);
};

template <unsigned A, unsigned B, bool Bis0>
struct ct_gcd_helper;
template <unsigned A, unsigned B>
struct ct_gcd_helper<A, B, false>
{
  static const unsigned A_mod_B_ = A % B;
  static const unsigned value =
      (::boost::details::pool::details::ct_gcd_helper<
        B, static_cast<unsigned>(A_mod_B_),
        ::boost::details::pool::details::ice_eq<A_mod_B_, 0>::value
        >::value);
};
template <unsigned A, unsigned B>
struct ct_gcd_helper<A, B, true>
{
  static const unsigned value = A;
};
} // namespace details

template <unsigned A, unsigned B>
struct ct_gcd
{
  BOOST_STATIC_ASSERT(A != 0 && B != 0);
  static const unsigned value =
      (::boost::details::pool::details::ct_gcd_helper<A, B, false>::value);
};

//
// ct_lcm is a compile-time algorithm that calculates the least common
//  multiple of two unsigned integers.
//
// assumes: A != 0 && B != 0
//
template <unsigned A, unsigned B>
struct ct_lcm
{
  static const unsigned value =
      (A / ::boost::details::pool::ct_gcd<A, B>::value * B);
};

} // namespace pool
} // namespace details

} // namespace boost

#endif

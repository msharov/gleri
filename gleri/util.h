// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#define __STDC_LIMIT_MACROS	// Global macros turning on library features
#include "config.h"		// Standard includes
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

/// Returns the number of elements in a static vector
template <typename T, size_t N> constexpr static inline size_t ArraySize (T(&)[N]) { return (N); }
/// Returns the end() for a static vector
template <typename T, size_t N> constexpr static inline T* ArrayEnd (T(&a)[N]) { return (&a[N]); }
/// Expands into a ptr,size expression for the given static vector; useful as link arguments.
#define ArrayBlock(v)	&(v)[0], ArraySize(v)
/// Expands into a begin,end expression for the given static vector; useful for algorithm arguments.
#define ArrayRange(v)	&(v)[0], ArrayEnd(v)

// Macros to expand macro values into a string
#define PP_STRINGIFY(x)		#x
#define PP_STRINGIFY_I(x)	PP_STRINGIFY(x)

#define PP_COUNT_ARGS(...)		\
    _PP_COUNT_ARGS_COUNT(,##__VA_ARGS__,\
			  63,62,61,60,	\
	59,58,57,56,55,54,53,52,51,50,	\
	49,48,47,46,45,44,43,42,41,40,	\
	39,38,37,36,35,34,33,32,31,30,	\
	29,28,27,26,25,24,23,22,21,20,	\
	19,18,17,16,15,14,13,12,11,10,	\
	 9, 8, 7, 6, 5, 4, 3, 2, 1, 0	\
    )
#define _PP_COUNT_ARGS_COUNT(			\
				a63,a62,a61,a60,\
	a59,a58,a57,a56,a55,a54,a53,a52,a51,a50,\
	a49,a48,a47,a46,a45,a44,a43,a42,a41,a40,\
	a39,a38,a37,a36,a35,a34,a33,a32,a31,a30,\
	a29,a28,a27,a26,a25,a24,a23,a22,a21,a20,\
	a19,a18,a17,a16,a15,a14,a13,a12,a11,a10,\
	a09,a08,a07,a06,a05,a04,a03,a02,a01,a00,\
	n,...)					n

namespace {
// Endian-dependent stuff
#if USTL_BYTE_ORDER == USTL_LITTLE_ENDIAN
constexpr static inline uint16_t vpack2 (uint8_t a, uint8_t b)
    { return ((uint16_t(b)<<8)|a); }
constexpr static inline uint32_t vpack4 (uint16_t a, uint16_t b)
    { return ((uint32_t(b)<<16)|a); }
static inline void vunpack2 (uint16_t v, uint8_t& a, uint8_t& b)
    { a = v; b = v>>8; }
static inline void vunpack4 (uint32_t v, uint16_t& a, uint16_t& b)
    { a = v; b = v>>16; }
static inline void vunpack4 (uint32_t v, uint8_t& a, uint8_t& b, uint8_t& c, uint8_t& d)
    { a = v; b = v>>8; c = v>>16; d = v>>24; }
constexpr static inline uint16_t bole_swap2 (uint16_t v)
    { return (v); }
constexpr static inline uint32_t bole_swap4 (uint32_t v)
    { return (v); }
constexpr static inline uint64_t bole_swap8 (uint64_t v)
    { return (v); }
#else
constexpr static inline uint16_t vpack2 (uint8_t a, uint8_t b)
    { return ((uint16_t(a)<<8)|b); }
constexpr static inline uint32_t vpack4 (uint16_t a, uint16_t b)
    { return ((uint32_t(a)<<16)|b); }
static inline void vunpack2 (uint16_t v, uint8_t& a, uint8_t& b)
    { b = v; a = v>>8; }
static inline void vunpack4 (uint32_t v, uint16_t& a, uint16_t& b)
    { b = v; a = v>>16; }
static inline void vunpack4 (uint32_t v, uint8_t& a, uint8_t& b, uint8_t& c, uint8_t& d)
    { d = v; c = v>>8; b = v>>16; a = v>>24; }
constexpr static inline uint16_t bole_swap2 (uint16_t v)
    { return (v>>8|v<<8); }
constexpr static inline uint32_t bole_swap4 (uint32_t v)
    { return (uint32_t(bole_swap2(v))<<16|bole_swap2(v>>16)); }
constexpr static inline uint64_t bole_swap8 (uint64_t v)
    { return (uint64_t(bole_swap4(v))<<32|bole_swap4(v>>32)); }
#endif
constexpr static inline uint32_t vpack4 (uint8_t a, uint8_t b, uint8_t c, uint8_t d)
    { return (vpack4(vpack2(a,b),vpack2(c,d))); }
} // namespace

inline const char* strnext (const char* s, unsigned& n)
{
#if __i386__ || __x86_64__
    if (!__builtin_constant_p(strlen(s)))
	asm("repnz\tscasb":"+D"(s),"+c"(n):"a"('\0'));
    else
#endif
	s+=strlen(s)+1;
    return (s);
}

#if USE_USTL
template <typename T> struct remove_const { typedef T type; };
template <typename T> struct remove_const<const T> { typedef T type; };
template <typename T> struct remove_pointer { typedef T type; };
template <typename T> struct remove_pointer<T*> { typedef typename remove_const<T>::type type; };
#endif

/// Dereferencing iterator for containers of pointers
template <typename I>
class dereferencing_iterator {
    I _i;
public:
#if !USE_USTL
    typedef typename I::iterator_category iterator_category;
#endif
    typedef I				iterator_type;
    typedef typename iterator_traits<iterator_type>::difference_type difference_type;
    typedef typename iterator_traits<iterator_type>::value_type pointer;
    typedef typename remove_pointer<pointer>::type value_type;
    typedef const value_type*		const_pointer;
    typedef value_type&			reference;
    typedef const value_type&		const_reference;
public:
    inline				dereferencing_iterator (iterator_type i) :_i(i) {}
    inline iterator_type		base (void)		{ return (_i); }
    inline const iterator_type&		base (void) const	{ return (_i); }
    inline bool				operator== (const dereferencing_iterator& i) const { return (base() == i.base()); }
    inline bool				operator!= (const dereferencing_iterator& i) const { return (base() != i.base()); }
    inline bool				operator< (const dereferencing_iterator& i) const { return (base() < i.base()); }
    inline reference			operator* (void)	{ return (**_i); }
    inline const_reference		operator* (void) const	{ return (**_i); }
    inline pointer			operator-> (void)	{ return (*_i); }
    inline const_pointer		operator-> (void) const	{ return (*_i); }
    inline dereferencing_iterator&	operator++ (void)	{ ++_i; return (*this); }
    inline dereferencing_iterator&	operator-- (void)	{ --_i; return (*this); }
    inline dereferencing_iterator&	operator+= (int n)	{ _i += n; return (*this); }
    inline dereferencing_iterator&	operator-= (int n)	{ _i -= n; return (*this); }
    inline dereferencing_iterator	operator+ (int n) const	{ return (dereferencing_iterator(*this) += n); }
    inline dereferencing_iterator	operator- (int n) const	{ return (dereferencing_iterator(*this) -= n); }
    inline difference_type		operator- (const dereferencing_iterator& i)	{ return (base()-i.base()); }
};

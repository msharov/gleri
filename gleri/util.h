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

namespace {

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

} // namespace

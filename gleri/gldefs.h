// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "config.h"

//----------------------------------------------------------------------
// OpenGL constants

namespace G {

enum EType : uint16_t {
    BYTE = 0x1400,
    UNSIGNED_BYTE,
    SHORT,
    UNSIGNED_SHORT,
    INT,
    UNSIGNED_INT,
    FLOAT,
    BYTES2,
    BYTES3,
    BYTES4,
    DOUBLE
};

enum EBufferType : uint16_t {
    ARRAY_BUFFER		= 0x8892,
    ELEMENT_ARRAY_BUFFER,
    PIXEL_PACK_BUFFER		= 0x88EB,
    PIXEL_UNPACK_BUFFER,
    ATOMIC_COUNTER_BUFFER	= 0x92C0,
    COPY_READ_BUFFER		= 0x8F36,
    COPY_WRITE_BUFFER,
    DISPATCH_INDIRECT_BUFFER	= 0x90EE,
    DRAW_INDIRECT_BUFFER	= 0x8F3F,
    SHADER_STORAGE_BUFFER	= 0x90D2,
    TEXTURE_BUFFER		= 0x8C2A,
    TRANSFORM_FEEDBACK_BUFFER	= 0x8C8E,
    UNIFORM_BUFFER		= 0x8A11
};

enum EShape : uint32_t {
    POINTS,
    LINES,
    LINE_LOOP,
    LINE_STRIP,
    TRIANGLES,
    TRIANGLE_STRIP,
    TRIANGLE_FAN,
    LINES_ADJACENCY = 0xA,
    LINE_STRIP_ADJACENCY,
    TRIANGLES_ADJACENCY,
    TRIANGLE_STRIP_ADJACENCY,
    PATCHES
};

enum EBufferHint : uint16_t {
    STREAM_DRAW = 0x88E0,
    STREAM_READ,
    STREAM_COPY,
    STATIC_DRAW = 0x88E4,
    STATIC_READ,
    STATIC_COPY,
    DYNAMIC_DRAW = 0x88E8,
    DYNAMIC_READ,
    DYNAMIC_COPY
};

enum EStdParameter : uint8_t {
    VERTEX,
    TEXTURE_COORD,
    TEXT_DATA
};

enum class EResource : uint16_t {
    DATAPAK,
    SHADER,
    TEXTURE,
    FONT,
    BUFFER_VERTEX		= ARRAY_BUFFER,
    BUFFER_INDEX		= ELEMENT_ARRAY_BUFFER,
    BUFFER_PIXEL_PACK		= PIXEL_PACK_BUFFER,
    BUFFER_PIXEL_UNPACK		= PIXEL_UNPACK_BUFFER,
    BUFFER_ATOMIC_COUNTER	= ATOMIC_COUNTER_BUFFER,
    BUFFER_COPY_READ		= COPY_READ_BUFFER,
    BUFFER_COPY_WRITE		= COPY_WRITE_BUFFER,
    BUFFER_DISPATCH_INDIRECT	= DISPATCH_INDIRECT_BUFFER,
    BUFFER_DRAW_INDIRECT	= DRAW_INDIRECT_BUFFER,
    BUFFER_SHADER_STORAGE	= SHADER_STORAGE_BUFFER,
    BUFFER_TEXTURE		= TEXTURE_BUFFER,
    BUFFER_TRANSFORM_FEEDBACK	= TRANSFORM_FEEDBACK_BUFFER,
    BUFFER_UNIFORM		= UNIFORM_BUFFER
};

} // namespace G

//----------------------------------------------------------------------
// Exceptions

class XError {
public:
    template <typename... T>
    inline explicit	XError (const char* fmt, T... args) noexcept __attribute__((__format__(__printf__,2,0)));
    inline		XError (bool, char*& msg)	:_msg(msg) { msg = nullptr; }
    inline		~XError (void) noexcept		{ free (_msg); }
    inline const char*	what (void) const noexcept	{ return (_msg); }
private:
    char*		_msg;
};

template <typename... T>
inline XError::XError (const char* fmt, T... args) noexcept
    { asprintf (&_msg, fmt, args...); }

//----------------------------------------------------------------------
// Utility functions

void hexdump (const void* pv, size_t n);

template <typename Ctr, typename Condition>
inline void erase_if (Ctr& v, Condition f)
{
    for (typename Ctr::iterator i = v.begin(); i != v.end(); ++i)
	if (f(*i))
	    --(i = v.erase(i));
}

/// Creates uint32_t color value from components
inline constexpr uint32_t RGBA (uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    { return (vpack4(r,g,b,a)); }

/// Creates uint32_t color value from components
inline static uint32_t ARGB (uint32_t c)
{
#if __x86_64__
    if (!__builtin_constant_p(c)) asm("rol\t$8,%0":"+r"(c)); else
#endif
	c = (c<<8)|(c>>24);
    return (__builtin_bswap32(c));
}

/// Creates uint32_t color value from components
inline constexpr uint32_t RGB (uint8_t r, uint8_t g, uint8_t b)
    { return (RGBA(r,g,b,UINT8_MAX)); }

/// Creates uint32_t color value from components
inline static uint32_t RGB (uint32_t c)
#if USTL_BYTE_ORDER == USTL_LITTLE_ENDIAN
    { return (ARGB((UINT8_MAX<<24)|c)); }
#else
    { return (ARGB(UINT8_MAX|c)); }
#endif

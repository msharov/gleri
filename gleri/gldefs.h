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
    ARRAY_BUFFER = 0x8892,
    ELEMENT_ARRAY_BUFFER
};

enum EPrimitive : uint32_t {
    POINTS,
    LINES,
    LINE_LOOP,
    LINE_STRIP,
    TRIANGLES,
    TRIANGLE_STRIP,
    TRIANGLE_FAN
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
    TEXTURE_COORD
};

} // namespace G

//----------------------------------------------------------------------
// Exceptions

class XError {
public:
    template <typename... T>
    inline explicit	XError (const char* fmt, T... args) noexcept __attribute__((__format__(__printf__,1,2)));
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

namespace {

/// Creates uint32_t color value from components
inline constexpr uint32_t RGBA (uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    { return ((a<<24)|(b<<16)|(g<<8)|r); }

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
    { return (ARGB((UINT8_MAX<<24)|c)); }

} // namespace

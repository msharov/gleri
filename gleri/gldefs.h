// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "util.h"

//----------------------------------------------------------------------
// OpenGL constants

namespace G {

typedef uint32_t	goid_t;
typedef int16_t		coord_t;
typedef uint16_t	dim_t;
typedef uint32_t	color_t;

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

enum EFeature : uint16_t {
    CAP_BLEND,
    CAP_CULL_FACE,
    CAP_DEPTH_CLAMP,
    CAP_DEPTH_TEST,
    CAP_MULTISAMPLE,
    CAP_N
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

enum EDefaultResource : goid_t {
    default_FlatShader,
    default_TextureShader,
    default_FontShader,
    default_Font,
    default_Resources
};

namespace Pixel {
enum EComp : uint16_t {
    RED		= 0x1903,
    RG		= 0x8227,
    RGB		= 0x1907,
    BGR		= 0x80e0,
    RGBA	= 0x1908,
    BGRA	= 0x80e1
};
enum EFmt : uint16_t {
    BYTE = 0x1400,
    UNSIGNED_BYTE,
    SHORT,
    UNSIGNED_SHORT,
    INT,
    UNSIGNED_INT,
    FLOAT,
    UNSIGNED_BYTE_3_3_2 = 0x8032,
    UNSIGNED_SHORT_4_4_4_4,
    UNSIGNED_SHORT_5_5_5_1,
    UNSIGNED_INT_8_8_8_8,
    UNSIGNED_INT_10_10_10_2,
    UNSIGNED_BYTE_2_3_3_REV = 0x8362,
    UNSIGNED_SHORT_5_6_5,
    UNSIGNED_SHORT_5_6_5_REV,
    UNSIGNED_SHORT_4_4_4_4_REV,
    UNSIGNED_SHORT_1_5_5_5_REV,
    UNSIGNED_INT_8_8_8_8_REV,
    UNSIGNED_INT_2_10_10_10_REV
};
} // namespace G::Pixel

struct alignas(8) STextureHeader {
    enum { Magic = vpack4('G','L','T','X') };
    uint32_t		magic,w;
    uint16_t		h,d;
    G::Pixel::EComp	comp;
    G::Pixel::EFmt	fmt;
};

struct alignas(4) SWinInfo {
    coord_t	x,y;
    dim_t	w,h;
    uint16_t	parent;
    uint8_t	mingl,maxgl;
    uint8_t	aa;
    enum EWinType : uint8_t {
	type_Normal,
	type_Desktop,
	type_Dock,
	type_Dialog,
	type_Toolbar,
	type_Utility,
	type_Menu,
	type_PopupMenu,
	type_DropdownMenu,
	type_ComboMenu,
	type_Notification,
	type_Tooltip,
	type_Splash,
	type_Dragged,
	type_FirstParented = type_Dialog,
	type_FirstDecoless = type_PopupMenu,
	type_LastParented = type_Splash,
	type_LastDecoless = type_Dragged
    }		wtype;
    enum EWinState : uint8_t {
	state_Normal,
	state_MaximizedX,
	state_MaximizedY,
	state_Maximized,
	state_Hidden,
	state_Fullscreen,
	state_Gamescreen
    }		wstate;
    enum EWinFlag : uint8_t {
	flag_None,
	flag_Modal		= (1<<0),
	flag_Attention		= (1<<1),
	flag_Focused		= (1<<2),
	flag_Sticky		= (1<<3),
	flag_NotOnTaskbar	= (1<<4),
	flag_NotOnPager		= (1<<5),
	flag_Above		= (1<<5),
	flag_Below		= (1<<7)
    };
    uint8_t	flags;
    inline bool	Parented (void) const	{ return (wtype >= type_FirstParented && wtype <= type_LastParented); }
    inline bool	Decoless (void) const	{ return (wtype >= type_FirstDecoless && wtype <= type_LastDecoless); }
};

// These are in rglp.cc
extern const char* TypeName (EType t) noexcept __attribute__((const));
extern const char* ShapeName (EShape s) noexcept __attribute__((const));

} // namespace G

//----------------------------------------------------------------------
// Exceptions

class XError {
public:
    template <typename... T>
    inline explicit	XError (const char* fmt, T... args) noexcept __attribute__((__format__(__printf__,2,0)));
    inline		XError (bool, char*& msg)	:_msg(msg) { msg = nullptr; }
    inline		XError (const XError& e)	:_msg(strdup(e.what())) {}
    inline		~XError (void) noexcept		{ free (_msg); }
    inline const char*	what (void) const noexcept	{ return (_msg); }
    static void		emit (const char* e) NORETURN	{ throw XError (e); }
private:
    char*		_msg;
};

template <typename... T>
inline XError::XError (const char* fmt, T... args) noexcept
    { asprintf (&_msg, fmt, args...); }

//----------------------------------------------------------------------
// Utility functions

void hexdump (const void* pv, size_t n) noexcept;

template <typename Ctr, typename Condition>
inline void erase_if (Ctr& v, Condition f)
{
    for (typename Ctr::iterator i = v.begin(); i != v.end(); ++i)
	if (f(*i))
	    --(i = v.erase(i));
}

/// Creates uint32_t color value from components
inline constexpr G::color_t RGBA (uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    { return (vpack4(r,g,b,a)); }

/// Creates uint32_t color value from components
inline static G::color_t ARGB (G::color_t c)
{
#if __x86_64__
    if (!__builtin_constant_p(c)) asm("rol\t$8,%0":"+r"(c)); else
#endif
	c = (c<<8)|(c>>24);
    return (__builtin_bswap32(c));
}

/// Creates uint32_t color value from components
inline constexpr G::color_t RGB (uint8_t r, uint8_t g, uint8_t b)
    { return (RGBA(r,g,b,UINT8_MAX)); }

/// Creates uint32_t color value from components
inline static G::color_t RGB (G::color_t c)
#if USTL_BYTE_ORDER == USTL_LITTLE_ENDIAN
    { return (ARGB((UINT8_MAX<<24)|c)); }
#else
    { return (ARGB(UINT8_MAX|c)); }
#endif

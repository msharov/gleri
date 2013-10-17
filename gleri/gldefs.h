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

enum : goid_t { GoidNull = numeric_limits<goid_t>::max() };

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
    param_Vertex,
    param_Color,
    param_TexCoord = param_Color
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
    default_GradientShader,
    default_TextureShader,
    default_FontShader,
    default_Font,
    default_ResourcePak,
    default_ResourceMaxId = 0x10000
};

namespace Pixel {
enum Fmt : uint16_t {
    // Core formats
    STENCIL_INDEX	= 0x1901,
    DEPTH_COMPONENT	= 0x1902,
    DEPTH_STENCIL	= 0x84f9,
    RED			= 0x1903,
    RG			= 0x8227,
    RGB			= 0x1907,
    BGR			= 0x80e0,
    RGBA		= 0x1908,
    BGRA		= 0x80e1,
    RED_INTEGER		= 0x8d94,
    RG_INTEGER		= 0x8228,
    RGB_INTEGER		= 0x8d98,
    BGR_INTEGER		= 0x8d9a,
    RGBA_INTEGER	= 0x8d99,
    BGRA_INTEGER	= 0x8d9b,
    //{{{ Sized formats
    // Red
    R8			= 0x8229,
    R8I			= 0x8231,
    R8UI		= 0x8232,
    R8_SNORM		= 0x8F94,
    R3_G3_B2		= 0x2A10,
    R11F_G11F_B10F	= 0x8C3A,
    R16			= 0x822A,
    R16F		= 0x822D,
    R16I		= 0x8233,
    R16UI		= 0x8234,
    R16_SNORM		= 0x8F98,
    R32F		= 0x822E,
    R32I		= 0x8235,
    R32UI		= 0x8236,
    // RG
    RG8			= 0x822B,
    RG8I		= 0x8237,
    RG8UI		= 0x8238,
    RG8_SNORM		= 0x8F95,
    RG16		= 0x822C,
    RG16F		= 0x822F,
    RG16I		= 0x8239,
    RG16UI		= 0x823A,
    RG16_SNORM		= 0x8F99,
    RG32F		= 0x8230,
    RG32I		= 0x823B,
    RG32UI		= 0x823C,
    // RGB
    RGB4		= 0x804F,
    RGB5		= 0x8050,
    RGB8		= 0x8051,
    RGB10		= 0x8052,
    RGB12		= 0x8053,
    RGB16		= 0x8054,
    RGB8I		= 0x8D8F,
    RGB8UI		= 0x8D7D,
    RGB8_SNORM		= 0x8F96,
    RGB9_E5		= 0x8C3D,
    RGB10_A2UI		= 0x906F,
    RGB16F		= 0x881B,
    RGB16I		= 0x8D89,
    RGB16UI		= 0x8D77,
    RGB16_SNORM		= 0x8F9A,
    RGB565		= 0x8D62,
    RGB32F		= 0x8815,
    RGB32I		= 0x8D83,
    RGB32UI		= 0x8D71,
    // RGBA
    RGBA2		= 0x8055,
    RGBA4		= 0x8056,
    RGBA8		= 0x8058,
    RGBA12		= 0x805A,
    RGBA16		= 0x805B,
    RGBA8I		= 0x8D8E,
    RGBA8UI		= 0x8D7C,
    RGBA8_SNORM		= 0x8F97,
    RGB10_A2		= 0x8059,
    RGB5_A1		= 0x8057,
    RGBA16F		= 0x881A,
    RGBA16I		= 0x8D88,
    RGBA16UI		= 0x8D76,
    RGBA16_SNORM	= 0x8F9B,
    RGBA32F		= 0x8814,
    RGBA32I		= 0x8D82,
    RGBA32UI		= 0x8D70,
    SRGB		= 0x8C40,
    SRGB8		= 0x8C41,
    SRGB8_ALPHA8	= 0x8C43,
    SRGB_ALPHA		= 0x8C42,
    //}}}
    //{{{ Compressed formats
    // Generic compressed formats
    COMPRESSED_RED			= 0x8225,
    COMPRESSED_RG			= 0x8226,
    COMPRESSED_RGB			= 0x84ED,
    COMPRESSED_RGBA			= 0x84EE,
    COMPRESSED_SRGB			= 0x8C48,
    COMPRESSED_SRGB_ALPHA		= 0x8C49,
    COMPRESSED_RED_RGTC1		= 0x8DBB,
    COMPRESSED_SIGNED_RED_RGTC1		= 0x8DBC,
    COMPRESSED_RG_RGTC2			= 0x8DBD,
    COMPRESSED_SIGNED_RG_RGTC2		= 0x8DBE,
    COMPRESSED_RGBA_BPTC_UNORM		= 0x8E8C,
    COMPRESSED_SRGB_ALPHA_BPTC_UNORM	= 0x8E8D,
    COMPRESSED_RGB_BPTC_SIGNED_FLOAT	= 0x8E8E,
    COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT	= 0x8E8F,
    // ETC2 compression
    COMPRESSED_RGB8_ETC2		= 0x9274,
    COMPRESSED_SRGB8_ETC2		= 0x9275,
    COMPRESSED_RGBA8_ETC2_EAC		= 0x9278,
    COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 = 0x9276,
    COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 = 0x9277,
    COMPRESSED_R11_EAC			= 0x9270,
    COMPRESSED_SIGNED_R11_EAC		= 0x9271,
    COMPRESSED_SIGNED_RG11_EAC		= 0x9273,
    // 3DFX extensions
    COMPRESSED_RGB_FXT1_3DFX		= 0x86B0,
    COMPRESSED_RGBA_FXT1_3DFX		= 0x86B1,
    // S3TC extensions
    COMPRESSED_RGB_S3TC_DXT1_EXT	= 0x83F0,
    COMPRESSED_RGBA_S3TC_DXT1_EXT	= 0x83F1,
    COMPRESSED_RGBA_S3TC_DXT3_EXT	= 0x83F2,
    COMPRESSED_RGBA_S3TC_DXT5_EXT	= 0x83F3,
    COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT	= 0x8C4D,
    COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT	= 0x8C4E,
    COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT	= 0x8C4F,
    COMPRESSED_SRGB_S3TC_DXT1_EXT	= 0x8C4C,
    //}}}
};

enum Comp : uint16_t {
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

namespace Texture {
enum Type : uint16_t {
    TEXTURE_2D	= 0x0DE1
};
enum Parameter : uint16_t {
    MAG_FILTER,
    MIN_FILTER,
    NPARAMS
};
enum Filter : uint16_t {
    NEAREST = 0x2600,
    LINEAR,
    NEAREST_MIPMAP_NEAREST = 0x2700,
    LINEAR_MIPMAP_NEAREST,
    NEAREST_MIPMAP_LINEAR,
    LINEAR_MIPMAP_LINEAR
};
} // namespace G::Texture

struct alignas(8) STextureHeader {
    enum { Magic = vpack4('G','L','T','X') };
    uint32_t		magic,w;
    uint16_t		h,d;
    G::Pixel::Fmt	fmt;
    G::Pixel::Comp	comp;
};

struct SFontInfo {
    inline dim_t	Height (void) const		{ return (h); }
    inline dim_t	Width (void) const		{ return (w); }
    inline dim_t	Width (const char* s) const	{ return (Width()*strlen(s)); }
    dim_t		w,h;
};

struct alignas(4) SWinInfo {
    coord_t	x,y;
    dim_t	w,h;
    uint16_t	parent;
    uint8_t	mingl,maxgl;
    enum EMSAA : uint8_t {
	MSAA_OFF,
	MSAA_2X,
	MSAA_4X,
	MSAA_8X,
	MSAA_16X,
	MSAA_MAX = MSAA_16X,
    }		aa;
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
	type_FirstPopupMenu = type_PopupMenu,
	type_LastParented = type_Splash,
	type_LastDecoless = type_Dragged,
	type_LastPopupMenu = type_ComboMenu
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
	flag_Above		= (1<<6),
	flag_Below		= (1<<7)
    };
    uint8_t	flags;
    inline bool	Parented (void) const	{ return (wtype >= type_FirstParented && wtype <= type_LastParented); }
    inline bool	Decoless (void) const	{ return (wtype >= type_FirstDecoless && wtype <= type_LastDecoless); }
    inline bool	PopupMenu (void) const	{ return (wtype >= type_FirstPopupMenu && wtype <= type_LastPopupMenu); }
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
inline constexpr G::color_t RGBA (G::color_t c)
    { return (__builtin_bswap32(c)); }

/// Creates uint32_t color value from components
inline static G::color_t ARGB (G::color_t c)
{
#if __x86_64__
    if (!__builtin_constant_p(c)) asm("rol\t$8,%0":"+r"(c)); else
#endif
	c = (c<<8)|(c>>24);
    return (RGBA(c));
}

/// Creates uint32_t color value from components
inline constexpr G::color_t RGB (uint8_t r, uint8_t g, uint8_t b)
    { return (RGBA(r,g,b,UINT8_MAX)); }
inline constexpr G::color_t RGB (G::color_t c)
    { return (RGBA((c<<8)|UINT8_MAX)); }

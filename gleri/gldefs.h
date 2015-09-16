// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "bstr.h"

//{{{ OpenGL constants ----------------------------------------------------

namespace G {

using goid_t	= uint32_t;
using coord_t	= int16_t;
using dim_t	= uint16_t;
using color_t	= uint32_t;

enum Type : uint16_t {
    Type_BASE = 0x1400,
    BYTE = Type_BASE,
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

enum BufferType : uint16_t {
    ARRAY_BUFFER,
    ELEMENT_ARRAY_BUFFER,
    PIXEL_PACK_BUFFER,
    PIXEL_UNPACK_BUFFER,
    ATOMIC_COUNTER_BUFFER,
    COPY_READ_BUFFER,
    COPY_WRITE_BUFFER,
    DISPATCH_INDIRECT_BUFFER,
    DRAW_INDIRECT_BUFFER,
    SHADER_STORAGE_BUFFER,
    TEXTURE_BUFFER,
    TRANSFORM_FEEDBACK_BUFFER,
    UNIFORM_BUFFER
};

enum TextureType : uint16_t {
    TEXTURE_1D,
    TEXTURE_2D,
    TEXTURE_2D_MULTISAMPLE,
    TEXTURE_RECTANGLE,
    TEXTURE_1D_ARRAY,
    TEXTURE_CUBE_MAP,
    TEXTURE_CUBE_MAP_ARRAY,
    TEXTURE_3D,
    TEXTURE_2D_ARRAY,
    TEXTURE_2D_MULTISAMPLE_ARRAY,
    TEXTURE_SAMPLER
};

enum FramebufferType : uint8_t {
    FRAMEBUFFER,
    DRAW_FRAMEBUFFER = FRAMEBUFFER,
    READ_FRAMEBUFFER
};

enum FramebufferAttachment : uint8_t {
    DEPTH_ATTACHMENT,
    STENCIL_ATTACHMENT,
    DEPTH_STENCIL_ATTACHMENT,
    COLOR_ATTACHMENT0,
    COLOR_ATTACHMENT1,
    COLOR_ATTACHMENT2,
    COLOR_ATTACHMENT3,
    COLOR_ATTACHMENT4,
    COLOR_ATTACHMENT5,
    COLOR_ATTACHMENT6,
    COLOR_ATTACHMENT7,
    COLOR_ATTACHMENT8,
    COLOR_ATTACHMENT9,
    COLOR_ATTACHMENT10,
    COLOR_ATTACHMENT11,
    COLOR_ATTACHMENT12,
    COLOR_ATTACHMENT13,
    COLOR_ATTACHMENT14,
    COLOR_ATTACHMENT15
};

enum Shape : uint32_t {
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

enum BufferHint : uint16_t {
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

enum StdParameter : uint8_t {
    param_Vertex,
    param_Color,
    param_TexCoord = param_Color
};

enum Feature : uint16_t {
    CAP_BLEND,
    CAP_CULL_FACE,
    CAP_DEPTH_CLAMP,
    CAP_DEPTH_TEST,
    CAP_MULTISAMPLE,
    CAP_N
};

enum DefaultResource : goid_t {
    GoidNull,
    default_Framebuffer,
    default_ResourcePak,
    default_FlatShader,
    default_GradientShader,
    default_TextureShader,
    default_FontShader,
    default_Font,
    default_ResourceMaxId = 0x10000
};

//}}}-------------------------------------------------------------------
//{{{ Pixel

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
    //{{{2 Sized formats
    // Depth
    DEPTH_COMPONENT16	= 0x81a5,
    DEPTH_COMPONENT24	= 0x81a6,
    DEPTH_COMPONENT32	= 0x81a7,
    DEPTH_COMPONENT32F	= 0x8cac,
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
    //}}}2
    //{{{2 Compressed formats
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
    //}}}2
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

uint16_t ComponentsPerPixel (Fmt fmt) noexcept CONST;
uint16_t ComponentSize (Comp comp) noexcept CONST;
size_t TextureSize (Fmt fmt, Comp comp, dim_t w, dim_t h) noexcept CONST;

} // namespace G::Pixel

//}}}-------------------------------------------------------------------
//{{{ Texture

namespace Texture {

enum Type : uint16_t {
    TEXTURE_1D			= 0x0de0,
    TEXTURE_2D			= 0x0de1,
    TEXTURE_2D_MULTISAMPLE	= 0x9100,
    TEXTURE_RECTANGLE		= 0x84f5,
    TEXTURE_1D_ARRAY		= 0x8c18,
    TEXTURE_CUBE_MAP		= 0x8513,
    TEXTURE_CUBE_MAP_POSITIVE_X	= 0x8515,
    TEXTURE_CUBE_MAP_NEGATIVE_X	= 0x8516,
    TEXTURE_CUBE_MAP_POSITIVE_Y	= 0x8517,
    TEXTURE_CUBE_MAP_NEGATIVE_Y	= 0x8518,
    TEXTURE_CUBE_MAP_POSITIVE_Z	= 0x8519,
    TEXTURE_CUBE_MAP_NEGATIVE_Z	= 0x851a,
    TEXTURE_CUBE_MAP_ARRAY	= 0x900b,
    TEXTURE_3D			= 0x806f,
    TEXTURE_2D_ARRAY		= 0x8c1a,
    TEXTURE_2D_MULTISAMPLE_ARRAY= 0x9102,
    TEXTURE_SAMPLER		= 0x8c2a
};

Type TypeFromTextureType (TextureType ttype) noexcept CONST;

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
struct alignas(8) Info {
    Type	type;
    dim_t	w,h,d;
    Pixel::Fmt	fmt;
    Pixel::Comp	comp;
    uint16_t	mipmaps;	// Mipmaps per image
    uint16_t	nImages;	// Number of images in the file
    dim_t	frameHeight;	// For animations, height of one frame
    uint16_t	delay;		//  and delay in ms between frames
    uint32_t	reserved1;
    uint32_t	reserved2;
    uint32_t	size;		// In info, total memory use. In file, size of first image, with other sizes following.
public:
    inline constexpr	Info (Type nt = TEXTURE_2D, dim_t nw = 0, dim_t nh = 0, dim_t nd = 0, Pixel::Fmt nfmt = Pixel::RGBA, Pixel::Comp ncomp = Pixel::UNSIGNED_BYTE)
	: type(nt),w(nw),h(nh),d(nd),fmt(nfmt),comp(ncomp),mipmaps(0),nImages(0),frameHeight(0),delay(0),reserved1(0),reserved2(0),size(0) {}
    inline void		read (bstri& is)	{ is.iread (*this); }
    inline void		write (bstro& os) const	{ os.iwrite (*this); }
    inline void		write (bstrs& ss) const	{ ss.iwrite (*this); }
};
struct alignas(8) GLTXHeader {
    enum {
	Version,
	Magic = vpack4('G','L','T','X')
    };
    uint32_t	magic;
    uint16_t	version;
    uint16_t	dataOffset;
    Info	info;
    // List of sizes for images after the first follow
public:
    inline constexpr	GLTXHeader (void) : magic(Magic),version(Version),dataOffset(sizeof(GLTXHeader)),info() {}
};
enum class Format : uint8_t {
    GLTX,
    JPEG,
    PNG,
    GIF
};

} // namespace G::Texture

//}}}-------------------------------------------------------------------
//{{{ Font

namespace Font {

class CPMap {
protected:
    struct CodePageRange {
	uint8_t		first;
	uint8_t		last;
	uint16_t	offset;
    };
    using cpra_t	= CodePageRange [256];
public:
    class iterator {
    public:
			iterator (const cpra_t& c, uint16_t v)	:_cpra(c),_v(v) {}
	iterator&	operator++ (void) noexcept;
	uint16_t	operator* (void) const			{ return _v; }
	bool		operator== (const iterator& i) const	{ return _v == i._v; }
	bool		operator!= (const iterator& i) const	{ return _v != i._v; }
	bool		operator< (const iterator& i) const	{ return _v < i._v; }
    private:
	const cpra_t&	_cpra;
	uint16_t	_v;
    };
    using charmap_t	= vector<uint16_t>;
public:
    inline		CPMap (void)		{ memset (_cpra, 0, sizeof(_cpra)); }
    void		Create (const charmap_t& cm) noexcept;
    iterator		begin (void) const noexcept PURE;
    iterator		end (void) const noexcept PURE;
    size_t		size (void) const noexcept PURE;
    uint16_t		operator[] (uint16_t i) const noexcept PURE;
    inline void		read (bstri& is)	{ is.read (_cpra, sizeof(_cpra)); }
    inline void		write (bstro& os) const	{ os.write (_cpra, sizeof(_cpra)); }
    inline void		write (bstrs& ss) const	{ ss.write (_cpra, sizeof(_cpra)); }
private:
    cpra_t		_cpra;
};

union KerningPair {
    struct {
	int16_t		d;
	uint16_t	r;
	uint16_t	c2;
	uint16_t	c1;
    };
    uint64_t		v;
    inline bool		operator< (const KerningPair& k) const	{ return v < k.v; }
};

class Info {
public:
    enum class Style : uint16_t {
	Regular,
	Bold,
	Italic,
	BoldItalic
    };
public:
			Info (void);
			Info (dim_t w, dim_t h);
    const string&	Name (void) const		{ return _name; }
    inline Style	FontStyle (void) const		{ return _style; }
    inline dim_t	Height (void) const		{ return _h; }
    inline dim_t	Width (void) const		{ return _w; }
    inline uint8_t	Baseline (void) const		{ return _b; }
    dim_t		Width (uint16_t c) const noexcept PURE;
    dim_t		Width (const char* s) const noexcept;
    int16_t		Kerning (uint16_t c1, uint16_t c2) const noexcept PURE;
    inline bool		IsFixed (void) const		{ return _varw.empty(); }
    inline bool		HasKerning (void) const		{ return !_kp.empty(); }
    void		read (bstri& is);
    void		write (bstro& os) const;
    void		write (bstrs& ss) const;
protected:
    dim_t		_w,_h;
    dim_t		_b;
    Style		_style;
    CPMap		_cpmap;
    string		_name;
    vector<uint8_t>	_varw;
    vector<KerningPair>	_kp;
};

} // namespace G::Font
//}}}-------------------------------------------------------------------
//{{{ Framebuffer

class alignas(4) FramebufferComponent {
public:
    FramebufferType		target;
    FramebufferAttachment	attachment;
    uint8_t			textype;
    uint8_t			level;
    goid_t			texture;
public:
    inline constexpr		FramebufferComponent (void)
				    :target(FRAMEBUFFER),attachment(DEPTH_ATTACHMENT),textype(TEXTURE_1D),level(0),texture(0) {}
    inline constexpr		FramebufferComponent (FramebufferType _target, FramebufferAttachment _attachment, uint8_t _textype, uint8_t _level, goid_t _texture)
				    :target(_target),attachment(_attachment),textype(_textype),level(_level),texture(_texture) {}
    inline void			read (bstri& is)	{ is.iread (*this); }
    inline void			write (bstro& os) const	{ os.iwrite (*this); }
    inline void			write (bstrs& ss) const	{ ss.iwrite (*this); }
};

//}}}-------------------------------------------------------------------
//{{{ WinInfo

class alignas(4) WinInfo {
public:
    coord_t	x,y;
    dim_t	w,h;
    uint16_t	parent;		// The iid of the parent window
    uint8_t	mingl,maxgl;	// Format: 0x33. maxgl=0 means give me the highest available
    enum MSAA : uint8_t {
	MSAA_OFF,
	MSAA_2X,
	MSAA_4X,
	MSAA_8X,
	MSAA_16X,
	MSAA_MAX = MSAA_16X,
    }		aa;
    enum WinType : uint8_t {
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
	type_Embedded,
	type_FirstParented = type_Dialog,
	type_FirstDecoless = type_PopupMenu,
	type_FirstPopupMenu = type_PopupMenu,
	type_LastParented = type_Splash,
	type_LastDecoless = type_Dragged,
	type_LastPopupMenu = type_ComboMenu
    }		wtype;
    enum WinState : uint8_t {
	state_Normal,
	state_MaximizedX,
	state_MaximizedY,
	state_Maximized,
	state_Hidden,
	state_Fullscreen,
	state_Gamescreen	// Like fullscreen, but possibly change resolution to fit
    }		wstate;
    enum WinFlag : uint8_t {
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
    uint32_t	wmwid;
    dim_t	scrw;
    dim_t	scrh;
    dim_t	scrmw;
    dim_t	scrmh;
    uint8_t	dpyn;
    uint8_t	scrn;
    uint8_t	scrd;
    uint8_t	reserved;
public:
    inline constexpr explicit WinInfo (coord_t _x = 0, coord_t _y = 0, dim_t _w = 1, dim_t _h = 1,
				uint16_t _parent = 0, uint8_t _mingl = 0x33, uint8_t _maxgl = 0,
				MSAA _aa = MSAA_OFF, WinType _wtype = type_Normal, WinState _wstate = state_Normal, uint8_t _flags = flag_None)
				:x(_x),y(_y),w(_w),h(_h)
				,parent(_parent),mingl(_mingl),maxgl(_maxgl)
				,aa(_aa),wtype(_wtype),wstate(_wstate),flags(_flags)
				,wmwid(0),scrw(0),scrh(0),scrmw(0),scrmh(0),dpyn(0),scrn(0),scrd(0),reserved(0) {}
    inline void read (bstri& is)	{ is.iread (*this); }
    inline void write (bstro& os) const	{ os.iwrite (*this); }
    inline void write (bstrs& ss) const	{ ss.iwrite (*this); }
    inline bool	IsParented (void) const	{ return wtype >= type_FirstParented && wtype <= type_LastParented; }
    inline bool	IsDecoless (void) const	{ return wtype >= type_FirstDecoless && wtype <= type_LastDecoless; }
    inline bool	IsPopupMenu (void)const	{ return wtype >= type_FirstPopupMenu && wtype <= type_LastPopupMenu; }
};

const char* TypeName (Type t) noexcept __attribute__((const));
const char* ShapeName (Shape s) noexcept __attribute__((const));

#define GLERI_WININFO_SIGNATURE	"(nnqqqyyyyyyuqqqqyyyy)"

} // namespace G

//}}}-------------------------------------------------------------------
//{{{ Exceptions

class XError {
public:
    template <typename... T>
    inline explicit	XError (const char* fmt, T... args) noexcept __attribute__((__format__(__printf__,2,0)));
    inline		XError (bool, char*& msg)	:_msg(msg) { msg = nullptr; }
    inline		XError (const XError& e)	:_msg(strdup(e.what())) {}
    inline		~XError (void) noexcept		{ free (_msg); }
    inline const char*	what (void) const noexcept	{ return _msg; }
    static void		emit (const char* e) NORETURN	{ throw XError (e); }
private:
    char*		_msg;
};

template <typename... T>
inline XError::XError (const char* fmt, T... args) noexcept
    { asprintf (&_msg, fmt, args...); }

//}}}-------------------------------------------------------------------
//{{{ Utility functions

void hexdump (const void* pv, size_t n) noexcept;

template <typename Ctr, typename Condition>
inline void erase_if (Ctr& v, Condition f)
{
    for (auto i = v.begin(); i != v.end(); ++i)
	if (f(*i))
	    --(i = v.erase(i));
}

/// Creates uint32_t color value from components
inline constexpr G::color_t RGBA (uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    { return vpack4(r,g,b,a); }
inline constexpr G::color_t RGBA (G::color_t c)
    { return __builtin_bswap32(c); }

/// Creates uint32_t color value from components
inline static G::color_t ARGB (G::color_t c)
{
#if __x86_64__
    if (!__builtin_constant_p(c)) asm("rol\t$8,%0":"+r"(c)); else
#endif
	c = (c<<8)|(c>>24);
    return RGBA(c);
}

/// Creates uint32_t color value from components
inline constexpr G::color_t RGB (uint8_t r, uint8_t g, uint8_t b)
    { return RGBA(r,g,b,UINT8_MAX); }
inline constexpr G::color_t RGB (G::color_t c)
    { return RGBA((c<<8)|UINT8_MAX); }

// Macro for defining enums for ranges in a vertex array
#define VRENUM(name,size)\
    v_##name##Offset, v_##name##Size = size, v_##name##Last = v_##name##Offset+v_##name##Size-1

//}}}-------------------------------------------------------------------
//{{{ Tesselator macros for vertex arrays
//
// These handle the unpleasantries of offsetting, and different sizes of
// solid and line primitives due to OpenGL ending rasterization in
// different places and making it difficult to specify exact dimensions.

/// Emits a point, cast to coord_t
#define VGEN_POINT(x,y)		G::coord_t(x),G::coord_t(y)
/// Generates 4 points to draw a rectangular line loop
#define VGEN_LLRECT(x,y,w,h)	VGEN_POINT(x,y), VGEN_POINT(x,(y)+(h)-1), VGEN_POINT((x)+(w)-1,(y)+(h)-1), VGEN_POINT((x)+(w)-1,y)
/// Generates 4 points to draw a rectangular triangle strip
#define VGEN_TSRECT(x,y,w,h)	VGEN_POINT(x,(y)-1), VGEN_POINT(x,(y)+(h)-1), VGEN_POINT((x)+(w),(y)-1), VGEN_POINT((x)+(w),(y)+(h)-1)
/// Generates 4 points to draw a rectangular triangle fan
#define VGEN_TFRECT(x,y,w,h)	VGEN_POINT(x,(y)-1), VGEN_POINT(x,(y)+(h)-1), VGEN_POINT((x)+(w),(y)+(h)-1), VGEN_POINT((x)+(w),(y)-1)

// Trig values for ellipse tesselation
#define VGEN_SIN45(r)	((r)*0.707107+0.5)
#define VGEN_SIN34(r)	((r)*0.555570+0.5)
#define VGEN_COS34(r)	((r)*0.831470+0.5)
#define VGEN_SIN22(r)	((r)*0.382683+0.5)
#define VGEN_COS22(r)	((r)*0.923880+0.5)
#define VGEN_SIN11(r)	((r)*0.195090+0.5)
#define VGEN_COS11(r)	((r)*0.980785+0.5)

///{{{2 VGEN_LLELLIPSE_N - generates ellipse line loop with 8, 16, or 32 points
#define VGEN_LLELLIPSE_8(x,y,rx,ry)				\
	    VGEN_POINT (x,y-ry),				\
	    VGEN_POINT (x-VGEN_SIN45(rx),y-VGEN_SIN45(ry)),	\
	    VGEN_POINT (x-rx,y),				\
	    VGEN_POINT (x-VGEN_SIN45(rx),y+VGEN_SIN45(ry)),	\
	    VGEN_POINT (x,y+ry),				\
	    VGEN_POINT (x+VGEN_SIN45(rx),y+VGEN_SIN45(ry)),	\
	    VGEN_POINT (x+rx,y),				\
	    VGEN_POINT (x+VGEN_SIN45(rx),y-VGEN_SIN45(ry))

#define VGEN_LLELLIPSE_16(x,y,rx,ry)				\
	    VGEN_POINT (x,y-ry),				\
	    VGEN_POINT (x-VGEN_SIN22(rx),y-VGEN_COS22(ry)),	\
	    VGEN_POINT (x-VGEN_SIN45(rx),y-VGEN_SIN45(ry)),	\
	    VGEN_POINT (x-VGEN_COS22(rx),y-VGEN_SIN22(ry)),	\
	    VGEN_POINT (x-rx,y),				\
	    VGEN_POINT (x-VGEN_COS22(rx),y+VGEN_SIN22(ry)),	\
	    VGEN_POINT (x-VGEN_SIN45(rx),y+VGEN_SIN45(ry)),	\
	    VGEN_POINT (x-VGEN_SIN22(rx),y+VGEN_COS22(ry)),	\
	    VGEN_POINT (x,y+ry),				\
	    VGEN_POINT (x+VGEN_SIN22(rx),y+VGEN_COS22(ry)),	\
	    VGEN_POINT (x+VGEN_SIN45(rx),y+VGEN_SIN45(ry)),	\
	    VGEN_POINT (x+VGEN_COS22(rx),y+VGEN_SIN22(ry)),	\
	    VGEN_POINT (x+rx,y),				\
	    VGEN_POINT (x+VGEN_COS22(rx),y-VGEN_SIN22(ry)),	\
	    VGEN_POINT (x+VGEN_SIN45(rx),y-VGEN_SIN45(ry)),	\
	    VGEN_POINT (x+VGEN_SIN22(rx),y-VGEN_COS22(ry))

#define VGEN_LLELLIPSE_32(x,y,rx,ry)				\
	    VGEN_POINT (x,y-ry),				\
	    VGEN_POINT (x-VGEN_SIN11(rx),y-VGEN_COS11(ry)),	\
	    VGEN_POINT (x-VGEN_SIN22(rx),y-VGEN_COS22(ry)),	\
	    VGEN_POINT (x-VGEN_SIN34(rx),y-VGEN_COS34(ry)),	\
	    VGEN_POINT (x-VGEN_SIN45(rx),y-VGEN_SIN45(ry)),	\
	    VGEN_POINT (x-VGEN_COS34(rx),y-VGEN_SIN34(ry)),	\
	    VGEN_POINT (x-VGEN_COS22(rx),y-VGEN_SIN22(ry)),	\
	    VGEN_POINT (x-VGEN_COS11(rx),y-VGEN_SIN11(ry)),	\
	    VGEN_POINT (x-rx,y),				\
	    VGEN_POINT (x-VGEN_COS11(rx),y+VGEN_SIN11(ry)),	\
	    VGEN_POINT (x-VGEN_COS22(rx),y+VGEN_SIN22(ry)),	\
	    VGEN_POINT (x-VGEN_COS34(rx),y+VGEN_SIN34(ry)),	\
	    VGEN_POINT (x-VGEN_SIN45(rx),y+VGEN_SIN45(ry)),	\
	    VGEN_POINT (x-VGEN_SIN34(rx),y+VGEN_COS34(ry)),	\
	    VGEN_POINT (x-VGEN_SIN22(rx),y+VGEN_COS22(ry)),	\
	    VGEN_POINT (x-VGEN_SIN11(rx),y+VGEN_COS11(ry)),	\
	    VGEN_POINT (x,y+ry),				\
	    VGEN_POINT (x+VGEN_SIN11(rx),y+VGEN_COS11(ry)),	\
	    VGEN_POINT (x+VGEN_SIN22(rx),y+VGEN_COS22(ry)),	\
	    VGEN_POINT (x+VGEN_SIN34(rx),y+VGEN_COS34(ry)),	\
	    VGEN_POINT (x+VGEN_SIN45(rx),y+VGEN_SIN45(ry)),	\
	    VGEN_POINT (x+VGEN_COS34(rx),y+VGEN_SIN34(ry)),	\
	    VGEN_POINT (x+VGEN_COS22(rx),y+VGEN_SIN22(ry)),	\
	    VGEN_POINT (x+VGEN_COS11(rx),y+VGEN_SIN11(ry)),	\
	    VGEN_POINT (x+rx,y),				\
	    VGEN_POINT (x+VGEN_COS11(rx),y-VGEN_SIN11(ry)),	\
	    VGEN_POINT (x+VGEN_COS22(rx),y-VGEN_SIN22(ry)),	\
	    VGEN_POINT (x+VGEN_COS34(rx),y-VGEN_SIN34(ry)),	\
	    VGEN_POINT (x+VGEN_SIN45(rx),y-VGEN_SIN45(ry)),	\
	    VGEN_POINT (x+VGEN_SIN34(rx),y-VGEN_COS34(ry)),	\
	    VGEN_POINT (x+VGEN_SIN22(rx),y-VGEN_COS22(ry)),	\
	    VGEN_POINT (x+VGEN_SIN11(rx),y-VGEN_COS11(ry))
//}}}2

// Curiously, no offsetting is required for these...
#define VGEN_TFELLIPSE_10(x,y,rx,ry)		\
	    VGEN_POINT(x,y),			\
	    VGEN_LLELLIPSE_8(x,y,rx,ry),	\
	    VGEN_POINT(x,y-ry)

#define VGEN_TFELLIPSE_18(x,y,rx,ry)		\
	    VGEN_POINT(x,y),			\
	    VGEN_LLELLIPSE_16(x,y,rx,ry),	\
	    VGEN_POINT(x,y-ry)

#define VGEN_TFELLIPSE_34(x,y,rx,ry)		\
	    VGEN_POINT(x,y),			\
	    VGEN_LLELLIPSE_32(x,y,rx,ry),	\
	    VGEN_POINT(x,y-ry)

#define VGEN_LLCIRCLE_8(x,y,r)	VGEN_LLELLIPSE_8(x,y,r,r)
#define VGEN_LLCIRCLE_16(x,y,r)	VGEN_LLELLIPSE_16(x,y,r,r)
#define VGEN_LLCIRCLE_32(x,y,r)	VGEN_LLELLIPSE_32(x,y,r,r)

#define VGEN_TFCIRCLE_10(x,y,r)	VGEN_TFELLIPSE_10(x,y,r,r)
#define VGEN_TFCIRCLE_18(x,y,r)	VGEN_TFELLIPSE_18(x,y,r,r)
#define VGEN_TFCIRCLE_34(x,y,r)	VGEN_TFELLIPSE_34(x,y,r,r)

//}}}-------------------------------------------------------------------

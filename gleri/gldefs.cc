// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gldefs.h"

//{{{ Pixel ------------------------------------------------------------

namespace G {
namespace Pixel {

uint16_t ComponentsPerPixel (Fmt fmt) noexcept
{
    //{{{2 sizemap (number of components per pixel)
    struct { Fmt f; uint16_t n; } sizemap[] = {
	{ RGBA,			4 },
	{ RGB,			3 },
	{ DEPTH_COMPONENT,	1 },
	{ STENCIL_INDEX,	1 },
	{ DEPTH_STENCIL,	1 },
	{ RED,			1 },
	{ RG,			2 },
	{ BGR,			3 },
	{ BGRA,			4 },
	{ RED_INTEGER,		1 },
	{ RG_INTEGER,		2 },
	{ RGB_INTEGER,		3 },
	{ BGR_INTEGER,		3 },
	{ RGBA_INTEGER,		4 },
	{ BGRA_INTEGER,		4 },
	{ DEPTH_COMPONENT16,	1 },
	{ DEPTH_COMPONENT24,	1 },
	{ DEPTH_COMPONENT32,	1 },
	{ DEPTH_COMPONENT32F,	1 },
	{ R8,			1 },
	{ R8I,			1 },
	{ R8UI,			1 },
	{ R8_SNORM,		1 },
	{ R3_G3_B2,		1 },
	{ R11F_G11F_B10F,	1 },
	{ R16,			1 },
	{ R16F,			1 },
	{ R16I,			1 },
	{ R16UI,		1 },
	{ R16_SNORM,		1 },
	{ R32F,			1 },
	{ R32I,			1 },
	{ R32UI,		1 },
	{ RG8,			2 },
	{ RG8I,			2 },
	{ RG8UI,		2 },
	{ RG8_SNORM,		2 },
	{ RG16,			2 },
	{ RG16F,		2 },
	{ RG16I,		2 },
	{ RG16UI,		2 },
	{ RG16_SNORM,		2 },
	{ RG32F,		2 },
	{ RG32I,		2 },
	{ RG32UI,		2 },
	{ RGB4,			3 },
	{ RGB5,			3 },
	{ RGB8,			3 },
	{ RGB10,		3 },
	{ RGB12,		3 },
	{ RGB16,		3 },
	{ RGB8I,		3 },
	{ RGB8UI,		3 },
	{ RGB8_SNORM,		3 },
	{ RGB9_E5,		3 },
	{ RGB10_A2UI,		3 },
	{ RGB16F,		3 },
	{ RGB16I,		3 },
	{ RGB16UI,		3 },
	{ RGB16_SNORM,		3 },
	{ RGB565,		1 },
	{ RGB32F,		3 },
	{ RGB32I,		3 },
	{ RGB32UI,		3 },
	{ RGBA2,		1 },
	{ RGBA4,		2 },
	{ RGBA8,		4 },
	{ RGBA12,		4 },
	{ RGBA16,		4 },
	{ RGBA8I,		4 },
	{ RGBA8UI,		4 },
	{ RGBA8_SNORM,		4 },
	{ RGB10_A2,		4 },
	{ RGB5_A1,		4 },
	{ RGBA16F,		4 },
	{ RGBA16I,		4 },
	{ RGBA16UI,		4 },
	{ RGBA16_SNORM,		4 },
	{ RGBA32F,		4 },
	{ RGBA32I,		4 },
	{ RGBA32UI,		4 },
	{ SRGB,			4 },
	{ SRGB8,		4 },
	{ SRGB8_ALPHA8,		4 },
	{ SRGB_ALPHA,		4 }
    };
    //}}}2
    for (auto i = 0u; i < ArraySize(sizemap); ++i)
	if (fmt == sizemap[i].f)
	    return sizemap[i].n;
    return 0;
};

uint16_t ComponentSize (Comp comp) noexcept
{
    //{{{2 sizemap (number of components per pixel)
    struct { Comp c; uint16_t sz; } sizemap[] = {
	{ BYTE,				1 },
	{ UNSIGNED_BYTE,		1 },
	{ SHORT,			2 },
	{ UNSIGNED_SHORT,		2 },
	{ INT,				4 },
	{ UNSIGNED_INT,			4 },
	{ FLOAT,			4 },
	{ UNSIGNED_BYTE_3_3_2,		1 },
	{ UNSIGNED_SHORT_4_4_4_4,	2 },
	{ UNSIGNED_SHORT_5_5_5_1,	2 },
	{ UNSIGNED_INT_8_8_8_8,		4 },
	{ UNSIGNED_INT_10_10_10_2,	4 },
	{ UNSIGNED_BYTE_2_3_3_REV,	1 },
	{ UNSIGNED_SHORT_5_6_5,		2 },
	{ UNSIGNED_SHORT_5_6_5_REV,	2 },
	{ UNSIGNED_SHORT_4_4_4_4_REV,	2 },
	{ UNSIGNED_SHORT_1_5_5_5_REV,	2 },
	{ UNSIGNED_INT_8_8_8_8_REV,	4 },
	{ UNSIGNED_INT_2_10_10_10_REV,	4 }
    };
    //}}}2
    for (auto i = 0u; i < ArraySize(sizemap); ++i)
	if (comp == sizemap[i].c)
	    return sizemap[i].sz;
    return 0;
}

size_t TextureSize (Fmt fmt, Comp comp, dim_t w, dim_t h) noexcept
{
    return Align (ComponentsPerPixel(fmt) * ComponentSize(comp) * w, 4) * h;
}

} // namespace Pixel

//}}}-------------------------------------------------------------------
//{{{ Font

namespace Font {

CPMap::iterator& CPMap::iterator::operator++ (void) noexcept
{
    if (_v == UINT16_MAX)
	return *this;
    ++_v;
    uint8_t cpi = _v >> 8, cpo = _v;
    do {
	auto& r = _cpra[cpi];
	if (cpo < r.first)
	    cpo = r.first;
	if (cpo >= r.last) {
	    if (cpi == UINT8_MAX)
		cpo = UINT8_MAX;
	    else {
		cpo = 0;
		++cpi;
		continue;
	    }
	}
    } while (false);
    _v = (cpi << 8)|cpo;
    return *this;
}

CPMap::iterator CPMap::begin (void) const noexcept
{
    auto i = 0u;
    while (i < ArraySize(_cpra) && _cpra[i].first == _cpra[i].last)
	++i;
    return i == ArraySize(_cpra) ? end() : iterator (_cpra, (i << 8) + _cpra[i].first);
}

CPMap::iterator CPMap::end (void) const noexcept
{
    return iterator (_cpra, UINT16_MAX);
}

size_t CPMap::size (void) const noexcept
{
    auto i = ArraySize(_cpra);
    while (--i && _cpra[i].first == _cpra[i].last) {}
    return _cpra[i].offset + _cpra[i].last - _cpra[i].first + 1;
}

void CPMap::Create (const charmap_t& cm) noexcept
{
    uint16_t offset = 1;
    for (auto cp = 0u; cp < ArraySize(_cpra); ++cp) {
	auto& r = _cpra[cp];
	const auto cmf = &cm[cp*256];
	auto i = 0u;
	while (i < 256 && !cmf[i]) ++i;
	if (i >= 256)
	    continue;
	r.first = i;
	auto j = 256u;
	while (--j && !cmf[j]) {}
	r.last = j;
	r.offset = offset;
	offset += r.last - r.first + 1;
    }
}

uint16_t CPMap::operator[] (uint16_t i) const noexcept
{
    uint8_t cpi = i >> 8, cpo = i;
    auto& r = _cpra[cpi];
    cpo -= r.first;
    return cpo < r.last - r.first + 1 ? r.offset + cpo : 0;
}

//----------------------------------------------------------------------

Info::Info (void)
: FixedInfo()
,_cpmap()
,_name()
,_varw()
,_kp()
{
}

Info::Info (dim_t w, dim_t h)
: FixedInfo (w,h)
,_cpmap()
,_name()
,_varw()
,_kp()
{
}

dim_t Info::Width (uint16_t c) const noexcept
{
    return IsFixed() ? Width() : _varw[_cpmap[c]];
}

dim_t Info::Width (const char* s) const noexcept
{
    dim_t w = 0;
    const auto send = s+strlen(s);
    uint16_t prevc = 0;
    for (auto i = utf8in(s), iend = utf8in(send); i < iend; prevc = *i++)
	w += Width(*i) + Kerning (prevc, *i);
    return w;
}

int16_t Info::Kerning (uint16_t c1, uint16_t c2) const noexcept
{
    if (!HasKerning())
	return 0;
    const KerningPair m = { 0, 0, c2, c1 };
    auto f = lower_bound (_kp.begin(), _kp.end(), m);
    if (f < _kp.end() && f->c1 == c1 && f->c2 == c2)
	return f->d;
    return 0;
}

void Info::read (bstri& is)
{
    uint16_t nvarw, nkp;
    const char* name = nullptr;
    FixedInfo::read (is);
    is >> nvarw >> nkp >> name;
    _varw.resize (nvarw);
    _kp.resize (nkp);
    _name.clear();
    if (name)
	_name = name;
    if (nvarw) {
	is >> _cpmap;
	if (nvarw != _cpmap.size())
	    XError::emit ("invalid font info");
	is.read (&_varw[0], nvarw * sizeof(decltype(_varw)::value_type));
	is.align (sizeof(decltype(_varw)::value_type));
	is.read (&_kp[0], nkp * sizeof(decltype(_varw)::value_type));
    }
}

void Info::write (bstro& os) const
{
    const uint16_t nvarw = _varw.size(), nkp = _kp.size();
    FixedInfo::write (os);
    os << nvarw << nkp << _name.c_str();
    if (nvarw) {
	os << _cpmap;
	os.write (&_varw[0], _varw.size() * sizeof(decltype(_varw)::value_type));
	os.align (sizeof(decltype(_varw)::value_type));
	os.write (&_kp[0], _kp.size() * sizeof(decltype(_varw)::value_type));
    }
}

void Info::write (bstrs& ss) const
{
    const uint16_t nvarw = _varw.size(), nkp = _kp.size();
    FixedInfo::write (ss);
    ss << nvarw << nkp << _name.c_str();
    if (nvarw) {
	ss << _cpmap;
	ss.write (&_varw[0], _varw.size() * sizeof(decltype(_varw)::value_type));
	ss.align (sizeof(decltype(_varw)::value_type));
	ss.write (&_kp[0], _kp.size() * sizeof(decltype(_varw)::value_type));
    }
}

} // namespace Font

//}}}-------------------------------------------------------------------
//{{{ Texture

namespace Texture {

Type TypeFromTextureType (TextureType ttype) noexcept
{
    static const Type c_TextureTypeEnum[] = {
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
	G::Texture::TEXTURE_SAMPLER
    };
    static_assert (ArraySize(c_TextureTypeEnum)-1 == G::TEXTURE_SAMPLER, "unaccounted texture types");
    return c_TextureTypeEnum[min<uint16_t>(ttype,ArraySize(c_TextureTypeEnum)-1)];
}

} // namespace Texture

//}}}-------------------------------------------------------------------
//{{{ Type names

const char* TypeName (Type t) noexcept
{
    switch (t) {
	case BYTE:		return "BYTE"; break;
	case UNSIGNED_BYTE:	return "UNSIGNED_BYTE"; break;
	case SHORT:		return "SHORT"; break;
	case UNSIGNED_SHORT:	return "UNSIGNED_SHORT"; break;
	case INT:		return "INT"; break;
	case UNSIGNED_INT:	return "UNSIGNED_INT"; break;
	case FLOAT:		return "FLOAT"; break;
	case BYTES2:		return "BYTES2"; break;
	case BYTES3:		return "BYTES3"; break;
	case BYTES4:		return "BYTES4"; break;
	case DOUBLE:		return "DOUBLE"; break;
    }
    return "INVALID_TYPE";
}

const char* ShapeName (Shape s) noexcept
{
    switch (s) {
	case POINTS:		return "POINTS"; break;
	case LINES:		return "LINES"; break;
	case LINE_LOOP:		return "LINE_LOOP"; break;
	case LINE_STRIP:	return "LINE_STRIP"; break;
	case TRIANGLES:		return "TRIANGLES"; break;
	case TRIANGLE_STRIP:	return "TRIANGLE_STRIP"; break;
	case TRIANGLE_FAN:	return "TRIANGLE_FAN"; break;
	case LINES_ADJACENCY:	return "LINES_ADJACENCY"; break;
	case LINE_STRIP_ADJACENCY:	return "LINE_STRIP_ADJACENCY"; break;
	case TRIANGLES_ADJACENCY:	return "TRIANGLES_ADJACENCY"; break;
	case TRIANGLE_STRIP_ADJACENCY:	return "TRIANGLE_STRIP_ADJACENCY"; break;
	case PATCHES:		return "PATCHES"; break;
    }
    return "INVALID_SHAPE";
}

} // namespace G

//}}}-------------------------------------------------------------------

// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "gotex.h"

class CFont : public CTexture {
public:
    using CPMap		= G::Font::CPMap;
    using charmap_t	= CPMap::charmap_t;
    using kernvec_t	= vector<G::Font::KerningPair>;
    class FontInfo : public G::Font::Info {
    public:
	struct GlyphInfo {
	    uint16_t	x;
	    uint16_t	y;
	    uint8_t	w;
	    uint8_t	h;
	    int8_t	bx;
	    int8_t	by;
	};
    public:
	inline			FontInfo (void)		: G::Font::Info(),_glyphs(1) {}
	inline void		SetSize (G::dim_t w, G::dim_t h)	{ _w = w; _h = h; }
	inline void		SetMSize (G::dim_t w, G::dim_t h)	{ _mw = w; _mh = h; }
	inline void		SetBaseline (G::dim_t b)		{ _b = b; }
	void			CreateCharmap (const charmap_t& cm);
	void			InitVarWidthMap (void)			{ _varw.resize (_cpmap.size()); }
	const CPMap&		Charmap (void) const	{ return _cpmap; }
	const GlyphInfo&	Glyph (uint16_t i)const	{ return _glyphs[_cpmap[i]]; }
	GlyphInfo&		Glyph (uint16_t i)	{ return _glyphs[_cpmap[i]]; }
	kernvec_t&		KerningPairs (void)	{ return _kp; }
	void			SetWidth (uint16_t c, uint8_t w)	{ _varw[_cpmap[c]] = w; }
    private:
	vector<GlyphInfo>	_glyphs;
    };
    using rcfi_t	= const FontInfo&;
public:
			CFont (GLXContext ctx, goid_t cid, const GLubyte* p, GLuint psz, uint8_t fontSize);
    inline explicit	CFont (CFont&& v)		: CTexture(move(v)),_info(v._info),_rowwidth(v._rowwidth) {}
    inline CFont&	operator= (CFont&& v)		{ CGObject::operator= (move(v)); _info = move(v._info); _rowwidth = v._rowwidth; return *this; }
    inline rcfi_t	Info (void) const		{ return _info; }
    inline rcti_t	TextureInfo (void) const	{ return CTexture::Info(); }
private:
    void		ReadPSF (const uint8_t* p, unsigned psz);
    void		ReadFreetype (const uint8_t* p, unsigned psz, uint8_t fontSize);
private:
    FontInfo		_info;
    GLushort		_rowwidth;
};

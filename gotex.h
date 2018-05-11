// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "gob.h"

class CTexture : public CGObject {
public:
    class CParam {
    public:
	inline		CParam (void)	{ copy_n (c_Defaults, G::Texture::NPARAMS, _p); }
	inline int	Get (G::TextureType, G::Texture::Parameter p) const	{ assert (p < G::Texture::NPARAMS); return _p[p]; }
	inline void	Set (G::TextureType, G::Texture::Parameter p, int v)	{ if (p < G::Texture::NPARAMS) _p[p] = v; }
	inline bool	IsDefault (G::Texture::Parameter p) const		{ assert (p < G::Texture::NPARAMS); return _p[p] == c_Defaults[p]; }
	inline uint16_t	GLCode (G::Texture::Parameter p) const			{ assert (p < G::Texture::NPARAMS); return c_GLCode[p]; }
    private:
	int		_p [G::Texture::NPARAMS];
	static const int c_Defaults [G::Texture::NPARAMS];
	static const uint16_t c_GLCode [G::Texture::NPARAMS];
    };
    enum {
	c_MaxWidth = 1u<<14,
	c_MaxHeight = c_MaxWidth
    };
    using rcti_t	= const G::Texture::Info&;
public:
			CTexture (GLXContext ctx, goid_t cid, const GLubyte* p, GLuint psz, G::Pixel::Fmt storeas, G::TextureType ttype, const CParam& param);
    inline		~CTexture (void) noexcept { Free(); }
    inline explicit	CTexture (CTexture&& v)	: CGObject(move(v)),_info(v._info) {}
    inline CTexture&	operator= (CTexture&& v){ CGObject::operator= (move(v)); _info = move(v._info); return *this; }
    inline rcti_t	Info (void) const	{ return _info; }
    inline GLenum	Type (void) const	{ return Info().type; }
    inline GLushort	Width (void) const	{ return Info().w; }
    inline GLushort	Height (void) const	{ return Info().h; }
    inline GLushort	Depth (void) const	{ return Info().d; }
    void		Free (void) noexcept;
    static void		Save (int fd, GLuint x, GLuint y, GLuint w, GLuint h, G::Texture::Format, uint8_t quality);
protected:
    class CTexBuf {
    public:
	using texhdr_t		= G::Texture::GLTXHeader;
	using value_type	= uint32_t;
	using pointer		= value_type*;
	using const_pointer	= const value_type*;
    public:
	inline			CTexBuf (void)		:_info(),_imgd(),_imgsz() {}
	inline			CTexBuf (CTexBuf&& b)	:_info(move(b._info)),_imgd(move(b._imgd)) {}
				CTexBuf (G::Pixel::Fmt fmt, G::Pixel::Comp comp, uint16_t w, uint32_t roww=0, uint16_t h=1, uint16_t d=0);
	inline CTexBuf&		operator= (CTexBuf&& b)	{ _info = move(b._info); _imgd = move(b._imgd); return *this; }
	inline const_pointer	Data (void) const	{ return _imgd.empty() ? nullptr : reinterpret_cast<const_pointer>(&_imgd[0]); }
	inline pointer		Data (void)		{ return _imgd.empty() ? nullptr : reinterpret_cast<pointer>(&_imgd[0]); }
	inline uint32_t		Size (void) const	{ return _imgd.size(); }
	const_pointer		Data (unsigned i) const;
	uint32_t		Size (unsigned i) const;
	inline rcti_t		Info (void) const	{ return _info; }
	void			Resize (uint16_t w, uint32_t roww, uint16_t h);
	void			Load (const GLubyte* p, GLuint psz);
	void			Save (int fd) const;
    private:
	G::Texture::Info	_info;
	vector<uint8_t>		_imgd;
	vector<uint32_t>	_imgsz;
    };
protected:
				CTexture (GLXContext ctx, goid_t cid);
    inline GLuint		GenId (void) const	{ GLuint id; glGenTextures (1, &id); return id; }
    void			Create (const CTexBuf& tbuf, G::Pixel::Fmt storeas, G::TextureType ttype, const CParam& param);
private:
    static inline CTexBuf	Load (const GLubyte* p, GLuint psz);
    static CTexBuf		LoadGLTX (const GLubyte* p, GLuint psz);
#if __has_include(<png.h>)
    static CTexBuf		LoadPNG (const GLubyte* p, GLuint psz);
    static void			SavePNG (int fd, const CTexBuf& tbuf);
#endif
#if __has_include(<jpeglib.h>)
    static inline CTexBuf	LoadJPG (const GLubyte* p, GLuint psz);
    static inline void		SaveJPG (int fd, const CTexBuf& tbuf, uint8_t quality);
#endif
#if __has_include(<gif_lib.h>)
    static inline CTexBuf	LoadGIF (const GLubyte* p, GLuint psz);
#endif
protected:
    G::Texture::Info		_info;
};

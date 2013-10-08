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
	inline int	Get (G::Texture::Type, G::Texture::Parameter p) const	{ assert (p < G::Texture::NPARAMS); return (_p[p]); }
	inline void	Set (G::Texture::Type, G::Texture::Parameter p, int v)	{ if (p < G::Texture::NPARAMS) _p[p] = v; }
    private:
	int		_p [G::Texture::NPARAMS];
	static const int c_Defaults [G::Texture::NPARAMS];
    };
public:
			CTexture (GLXContext ctx, const GLubyte* p, GLuint psz, G::Pixel::Fmt storeas, const CParam& param) noexcept;
    inline		~CTexture (void) noexcept { Free(); }
    inline explicit	CTexture (CTexture&& v)	: CGObject(forward<CTexture>(v)),_width(v._width),_height(v._height) {}
    inline CTexture&	operator= (CTexture&& v){ CGObject::operator= (forward<CTexture>(v)); _width = v._width; _height = v._height; return (*this); }
    inline GLushort	Width (void) const	{ return (_width); }
    inline GLushort	Height (void) const	{ return (_height); }
    void		Free (void) noexcept;
private:
    class CTexBuf {
	typedef G::STextureHeader	texhdr_t;
	typedef uint32_t		value_type;
	typedef value_type*		pointer;
	typedef const value_type*	const_pointer;
    public:
	inline constexpr	CTexBuf (void)		:_h(),_sz(0),_p(nullptr) {}
	inline constexpr	CTexBuf (const texhdr_t& h)
				    :_h(h),_sz(0),_p((pointer)const_cast<texhdr_t*>(&h+1)) {}
	inline			CTexBuf (G::Pixel::Fmt fmt, G::Pixel::Comp comp, uint32_t w, uint16_t h=1, uint16_t d=1)
				    :_h { texhdr_t::Magic, w,h,d,fmt,comp }
				    ,_sz (w*h*sizeof(value_type))
				    ,_p ((pointer)malloc(_sz)) {}
	inline			~CTexBuf (void)		{ if (_p && _sz) free(_p); }
	inline const_pointer	Data (void) const	{ return (_p); }
	inline pointer		Data (void)		{ return (_p); }
	inline size_t		Size (void) const	{ return (_sz); }
	inline const texhdr_t&	Header (void) const	{ return (_h); }
    private:
	texhdr_t		_h;
	size_t			_sz;
	pointer			_p;
    };
private:
    inline GLuint		GenId (void) const	{ GLuint id; glGenTextures (1, &id); return (id); }
    static inline CTexBuf	Load (const GLubyte* p, GLuint psz) noexcept;
    static inline CTexBuf	LoadPNG (const GLubyte* p, GLuint psz) noexcept;
    static inline CTexBuf	LoadJPG (const GLubyte* p, GLuint psz) noexcept;
private:
    GLushort		_width;
    GLushort		_height;
};

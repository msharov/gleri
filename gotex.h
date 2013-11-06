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
	inline int	Get (G::TextureType, G::Texture::Parameter p) const	{ assert (p < G::Texture::NPARAMS); return (_p[p]); }
	inline void	Set (G::TextureType, G::Texture::Parameter p, int v)	{ if (p < G::Texture::NPARAMS) _p[p] = v; }
    private:
	int		_p [G::Texture::NPARAMS];
	static const int c_Defaults [G::Texture::NPARAMS];
    };
public:
			CTexture (GLXContext ctx, goid_t cid, const GLubyte* p, GLuint psz, G::Pixel::Fmt storeas, G::TextureType ttype, const CParam& param) noexcept;
    inline		~CTexture (void) noexcept { Free(); }
    inline explicit	CTexture (CTexture&& v)	: CGObject(forward<CTexture>(v)),_h(v._h) {}
    inline CTexture&	operator= (CTexture&& v){ CGObject::operator= (forward<CTexture>(v)); _h = v._h; return (*this); }
    inline const G::Texture::Header&	Info (void) const	{ return (_h); }
    inline GLenum	Type (void) const	{ return (_h.type); }
    inline GLushort	Width (void) const	{ return (_h.w); }
    inline GLushort	Height (void) const	{ return (_h.h); }
    inline GLushort	Depth (void) const	{ return (_h.d); }
    void		Free (void) noexcept;
    static G::Texture::Type	GLenumFromTextureType (G::TextureType ttype) noexcept;
    static void		Save (int fd, GLuint x, GLuint y, GLuint w, GLuint h, G::Texture::Format, uint8_t quality);
protected:
    inline		CTexture (GLXContext ctx, goid_t cid) : CGObject(ctx,cid,GenId()),_h({0,G::Texture::TEXTURE_2D,0,0,0,G::Pixel::RGBA,G::Pixel::UNSIGNED_BYTE}) {}
    inline GLuint	GenId (void) const	{ GLuint id; glGenTextures (1, &id); return (id); }
private:
    class CTexBuf {
    public:
	typedef G::Texture::Header	texhdr_t;
	typedef uint32_t		value_type;
	typedef value_type*		pointer;
	typedef const value_type*	const_pointer;
    public:
	inline constexpr	CTexBuf (void)		:_h(),_sz(0),_p(nullptr) {}
	inline constexpr	CTexBuf (const texhdr_t& h, const_pointer p)
				    :_h(h),_sz(0),_p(const_cast<pointer>(p)) {}
	inline			CTexBuf (CTexBuf&& b)	:_h(b._h),_sz(b._sz),_p(b._p) { b._p = nullptr; }
	inline			CTexBuf (G::Pixel::Fmt fmt, G::Pixel::Comp comp, uint16_t w, uint16_t h=1, uint16_t d=0)
				    :_h { texhdr_t::Magic, G::Texture::TEXTURE_2D, w,h,d,fmt,comp }
				    ,_sz (w*h*sizeof(value_type))
				    ,_p ((pointer)malloc(_sz)) {}
	inline			~CTexBuf (void)		{ if (_p && _sz) free(_p); }
	inline CTexBuf&		operator= (CTexBuf&& b)	{ swap(_h,b._h); swap(_sz,b._sz); swap(_p,b._p); return (*this); }
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
    static inline CTexBuf	Load (const GLubyte* p, GLuint psz) noexcept;
    static CTexBuf		LoadPNG (const GLubyte* p, GLuint psz) noexcept;
    static inline CTexBuf	LoadJPG (const GLubyte* p, GLuint psz) noexcept;
    static void			SavePNG (int fd, const CTexBuf& tbuf);
    static inline void		SaveJPG (int fd, const CTexBuf& tbuf, uint8_t quality);
private:
    G::Texture::Header		_h;
};

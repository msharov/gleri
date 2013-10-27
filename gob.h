// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "config.h"
#include "gleri/gldefs.h"
#include "gleri/bstr.h"

//{{{ Debug tracing ----------------------------------------------------

template <typename... Args>
#ifndef NDEBUG
inline void DTRACE (const char* fmt, Args... args) noexcept
{
    extern bool g_bDebugTrace;
    if (g_bDebugTrace) {
	printf (fmt, args...);
	fflush (stdout);
    }
}
inline void DHEXDUMP (const void* p, size_t sz) noexcept
{
    extern bool g_bDebugTrace;
    if (g_bDebugTrace) {
	hexdump (p, sz);
	fflush (stdout);
    }
}
#else
inline void DTRACE (const char*, Args...) noexcept {}
inline void DHEXDUMP (const void*, size_t) noexcept {}
#endif
inline void DHEXDUMP (const bstri& is) noexcept
    { DHEXDUMP (is.ipos(), is.remaining()); }

//}}}-------------------------------------------------------------------

class CGObject {
public:
    enum : GLuint { NoObject = numeric_limits<GLuint>::max() };
    typedef G::goid_t	goid_t;
public:
    inline		CGObject (GLXContext ctx, goid_t cid, GLuint id) :_ctx(ctx),_id(id),_cid(cid) {}
    inline explicit	CGObject (CGObject&& v)			:_ctx(v._ctx),_id(v._id),_cid(v._cid) { v._ctx = nullptr; v._id = NoObject; v._cid = G::GoidNull; }
    inline virtual	~CGObject (void)			{ }
    inline CGObject&	operator= (CGObject&& v)		{ swap(_ctx, v._ctx); swap(_id,v._id); swap(_cid,v._cid); return (*this); }
    inline bool		operator== (const CGObject& o) const	{ return (_cid == o.CId()); }
    inline bool		operator== (goid_t cid) const		{ return (_cid == cid); }
    inline bool		operator< (const CGObject& o) const	{ return (_cid < o.CId()); }
    inline bool		operator< (goid_t cid) const		{ return (_cid < cid); }
    inline GLXContext	Context (void) const			{ return (_ctx); }
    inline GLuint	Id (void) const				{ return (_id); }
    inline goid_t	CId (void) const			{ return (_cid); }
protected:
    inline void		ResetId (GLuint id = NoObject)		{ _id = id; }
private:
    GLXContext		_ctx;
    GLuint		_id;
    goid_t		_cid;
};

//----------------------------------------------------------------------

class CContext : public CGObject {
public:
    inline		CContext (GLXContext ctx, goid_t cid, Window win) : CGObject(ctx, cid, win) {}
    inline Window	Drawable (void) const			{ return (Id()); }
    inline void		SetDrawable (Window w)			{ ResetId (w); }
};

//----------------------------------------------------------------------

class CBuffer : public CGObject {
public:
			CBuffer (GLXContext ctx, goid_t cid, const void* data, GLuint dsz, G::BufferHint mode, G::BufferType btype) noexcept;
    virtual		~CBuffer (void) noexcept;
    inline explicit	CBuffer (CBuffer&& v)	: CGObject(forward<CBuffer>(v)) {}
    inline CBuffer&	operator= (CBuffer&& v)	{ CGObject::operator= (forward<CBuffer>(v)); return (*this); }
    inline GLenum	Type (void) const	{ return (_btype); }
private:
    inline GLuint	GenId (void) const	{ GLuint id; glGenBuffers (1, &id); return (id); }
   inline static GLenum	GLenumFromBufferType (G::BufferType btype) noexcept;
private:
    GLenum		_btype;
};

//----------------------------------------------------------------------

class CDatapak : public CGObject {
public:
			CDatapak (GLXContext ctx, goid_t cid, GLubyte* p, GLuint psz) noexcept;
    virtual		~CDatapak (void) noexcept;
    inline explicit	CDatapak (CDatapak&& v)	: CGObject(forward<CDatapak>(v)), _sz(v._sz), _p(v._p) { v._sz = 0; v._p = nullptr; }
    inline CDatapak&	operator= (CDatapak&& v){ CGObject::operator= (forward<CDatapak>(v)); swap(_sz,v._sz); swap(_p,v._p); return (*this); }
    const GLubyte*	File (const char* filename, GLuint& sz) const noexcept;
    inline GLuint	Size (void) const	{ return (_sz); }
    static GLubyte*	DecompressBlock (const GLubyte* p, unsigned isz, unsigned& osz);
private:
    inline GLuint	GenId (void) const	{ GLuint id; glGenBuffers (1, &id); return (id); }
private:
    GLuint		_sz;
    GLubyte*		_p;
};

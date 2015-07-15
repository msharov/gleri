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
    using goid_t	= G::goid_t;
public:
    inline		CGObject (GLXContext ctx, goid_t cid, GLuint id) :_ctx(ctx),_id(id),_cid(cid) {}
    inline explicit	CGObject (CGObject&& v)			:_ctx(v._ctx),_id(v._id),_cid(v._cid) { v._ctx = nullptr; v._id = NoObject; v._cid = G::GoidNull; }
    inline virtual	~CGObject (void)			{ }
    inline CGObject&	operator= (CGObject&& v)		{ swap(_ctx, v._ctx); swap(_id,v._id); swap(_cid,v._cid); return *this; }
    inline bool		operator== (const CGObject& o) const	{ return _cid == o.CId(); }
    inline bool		operator== (goid_t cid) const		{ return _cid == cid; }
    inline bool		operator< (const CGObject& o) const	{ return _cid < o.CId(); }
    inline bool		operator< (goid_t cid) const		{ return _cid < cid; }
    inline GLXContext	Context (void) const			{ return _ctx; }
    inline GLuint	Id (void) const				{ return _id; }
    inline goid_t	CId (void) const			{ return _cid; }
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
    inline Window	Drawable (void) const			{ return Id(); }
    inline void		SetDrawable (Window w)			{ ResetId (w); }
};

//----------------------------------------------------------------------

class CBuffer : public CGObject {
public:
			CBuffer (GLXContext ctx, goid_t cid, const void* data, GLuint dsz, G::BufferHint mode, G::BufferType btype) noexcept;
    virtual		~CBuffer (void) noexcept;
    inline explicit	CBuffer (CBuffer&& v)	: CGObject(move(v)) {}
    inline CBuffer&	operator= (CBuffer&& v)	{ CGObject::operator= (move(v)); return *this; }
    inline GLenum	Type (void) const	{ return _btype; }
private:
    inline GLuint	GenId (void) const	{ GLuint id; glGenBuffers (1, &id); return id; }
   inline static GLenum	GLenumFromBufferType (G::BufferType btype) noexcept;
private:
    GLenum		_btype;
};

//----------------------------------------------------------------------

class CDatapak : public CGObject {
public:
			CDatapak (GLXContext ctx, goid_t cid, unique_c_ptr<GLubyte>&& p, GLuint psz) noexcept;
    virtual		~CDatapak (void) noexcept;
    inline explicit	CDatapak (CDatapak&& v)	: CGObject(move(v)), _sz(v._sz), _p(move(v._p)) { v._sz = 0; }
    inline CDatapak&	operator= (CDatapak&& v){ CGObject::operator= (move(v)); swap(_sz,v._sz); _p.operator= (move(v._p)); return *this; }
    const GLubyte*	File (const char* filename, GLuint& sz) const noexcept;
    inline GLuint	Size (void) const	{ return _sz; }
    static unique_c_ptr<GLubyte>	DecompressBlock (const GLubyte* p, unsigned isz, unsigned& osz);
private:
    inline GLuint	GenId (void) const	{ GLuint id; glGenBuffers (1, &id); return id; }
private:
    GLuint			_sz;
    unique_c_ptr<GLubyte>	_p;
};

//----------------------------------------------------------------------

class CIConn;
class CTexture;

class CFramebuffer : public CGObject {
public:
    inline		CFramebuffer (GLXContext ctx, goid_t cid, GLuint id) : CGObject (ctx, cid, id), _w(0), _h(0) {}
			CFramebuffer (GLXContext ctx, goid_t cid, const GLubyte* p, GLuint psz, const CIConn& conn);
    virtual		~CFramebuffer (void) noexcept	{ Free(); }
    void		Attach (const G::FramebufferComponent& c, const CTexture& tex) const noexcept;
    inline GLushort	Width (void) const	{ return _w; }
    inline GLushort	Height (void) const	{ return _h; }
private:
    inline GLuint	GenId (void) const	{ GLuint id; glGenFramebuffers (1, &id); return id; }
    void		Free (void) noexcept;
private:
    GLushort		_w,_h;
};

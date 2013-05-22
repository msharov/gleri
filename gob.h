// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "config.h"
#include "gleri/gldefs.h"

//----------------------------------------------------------------------

template <typename... Args>
#ifndef NDEBUG
inline void DTRACE (const char* fmt, Args... args) noexcept
{
    extern bool g_bDebugTrace;
    if (g_bDebugTrace)
	printf (fmt, args...);
}
#else
inline void DTRACE (const char*, Args...) noexcept {}
#endif

//----------------------------------------------------------------------

class CGObject {
public:
    enum : GLuint { NoObject = UINT_MAX };
public:
    inline		CGObject (GLXContext ctx, GLuint id)	:_ctx(ctx),_id(id) {}
    inline explicit	CGObject (CGObject&& v)			:_ctx(v._ctx),_id(v._id) { v._ctx = nullptr; v._id = NoObject; }
    inline CGObject&	operator= (CGObject&& v)		{ swap(_ctx, v._ctx); swap(_id,v._id); return (*this); }
    inline bool		operator== (GLuint id) const		{ return (_id == id); }
    inline GLXContext	Context (void) const			{ return (_ctx); }
    inline GLuint	Id (void) const				{ return (_id); }
private:
    GLXContext		_ctx;
    GLuint		_id;
};

//----------------------------------------------------------------------

class CContext : public CGObject {
public:
    inline		CContext (GLXContext ctx, Window win)	: CGObject(ctx, win) {}
    Window		Drawable (void) const			{ return (Id()); }
};

//----------------------------------------------------------------------

class CBuffer : public CGObject {
public:
			CBuffer (GLXContext ctx, G::EBufferType btype) noexcept;
			~CBuffer (void) noexcept;
    inline explicit	CBuffer (CBuffer&& v)	: CGObject(forward<CBuffer>(v)) {}
    inline CBuffer&	operator= (CBuffer&& v)	{ CGObject::operator= (forward<CBuffer>(v)); return (*this); }
  inline G::EBufferType	Type (void) const	{ return (_btype); }
private:
    inline GLuint	GenId (void) const	{ GLuint id; glGenBuffers (1, &id); return (id); }
private:
    G::EBufferType	_btype;
};

//----------------------------------------------------------------------

class CDatapak : public CGObject {
public:
			CDatapak (GLXContext ctx, GLubyte* p, GLuint psz) noexcept;
			~CDatapak (void) noexcept;
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

//----------------------------------------------------------------------

template <typename Ctr>
inline typename Ctr::const_pointer FindGObject (const Ctr& ctr, GLuint id)
{
    foreach (auto, i, ctr)
	if (i->Id() == id)
	    return (&*i);
    return (nullptr);
}

template <typename Ctr>
inline typename Ctr::pointer FindGObject (Ctr& ctr, GLuint id)
    { return (const_cast<typename Ctr::pointer>(FindGObject (const_cast<const Ctr&>(ctr), id))); }

template <typename Ctr>
inline void EraseGObject (Ctr& ctr, GLuint id)
    { erase_if (ctr, [id](typename Ctr::const_reference i) { return (i.Id() == id); }); }

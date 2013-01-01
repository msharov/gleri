#pragma once
#include "config.h"
#include "gleri/gldefs.h"

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

class CDatapak;

//----------------------------------------------------------------------

template <typename Ctr>
inline typename Ctr::const_pointer FindGObject (const Ctr& ctr, GLuint id)
{
    for (const auto& i : ctr)
	if (i.Id() == id)
	    return (&i);
    return (nullptr);
}

template <typename Ctr>
inline typename Ctr::pointer FindGObject (Ctr& ctr, GLuint id)
    { return (const_cast<typename Ctr::pointer>(FindGObject (ctr, id))); }

template <typename Ctr>
inline void EraseGObject (Ctr& ctr, GLuint id)
    { erase_if (ctr, [id](typename Ctr::const_reference i) { return (i.Id() == id); }); }

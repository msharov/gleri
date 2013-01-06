// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "gob.h"

class CFont : public CGObject {
public:
			CFont (GLXContext ctx, const GLubyte* p, GLuint psz) noexcept;
    inline explicit	CFont (CFont&& v)		: CGObject(move(v)),_width(v._width),_height(v._height),_rowwidth(v._rowwidth) {}
			~CFont (void) noexcept;
    inline CFont&	operator= (CFont&& v)		{ CGObject::operator= (move(v)); _width = v._width; _height = v._height; _rowwidth = v._rowwidth; return (*this); }
    inline GLubyte	Width (void) const		{ return (_width); }
    inline GLubyte	Height (void) const		{ return (_height); }
    inline GLushort	LetterX (GLubyte c) const	{ return ((c%_rowwidth)*_width); }
    inline GLushort	LetterY (GLubyte c) const	{ return ((c/_rowwidth)*_height); }
private:
    inline GLuint	GenId (void) const		{ GLuint id; glGenTextures (1, &id); return (id); }
private:
    GLubyte		_width;
    GLubyte		_height;
    GLushort		_rowwidth;
};

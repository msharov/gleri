// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "gob.h"

class CTexture : public CGObject {
public:
			CTexture (GLXContext ctx, const GLubyte* p, GLuint psz) noexcept;
			~CTexture (void) noexcept;
    inline explicit	CTexture (CTexture&& v)	: CGObject(forward<CTexture>(v)),_width(v._width),_height(v._height) {}
    inline CTexture&	operator= (CTexture&& v){ CGObject::operator= (forward<CTexture>(v)); _width = v._width; _height = v._height; return (*this); }
    inline GLushort	Width (void) const	{ return (_width); }
    inline GLushort	Height (void) const	{ return (_height); }
private:
    inline GLuint	GenId (void) const	{ GLuint id; glGenTextures (1, &id); return (id); }
    inline GLubyte*	LoadPNG (const GLubyte* p, GLuint psz) noexcept;
private:
    GLushort		_width;
    GLushort		_height;
};

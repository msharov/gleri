// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "gotex.h"

class CFont : public CTexture {
public:
			CFont (GLXContext ctx, goid_t cid, const GLubyte* p, GLuint psz) noexcept;
    inline explicit	CFont (CFont&& v)		: CTexture(forward<CTexture>(v)),_letterw(v._letterw),_letterh(v._letterh),_rowwidth(v._rowwidth) {}
    inline CFont&	operator= (CFont&& v)		{ CGObject::operator= (forward<CFont>(v)); _letterw = v._letterw; _letterh = v._letterh; _rowwidth = v._rowwidth; return (*this); }
    inline GLubyte	LetterW (void) const		{ return (_letterw); }
    inline GLubyte	LetterH (void) const		{ return (_letterh); }
    inline GLushort	LetterX (GLubyte c) const	{ return ((c%_rowwidth)*LetterW()); }
    inline GLushort	LetterY (GLubyte c) const	{ return ((c/_rowwidth)*LetterH()); }
private:
    GLubyte		_letterw;
    GLubyte		_letterh;
    GLushort		_rowwidth;
};

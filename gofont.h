// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "gotex.h"

class CFont : public CTexture {
public:
			CFont (GLXContext ctx, goid_t cid, const GLubyte* p, GLuint psz) noexcept;
    inline explicit	CFont (CFont&& v)		: CTexture(move(v)),_info(v._info),_rowwidth(v._rowwidth) {}
    inline CFont&	operator= (CFont&& v)		{ CGObject::operator= (move(v)); _info = v._info; _rowwidth = v._rowwidth; return *this; }
    inline const G::Font::Info&	Info (void) const	{ return _info; }
    inline uint16_t	LetterW (void) const		{ return Info().Width(); }
    inline uint16_t	LetterH (void) const		{ return Info().Height(); }
    inline GLushort	LetterX (GLubyte c) const	{ return (c%_rowwidth)*LetterW(); }
    inline GLushort	LetterY (GLubyte c) const	{ return (c/_rowwidth)*LetterH(); }
private:
    G::Font::Info	_info;
    GLushort		_rowwidth;
};

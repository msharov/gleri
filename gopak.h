#pragma once
#include "gob.h"

class CDatapak : public CGObject {
public:
			CDatapak (GLXContext ctx, GLubyte* p, GLuint psz) noexcept;
    inline explicit	CDatapak (CDatapak&& v)	: CGObject(move(v)), _sz(v._sz), _p(v._p) { v._sz = 0; v._p = nullptr; }
    inline CDatapak&	operator= (CDatapak&& v){ CGObject::operator= (move(v)); swap(_sz,v._sz); swap(_p,v._p); return (*this); }
			~CDatapak (void) noexcept;
    const GLubyte*	File (const char* filename, GLuint& sz) const noexcept;
    inline GLuint	Size (void) const	{ return (_sz); }
private:
    inline GLuint	GenId (void) const	{ GLuint id; glGenBuffers (1, &id); return (id); }
private:
    GLuint		_sz;
    GLubyte*		_p;
};

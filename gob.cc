// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gob.h"
#include "gleri/mmfile.h"
#include <zlib.h>

//----------------------------------------------------------------------
#ifndef NDEBUG
bool g_bDebugTrace = false;
#endif
//----------------------------------------------------------------------

CBuffer::CBuffer (GLXContext ctx, G::EBufferType btype) noexcept : CGObject(ctx, GenId()), _btype(btype) {}
CBuffer::~CBuffer (void) noexcept
    { GLuint id = Id(); if (id != NoObject) glDeleteBuffers (1, &id); }

//{{{ CPIO file format definitions -------------------------------------
namespace {

// CPIO file format header copied from the spec
class CpioHeader {
public:
    inline			CpioHeader (void)	{ }
    inline GLuint		Filesize (void) const	{ return ((GLuint(filesize[0]) << 16) | filesize[1]); }
    inline bool			MagicOk (void) const	{ return (magic == 070707); }
    inline const char*		Filename (void) const	{ return ((const char*)this + sizeof(*this)); }
    inline const GLubyte*	Filedata (void) const	{ return ((const GLubyte*)(Filename()+EvenUp(namesize))); }
    inline const CpioHeader*	Next (void) const	{ return ((const CpioHeader*)(Filedata()+EvenUp(Filesize()))); }
protected:
    inline GLuint		EvenUp (GLuint v) const	{ return (v+v%2); }
protected:
    GLushort	magic;		///< Magic id 070707
    GLushort	dev;
    GLushort	ino;
    GLushort	mode;
    GLushort	uid;
    GLushort	gid;
    GLushort	nlink;
    GLushort	rdev;
    GLushort	mtime[2];
    GLushort	namesize;	///< Length of name right after the header, includes zero terminator. Name is padded to even offset.
    GLushort	filesize[2];	///< Length of the file data after the name. Also padded to even offset.
};

} // namespace
//}}}-------------------------------------------------------------------

CDatapak::CDatapak (GLXContext ctx, GLubyte* p, GLuint psz) noexcept
: CGObject(ctx, GenId())
,_sz (psz)
,_p (p)
{
}

CDatapak::~CDatapak (void) noexcept
{
    GLuint id = Id();
    if (id != NoObject)
	glDeleteBuffers (1, &id);
    if (_p)
	free(_p);
}

const GLubyte* CDatapak::File (const char* filename, GLuint& sz) const noexcept
{
    if (!_p)
	return (nullptr);
    typedef const CpioHeader* pch_t;
    for (pch_t pi = (pch_t)_p, pe = (pch_t)(_p+_sz); pi < pe; pi = pi->Next()) {
	if (!strcmp (pi->Filename(), filename)) {
	    sz = pi->Filesize();
	    return (pi->Filedata());
	}
    }
    return (nullptr);
}

//----------------------------------------------------------------------

/*static*/ GLubyte* CDatapak::DecompressBlock (const GLubyte* p, unsigned isz, unsigned& osz)
{
    z_stream zs;
    memset (&zs, 0, sizeof(zs));

    zs.avail_in = isz;
    zs.next_in = const_cast<Bytef*>(p);
    unsigned bufsz = 4096, bread = 0;
    unsigned chunksz = bufsz;
    GLubyte* out = (GLubyte*) malloc (bufsz);
    zs.avail_out = bufsz;
    zs.next_out = out;

    enum { USE_GZIP_FORMAT = 16 };	// zlib fairy dust
    if (Z_OK != inflateInit2 (&zs, USE_GZIP_FORMAT+MAX_WBITS))
	CFile::Error ("gzip");

    for (;;) {
	int r = inflate (&zs, Z_NO_FLUSH);
	if (r == Z_STREAM_END)
	    break;
	else if (r == Z_OK) {
	    bread += chunksz-zs.avail_out;
	    out = (GLubyte*) realloc (out, bufsz*2);
	    bufsz *= 2;
	    zs.next_out = out+bread;
	    zs.avail_out = chunksz = bufsz-bread;
	} else {
	    inflateEnd (&zs);
	    CFile::Error ("gzip");
	}
    }
    osz = bread + chunksz-zs.avail_out;
    inflateEnd (&zs);
    return (out);
}

//----------------------------------------------------------------------

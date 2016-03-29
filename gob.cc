// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gob.h"
#include "gleri/mmfile.h"
#include "iconn.h"
#include <zlib.h>

//----------------------------------------------------------------------
#ifndef NDEBUG
bool g_bDebugTrace = false;
#endif
//----------------------------------------------------------------------

CBuffer::CBuffer (GLXContext ctx, goid_t cid, const void* data, GLuint dsz, G::BufferHint hint, G::BufferType btype) noexcept
: CGObject(ctx, cid, GenId())
,_btype(GLenumFromBufferType(btype))
{
    if (dsz) {
	glBindBuffer (_btype, Id());
	glBufferData (_btype, dsz, data, hint);
    }
}

/*static*/ GLenum CBuffer::GLenumFromBufferType (G::BufferType btype) noexcept
{
    static const GLenum c_BufferTypeEnum[] = {
	GL_ARRAY_BUFFER,
	GL_ELEMENT_ARRAY_BUFFER,
	GL_PIXEL_PACK_BUFFER,
	GL_PIXEL_UNPACK_BUFFER,
	GL_ATOMIC_COUNTER_BUFFER,
	GL_COPY_READ_BUFFER,
	GL_COPY_WRITE_BUFFER,
	GL_DISPATCH_INDIRECT_BUFFER,
	GL_DRAW_INDIRECT_BUFFER,
	GL_SHADER_STORAGE_BUFFER,
	GL_TEXTURE_BUFFER,
	GL_TRANSFORM_FEEDBACK_BUFFER,
	GL_UNIFORM_BUFFER
    };
    return c_BufferTypeEnum[min<uint16_t>(btype,ArraySize(c_BufferTypeEnum)-1)];
}

CBuffer::~CBuffer (void) noexcept
{
    auto id = Id();
    if (id != NoObject)
	glDeleteBuffers (1, &id);
}

//{{{ CPIO file format definitions -------------------------------------
namespace {

// CPIO file format header copied from the spec
class CpioHeader {
public:
    inline			CpioHeader (void)	{ }
    inline GLuint		Filesize (void) const	{ return (GLuint(filesize[0]) << 16) | filesize[1]; }
    inline bool			MagicOk (void) const	{ return magic == 070707; }
    inline const char*		Filename (void) const	{ return (const char*)this + sizeof(*this); }
    inline const GLubyte*	Filedata (void) const	{ return (const GLubyte*)(Filename()+EvenUp(namesize)); }
    inline const CpioHeader*	Next (void) const	{ return (const CpioHeader*)(Filedata()+EvenUp(Filesize())); }
protected:
    inline GLuint		EvenUp (GLuint v) const	{ return v+v%2; }
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

CDatapak::CDatapak (GLXContext ctx, goid_t cid, unique_c_ptr<GLubyte>&& p, GLuint psz) noexcept
: CGObject(ctx, cid, GenId())
,_sz (psz)
,_p (move(p))
{
}

CDatapak::~CDatapak (void) noexcept
{
    auto id = Id();
    if (id != NoObject)
	glDeleteBuffers (1, &id);
}

const GLubyte* CDatapak::File (const char* filename, GLuint& sz) const noexcept
{
    if (!_p)
	return nullptr;
    using pch_t	= const CpioHeader*;
    for (auto pi = (pch_t)_p.get(), pe = (pch_t)(_p.get()+_sz); pi < pe; pi = pi->Next()) {
	if (!strcmp (pi->Filename(), filename)) {
	    sz = pi->Filesize();
	    return pi->Filedata();
	}
    }
    return nullptr;
}

//----------------------------------------------------------------------

/*static*/ unique_c_ptr<GLubyte> CDatapak::DecompressBlock (const GLubyte* p, unsigned isz, unsigned& osz)
{
    z_stream zs;
    memset (&zs, 0, sizeof(zs));

    zs.avail_in = isz;
    zs.next_in = const_cast<Bytef*>(p);
    auto bufsz = 4096u, bread = 0u, chunksz = bufsz;
    unique_c_ptr<GLubyte> out ((GLubyte*) malloc (bufsz));
    zs.avail_out = bufsz;
    zs.next_out = out.get();

    enum { USE_GZIP_FORMAT = 16 };	// zlib fairy dust
    if (Z_OK != inflateInit2 (&zs, USE_GZIP_FORMAT+MAX_WBITS))
	CFile::Error ("gzip");

    for (;;) {
	auto r = inflate (&zs, Z_NO_FLUSH);
	if (r == Z_STREAM_END)
	    break;
	else if (r == Z_OK) {
	    bread += chunksz-zs.avail_out;
	    out.realloc (bufsz*2);
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
    return out;
}

//----------------------------------------------------------------------

CFramebuffer::CFramebuffer (GLXContext ctx, goid_t cid, const GLubyte* p, GLuint psz, const CIConn& conn)
: CGObject (ctx, cid, GenId())
,_w(0)
,_h(0)
{
    try {
	glBindFramebuffer (GL_FRAMEBUFFER, Id());
	auto icomp = (const G::FramebufferComponent*) p;
	unsigned ncomp = psz / sizeof(G::FramebufferComponent);
	for (auto i = 0u; i < ncomp; ++i) {
	    auto& tex = conn.LookupTexture (icomp[i].texture);
	    Attach (icomp[i], tex);
	    _w = tex.Width();
	    _h = tex.Height();
	}
    } catch (...) {	// LookupTexture throws on invalid goid
	Free();
	throw;
    }
}

void CFramebuffer::Attach (const G::FramebufferComponent& c, const CTexture& tex) const noexcept
{
    //{{{ gldefs enums to glenum lookup arrays
    static const GLenum c_Target[] = { GL_FRAMEBUFFER, GL_READ_FRAMEBUFFER };
    static const GLenum c_Attachment[] = {
	GL_DEPTH_ATTACHMENT,
	GL_STENCIL_ATTACHMENT,
	GL_DEPTH_STENCIL_ATTACHMENT,
	GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
	GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7,
	GL_COLOR_ATTACHMENT8, GL_COLOR_ATTACHMENT9, GL_COLOR_ATTACHMENT10, GL_COLOR_ATTACHMENT11,
	GL_COLOR_ATTACHMENT12, GL_COLOR_ATTACHMENT13, GL_COLOR_ATTACHMENT14, GL_COLOR_ATTACHMENT15
    };
    //}}}
    DTRACE ("\tAttaching texture %x to target %u, attachment %u, level %u\n", tex.CId(), c.target, c.attachment, c.level);
    auto targ = c_Target [min<uint8_t>(c.target, ArraySize(c_Target)-1)];
    auto attach = c_Attachment [min<uint8_t>(c.attachment, ArraySize(c_Attachment)-1)];
    GLenum tt = G::Texture::TypeFromTextureType ((G::TextureType) c.textype);
    if (c.textype >= G::TEXTURE_3D)
	glFramebufferTexture3D (targ, attach, tt, tex.Id(), 0, c.level);
    else if (c.textype >= G::TEXTURE_2D)
	glFramebufferTexture2D (targ, attach, tt, tex.Id(), c.level);
    else
	glFramebufferTexture1D (targ, attach, tt, tex.Id(), c.level);
}

void CFramebuffer::Free (void) noexcept
{
    auto id = Id();
    if (id != NoObject) {
	ResetId();
	glDeleteFramebuffers (1, &id);
    }
}

#include "gopak.h"

//{{{2 CPIO file format definitions
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
private:
    inline GLuint		EvenUp (GLuint v) const	{ return (v+v%2); }
private:
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
//}}}2

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

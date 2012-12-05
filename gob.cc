#include "gleri.h"
#include "gob.h"
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <zlib.h>
#include <png.h>

//----------------------------------------------------------------------
//{{{ CMMFile

void CMMFile::Open (const char* filename)
{
    _fd = open (filename, O_RDONLY);
    if (_fd < 0)
	throw XError ("%s %s: %s", "open", filename, strerror(errno));
    _sz = lseek (_fd, 0, SEEK_END);
    _p = (GLubyte*) mmap (nullptr, _sz, PROT_READ, MAP_PRIVATE, _fd, 0);
    if (_p == MAP_FAILED) {
	close (_fd);
	throw XError ("%s %s: %s", "mmap", filename, strerror(errno));
    }
}

void CMMFile::Close (void)
{
    munmap (_p, _sz);
    close (_fd);
}

//}}}-------------------------------------------------------------------
//{{{ Decompress block

GLubyte* DecompressBlock (const GLubyte* p, GLuint isz, GLuint& osz)
{
    z_stream zs;
    memset (&zs, 0, sizeof(zs));

    zs.avail_in = isz;
    zs.next_in = const_cast<Bytef*>(p);
    GLuint bufsz = 4096, bread = 0;
    GLuint chunksz = bufsz;
    GLubyte* out = (GLubyte*) malloc (bufsz);
    zs.avail_out = bufsz;
    zs.next_out = out;

    enum { USE_GZIP_FORMAT = 16 };	// zlib fairy dust
    if (Z_OK != inflateInit2 (&zs, USE_GZIP_FORMAT+MAX_WBITS))
	throw XError ("gzip decompression error");

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
	    throw XError ("gzip decompression error");
	}
    }
    osz = bread + chunksz-zs.avail_out;
    inflateEnd (&zs);
    return (out);
}

//}}}-------------------------------------------------------------------
//{{{ Datapak

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

//}}}-------------------------------------------------------------------
//{{{ Shader loading

/*static*/ const GLushort CShader::Sources::c_ShaderType [shader_NStages] = {
    GL_VERTEX_SHADER,
    GL_TESS_CONTROL_SHADER,
    GL_TESS_EVALUATION_SHADER,
    GL_GEOMETRY_SHADER,
    GL_FRAGMENT_SHADER
};

inline void CShader::Sources::ShaderSource (GLuint id, GLuint idx) const noexcept
{
    glShaderSource (id, 1, const_cast<const char**>(&_stage[idx]), &_stageSize[idx]);
}

void CShader::Sources::LoadFromPak (const CDatapak& pak)
{
    GLuint sz;
    for (GLuint i = 0; i < shader_NStages; ++i) {
	if (!_stage[i]) continue;
	_stage[i] = (const char*) pak.File(_stage[i], sz);
	_stageSize[i] = sz;
    }
}

void CShader::Load (const Sources& src)
{
    GLint result = False;
    int infoLogLength = 0;

    GLuint stages [Sources::shader_NStages];
    for (GLuint i = 0; i < Sources::shader_NStages; ++i) {
	if (!src.HaveStage(i)) {
	    stages[i] = UINT_MAX;
	    continue;
	}
	stages[i] = glCreateShader (src.ShaderType(i));
	src.ShaderSource (stages[i], i);
	glCompileShader (stages[i]);
	glGetShaderiv (stages[i], GL_COMPILE_STATUS, &result);
	if (!result) {
	    glGetShaderiv (stages[i], GL_INFO_LOG_LENGTH, &infoLogLength);
	    char infoLog [infoLogLength];
	    glGetShaderInfoLog (stages[i], infoLogLength, nullptr, infoLog);
	    throw XError ("shader error:\n%s", infoLog);
	}
    }

    for (GLuint i = 0; i < Sources::shader_NStages; ++i)
	if (stages[i] != UINT_MAX)
	    glAttachShader (Id(), stages[i]);
    glLinkProgram (Id());

    glGetProgramiv (Id(), GL_LINK_STATUS, &result);
    if (!result) {
	glGetProgramiv (Id(), GL_INFO_LOG_LENGTH, &infoLogLength);
	char infoLog [infoLogLength];
	glGetProgramInfoLog (Id(), infoLogLength, nullptr, infoLog);
	throw XError ("shader error:\n%s", infoLog);
    }

    for (GLuint i = 0; i < Sources::shader_NStages; ++i)
	if (stages[i] != UINT_MAX)
	    glDeleteShader (stages[i]);
}

CShader::~CShader (void) noexcept
{
    if (Id() != NoObject)
	glDeleteProgram (Id());
}

//}}}-------------------------------------------------------------------
//{{{ PSF font loading

CFont::CFont (GLXContext ctx, const GLubyte* p, GLuint psz) noexcept
: CGObject (ctx, GenId())
,_width(8)
,_height(0)
,_rowwidth(0)
{
    //{{{2 PSF format definitions ------------------------------------------
    enum {
	PSF1_MAGIC	= 0x0436,
	PSF1_SEPARATOR	= 0xFFFF,
	PSF1_STARTSEQ	= 0xFFFE,
	PSF1_MODE512	= 1,
	PSF1_MODEHASTAB	= 2,
	PSF1_MODEHASSEQ	= 4,
	PSF1_MAXMODE	= 5
    };
    enum {
	PSF2_MAGIC	= 0x864ab572,
	PSF2_STARTSEQ	= 0xFE,
	PSF2_SEPARATOR	= 0xFF,
	PSF2_MAXVERSION	= 0,
	PSF2_HAS_UNICODE_TABLE = 1
    };
    struct SPsf1Header {
	uint16_t	magic;
	uint8_t	flags;
	uint8_t	height;
    };
    struct SPsf2Header {
	uint32_t	magic;
	uint32_t	version;
	uint32_t	headersize;
	uint32_t	flags;
	uint32_t	length;
	uint32_t	charsize;
	uint32_t	height;
	uint32_t	width;
    };
    //}}}2------------------------------------------------------------------

    const GLubyte* pend = p+psz;
    const SPsf1Header* ph1 = (const SPsf1Header*) p;
    const SPsf2Header* ph2 = (const SPsf2Header*) p;
    GLuint nChars = 256;
    if (ph1->magic == PSF1_MAGIC) {
	_height = ph1->height;
	p += sizeof(*ph1);
    } else if (ph2->magic == PSF2_MAGIC) {
	_width = ph2->width;
	_height = ph2->height;
	nChars = ph2->length;
	p += ph2->headersize;
    }
    if (!_width) return;
    _rowwidth = 256/_width;

    GLubyte* ftexbmp = (GLubyte*) malloc (256*256);
    GLubyte *rowo = ftexbmp, *colo = ftexbmp;
    const GLuint rowskip = _height*256, lineskip = 256-_width;
    for (GLuint c = 0, col = 0; c < nChars && p < pend; ++c, ++col, colo += _width) {
	if (col >= _rowwidth) {
	    col = 0;
	    rowo += rowskip;
	    colo = rowo;
	}
	GLubyte* o = colo;
	for (GLuint y = 0; y < _height; ++y, ++p, o += lineskip) {
	    GLubyte mask = 0x80;
	    for (GLuint x = 0; x < _width; ++x) {
		*o++ = (p[0] & mask) ? 0xff : 0;
		if (!(mask >>= 1)) {
		    mask = 0x80;
		    ++p;
		}
	    }
	}
    }

    glBindTexture (GL_TEXTURE_2D, Id());
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_COMPRESSED_RED, 256, 256, 0, GL_RED, GL_UNSIGNED_BYTE, ftexbmp);
    free (ftexbmp);
}

CFont::~CFont (void) noexcept
{
    GLuint id = Id();
    if (id != NoObject)
	glDeleteTextures (1, &id);
}

//}}}-------------------------------------------------------------------
//{{{ Texture loading

namespace {
static void png_data_source (png_structp rs, png_bytep p, png_size_t n)
{
    const uint8_t** rbuf = (const uint8_t**) png_get_io_ptr(rs);
    memcpy (p, *rbuf, n);
    *rbuf += n;
}
} // namespace

inline GLubyte* CTexture::LoadPNG (const GLubyte* p, GLuint) noexcept
{
    png_structp rs = png_create_read_struct (PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop infos = png_create_info_struct (rs);
    png_set_read_fn (rs, &p, png_data_source);
    png_read_info (rs, infos);

    _width = png_get_image_width (rs, infos);
    _height = png_get_image_height (rs, infos);
    if (png_get_valid (rs, infos, PNG_INFO_tRNS))
	png_set_tRNS_to_alpha (rs);
    else
	png_set_filler (rs, 0xff, PNG_FILLER_AFTER);
    switch (png_get_color_type(rs, infos)) {
	case PNG_COLOR_TYPE_PALETTE:	png_set_palette_to_rgb(rs); break;
	case PNG_COLOR_TYPE_RGB_ALPHA:	png_set_swap_alpha(rs); break;
    }

    GLubyte* idata = (GLubyte*) malloc (4*_width*_height);
    if (!idata) return (idata);
    GLubyte* rows [_height];
    for (GLuint i=0, rbw=4*_width; i < _height; ++i)
	rows[i] = idata+i*rbw;
    png_read_image (rs, &rows[0]);

    png_destroy_read_struct (&rs, &infos, nullptr);
    return (idata);
}

CTexture::CTexture (GLXContext ctx, const GLubyte* p, GLuint psz) noexcept
: CGObject (ctx, GenId())
,_width(0)
,_height(0)
{
    GLubyte* idata = LoadPNG (p, psz);
    if (!idata) return;
    glBindTexture (GL_TEXTURE_2D, Id());
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, idata);
    free (idata);
}

CTexture::~CTexture (void) noexcept
{
    GLuint id = Id();
    if (id != NoObject)
	glDeleteTextures (1, &id);
}

//}}}-------------------------------------------------------------------

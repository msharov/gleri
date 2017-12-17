// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gotex.h"
#include "gleri/mmfile.h"

//{{{ Param ------------------------------------------------------------

const int CTexture::CParam::c_Defaults [G::Texture::NPARAMS] = {
    GL_LINEAR,		// MAG_FILTER
    GL_NEAREST_MIPMAP_LINEAR,	// MIN_FILTER
    1000,		// MAX_LOD
    -1000,		// MIN_LOD
    0,			// BASE_LEVEL
    1000,		// MAX_LEVEL
    GL_REPEAT,		// WRAP_S
    GL_REPEAT,		// WRAP_T
    GL_REPEAT,		// WRAP_R
    0,			// BORDER_COLOR
    0,			// PRIORITY
    G::Texture::COMPARE_MODE_NONE,	// COMPARE_MODE
    G::Texture::COMPARE_LEQUAL,		// COMPARE_FUNC
    G::Texture::DEPTH_IS_LUMINANCE,	// DEPTH_TEXTURE_MODE
    false		// GENERATE_MIPMAP
};

const uint16_t CTexture::CParam::c_GLCode [G::Texture::NPARAMS] = {
    GL_TEXTURE_MAG_FILTER,
    GL_TEXTURE_MIN_FILTER,
    GL_TEXTURE_MAX_LOD,
    GL_TEXTURE_MIN_LOD,
    GL_TEXTURE_BASE_LEVEL,
    GL_TEXTURE_MAX_LEVEL,
    GL_TEXTURE_WRAP_S,
    GL_TEXTURE_WRAP_T,
    GL_TEXTURE_WRAP_R,
    GL_TEXTURE_BORDER_COLOR,
    GL_TEXTURE_PRIORITY,
    GL_TEXTURE_COMPARE_MODE,
    GL_TEXTURE_COMPARE_FUNC,
    GL_DEPTH_TEXTURE_MODE,
    GL_GENERATE_MIPMAP
};

//}}}-------------------------------------------------------------------
//{{{ TexBuf

CTexture::CTexBuf::CTexBuf (G::Pixel::Fmt fmt, G::Pixel::Comp comp, uint16_t w, uint32_t roww, uint16_t h, uint16_t d)
:_info()
,_imgd()
,_imgsz()
{
    _info.w = w;
    _info.h = h;
    _info.d = d;
    _info.fmt = fmt;
    _info.comp = comp;
    _info.nImages = 1;
    Resize (w, roww, h);
}

void CTexture::CTexBuf::Resize (uint16_t w, uint32_t roww, uint16_t h)
{
    _imgd.resize (max(roww,w*4u)*h);
    _info.size = _imgd.size();
}

auto CTexture::CTexBuf::Data (unsigned i) const -> const_pointer
{
    auto offset = 0u;
    for (auto j = 0u; j < i; ++i)
	offset += Size (j);
    if (offset >= _imgd.size())
	return nullptr;
    return reinterpret_cast<const_pointer>(&_imgd[offset]);
}

uint32_t CTexture::CTexBuf::Size (unsigned i) const
{
    if (!i)
	return Size();
    else if (--i >= _imgsz.size())
	XError::emit ("not all image sizes are specified");
    return _imgsz[i];
}

void CTexture::CTexBuf::Load (const GLubyte* p, GLuint psz)
{
    using namespace G::Texture;
    if (psz < sizeof(GLTXHeader))
	XError::emit ("invalid gltx file");
    auto h = reinterpret_cast<const GLTXHeader*>(p);
    _info = h->info;
    if (h->magic != GLTXHeader::Magic
	    || h->version != GLTXHeader::Version
	    || (Info().nImages && h->dataOffset != sizeof(GLTXHeader) + (Info().nImages - 1) * sizeof(uint32_t)))
	XError::emit ("invalid gltx file");
    if (Info().nImages) {
	_imgsz.resize (Info().nImages - 1);
	auto pimgsz = reinterpret_cast<const uint32_t*>(p + sizeof(GLTXHeader));
	copy_n (pimgsz, _imgsz.size(), _imgsz.begin());
    }
    auto wsize = Info().size;
    for (auto sz : _imgsz)
	wsize += sz;
    if (!wsize)
	wsize = psz - h->dataOffset;
    else if (wsize + h->dataOffset > psz)
	XError::emit ("invalid gltx file");
    _imgd.resize (wsize);
    copy_n (p + h->dataOffset, wsize, _imgd.begin());
}

void CTexture::CTexBuf::Save (int fd) const
{
    CFile f (fd);
    G::Texture::GLTXHeader h;
    h.info = Info();
    h.dataOffset = sizeof(G::Texture::GLTXHeader) + _imgsz.size() * sizeof(_imgsz[0]);
    f.Write (&h, sizeof(h));
    f.Write (&_imgsz[0], _imgsz.size() * sizeof(_imgsz[0]));
    f.Write (&_imgd[0], _imgd.size() * sizeof(_imgd[0]));
    f.Detach();
}

//}}}-------------------------------------------------------------------
//{{{ CTexture

CTexture::CTexture (GLXContext ctx, goid_t cid)
: CGObject(ctx,cid,GenId())
,_info()
{
}

CTexture::CTexture (GLXContext ctx, goid_t cid, const GLubyte* p, GLuint psz, G::Pixel::Fmt storeas, G::TextureType ttype, const CParam& param)
: CGObject (ctx, cid, GenId())
,_info()
{
    if (psz < sizeof(G::Texture::GLTXHeader))
	XError::emit ("invalid texture data");
    auto tbuf = Load (p, psz);
    if (psz > sizeof(G::Texture::GLTXHeader) && !tbuf.Data())
	XError::emit ("invalid texture data");
    Create (tbuf, storeas, ttype, param);
}

void CTexture::Create (const CTexBuf& tbuf, G::Pixel::Fmt storeas, G::TextureType ttype, const CParam& param)
{
    _info = tbuf.Info();
    _info.type = G::Texture::TypeFromTextureType (ttype);
    glBindTexture (_info.type, Id());
    for (auto p = 0u; p < G::Texture::NPARAMS; ++p)
	if (!param.IsDefault (G::Texture::Parameter(p)))
	    glTexParameteri (_info.type, param.GLCode(G::Texture::Parameter(p)), param.Get (ttype, G::Texture::Parameter(p)));
    auto pixels = tbuf.Data();
    if (pixels && tbuf.Size() < G::Pixel::TextureSize (_info.fmt, _info.comp, _info.w, _info.h + !_info.h) * (_info.d + !_info.d))
	XError::emit ("incomplete texture data");
    if (ttype >= G::TEXTURE_3D)
	glTexImage3D (_info.type, 0, storeas, _info.w, _info.h, _info.d, 0, _info.fmt, _info.comp, pixels);
    else if (ttype >= G::TEXTURE_2D)
	glTexImage2D (_info.type, _info.d, storeas, _info.w, _info.h, 0, _info.fmt, _info.comp, pixels);
    else
	glTexImage1D (_info.type, _info.d, storeas, _info.w, 0, _info.fmt, _info.comp, pixels);
    // Query actual texture parameters
    GLint tlp;
    glGetTexLevelParameteriv (_info.type, 0, GL_TEXTURE_INTERNAL_FORMAT, &tlp);
    _info.fmt = G::Pixel::Fmt(tlp);	// the actual compressed format, for example
    glGetTexLevelParameteriv (_info.type, 0, GL_TEXTURE_COMPRESSED, &tlp);
    if (tlp) {		// if compressed, then the compressed size can be retrieved
	glGetTexLevelParameteriv (_info.type, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &tlp);
	_info.size = tlp;
    }
}

void CTexture::Free (void) noexcept
{
    auto id = Id();
    if (id != NoObject) {
	ResetId();
	glDeleteTextures (1, &id);
    }
}

CTexture::CTexBuf CTexture::Load (const GLubyte* p, GLuint psz) // static
{
    auto& magic = *reinterpret_cast<const uint32_t*>(p);
    if (magic == G::Texture::GLTXHeader::Magic)
	return LoadGLTX (p, psz);
    #if HAVE_PNG_H
    else if (magic == vpack4(0x89,'P','N','G'))
	return LoadPNG (p, psz);
    #endif
    #if HAVE_JPEGLIB_H
    else if (uint16_t(magic) == vpack2(0xff,0xd8))
	return LoadJPG (p, psz);
    #endif
    #if HAVE_GIF_LIB_H
    else if (magic == vpack4('G','I','F','8'))
	return LoadGIF (p, psz);
    #endif
    else
	XError::emit ("unrecognized image file format");
}

void CTexture::Save (int fd, GLuint x, GLuint y, GLuint w, GLuint h, G::Texture::Format fmt, uint8_t quality) // static
{
    CTexBuf tbuf (G::Pixel::RGB, G::Pixel::UNSIGNED_BYTE, w, w*4, h);
    glReadPixels (x, y, w, h, tbuf.Info().fmt, tbuf.Info().comp, tbuf.Data());
    if (fmt == G::Texture::Format::GLTX)
	tbuf.Save (fd);
    #if HAVE_PNG_H
    else if (fmt == G::Texture::Format::PNG)
	SavePNG (fd, tbuf);
    #endif
    #if HAVE_JPEGLIB_H
    else if (fmt == G::Texture::Format::JPEG)
	SaveJPG (fd, tbuf, quality);
    #endif
    else
	XError::emit ("unrecognized image file format");
}

//}}}-------------------------------------------------------------------
//{{{ GLTX format

CTexture::CTexBuf CTexture::LoadGLTX (const GLubyte* p, GLuint psz) // static
{
    CTexBuf tbuf;
    tbuf.Load (p, psz);
    return tbuf;
}

//}}}-------------------------------------------------------------------
//{{{ PNG format

#if HAVE_PNG_H
#include <png.h>

namespace {
struct PngDataPtr {
    const GLubyte* p;
    GLuint sz;
};
static void png_data_source (png_structp rs, png_bytep p, png_size_t n)
{
    auto rbuf = (PngDataPtr*) png_get_io_ptr(rs);
    auto btc = min<GLuint> (rbuf->sz, n);
    memcpy (p, rbuf->p, btc);
    rbuf->p += btc;
    rbuf->sz -= btc;
}
} // namespace

CTexture::CTexBuf CTexture::LoadPNG (const GLubyte* p, GLuint sz) // static
{
    auto rs = png_create_read_struct (PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop infos = nullptr;
    if (rs)
	infos = png_create_info_struct (rs);
    if (setjmp (png_jmpbuf (rs)) || !infos) {
	png_destroy_read_struct (&rs, &infos, nullptr);
	XError::emit ("invalid png file");
    }
    PngDataPtr pdp = { p, sz };
    png_set_read_fn (rs, &pdp, png_data_source);
    png_read_info (rs, infos);

    if (png_get_valid (rs, infos, PNG_INFO_tRNS))
	png_set_tRNS_to_alpha (rs);
    else
	png_set_filler (rs, 0xff, PNG_FILLER_AFTER);
    if (png_get_bit_depth (rs, infos) > 8)
	png_set_scale_16 (rs);
    switch (png_get_color_type(rs, infos)) {
	case PNG_COLOR_TYPE_PALETTE:	png_set_palette_to_rgb(rs); break;
	case PNG_COLOR_TYPE_GRAY:	png_set_gray_to_rgb(rs); break;
    }
    unsigned w = png_get_image_width (rs, infos),
	    rb = png_get_rowbytes (rs, infos),
	     h = png_get_image_height (rs, infos);
    if (!w || !h || w > c_MaxWidth || h > c_MaxHeight || rb > c_MaxWidth) {
	png_destroy_read_struct (&rs, &infos, nullptr);
	XError::emit ("invalid png file");
    }
    CTexBuf tbuf (G::Pixel::RGBA, G::Pixel::UNSIGNED_BYTE, w, rb, h);

    auto idata = tbuf.Data();
    if (!idata)
	return tbuf;
    png_byte* rows [tbuf.Info().h];
    for (auto i = 0u; i < tbuf.Info().h; ++i)
	rows[i] = (png_byte*)(idata+((tbuf.Info().h-1)-i)*tbuf.Info().w);
    png_read_image (rs, rows);

    png_destroy_read_struct (&rs, &infos, nullptr);
    return tbuf;
}

void CTexture::SavePNG (int fd, const CTexBuf& tbuf) // static
{
    auto outfile = fdopen (fd, "wb");
    if (!outfile)
	XError::emit ("failed to open output png file");
    auto png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info_ptr = nullptr;
    if (png_ptr)
	info_ptr = png_create_info_struct (png_ptr);
    if (setjmp (png_jmpbuf (png_ptr)) || !info_ptr) {
	png_destroy_write_struct (&png_ptr, &info_ptr);
	XError::emit ("failed to write output png file");
    }
    png_init_io (png_ptr, outfile);

    png_set_IHDR (png_ptr, info_ptr, tbuf.Info().w, tbuf.Info().h,
		8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    auto ppix = const_cast<png_byte*>(reinterpret_cast<const GLubyte*>(tbuf.Data()));
    png_byte* pline [tbuf.Info().h];
    for (auto i = 0u; i < tbuf.Info().h; ++i)
	pline[i] = &ppix[((tbuf.Info().h-1)-i)*tbuf.Info().w*3];

    png_set_rows (png_ptr, info_ptr, pline);
    png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

    fflush (outfile);
    png_destroy_write_struct (&png_ptr, &info_ptr);
}

#endif
//}}}-------------------------------------------------------------------
//{{{ JPG format

#if HAVE_JPEGLIB_H
#include <jpeglib.h>
#include <jerror.h>

CTexture::CTexBuf CTexture::LoadJPG (const GLubyte* p, GLuint psz) // static
{
    jpeg_decompress_struct cinfo;
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error (&jerr);
    jpeg_create_decompress (&cinfo);
    jpeg_mem_src (&cinfo, const_cast<unsigned char*>(p), psz);
    jpeg_read_header (&cinfo, TRUE);
    jpeg_start_decompress (&cinfo);
    unsigned w = cinfo.output_width, h = cinfo.output_height, s = cinfo.output_components;
    if (!w || !h || w > c_MaxWidth || h > c_MaxHeight)
	XError::emit ("invalid jpg file");
    auto linew = Align(w*3,4);	// OpenGL requires line padding to 4 bytes
    CTexBuf imgbuf (G::Pixel::RGB, G::Pixel::UNSIGNED_BYTE, w, linew, h);
    auto ppd = (unsigned char*) imgbuf.Data();
    while (cinfo.output_scanline < h) {
	auto j = cinfo.output_scanline;
	JSAMPLE linebuf[w*s], *jsarr[1] = {linebuf};
	jpeg_read_scanlines (&cinfo, jsarr, 1);
	auto poline = &ppd[((h-1)-j)*linew];
	if (s == 3)
	    memcpy (poline, jsarr[0], w*s);
	else if (s == 1) {
	    for (auto i = 0u; i < w; ++i) {
		uint8_t c = jsarr[0][i];
		poline[i*3+0] = c;
		poline[i*3+1] = c;
		poline[i*3+2] = c;
	    }
	}
    }
    jpeg_finish_decompress (&cinfo);
    jpeg_destroy_decompress (&cinfo);
    return imgbuf;
}

void CTexture::SaveJPG (int fd, const CTexBuf& tbuf, uint8_t quality) // static
{
    auto outfile = fdopen (fd, "wb");
    if (!outfile)
	XError::emit ("failed to open output jpg file");
    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error (&jerr);
    jpeg_create_compress (&cinfo);
    jpeg_stdio_dest (&cinfo, outfile);

    cinfo.image_width = tbuf.Info().w;
    cinfo.image_height = tbuf.Info().h;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults (&cinfo);
    jpeg_set_quality (&cinfo, quality, TRUE);
    jpeg_start_compress (&cinfo, TRUE);

    auto ppix = const_cast<JSAMPROW>(reinterpret_cast<const GLubyte*>(tbuf.Data()));
    JSAMPROW pline [cinfo.image_height];
    for (auto i = 0u; i < cinfo.image_height; ++i)
	pline[i] = &ppix[((cinfo.image_height-1)-i)*cinfo.image_width*3];
    jpeg_write_scanlines (&cinfo, pline, cinfo.image_height);

    fflush (outfile);
    jpeg_finish_compress (&cinfo);
    jpeg_destroy_compress (&cinfo);
}

#endif
//}}}-------------------------------------------------------------------
//{{{ GIF format

#if HAVE_GIF_LIB_H
#include <gif_lib.h>

namespace {

struct GIFBuffer {
    const GifByteType*	p;
    int			size;
    int			pos;
};

static int GIFReader (GifFileType* giff, GifByteType* buf, int bufsz)
{
    auto pgr = reinterpret_cast<GIFBuffer*>(giff->UserData);
    auto btc = min (bufsz, pgr->size - pgr->pos);
    copy_n (&pgr->p[pgr->pos], btc, buf);
    pgr->pos += btc;
    return btc;
}

struct GIFFile {
    GifFileType* p;
    inline GIFFile (void) : p(nullptr) {}
    ~GIFFile (void) noexcept {
	if (p) {
	    auto err = 0;
	    DGifCloseFile (p, &err);
	    if (err)
		DTRACE ("DGifCloseFile failed with code %d\n", err);
	    p = nullptr;
	}
    }
};

} // namespace

CTexture::CTexBuf CTexture::LoadGIF (const GLubyte* p, GLuint psz) // static
{
    GIFBuffer gb = { p, int(psz), 0 };
    GIFFile gf;
    auto err = 0;
    gf.p = DGifOpen (&gb, &GIFReader, &err);
    if (err)
	XError::emit ("invalid gif file");
    if (!DGifSlurp (gf.p))
	XError::emit ("invalid gif file");
    auto psimg = gf.p->SavedImages;
    if (gf.p->ImageCount < 1
	    || !psimg
	    || !psimg->ImageDesc.Width
	    || !psimg->ImageDesc.Height
	    || (unsigned) psimg->ImageDesc.Width > (unsigned) c_MaxWidth
	    || (unsigned) psimg->ImageDesc.Height > (unsigned) c_MaxHeight)
	XError::emit ("invalid gif file");
    auto imgcmap = psimg->ImageDesc.ColorMap;
    if (!imgcmap)
	imgcmap = gf.p->SColorMap;
    if (!imgcmap || imgcmap->ColorCount > 256)
	XError::emit ("invalid gif file");

    // Convert colormap from RGB to RGBA
    G::color_t cmap [256];
    for (auto i = 0; i < imgcmap->ColorCount; ++i)
	cmap[i] = RGB(imgcmap->Colors[i].Red, imgcmap->Colors[i].Green, imgcmap->Colors[i].Blue);

    // Check if transparent color is set
    GraphicsControlBlock gcb;
    if (DGifSavedExtensionToGCB (gf.p, 0, &gcb))
	cmap[uint8_t(gcb.TransparentColor)] = RGBA(0,0,0,0);

    // Create the TexBuf
    unsigned w = psimg->ImageDesc.Width, h = psimg->ImageDesc.Height;
    CTexBuf imgbuf (G::Pixel::RGBA, G::Pixel::UNSIGNED_BYTE, w, 0, h);
    auto itex = imgbuf.Data();

    // Copy pixels, converting from palette index to RGBA
    for (auto y = 0u; y < h; ++y)
	for (auto x = 0u; x < w; ++x)
	    itex[y*w+x] = cmap[psimg->RasterBits[(h-1-y)*w+x]];	// GIF lines are upside down

    return imgbuf;
}

#endif
//}}}-------------------------------------------------------------------

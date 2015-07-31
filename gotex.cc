// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gotex.h"
#include "gleri/mmfile.h"

//{{{ Param ------------------------------------------------------------

/*static*/ const int CTexture::CParam::c_Defaults [G::Texture::NPARAMS] = {
    GL_NEAREST,		// MAG_FILTER
    GL_NEAREST		// MIN_FILTER
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
    if (wsize) {
	if (wsize + h->dataOffset > psz)
	    XError::emit ("invalid gltx file");
	_imgd.resize (wsize);
	copy_n (p + h->dataOffset, wsize, _imgd.begin());
    }
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
    glTexParameteri (_info.type, GL_TEXTURE_MAG_FILTER, param.Get (ttype, G::Texture::MAG_FILTER));
    glTexParameteri (_info.type, GL_TEXTURE_MIN_FILTER, param.Get (ttype, G::Texture::MIN_FILTER));
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

/*static*/ inline CTexture::CTexBuf CTexture::Load (const GLubyte* p, GLuint psz)
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
    else
	XError::emit ("unrecognized image file format");
}

/*static*/ void CTexture::Save (int fd, GLuint x, GLuint y, GLuint w, GLuint h, G::Texture::Format fmt, uint8_t quality)
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

/*static*/ CTexture::CTexBuf CTexture::LoadGLTX (const GLubyte* p, GLuint psz)
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
static void png_data_source (png_structp rs, png_bytep p, png_size_t n)
{
    auto rbuf = (const GLubyte**) png_get_io_ptr(rs);
    memcpy (p, *rbuf, n);
    *rbuf += n;
}
} // namespace

/*static*/ CTexture::CTexBuf CTexture::LoadPNG (const GLubyte* p, GLuint)
{
    auto rs = png_create_read_struct (PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop infos = nullptr;
    if (rs)
	infos = png_create_info_struct (rs);
    if (setjmp (png_jmpbuf (rs)) || !infos) {
	png_destroy_read_struct (&rs, &infos, nullptr);
	XError::emit ("invalid png file");
    }
    png_set_read_fn (rs, &p, png_data_source);
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
	case PNG_COLOR_TYPE_RGB_ALPHA:	png_set_swap_alpha(rs); break;
    }
    CTexBuf tbuf (G::Pixel::RGBA, G::Pixel::UNSIGNED_BYTE,
		png_get_image_width (rs, infos),
		png_get_rowbytes (rs, infos),
		png_get_image_height (rs, infos));

    auto idata = tbuf.Data();
    if (!idata) return tbuf;
    png_byte* rows [tbuf.Info().h];
    for (auto i = 0u; i < tbuf.Info().h; ++i)
	rows[i] = (png_byte*)(idata+((tbuf.Info().h-1)-i)*tbuf.Info().w);
    png_read_image (rs, rows);

    png_destroy_read_struct (&rs, &infos, nullptr);
    return tbuf;
}

/*static*/ void CTexture::SavePNG (int fd, const CTexBuf& tbuf)
{
    auto outfile = fdopen (fd, "wb");
    if (!outfile) return;
    auto png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info_ptr = nullptr;
    if (png_ptr)
	info_ptr = png_create_info_struct (png_ptr);
    if (setjmp (png_jmpbuf (png_ptr)) || !info_ptr) {
	png_destroy_write_struct (&png_ptr, &info_ptr);
	return;
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

/*static*/ CTexture::CTexBuf CTexture::LoadJPG (const GLubyte* p, GLuint psz) noexcept
{
    jpeg_decompress_struct cinfo;
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error (&jerr);
    jpeg_create_decompress (&cinfo);
    jpeg_mem_src (&cinfo, const_cast<unsigned char*>(p), psz);
    jpeg_read_header (&cinfo, TRUE);
    jpeg_start_decompress (&cinfo);
    unsigned w = cinfo.output_width, h = cinfo.output_height, s = cinfo.output_components;
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

/*static*/ void CTexture::SaveJPG (int fd, const CTexBuf& tbuf, uint8_t quality)
{
    auto outfile = fdopen (fd, "wb");
    if (!outfile) return;
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

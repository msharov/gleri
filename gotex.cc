// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gotex.h"

CTexture::CTexture (GLXContext ctx, const GLubyte* p, GLuint psz) noexcept
: CGObject (ctx, GenId())
,_width(0)
,_height(0)
{
    if (psz < sizeof(G::STextureHeader)) {
	Free();
	return;
    }
    CTexBuf tbuf = Load (p, psz);
    if (!tbuf.Data()) {
	Free();
	return;
    }
    glBindTexture (GL_TEXTURE_2D, Id());
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    const G::STextureHeader& h = tbuf.Header();
    glTexImage2D (GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA, h.w, h.h, 0, h.comp, h.fmt, tbuf.Data());
    _width = h.w; _height = h.h;
}

void CTexture::Free (void) noexcept
{
    GLuint id = Id();
    if (id != NoObject) {
	ResetId();
	glDeleteTextures (1, &id);
    }
}

/*static*/ inline CTexture::CTexBuf CTexture::Load (const GLubyte* p, GLuint psz) noexcept
{
    const G::STextureHeader& inh = *reinterpret_cast<const G::STextureHeader*>(p);
    switch (inh.magic) {
    #if HAVE_PNG_H
	case vpack4(0x89,'P','N','G'):	return (LoadPNG (p, psz));
    #endif
    #if HAVE_JPEGLIB_H
	case vpack4(0xff,0xd8,0xff,0xe0):return (LoadJPG (p, psz));
    #endif
	case G::STextureHeader::Magic:	return (CTexBuf (inh));
	default:			return (CTexBuf());
    };
}

//{{{ PNG format -------------------------------------------------------

#if HAVE_PNG_H
#include <png.h>

namespace {
static void png_data_source (png_structp rs, png_bytep p, png_size_t n)
{
    const GLubyte** rbuf = (const GLubyte**) png_get_io_ptr(rs);
    memcpy (p, *rbuf, n);
    *rbuf += n;
}
} // namespace

/*static*/ inline CTexture::CTexBuf CTexture::LoadPNG (const GLubyte* p, GLuint) noexcept
{
    png_structp rs = png_create_read_struct (PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop infos = png_create_info_struct (rs);
    png_set_read_fn (rs, &p, png_data_source);
    png_read_info (rs, infos);

    if (png_get_valid (rs, infos, PNG_INFO_tRNS))
	png_set_tRNS_to_alpha (rs);
    else
	png_set_filler (rs, 0xff, PNG_FILLER_AFTER);
    switch (png_get_color_type(rs, infos)) {
	case PNG_COLOR_TYPE_PALETTE:	png_set_palette_to_rgb(rs); break;
	case PNG_COLOR_TYPE_RGB_ALPHA:	png_set_swap_alpha(rs); break;
    }
    CTexBuf tbuf (G::Pixel::RGBA, G::Pixel::UNSIGNED_BYTE,
		png_get_image_width (rs, infos),
		png_get_image_height (rs, infos));

    auto idata = tbuf.Data();
    if (!idata) return (tbuf);
    png_byte* rows [tbuf.Header().h];
    for (GLuint i=0; i < tbuf.Header().h; ++i)
	rows[i] = (png_byte*)(idata+i*tbuf.Header().w);
    png_read_image (rs, &rows[0]);

    png_destroy_read_struct (&rs, &infos, nullptr);
    return (tbuf);
}

#endif
//}}}-------------------------------------------------------------------
//{{{ JPG format

#if HAVE_JPEGLIB_H
#include <jpeglib.h>
#include <jerror.h>

/*static*/ inline CTexture::CTexBuf CTexture::LoadJPG (const GLubyte* p, GLuint psz) noexcept
{
    jpeg_decompress_struct cinfo;
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error (&jerr);
    jpeg_create_decompress (&cinfo);
    jpeg_mem_src (&cinfo, const_cast<unsigned char*>(p), psz);
    jpeg_read_header (&cinfo, TRUE);
    jpeg_start_decompress (&cinfo);
    unsigned w = cinfo.output_width, h = cinfo.output_height, s = cinfo.output_components;
    CTexBuf imgbuf (G::Pixel::RGB, G::Pixel::UNSIGNED_BYTE, w, h);
    unsigned char* ppd = (unsigned char*) imgbuf.Data();
    while (cinfo.output_scanline < h) {
	unsigned j = cinfo.output_scanline;
	JSAMPLE linebuf[w*s], *jsarr[1] = {linebuf};
	jpeg_read_scanlines (&cinfo, jsarr, 1);
	unsigned char* poline = &ppd[j*w*s];
	if (s == 3)
	    memcpy (poline, jsarr[0], w*s);
	else if (s == 1) {
	    for (unsigned i = 0; i < w; ++i) {
		unsigned char c = jsarr[0][i];
		poline[i*3+0] = c;
		poline[i*3+1] = c;
		poline[i*3+2] = c;
	    }
	}
    }
    jpeg_finish_decompress (&cinfo);
    jpeg_destroy_decompress (&cinfo);
    return (imgbuf);
}

#endif
//}}}-------------------------------------------------------------------

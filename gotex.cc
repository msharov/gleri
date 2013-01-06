// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gotex.h"
#include <png.h>

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

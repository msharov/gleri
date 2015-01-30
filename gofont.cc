// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gofont.h"

CFont::CFont (GLXContext ctx, goid_t cid, const GLubyte* p, GLuint psz) noexcept
: CTexture (ctx, cid)
,_info()
,_rowwidth(0)
{
    //{{{ PSF format definitions ------------------------------------------
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
    //}}}------------------------------------------------------------------

    auto pend = p+psz;
    auto ph1 = (const SPsf1Header*) p;
    auto ph2 = (const SPsf2Header*) p;
    auto nChars = 256u;
    if (ph1->magic == PSF1_MAGIC) {
	_info.SetWidth (8);
	_info.SetHeight (ph1->height);
	p += sizeof(*ph1);
    } else if (ph2->magic == PSF2_MAGIC) {
	_info.SetWidth (ph2->width);
	_info.SetHeight (ph2->height);
	nChars = ph2->length;
	p += ph2->headersize;
    }
    if (!_info.Width()) return;
    _rowwidth = 256/_info.Width();

    auto ftexbmp = (GLubyte*) malloc (256*256), rowo = ftexbmp, colo = ftexbmp;
    const GLuint rowskip = _info.Height()*256, lineskip = 256-_info.Width();
    for (GLuint c = 0, col = 0; c < nChars && p < pend; ++c, ++col, colo += _info.Width()) {
	if (col >= _rowwidth) {
	    col = 0;
	    rowo += rowskip;
	    colo = rowo;
	}
	GLubyte* o = colo;
	for (GLuint y = 0; y < _info.Height(); ++y, ++p, o += lineskip) {
	    GLubyte mask = 0x80;
	    for (GLuint x = 0; x < _info.Width(); ++x) {
		*o++ = (p[0] & mask) ? 0xff : 0;
		if (!(mask >>= 1)) {
		    mask = 0x80;
		    ++p;
		}
	    }
	}
    }

    glBindTexture (GL_TEXTURE_2D, Id());
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_COMPRESSED_RED, 256, 256, 0, GL_RED, GL_UNSIGNED_BYTE, ftexbmp);
    free (ftexbmp);
}

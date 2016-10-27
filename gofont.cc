// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gofont.h"
#if HAVE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#else
//{{{ Freetype-less fallback structs
enum {
    FT_PIXEL_MODE_MONO,
    FT_PIXEL_MODE_GRAY
};
struct FT_Bitmap {
    uint16_t	rows;
    uint16_t	width;
    uint16_t	pitch;
    uint16_t	pixel_mode;
    uint8_t*	buffer;
};
struct FT_Vector {
    uint8_t	x,y;
};
struct FT_GlyphSlotRec {
    FT_Bitmap	bitmap;
    FT_Vector	advance;
    int		bitmap_top;
    int		bitmap_left;
};
//}}}
#endif

//{{{ PSF format definitions -----------------------------------------

enum {
    PSF1_MODE512	= 1,
    PSF1_MODEHASTAB,
    PSF1_MODEHASSEQ	= 4,
    PSF1_MAXMODE,
    PSF1_MAGIC		= 0x0436,
    PSF1_STARTSEQ	= 0xFFFE,
    PSF1_SEPARATOR
};
struct SPsf1Header {
    uint16_t	magic;
    uint8_t	flags;
    uint8_t	height;
};
enum {
    PSF2_MAXVERSION,
    PSF2_HAS_UNICODE_TABLE,
    PSF2_MAGIC		= 0x864ab572,
    PSF2_STARTSEQ	= 0xFE,
    PSF2_SEPARATOR
};
struct SPsf2Header {
    uint32_t	magic;
    uint32_t	version;
    uint32_t	headersize;
    uint32_t	flags;
    uint32_t	nglyphs;
    uint32_t	glyphsize;
    uint32_t	height;
    uint32_t	width;
};

//}}}------------------------------------------------------------------
//{{{ CFont::FontInfo

void CFont::FontInfo::CreateCharmap (const charmap_t& cm)
{
    _cpmap.Create (cm);
    _glyphs.clear();
    _glyphs.resize (_cpmap.size());
}

//}}}-------------------------------------------------------------------
//{{{ Common

CFont::CFont (GLXContext ctx, goid_t cid, const GLubyte* p, GLuint psz, uint8_t fontSize)
: CTexture (ctx, cid)
,_info()
,_rowwidth(0)
{
    if (psz > 4 && (*(const uint32_t*)p == PSF2_MAGIC || *(const uint16_t*)p == PSF1_MAGIC))
	ReadPSF (p, psz);
    else
	ReadFreetype (p, psz, fontSize);
}

static void RenderGlyphOnTexture (const FT_Bitmap& gbmp, uint8_t* o, unsigned texw) noexcept
{
    auto cbp = gbmp.buffer;
    for (auto gy = 0u; gy < gbmp.rows; ++gy, o += texw - gbmp.width) {
	if (gbmp.pixel_mode == FT_PIXEL_MODE_GRAY) {
	    copy_n (cbp, gbmp.width, o);
	    o += gbmp.width;
	} else if (gbmp.pixel_mode == FT_PIXEL_MODE_MONO) {
	    uint8_t mask = 0x80;
	    for (auto gx = 0u, bi = 0u; gx < gbmp.width; ++gx) {
		*o++ = (cbp[bi] & mask) ? 0xff : 0;
		if (!(mask >>= 1)) {
		    mask = 0x80;
		    ++bi;
		}
	    }
	}
	cbp += gbmp.pitch;
    }
}

//}}}-------------------------------------------------------------------
//{{{ ReadPSF

void CFont::ReadPSF (const uint8_t* p, unsigned psz)
{
    auto pend = p+psz;
    auto ph1 = (const SPsf1Header*) p;
    auto ph2 = (const SPsf2Header*) p;
    auto nGlyphs = 256u, glyphsize = 0u;
    G::Font::CPMap::charmap_t charmap (1<<16);

    //{{{2 Read PSF1 header -----------------------------------------------
    if (ph1->magic == PSF1_MAGIC && psz > sizeof(*ph1)) {
	_info.SetSize (8, ph1->height, ph1->height);
	glyphsize = ph1->height;
	p += sizeof(*ph1);
	psz -= sizeof(*ph1);
	if (ph1->flags & PSF1_MODEHASTAB) {
	    auto g = 0u;
	    for (const uint16_t* utp = (const uint16_t*)(p + nGlyphs * glyphsize); utp < (const uint16_t*)pend; ++utp) {
		if (*utp == PSF1_SEPARATOR)
		    ++g;
		else if (*utp == PSF1_STARTSEQ) {
		    while (utp+1 < (const uint16_t*)pend && utp[1] != PSF2_SEPARATOR)
			++utp;
		} else
		    charmap[*utp] = g;
	    }
	} else
	    for (uint16_t i = 0u; i < nGlyphs; ++i)
		charmap[i] = i;
    //}}}2
    //{{{2 Read PSF2 header -----------------------------------------------
    } else if (ph2->magic == PSF2_MAGIC && psz > sizeof(*ph2)) {
	_info.SetSize (ph2->width, ph2->height, ph2->height);
	nGlyphs = ph2->nglyphs;
	glyphsize = ph2->glyphsize;
	p += ph2->headersize;
	psz -= sizeof(*ph1);
	if (ph2->flags & PSF2_HAS_UNICODE_TABLE) {
	    auto g = 1u;	// 0 is no-glyph
	    for (auto utp = p + nGlyphs * glyphsize; utp < pend;) {
		if (*utp == PSF2_SEPARATOR) {
		    ++g;
		    ++utp;
		} else if (*utp == PSF2_STARTSEQ) {
		    while (++utp < pend && *utp != PSF2_SEPARATOR) {}
		} else {
		    auto cs = Utf8SequenceBytes (*utp);
		    if (utp+cs >= pend)
			XError::emit ("invalid font file");
		    uint16_t c = *utf8in (utp);
		    utp += cs;
		    charmap[c] = g;
		}
	    }
	} else
	    for (uint16_t i = 0u; i < nGlyphs; ++i)
		charmap[i] = i;
    }
    //}}}2-----------------------------------------------------------------
    if (!_info.Width() || !_info.Height() || psz < nGlyphs * glyphsize)
	XError::emit ("invalid font file");

    _info.CreateCharmap (charmap);

    // Try to make the texture close to a square with a power of 2 width
    auto texwe = (FirstBit (min<unsigned>(_info.Charmap().size(), nGlyphs) * _info.Width() * _info.Height(), 0) + 1) / 2;
    if (texwe > 12)	// because on-demand loading is not currently practical, limit texture size to ~16M
	XError::emit ("font too large");
    auto texw = 1u << texwe;
    auto texh = 256u;

    _rowwidth = texw/_info.Width();

    FT_GlyphSlotRec glyph;
    memset (&glyph, 0, sizeof(glyph));
    glyph.advance.x = _info.Width() * 64;
    glyph.advance.y = _info.Height() * 64;
    glyph.bitmap_top = _info.Height();
    glyph.bitmap.rows = _info.Height();
    glyph.bitmap.width = _info.Width();
    glyph.bitmap.pitch = glyphsize / glyph.bitmap.rows;
    glyph.bitmap.pixel_mode = FT_PIXEL_MODE_MONO;

    vector<uint16_t> usedglyphs (nGlyphs+1);
    vector<GLubyte> ftexbmp (texw*texh);

    uint16_t x = 0, y = 0, rh = 0;
    for (auto c : _info.Charmap()) {
	auto g = charmap[c];
	if (!g)			// No glyph for char
	    continue;
	if (usedglyphs[g]) {	// Reuse previously rendered glyph
	    _info.Glyph(c) = _info.Glyph(usedglyphs[g]);
	    continue;
	}
	usedglyphs[g] = c;
	auto& gi = _info.Glyph(c);

	// Build the freetype FT_Bitmap
	glyph.bitmap.buffer = const_cast<uint8_t*>(&p[(g-1)*glyphsize]);

	// Position the glyph in the texture
	gi.w = glyph.bitmap.width+1;	// +1 to account for OpenGL filled primitive non-inclusive top and right edge
	gi.h = glyph.bitmap.rows+1;
	if (x + gi.w > texw) {		// Overflow right, next row
	    x = 0;
	    y += rh;
	    rh = 0;
	}
	gi.x = x;
	gi.y = y-1;			// -1 to adjust for filled primitive non-inclusive top edge
	rh = max<uint16_t> (rh, gi.h);
	while (y + rh > texh)		// Overflow bottom, expand texture
	    ftexbmp.resize (texw * (texh *= 2));

	auto o = &ftexbmp[(y << texwe) + x];
	RenderGlyphOnTexture (glyph.bitmap, o, texw);
	x += gi.w;
    }
    texh = y + rh;

    CTexture::_info.w = texw;
    CTexture::_info.h = texh;
    glBindTexture (GL_TEXTURE_2D, Id());
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_COMPRESSED_RED, texw, texh, 0, GL_RED, GL_UNSIGNED_BYTE, &ftexbmp[0]);
}

//}}}-------------------------------------------------------------------
//{{{ Read freetype

#if !HAVE_FREETYPE
void CFont::ReadFreetype (const uint8_t* p UNUSED, unsigned psz UNUSED, uint8_t fontSize UNUSED)
    { XError::emit ("TrueType font support unavailable"); }
#else

//{{{2 OFT_Library and OFT_Face, wrappers of freetype structs for cleanup on exceptions
class OFT_Library {
    FT_Library	_library;
public:
    inline OFT_Library (void) {
	if (FT_Init_FreeType (&_library))
	    XError::emit ("Freetype library init failed");
    }
    inline ~OFT_Library (void) { FT_Done_FreeType (_library); }
    inline operator FT_Library (void)	{ return _library; }
};

class OFT_Face {
    FT_Face	_face;
public:
    inline OFT_Face (FT_Library library, const uint8_t* p, unsigned psz) {
	if (FT_New_Memory_Face (library, p, psz, 0, &_face))
	    XError::emit ("failed to load font");
    }
    inline ~OFT_Face (void) { FT_Done_Face (_face); }
    inline operator FT_Face (void)	{ return _face; }
    inline FT_Face operator-> (void)	{ return _face; }
};
//}}}2

void CFont::ReadFreetype (const uint8_t* p, unsigned psz, uint8_t fontSize)
{
    OFT_Library library;
    OFT_Face face (library, p, psz);

    if (FT_IS_SCALABLE (face))
	FT_Set_Pixel_Sizes (face, 0, fontSize);

    charmap_t cm (1<<16);
    for (auto i = 0u; i < cm.size(); ++i)
	cm[i] = FT_Get_Char_Index (face, i);
    _info.CreateCharmap (cm);
    if (!FT_IS_FIXED_WIDTH (face))
	_info.InitVarWidthMap();

    if (FT_HAS_KERNING (face)) {
	//{{{2 Freetype kerning pair characters
	// Freetype has no API to iterate over kerning pairs, hence brute
	// force search is necessary. This array limits character range.
	static const uint16_t ckern[] = {
	    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0027, 0x0028, 0x0029,
	    0x002a, 0x002c, 0x002d, 0x002e, 0x002f, 0x0030, 0x0031, 0x0032,
	    0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a,
	    0x003b, 0x003f, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045,
	    0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d,
	    0x004e, 0x004f, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055,
	    0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d,
	    0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068,
	    0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 0x0070,
	    0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078,
	    0x0079, 0x007a, 0x007b, 0x007d, 0x00a0, 0x00c0, 0x00c1, 0x00c2,
	    0x00c3, 0x00c4, 0x00c5, 0x00ff, 0x0391, 0x0392, 0x0393, 0x0394,
	    0x0396, 0x0398, 0x039a, 0x039b, 0x039f, 0x03a1, 0x03a3, 0x03a4,
	    0x03a5, 0x03a6, 0x03a7, 0x03a8, 0x03a9, 0x03b2, 0x03b3, 0x03b4,
	    0x03b5, 0x03b6, 0x03b7, 0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc,
	    0x03bd, 0x03be, 0x03bf, 0x03c0, 0x03c1, 0x03c2, 0x03c3, 0x03c4,
	    0x03c5, 0x03c6, 0x03c7, 0x03c8, 0x03c9, 0x0401, 0x0410, 0x0411,
	    0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 0x041a, 0x041b,
	    0x041e, 0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0427,
	    0x042a, 0x042c, 0x042d, 0x042e, 0x042f, 0x0430, 0x0431, 0x0432,
	    0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 0x0438, 0x0439, 0x043a,
	    0x043b, 0x043c, 0x043d, 0x043e, 0x043f, 0x0440, 0x0441, 0x0442,
	    0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 0x0448, 0x0449, 0x044a,
	    0x044c, 0x044d, 0x044e, 0x0451, 0x0452, 0x2010, 0x2011, 0x2012,
	    0x2013, 0x2014, 0x2015, 0x2018, 0x2019, 0x201a, 0x201c, 0x201d,
	    0x201e, 0x2026, 0x2039, 0x203a, 0x2122, 0x220f, 0x2211, 0x2219,
	    0xfb00, 0xfd3e, 0xfd3f
	};
	//}}}2
	auto& kerns = _info.KerningPairs();
	for (uint16_t i1 = 0u; i1 < ArraySize(ckern); ++i1) {
	    auto c1 = ckern[i1];
	    auto g1 = cm[c1];
	    if (!g1)
		continue;
	    for (uint16_t i2 = 0u; i2 < ArraySize(ckern); ++i2) {
		auto c2 = ckern[i2];
		auto g2 = cm[c2];
		if (!g2)
		    continue;
		FT_Vector delta;
		delta.x = 0;
		FT_Get_Kerning (face, g1, g2, FT_KERNING_DEFAULT, &delta);
		if (delta.x)
		    kerns.push_back ((G::Font::KerningPair){int16_t(delta.x>>6),0,c2,c1});
	    }
	}
    }

    auto mw = fontSize, mh = fontSize, bl = fontSize;
    if (cm['M']) {
	if (FT_Load_Char (face, 'M', FT_LOAD_RENDER))
	    XError::emit ("Freetype error FT_Load_Char");
	mw = face->glyph->advance.x / 64;
	mh = face->glyph->bitmap.rows;
	bl = face->glyph->bitmap_top;
	if (!FT_IS_SCALABLE (face))
	    fontSize = mh;
    }
    if (FT_IS_SCALABLE (face)) {
	enum { VLINE_CHAR = 0x2502 };
	if (cm[VLINE_CHAR]) {
	    if (FT_Load_Char (face, VLINE_CHAR, FT_LOAD_RENDER))
		XError::emit ("Freetype error FT_Load_Char");
	    fontSize = face->glyph->bitmap.rows;
	    bl = face->glyph->bitmap_top;
	} else {
	    // Available fonts do not appear to have any way to get line height
	    // These metrics are close, but usually off by a few pixels
	    fontSize = face->size->metrics.height/64;
	    bl = fontSize + face->size->metrics.descender/64;
	}
    }
    _info.SetSize (mw, fontSize, bl);
    _info.SetName (face->family_name);

    // Try to make the texture close to a square with a power of 2 width
    auto texwe = (FirstBit (min<unsigned>(_info.Charmap().size(), face->num_glyphs) * _info.Width() * _info.Height(), 0) + 1) / 2;
    if (texwe < 6)
	texwe = 6;
    auto texw = 1u << texwe;
    if (texwe > 12 || texw < 2*mw)	// because on-demand loading is not currently practical, limit texture size to ~16M
	XError::emit ("font too large");
    auto texh = face->num_glyphs * _info.Width() * _info.Height() / texw;
    if (texh < mh)
	texh = mh+1;

    vector<uint16_t> usedglyphs (face->num_glyphs+1);
    vector<GLubyte> ftexbmp (texw*texh);

    uint16_t x = 0, y = 0, rh = 0;
    for (auto c : _info.Charmap()) {
	auto g = cm[c];
	if (!g)			// No glyph for char
	    continue;
	if (usedglyphs[g]) {	// Reuse previously rendered glyph
	    _info.Glyph(c) = _info.Glyph(usedglyphs[g]);
	    continue;
	}
	usedglyphs[g] = c;

	// Render the glyph
	if (FT_Load_Char (face, c, FT_LOAD_RENDER))
	    XError::emit ("Freetype error FT_Load_Char");
	auto& glyph = *face->glyph;

	// Position the glyph in the texture
	auto& gi = _info.Glyph(c);
	gi.w = glyph.bitmap.width+1;	// +1 to account for OpenGL filled primitive non-inclusive top and right edge
	gi.h = glyph.bitmap.rows+1;
	if (x + gi.w + 1u > texw) {	// Overflow right, next row (+1 to add empty margin for MSAA)
	    x = 0;
	    y += rh;
	    rh = 0;
	}
	gi.x = x;
	gi.y = y-1;			// -1 to adjust for filled primitive non-inclusive top edge
	gi.bx = glyph.bitmap_left;
	gi.by = _info.Baseline() - glyph.bitmap_top;
	if (!FT_IS_FIXED_WIDTH(face))
	    _info.SetWidth (c, glyph.advance.x / 64);

	rh = max<uint16_t> (rh, gi.h+1);
	while (y + rh > texh)		// Overflow bottom, expand texture
	    ftexbmp.resize (texw * (texh *= 2));

	auto o = &ftexbmp[(y << texwe) + x];
	RenderGlyphOnTexture (glyph.bitmap, o, texw);
	x += gi.w+1;
    }
    texh = y + rh;

    CTexture::_info.w = texw;
    CTexture::_info.h = texh;
    glBindTexture (GL_TEXTURE_2D, Id());
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_COMPRESSED_RED, texw, texh, 0, GL_RED, GL_UNSIGNED_BYTE, &ftexbmp[0]);
}
#endif

//}}}-------------------------------------------------------------------

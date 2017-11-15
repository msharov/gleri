// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "twin.h"
#include "../config.h"	// to see which image libraries are linked in

//{{{ Vertex data ------------------------------------------------------
namespace {

static const CTestWindow::coord_t _vdata1[] = {
    VGEN_LLRECT (0,0, 640,480),
    VGEN_TFRECT (50,50, 100,250),
    50,50, 200,200, 300,200, 300,300,
    250,250, 250,400, 350,250, 500,450,
    200,60, 270,120, 300,50, 400,100,
    0,0, 0,-1, 1,0, 1,-1,
    0,0, 0,239, 319,239, 119,39, 319,239, 319,0,
    VGEN_LLRECT (-1,-1, 322,242)
};
static constexpr CTestWindow::color_t _cdata1[] = {
    RGB(0xff0000), RGB(0x00ff00), RGB(0x0000ff), RGBA(0x80808080)
};
enum {
    VRENUM (WindowBorder, 4),
    VRENUM (PurpleQuad, 4),
    VRENUM (BrokenLine, 4),
    VRENUM (TransparentStrip, 4),
    VRENUM (SkewQuad, 4),
    VRENUM (FanOverlay, 4),
    VRENUM (SmallFbBorder, 6),
    VRENUM (RedBorder, 4)
};
enum {
    walk_SpriteW = 64,
    walk_SpriteH = walk_SpriteW,
    walk_StripUp = walk_SpriteH*0,
    walk_StripLeft = walk_SpriteH*1,
    walk_StripDown = walk_SpriteH*2,
    walk_StripRight = walk_SpriteH*3,
    walk_StripLength = walk_SpriteW*9
};

//}}}-------------------------------------------------------------------
//{{{ Gradient shader

static const char c_gradShader_v[] =
"#version 330 core\n"
"\n"
"uniform vec4 Color;\n"
"layout(location=0) in vec2 Vertex;\n"
"out vec4 f_color;\n"
"\n"
"void main() {\n"
"    gl_Position = vec4(Vertex,1,1);\n"
"    f_color = Color*vec4(1,1,1,gl_Position.x*-gl_Position.y);\n"
"}";

static const char c_gradShader_f[] =
"#version 330 core\n"
"\n"
"in vec4 f_color;\n"
"out vec4 FragColor;\n"
"\n"
"void main() {\n"
"    FragColor = f_color;\n"
"}";

} // namespace
//}}}-------------------------------------------------------------------

CTestWindow::CTestWindow (iid_t wid)
: CWindow(wid)
,_vbuf(0)
,_cbuf(0)
,_gradShader(0)
,_walk(0)
,_cat(0)
,_smalldepth(0)
,_smallcol(0)
,_smallfb(0)
,_ofscrdepth(0)
,_ofscrcol(0)
,_ofscrfb(0)
,_vwfont(0)
,_selrectbuf(0)
,_wx(0)
,_wy(0)
,_wsx(0)
,_wsy(0)
,_wtimer(NotWaitingForVSync)
,_screenshot(nullptr)
,_selrectpts{{0}}
{
    strcpy (_hellomsg, "Hello world from OpenGL!");
}

void CTestWindow::OnInit (void)
{
    CWindow::OnInit();
    Open ("GLERI Test Program", 640, 480);
    printf ("Initializing test window\n");
    _vbuf = BufferData (G::ARRAY_BUFFER, _vdata1, sizeof(_vdata1));
    _cbuf = BufferData (G::ARRAY_BUFFER, _cdata1, sizeof(_cdata1));
    _selrectbuf = BufferData (G::ARRAY_BUFFER, _selrectpts, sizeof(_selrectpts));
#if HAVE_PNG_H
    TexParameter (G::Texture::MAG_FILTER, G::Texture::LINEAR);
    _walk = LoadTexture (G::TEXTURE_2D, "test/princess.png", G::Pixel::COMPRESSED_RGBA);
#endif
#if HAVE_JPEGLIB_H
    _cat = LoadTexture (G::TEXTURE_2D, "test/pgcat.jpg", G::Pixel::RGB);
#endif
    _gradShader = LoadShader (c_gradShader_v, c_gradShader_f);

    _smalldepth = CreateDepthTexture (320, 240);
    _smallcol = CreateTexture (G::TEXTURE_2D, 320, 240, 0, G::Pixel::RGBA);
    _smallfb = CreateFramebuffer (_smalldepth, _smallcol);

    _ofscrdepth = CreateDepthTexture (640, 480);
    _ofscrcol = CreateTexture (G::TEXTURE_2D, 640, 480);
    _ofscrfb = CreateFramebuffer (_ofscrdepth, _ofscrcol);

#if HAVE_FREETYPE
    #define VWFONT_NAME	"/usr/share/fonts/TTF/times.ttf"
    if (access (VWFONT_NAME, R_OK) == 0)
	_vwfont = LoadFont (VWFONT_NAME, 21);
#endif
}

void CTestWindow::OnResize (dim_t w, dim_t h)
{
    CWindow::OnResize (w,h);
    printf ("Test window OnResize\n");
    const coord_t sw = w-1, sh = h-1,
		_vdata1[] = { sh, sw,sh, sw };
    BufferSubData (_vbuf, _vdata1, sizeof(_vdata1), 3*sizeof(int16_t));
    _wx = 0; _wy = (h-walk_SpriteH)/2;
    _wsx = 0*walk_SpriteW; _wsy = walk_StripRight;
    //WaitForTime (_wtimer = NowMS()+1000/30);
}

void CTestWindow::OnKey (key_t key)
{
    CWindow::OnKey (key);
    if (key == 'q' || key == Key::Escape) {
	printf ("Event received, quitting\n");
	Close();
    } else if (key == Key::Up || key == 'k') {
	if (--_wy < 0)
	    _wy = 0;
	_wsy = walk_StripUp;
	_wsx += walk_SpriteW;
    } else if (key == Key::Down || key == 'j') {
	if (++_wy > Info().h-walk_SpriteH)
	    _wy = Info().h-walk_SpriteH;
	_wsy = walk_StripDown;
	_wsx += walk_SpriteW;
    } else if (key == Key::Left || key == 'h') {
	if (--_wx < 0)
	    _wx = 0;
	_wsy = walk_StripLeft;
	_wsx += walk_SpriteW;
    } else if (key == Key::Right || key == 'l') {
	if (++_wx > Info().w-walk_SpriteW)
	    _wx = Info().w-walk_SpriteW;
	_wsy = walk_StripRight;
	_wsx += walk_SpriteW;
    } else if (key == 's') {
	_screenshot = "screen.jpg";
	Draw();
	_screenshot = nullptr;
    } else if (key == Key::Menu)
	OnButton (Button::Right, 10, 10);

    if (_wsx >= walk_StripLength)
	_wsx = 0;
    if (_wy < 100)
	SetCursor (G::Cursor(_wx/16));
    Draw();
}

static constexpr const char c_SelText[] = "A quick brown fox jumps over the lazy dog";
enum {
    sel_X = 300,
    sel_Y = 420
};

void CTestWindow::OnButton (key_t b, coord_t x, coord_t y)
{
    CWindow::OnButton (b,x,y);
    auto pfi = Font();
    const coord_t xselright = sel_X+strlen(c_SelText)*pfi->Width();
    if (b == Button::Right) {
	BEGIN_MENU (c_TestMenu)
	    MENUITEM ("Entry 1", "1", "entry1")
	    MENUITEM ("Entry 2", "2", "entry2")
	    MENUITEM ("Entry 3", "3", "entry3")
	    MENUITEM ("Take screenshot", "s", "screenshot")
	    MENUITEM ("Offscreen draw", "o", "offscreen")
	END_MENU
	CPopupMenu::Create (IId(), x, y, c_TestMenu);
    } else if (b == Button::Left
	    && !_selrectpts[0][0]
	    && x >= sel_X && x < xselright && y >= sel_Y && y < sel_Y+pfi->Height()) {
	auto x1 = sel_X + AlignDown (x-sel_X, pfi->Width());
	_selrectpts[0][0] = x1;
	_selrectpts[1][0] = x1;
	_selrectpts[0][1] = sel_Y;
	_selrectpts[1][1] = sel_Y+pfi->Height();
	_selrectpts[2][0] = x1+pfi->Width();
	_selrectpts[3][0] = x1+pfi->Width();
	_selrectpts[2][1] = sel_Y;
	_selrectpts[3][1] = sel_Y+pfi->Height();
	BufferSubData (_selrectbuf, _selrectpts, sizeof(_selrectpts));
	Draw();
    } else if (b == Button::Middle)
	GetClipboard();
}

void CTestWindow::OnMotion (coord_t x, coord_t y, key_t b)
{
    CWindow::OnMotion (x,y,b);
    auto pfi = Font();
    const coord_t xselright = sel_X+strlen(c_SelText)*pfi->Width();
    if ((b & KMod::Left) && x > _selrectpts[0][0] && x <= xselright) {
	auto x2 = sel_X + Align (x-sel_X, pfi->Width());
	if (x2 != _selrectpts[2][0]) {
	    _selrectpts[2][0] = x2;
	    _selrectpts[3][0] = x2;
	    BufferSubData (_selrectbuf, _selrectpts, sizeof(_selrectpts));
	    Draw();
	}
    }
}

void CTestWindow::OnButtonUp (key_t b, coord_t x, coord_t y)
{
    CWindow::OnButtonUp (b,x,y);
    if (b == Button::Left) {
	auto pfi = Font();
	unsigned tx1 = (_selrectpts[0][0]-sel_X)/pfi->Width(), tx2 = (_selrectpts[3][0]-sel_X)/pfi->Width();
	memset (_selrectpts, 0, sizeof(_selrectpts));
	if (tx2 < strlen(c_SelText) && tx1 < tx2) {
	    char selbuf [ArraySize(c_SelText)];
	    auto selsz = tx2-tx1;
	    memcpy (selbuf, &c_SelText[tx1], selsz);
	    selbuf[selsz] = 0;
	    SetClipboard (selbuf);
	}
    }
}

void CTestWindow::OnCommand (const char* cmd)
{
    CWindow::OnCommand (cmd);
    strcpy (_hellomsg, cmd);
    if (!strcmp (cmd, "screenshot")) {
	_screenshot = "screen.jpg";
	Draw();
	_screenshot = nullptr;
    } else if (!strcmp (cmd, "offscreen"))
	DrawOffscreen (_ofscrfb);
}

void CTestWindow::OnClipboardData (G::Clipboard ci, G::ClipboardFmt fmt, const char* d)
{
    CWindow::OnClipboardData (ci, fmt, d);
    printf ("Received clipboard data '%s'\n", d);
    auto dlen = min (ArraySize(_hellomsg)-1u, strlen(d));
    memcpy (_hellomsg, d, dlen);
    _hellomsg[dlen] = 0;
    Draw();
}

void CTestWindow::OnClipboardOp (ClipboardOp op, G::Clipboard ci, G::ClipboardFmt fmt)
{
    CWindow::OnClipboardOp (op, ci, fmt);
    if (op == ClipboardOp::Accepted) {
	printf ("Clipboard data accepted\n");
	BufferSubData (_selrectbuf, _selrectpts, sizeof(_selrectpts));
	Draw();
    } else if (op == ClipboardOp::Rejected)
	printf ("Clipboard data rejected\n");
    else if (op == ClipboardOp::Read)
	printf ("Clipboard data read\n");
    else if (op == ClipboardOp::Cleared)
	printf ("Clipboard data cleared\n");
}

void CTestWindow::OnTimer (uint64_t tms)
{
    CWindow::OnTimer (tms);
    if (tms != _wtimer)
	return;

    if (++_wx >= Info().w)
	_wx = -walk_SpriteW;
    _wsy = walk_StripRight;
    _wsx += walk_SpriteW;
    if (_wsx >= walk_StripLength)
	_wsx = 0;
    Draw();

    uint32_t wt = 1000/60;
    if (RefreshTimeNS())
	wt = RefreshTimeNS()/1000000;
    WaitForTime (_wtimer += wt);
}

ONDRAWIMPL(CTestWindow)::OnDraw (Drw& drw) const
{
    CWindow::OnDraw (drw);

    drw.Clear (RGB(0,0,64));

    drw.VertexPointer (_vbuf);

    drw.Color (0,255,255);
    drw.LineLoop (v_WindowBorderOffset, v_WindowBorderSize);
    drw.Color (255,255,255);
    drw.PointSize (3);
    drw.LineStrip (v_BrokenLineOffset, v_BrokenLineSize);

    #if HAVE_JPEGLIB_H
	drw.Image (700, 20, _cat);
    #endif
    #if HAVE_PNG_H
	drw.Image (200, 75, _walk);
	drw.Offset (_wx, _wy);
	drw.Sprite (0, 0, _walk, _wsx, _wsy, walk_SpriteW, walk_SpriteH);
	drw.Offset (0, 0);
    #endif

    drw.Color (ARGB(0xc0804040));
    drw.TriangleStrip (v_TransparentStripOffset, v_TransparentStripSize);

    drw.Shader (G::default_GradientShader);
    drw.Parameter ("Vertex", _vbuf, G::SHORT, 2, v_SkewQuadOffset*(2*sizeof(short)));
    drw.ColorPointer (_cbuf);
    drw.TriangleStrip (0, v_SkewQuadSize);
    drw.DefaultShader();
    drw.VertexPointer (_vbuf);

    drw.Color (0,240,255,128);
    drw.Text (300, 250, _hellomsg);
    drw.Textf (300, 350, "Display %hu.%hu: %hux%hu (%hux%hu mm), depth %hu, wid %x", Info().dpyn, Info().scrn, Info().scrw, Info().scrh, Info().scrmw, Info().scrmh, Info().scrd, Info().wmwid);

    drw.Color (255,255,255);
    drw.Text (sel_X, sel_Y, c_SelText);
    auto lrt = LastRenderTimeNS(), lft = RefreshTimeNS();
    drw.Textf (10,10, "FPS %6u, VSync %3u", 1000000000/(lrt+!lrt), 1000000000/(lft+!lft));

    #if HAVE_FREETYPE
	if (_vwfont) {
	    drw.Font (_vwfont);
	    drw.Text (300, 520, "A quick brown fox jumps over the lazy dog");
	    static const uint16_t c_WideChars[] = { 0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510 };
	    char u8buf [ArraySize(c_WideChars)*4];
	    memset (u8buf, 0, sizeof(u8buf));
	    auto u8out = utf8out (u8buf);
	    for (auto i = 0u; i < ArraySize(c_WideChars); ++i)
		*u8out++ = c_WideChars[i];
	    *u8out = 0;
	    drw.Text (300, 540, u8buf);
	}
    #endif

    drw.Color (128,90,150,220);
    drw.TriangleFan (v_PurpleQuadOffset, v_PurpleQuadSize);

    drw.Shader (_gradShader);
    drw.Color (0,128,128);
    drw.TriangleStrip (v_FanOverlayOffset, v_FanOverlaySize);
    drw.DefaultShader();

    if (_selrectbuf) {
	drw.VertexPointer (_selrectbuf);
	drw.Color (128,128,128,128);
	drw.TriangleStrip (0, 4);
	drw.VertexPointer (_vbuf);
    }

    drw.Framebuffer (_smallfb);
    drw.Clear (RGBA(0,0,48,128));
    #if HAVE_JPEGLIB_H
	drw.Image (100, 120, _cat);
    #endif
    drw.Color (128,90,150,220);
    drw.TriangleFan (v_PurpleQuadOffset, v_PurpleQuadSize);
    drw.Color (0,128,128);
    drw.LineLoop (v_SmallFbBorderOffset, v_SmallFbBorderSize);
    drw.Text (32, 64, "Offscreen");
    drw.DefaultFramebuffer();

    drw.Offset (990, 120);
    drw.Color (192,0,0);
    drw.LineLoop (v_RedBorderOffset, v_RedBorderSize);
    drw.Color (255,255,255);
    drw.LineLoop (v_SmallFbBorderOffset, v_SmallFbBorderSize);
    drw.Image (0, 0, _smallcol);
    drw.Offset (0, 0);
    if (_screenshot)
	drw.Screenshot (_screenshot);
}

DRAWFBIMPL(CTestWindow,Offscreen)
{
    drw.Clear (RGB(0,64,64));
    drw.VertexPointer (_vbuf);

    drw.Color (255,255,255);
    drw.LineStrip (v_BrokenLineOffset, v_BrokenLineSize);

    #if HAVE_JPEGLIB_H
	drw.Image (320, 240, _cat);
    #endif

    drw.Color (ARGB(0xc0804040));
    drw.TriangleStrip (v_TransparentStripOffset, v_TransparentStripSize);

    drw.Shader (G::default_GradientShader);
    drw.VertexPointer (_vbuf, G::SHORT, 2, v_SkewQuadOffset*(2*sizeof(short)));
    drw.ColorPointer (_cbuf);
    drw.TriangleStrip (0, v_SkewQuadSize);

    drw.Color (128,128,128);
    drw.Text (130, 400, "Offscreen rendered framebuffer");

    drw.SaveFramebuffer (0,0,0,0,"offscreen.png", G::Texture::Format::PNG);
}

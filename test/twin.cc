// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "twin.h"
#include "../config.h"	// to see which image libraries are linked in

//{{{ Vertex data ------------------------------------------------------
namespace {

static const CTestWindow::coord_t _vdata1[] = {
    0,0, 0,479, 639,479, 639,0,
    50,50, 50,300, 150,300, 150,50,
    100,100, 200,200, 300,200, 300,300,
    250,250, 250,400, 350,250, 500,450,
    200,60, 270,120, 300,50, 400,100,
    0,0, 0,-1, 1,0, 1,-1
};
static constexpr CTestWindow::color_t _cdata1[] = {
    RGB(0xff0000), RGB(0x00ff00), RGB(0x0000ff), RGBA(0x80808080)
};
enum {
    vb_WindowBorderOffset,
    vb_WindowBorderSize = 4,	// In vertices
    vb_PurpleQuadOffset = vb_WindowBorderOffset+vb_WindowBorderSize,
    vb_PurpleQuadSize = 4,
    vb_BrokenLineOffset = vb_PurpleQuadOffset+vb_PurpleQuadSize,
    vb_BrokenLineSize = 4,
    vb_TransparentStripOffset = vb_BrokenLineOffset+vb_BrokenLineSize,
    vb_TransparentStripSize = 4,
    vb_SkewQuadOffset = vb_TransparentStripOffset+vb_TransparentStripSize,
    vb_SkewQuadSize = 4,
    vb_FanOverlayOffset = vb_SkewQuadOffset + vb_SkewQuadSize,
    vb_FanOverlaySize = 4
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
"invariant out vec4 gl_Position;\n"
"invariant out vec4 f_color;\n"
"\n"
"void main() {\n"
"    gl_Position = vec4(Vertex,1,1);\n"
"    f_color = Color*vec4(1,1,1,gl_Position.x*-gl_Position.y);\n"
"}";

static const char c_gradShader_f[] =
"#version 330 core\n"
"\n"
"invariant in vec4 f_color;\n"
"invariant out vec4 gl_FragColor;\n"
"\n"
"void main() {\n"
"    gl_FragColor = f_color;\n"
"}";

} // namespace
//}}}-------------------------------------------------------------------

void CTestWindow::OnInit (void)
{
    CWindow::OnInit();
    Open ("GLERI Test Program", 640, 480);
    printf ("Initializing test window\n");
    _vbuf = BufferData (_vdata1, sizeof(_vdata1));
    _cbuf = BufferData (_cdata1, sizeof(_cdata1));
#if HAVE_PNG_H
    TexParameter (G::Texture::MAG_FILTER, G::Texture::LINEAR);
    _walk = LoadTexture ("test/princess.png", G::Pixel::COMPRESSED_RGBA);
#endif
#if HAVE_JPEGLIB_H
    _cat = LoadTexture ("test/pgcat.jpg");
#endif
    _gradShader = LoadShader (c_gradShader_v, c_gradShader_f);
}

void CTestWindow::OnResize (dim_t w, dim_t h)
{
    CWindow::OnResize (w,h);
    printf ("Test window OnResize\n");
    const coord_t sw = w-1, sh = h-1;
    const coord_t _vdata1[] = { sh, sw,sh, sw };
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
    } else if (key == Key::Up) {
	if (--_wy < 0)
	    _wy = 0;
	_wsy = walk_StripUp;
	_wsx += walk_SpriteW;
    } else if (key == Key::Down) {
	if (++_wy > Info().h-walk_SpriteH)
	    _wy = Info().h-walk_SpriteH;
	_wsy = walk_StripDown;
	_wsx += walk_SpriteW;
    } else if (key == Key::Left) {
	if (--_wx < 0)
	    _wx = 0;
	_wsy = walk_StripLeft;
	_wsx += walk_SpriteW;
    } else if (key == Key::Right) {
	if (++_wx > Info().w-walk_SpriteW)
	    _wx = Info().w-walk_SpriteW;
	_wsy = walk_StripRight;
	_wsx += walk_SpriteW;
    }
    if (_wsx >= walk_StripLength)
	_wsx = 0;
    Draw();
}

void CTestWindow::OnButton (key_t b, coord_t x, coord_t y)
{
    CWindow::OnButton (b,x,y);
    if (b == Button::Right) {
	BEGIN_MENU (c_TestMenu)
	    MENUITEM ("Entry 1", "1", "entry1")
	    MENUITEM ("Entry 2", "2", "entry2")
	    MENUITEM ("Entry 3", "3", "entry3")
	END_MENU
	CPopupMenu::Create (IId(), x, y, c_TestMenu);
    }
}

void CTestWindow::OnCommand (const char* cmd)
{
    CWindow::OnCommand (cmd);
    strcpy (_hellomsg, cmd);
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
    drw.LineLoop (vb_WindowBorderOffset, vb_WindowBorderSize);
    drw.Color (255,255,255);
    drw.LineStrip (vb_BrokenLineOffset, vb_BrokenLineSize);

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
    drw.TriangleStrip (vb_TransparentStripOffset, vb_TransparentStripSize);

    drw.Shader (G::default_GradientShader);
    drw.VertexPointer (_vbuf, G::SHORT, 2, vb_SkewQuadOffset*(2*sizeof(short)));
    drw.ColorPointer (_cbuf);
    drw.TriangleStrip (0, vb_SkewQuadSize);
    drw.DefaultShader();
    drw.VertexPointer (_vbuf);

    drw.Color (0,240,255,128);
    drw.Text (300, 250, _hellomsg);

    drw.Color (255,255,255);
    drw.Text (300, 420, "A quick brown fox jumps over the lazy dog");
    uint32_t lrt = LastRenderTimeNS();
    uint32_t lft = RefreshTimeNS();
    drw.Textf (10,10, "FPS %6u, VSync %3u", 1000000000/(lrt?lrt:1), 1000000000/(lft?lft:1));

    drw.Color (128,90,150,220);
    drw.TriangleFan (vb_PurpleQuadOffset, vb_PurpleQuadSize);

    drw.Shader (_gradShader);
    drw.Color (0,128,128);
    drw.TriangleStrip (vb_FanOverlayOffset, vb_FanOverlaySize);
}

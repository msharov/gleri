// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "twin.h"

//{{{ Vertex data ------------------------------------------------------
namespace {

static const CTestWindow::coord_t _vdata1[] = {
    0,0, 0,479, 639,479, 639,0,
    50,50, 50,300, 150,300, 150,50,
    100,100, 200,200, 300,200, 300,300,
    250,250, 250,400, 350,250, 500,450,
    250,50, 300,100, 300,50, 350,75,
    0,0, 0,-1, 1,0, 1,-1
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
    Open (640, 480);
    printf ("Initializing test window\n");
    _vbuf = BufferData (_vdata1, sizeof(_vdata1));
    _walk = LoadTexture ("test/princess.png");
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
    WaitForTime (_wtimer = NowMS()+1000/30);
}

void CTestWindow::OnKey (key_t key)
{
    CWindow::OnKey (key);
    if (key == 'q' || key == Key::Escape) {
	printf ("Event received, quitting\n");
	CApp::Instance().Quit();
    } else if (key == Key::Up) {
	if (--_wy < 0)
	    _wy = 0;
	_wsy = walk_StripUp;
	_wsx += walk_SpriteW;
	if (_wsx >= walk_StripLength)
	    _wsx = 0;
	Draw();
    } else if (key == Key::Down) {
	if (++_wy > Info().h-walk_SpriteH)
	    _wy = Info().h-walk_SpriteH;
	_wsy = walk_StripDown;
	_wsx += walk_SpriteW;
	if (_wsx >= walk_StripLength)
	    _wsx = 0;
	Draw();
    } else if (key == Key::Left) {
	if (--_wx < 0)
	    _wx = 0;
	_wsy = walk_StripLeft;
	_wsx += walk_SpriteW;
	if (_wsx >= walk_StripLength)
	    _wsx = 0;
	Draw();
    } else if (key == Key::Right) {
	if (++_wx > Info().w-walk_SpriteW)
	    _wx = Info().w-walk_SpriteW;
	_wsy = walk_StripRight;
	_wsx += walk_SpriteW;
	if (_wsx >= walk_StripLength)
	    _wsx = 0;
	Draw();
    }
}

void CTestWindow::OnTimer (uint64_t tms)
{
    CWindow::OnTimer (tms);
    if (tms != _wtimer)
	return;

    if (++_wx > Info().w-walk_SpriteW)
	_wx = Info().w-walk_SpriteW;
    _wsy = walk_StripRight;
    _wsx += walk_SpriteW;
    if (_wsx >= walk_StripLength)
	_wsx = 0;
    Draw();

    WaitForTime (_wtimer += 1000/30);
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

    drw.Image (200, 75, _walk);
    drw.Offset (_wx, _wy);
    drw.Sprite (0, 0, _walk, _wsx, _wsy, walk_SpriteW, walk_SpriteH);
    drw.Offset (0, 0);

    drw.Color (ARGB(0xc0804040));
    drw.TriangleStrip (vb_TransparentStripOffset, vb_TransparentStripSize);
    drw.Color (128,170,170);
    drw.TriangleStrip (vb_SkewQuadOffset, vb_SkewQuadSize);

    drw.Color (0,240,255,128);
    drw.Text (300, 250, "Hello world from OpenGL!");

    drw.Color (255,255,255);
    drw.Text (300, 420, "A quick brown fox jumps over the lazy dog");
    uint32_t lrt = LastRenderTimeNS();
    drw.Textf (10,10, "FPS %u", 1000000000/(lrt ? lrt : 1));

    drw.Color (128,90,150,220);
    drw.TriangleFan (vb_PurpleQuadOffset, vb_PurpleQuadSize);

    drw.Shader (_gradShader);
    drw.Color (0,128,128);
    drw.TriangleStrip (vb_FanOverlayOffset, vb_FanOverlaySize);
    drw.DefaultShader();
}

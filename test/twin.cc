// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "twin.h"

//{{{ Vertex data ------------------------------------------------------
namespace {

static const int16_t _vdata1[] = {
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
    _vbuf = BufferData (_vdata1, sizeof(_vdata1));
    _walk = LoadTexture ("test/princess.png");
    _gradShader = LoadShader (c_gradShader_v, c_gradShader_f);
}

void CTestWindow::OnResize (uint16_t w, uint16_t h)
{
    CWindow::OnResize (w,h);
    printf ("Test window OnResize\n");
    const int16_t sw = w-1, sh = h-1;
    const int16_t _vdata1[] = { sh, sw,sh, sw };
    BufferSubData (_vbuf, _vdata1, sizeof(_vdata1), 3*sizeof(int16_t));
}

void CTestWindow::OnEvent (uint32_t key)
{
    CWindow::OnEvent (key);
    if (key == 24 || key == 9) {
	printf ("Event received. Quitting\n");
	CApp::Instance().Quit();
    }
}

ONDRAWIMPL(CTestWindow)::OnDraw (Drw& drw) const
{
    if (Drw::is_writing)
	printf ("Drawing test window\n");
    CWindow::OnDraw (drw);

    drw.Clear (RGB(0,0,64));

    drw.VertexPointer (_vbuf);

    drw.Color (0,255,255);
    drw.LineLoop (vb_WindowBorderOffset, vb_WindowBorderSize);
    drw.Color (255,255,255);
    drw.LineStrip (vb_BrokenLineOffset, vb_BrokenLineSize);

    drw.Sprite (200, 75, _walk);

    drw.Color (ARGB(0xc0804040));
    drw.TriangleStrip (vb_TransparentStripOffset, vb_TransparentStripSize);
    drw.Color (128,170,170);
    drw.TriangleStrip (vb_SkewQuadOffset, vb_SkewQuadSize);

    drw.Color (0,240,255,128);
    drw.Text (300, 250, "Hello world from OpenGL!");

    drw.Color (255,255,255);
    drw.Text (300, 420, "A quick brown fox jumps over the lazy dog");

    drw.Color (128,90,150,220);
    drw.TriangleFan (vb_PurpleQuadOffset, vb_PurpleQuadSize);

    drw.Shader (_gradShader);
    drw.Color (0,128,128);
    drw.TriangleStrip (vb_FanOverlayOffset, vb_FanOverlaySize);
    drw.DefaultShader();
}

#include "twin.h"

//{{{ Vertex data ------------------------------------------------------

namespace {

static const uint16_t _vdata1[] = {
    0,0, 0,479, 639,479, 639,0,
    50,50, 50,300, 150,300, 150,50,
    100,100, 200,200, 300,200, 300,300,
    250,250, 250,400, 350,250, 500,450,
    250,50, 300,100, 300,50, 350,75,
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
    vb_nVertices
};

} // namespace

//}}}-------------------------------------------------------------------

void CTestWindow::OnInit (void)
{
    CWindow::OnInit();
    Open (640, 480);
    BufferData (_vbuf = CreateBuffer(), _vdata1, sizeof(_vdata1));
    LoadTexture (_walk = CreateTexture(), "bvt/princess.png");
}

void CTestWindow::OnResize (uint16_t w, uint16_t h)
{
    CWindow::OnResize (w,h);
    printf ("Test window OnResize\n");
    const uint16_t sw = w-1, sh = h-1;
    const uint16_t _vdata1[] = { sh, sw,sh, sw };
    BufferSubData (_vbuf, _vdata1, sizeof(_vdata1), 3*sizeof(short));
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
}

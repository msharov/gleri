// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gwin.h"
#include <sys/time.h>

//{{{ GLWindow window-level functionality ------------------------------

CGLWindow::CGLWindow (iid_t iid, const WinInfo& winfo, Window win, GLXContext ctx, CIConn* pconn)
: PRGLR(iid)
,_ctx(ctx,iid,win)
,_pendingFrame()
,_pconn (pconn)
,_color (0xffffffff)
,_syncEvent (CEvent::VSync, 1000000000/60)
,_nextVSync (NotWaitingForVSync)
,_curShaderId (CGObject::NoObject)
,_curShader (G::GoidNull)
,_curBuffer (G::GoidNull)
,_curTexture (G::GoidNull)
,_curFont (G::GoidNull)
,_curFb (G::default_Framebuffer)
,_winfo (winfo)
{
    DTRACE ("[%x] Create: window %x, context %x\n", iid, win, ctx);
    _winfo.h = 0;	// Make invalid until explicit resize
    _query[query_FrameEnd] = 0;
    _vao[0] = CGObject::NoObject;
    _vao[1] = CGObject::NoObject;
}

void CGLWindow::Activate (void)
{
    if (_vao[0] != CGObject::NoObject && CIConn::HaveDefaultResources()) {
	SetDefaultShader();
	CheckForErrors();
    }
}

void CGLWindow::Init (void)
{
    glEnable (GL_BLEND);
    glEnable (GL_CULL_FACE);
    glEnable (GL_SCISSOR_TEST);
    glDisable (GL_DEPTH_TEST);
    glDepthMask (GL_FALSE);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glXSwapIntervalSGI (1);
    glGenQueries (ArraySize(_query), _query);
    glGenVertexArrays (ArraySize(_vao), _vao);
    Activate();
}

CGLWindow::~CGLWindow (void) noexcept
{
    glDeleteQueries (ArraySize(_query), _query);
    glDeleteVertexArrays (ArraySize(_vao), _vao);
}

void CGLWindow::Deactivate (void)
{
    _curShader = _curBuffer = _curTexture = _curFont = G::GoidNull;
}

void CGLWindow::Resize (int16_t x, int16_t y, uint16_t w, uint16_t h) noexcept
{
    DTRACE ("[%x] Resize %hux%hu+%hd+%hd\n", IId(), w,h,x,y);
    _winfo.x = x; _winfo.y = y;
    if (_winfo.w == w && _winfo.h == h)
	return;
    _fbsz.w = _winfo.w = w;
    _fbsz.h = _winfo.h = h;
    Viewport (0, 0, w, h);
    PRGLR::Restate (_winfo);
}

void CGLWindow::Viewport (GLint x, GLint y, GLsizei w, GLsizei h) noexcept
{
    if (!w) w = _fbsz.w;
    if (!h) h = _fbsz.h;
    DTRACE ("[%x] Viewport %hux%hu+%hd+%hd\n", IId(), w,h,x,y);
    _viewport.x = x;
    _viewport.y = y;
    _viewport.w = w;
    _viewport.h = h;
    memset (_proj, 0, sizeof(_proj));
    y = _fbsz.h-y-h;
    glViewport (x,y,w,h);
    glScissor (x,y,w,h);
    Scale (1, 1);
    Offset (0, 0);
}

void CGLWindow::Offset (GLint x, GLint y) noexcept
{
    DTRACE ("[%x] Offset %hd:%hd\n", IId(), x,y);		// OpenGL 0,0 is at screen center, screen width 2
    _proj[3][0] = float(-(_viewport.w-2*x-1))/_viewport.w;	// 0.5 pixel center adjustment (w=2,1/w adjusts by 0.5)
    _proj[3][1] = float(_viewport.h-2*y-1)/_viewport.h;		// Same as x, but with y inverted the adjustment is +up
    UniformMatrix ("Transform", Proj());
}

void CGLWindow::Scale (float x, float y) noexcept
{
    DTRACE ("[%x] Scale %g:%g\n", IId(), x,y);
    _proj[0][0] = 2.f*x/_viewport.w;
    _proj[1][1] = -2.f*y/_viewport.h;				// invert y to 0,0 at top left
    _proj[2][2] = 1.f;
    _proj[3][3] = 1.f;
    UniformMatrix ("Transform", Proj());
}

void CGLWindow::ParseDrawlist (goid_t fbid, bstri cmdis)
{
    // Clear VAO slots. Need only to do it here because buffers can not be freed during drawing, so all slots remain valid
    glBindVertexArray (_vao[0]);
    for (auto i = 0u; i < MAX_VAO_SLOTS; ++i)
	glDisableVertexAttribArray (i);
    // Clear GL state remembered from the previous frame
    _curShader = _curBuffer = _curTexture = _curFont = G::GoidNull;
    BindFramebuffer (LookupFramebuffer (fbid), G::FRAMEBUFFER);
    // Now that everything is reset, parse the drawlist
    PDraw<bstri>::Parse (*this, cmdis);
}

uint64_t CGLWindow::DrawFrame (bstri cmdis, Display* dpy)
{
    if (_nextVSync != NotWaitingForVSync) {
	DTRACE ("[%x] Waiting for vsync: ", IId());
	bool bHaveQuery;
	for (auto n = 0u; !(bHaveQuery = QueryResultAvailable(_query[query_FrameEnd])) && ++n < 64;)
	    usleep(256);
	if (bHaveQuery) {
	    uint64_t times[ArraySize(_query)];	// Query times are in ns
	    for (auto i = 0u; i < ArraySize(_query); ++i)
		glGetQueryObjectui64v (_query[i], GL_QUERY_RESULT, &times[i]);
	    _syncEvent.time = times[query_RenderEnd] - times[query_RenderBegin];
	    _syncEvent.key = times[query_FrameEnd] - times[query_RenderBegin];
	    Event (_syncEvent);
	    DTRACE ("Got it. Draw time %u ns, refresh %u ns\n", _syncEvent.time, _syncEvent.key);
	} else	// technically should never happen, but timing out avoids a hang in case of driver problems
	    DTRACE ("query lost\n");
	_nextVSync = NotWaitingForVSync;
    }
    if (cmdis.remaining()) {
	_nextVSync = CApp::NowMS() + LastFrameTime()/1000000 + 1;	// Round up to try to avoid busywait above
	DTRACE ("[%x] Parsing drawlist\n", IId());
	PostQuery (_query[query_RenderBegin]);

	ParseDrawlist (G::default_Framebuffer, cmdis);

	// End of frame swap and queries
	PostQuery (_query[query_RenderEnd]);
	glXSwapBuffers (dpy, Drawable());
	PostQuery (_query[query_FrameEnd]);
    }
    return _nextVSync;
}

uint64_t CGLWindow::DrawFrameNoWait (bstri cmdis, Display* dpy)
{
    if (_nextVSync != NotWaitingForVSync) {
	_pendingFrame.assign (cmdis.ipos(), cmdis.end());
	return _nextVSync;
    }
    return DrawFrame (cmdis, dpy);
}

uint64_t CGLWindow::DrawPendingFrame (Display* dpy)
{
    return DrawFrame (bstri (&*_pendingFrame.begin(), _pendingFrame.size()), dpy);
}

//}}}-------------------------------------------------------------------
//{{{ Buffer

void CGLWindow::BindBuffer (const CBuffer& buf) noexcept
{
    if (Buffer() == buf.Id())
	return;
    DTRACE ("[%x] BindBuffer %x\n", IId(), buf.CId());
    SetBuffer (buf.CId());
    glBindBuffer (buf.Type(), buf.Id());
}

//}}}-------------------------------------------------------------------
//{{{ Shader

void CGLWindow::Shader (const CShader& sh) noexcept
{
    if (Shader() == sh.CId())
	return;
    DTRACE ("[%x] SetShader %x\n", IId(), sh.CId());
    SetShaderId (sh.Id());
    SetShader (sh.CId());
    glUseProgram (sh.Id());
    glBindVertexArray (_vao[sh.CId() == G::default_FontShader]);
    UniformMatrix ("Transform", Proj());
    Color (Color());
}

void CGLWindow::Parameter (const char* varname, const CBuffer& buf, G::Type type, GLuint nels, GLuint offset, GLuint stride) noexcept
{
    GLuint slot;
    if (varname && !varname[1] && varname[0] <= ' ')
	slot = varname[0];
    else
	slot = glGetAttribLocation (ShaderId(), varname);
    Parameter (slot, buf, type, nels, offset, stride);
}

void CGLWindow::Parameter (GLuint slot, const CBuffer& buf, G::Type type, GLuint nels, GLuint offset, GLuint stride) noexcept
{
    BindBuffer (buf);
    DTRACE ("[%x] Parameter %u set to %x, type %s[%u], +%u/%u\n", IId(), slot, buf.Id(), G::TypeName(type), nels, offset, stride);
    glEnableVertexAttribArray (slot);
    glVertexAttribPointer (slot, nels, type, GL_FALSE, stride, BufferOffset(offset));
}

void CGLWindow::Uniform4f (const char* varname, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const noexcept
{
    auto slot = glGetUniformLocation (ShaderId(), varname);
    if (slot < 0) return;
    DTRACE ("[%x] Uniform4f %s = %g,%g,%g,%g\n", IId(), varname, x,y,z,w);
    glUniform4f (slot, x, y, z, w);
}

void CGLWindow::Uniform4iv (const char* varname, const GLint* v) const noexcept
{
    auto slot = glGetUniformLocation (ShaderId(), varname);
    if (slot < 0) return;
    DTRACE ("[%x] Uniform4iv %s = %d,%d,%d,%d\n", IId(), varname, v[0],v[1],v[2],v[3]);
    glUniform4iv (slot, 4, v);
}

void CGLWindow::UniformMatrix (const char* varname, const GLfloat* mat) const noexcept
{
    auto slot = glGetUniformLocation (ShaderId(), varname);
    if (slot < 0) return;
    DTRACE ("[%x] UniformMatrix %s =\n\t%g,%g,%g,%g\n\t%g,%g,%g,%g\n\t%g,%g,%g,%g\n\t%g,%g,%g,%g\n", IId(), varname, mat[0],mat[1],mat[2],mat[3], mat[4],mat[5],mat[6],mat[7], mat[8],mat[9],mat[10],mat[11], mat[12],mat[13],mat[14],mat[15]);
    glUniformMatrix4fv (slot, 1, GL_FALSE, mat);
}

void CGLWindow::UniformTexture (const char* varname, const CTexture& t, GLuint itex) noexcept
{
    if (Texture() == t.CId()) return;
    auto slot = glGetUniformLocation (ShaderId(), varname);
    if (slot < 0) return;
    DTRACE ("[%x] UniformTexture %s = %x slot %u\n", IId(), varname, t.CId(), itex);
    glActiveTexture (GL_TEXTURE0+itex);
    glBindTexture (t.Type(), t.Id());
    SetTexture (t.CId());
    glUniform1i (slot, itex);
}

void CGLWindow::Color (GLuint c) noexcept
{
    SetColor(c);
    float r,g,b,a;
    UnpackColorToFloats (c,r,g,b,a);
    DTRACE ("[%x] Color 0x%08x\n", IId(), c);
    Uniform4f ("Color", r, g, b, a);
}

void CGLWindow::Clear (GLuint c) noexcept
{
    float r,g,b,a;
    UnpackColorToFloats (c,r,g,b,a);
    DTRACE ("[%x] Clear 0x%08x\n", IId(), c);
    glClearColor (r,g,b,a);
    glClear (GL_COLOR_BUFFER_BIT);
}

void CGLWindow::DrawCmdInit (void) noexcept
{
    if (Shader() == G::GoidNull || Shader() == G::default_TextureShader || Shader() == G::default_FontShader)
	SetDefaultShader();
}

void CGLWindow::Enable (G::Feature f, uint16_t o) noexcept
{
    static const GLenum c_Features[G::CAP_N] =	// Parallel to G::Feature
	{ GL_BLEND, GL_CULL_FACE, GL_DEPTH_CLAMP, GL_DEPTH_TEST, GL_MULTISAMPLE };
    if (f >= G::CAP_N)
	return;
    if (o)
	glEnable (c_Features[f]);
    else
	glDisable (c_Features[f]);
}

//}}}-------------------------------------------------------------------
//{{{ Texture

void CGLWindow::Sprite (const CTexture& t, coord_t x, coord_t y)
{
    DTRACE ("[%x] Sprite %x at %d:%d\n", IId(), t.CId(), x,y);
    SetTextureShader();
    UniformTexture ("Texture", t);
    // Solid primitives have the far edge unfilled, for example, 0,0-4,4
    // will draw a 3x3 square. Because y coordinate is inverted, the 3x3
    // is at the bottom left, with top and right pixel strips unfilled.
    // Consequently, to draw a WxH image, need to draw to a (W+1)x(H+1)
    // triangle strip (created by geometry shader), and offset y by -1.
    Uniform4f ("ImageRect", x, y-1, t.Width(), t.Height());
    Uniform4f ("SpriteRect", 0, 0, t.Width(), t.Height());
    glDrawArrays (GL_POINTS, 0, 1);
}

void CGLWindow::Sprite (const CTexture& t, coord_t x, coord_t y, coord_t sx, coord_t sy, dim_t sw, dim_t sh)
{
    DTRACE ("[%x] Sprite %x at %d:%d, src %ux%u+%d+%d\n", IId(), t.CId(), x,y, sw,sh,sx,sy);
    SetTextureShader();
    UniformTexture ("Texture", t);
    // See Sprite above for explanation of -1 and WxH
    Uniform4f ("ImageRect", x, y-1, t.Width(), t.Height());
    Uniform4f ("SpriteRect", sx, sy, sw, sh);
    glDrawArrays (GL_POINTS, 0, 1);
}

//}}}-------------------------------------------------------------------
//{{{ Framebuffer

void CGLWindow::BindFramebuffer (const CFramebuffer& fb, G::FramebufferType bindas)
{
    DTRACE ("[%x] Bind framebuffer %x to target %u\n", IId(), fb.CId(), bindas);
    static const GLenum c_Target[] = { GL_FRAMEBUFFER, GL_READ_FRAMEBUFFER };
    GLenum targ = c_Target [min<uint8_t>(bindas, ArraySize(c_Target)-1)];
    glBindFramebuffer (targ, fb.Id());
    _curFb = fb.CId();
    auto w = fb.Width(), h = fb.Height();
    if (!fb.Id()) {
	w = _winfo.w;
	h = _winfo.h;
    }
    Viewport (0, 0, _fbsz.w = w, _fbsz.h = h);
}

void CGLWindow::SaveFramebuffer (coord_t x, coord_t y, coord_t w, coord_t h, const char* filename, G::Texture::Format fmt, uint8_t quality)
{
    if (!w) {
	x = _viewport.x;
	y = _viewport.y;
	w = _viewport.w;
	h = _viewport.h;
    }
    DTRACE ("[%x] Save framebuffer %ux%u+%d+%d to \"%s\" fmt %u quality %u\n", IId(), w,h,x,y, filename, fmt, quality);
    char tmpfilename[] = SAVEFB_TMPFILE;
    {
	CFile of;
	if (CanPassFd())
	    of.Open (filename, O_WRONLY| O_CREAT| O_TRUNC| O_CLOEXEC, 0600);
	else
	    of.Attach (mkstemps (tmpfilename, strlen(".jpg")));
	CTexture::Save (of.Fd(), x, y, w, h, fmt, quality);
	SaveFB (_curFb, filename, of);
    }
    if (CanPassFd())
	unlink (tmpfilename);
}

//}}}-------------------------------------------------------------------
//{{{ Font

void CGLWindow::Text (coord_t x, coord_t y, const char* s)
{
    const auto& f = (Font() == G::GoidNull ? _pconn->DefaultFont() : LookupFont(Font()));
    SetFontShader();

    DTRACE ("[%x] Text at %d:%d: '%s'\n", IId(), x,y,s);
    uint16_t ws [strlen(s)];
    unsigned nChars = 0;
    for (auto i = utf8in(s); *i; ++i)
	ws[nChars++] = *i;
    struct { GLshort x,y,w,h,s,t; } v [nChars];
    const auto& fi = f.Info();
    uint16_t prevc = 0;
    for (unsigned i = 0, lx = x; i < nChars; ++i) {
	auto c = ws[i];
	auto& gi = fi.Glyph (c);
	v[i].y = y + gi.by;
	v[i].w = gi.w;
	v[i].h = gi.h;
	v[i].s = gi.x;
	v[i].t = gi.y;
	lx -= fi.Kerning (prevc, c);
	prevc = c;
	v[i].x = lx + gi.bx;
	lx += fi.Width (c);
    }
    GLuint buf;
    glGenBuffers (1, &buf);
    glBindBuffer (GL_ARRAY_BUFFER, buf);
    glBufferData (GL_ARRAY_BUFFER, sizeof(v), v, GL_STREAM_DRAW);
    glEnableVertexAttribArray (G::param_Vertex);
    glEnableVertexAttribArray (G::param_TexCoord);
    glVertexAttribPointer (G::param_Vertex, 4, GL_SHORT, GL_FALSE, sizeof(v[0]), 0);
    glVertexAttribPointer (G::param_TexCoord, 2, GL_SHORT, GL_FALSE, sizeof(v[0]), (GLvoid*)(4*sizeof(GLshort)));
    UniformTexture ("Texture", f);
    Uniform4f ("FontTextureSize", f.TextureInfo().w, f.TextureInfo().h, f.TextureInfo().w, f.TextureInfo().h);
    glDrawArrays (GL_POINTS, 0, nChars);
    glDisableVertexAttribArray (G::param_Vertex);
    glDisableVertexAttribArray (G::param_TexCoord);
    glDeleteBuffers (1, &buf);
}

//}}}-------------------------------------------------------------------
//{{{ Queries

inline void CGLWindow::PostQuery (GLuint q)
{
    glQueryCounter (q, GL_TIMESTAMP);
    QueryResultAvailable (q);
}

inline bool CGLWindow::QueryResultAvailable (GLuint q) const
{
    GLint haveQuery;
    glGetQueryObjectiv (q, GL_QUERY_RESULT_AVAILABLE, &haveQuery);
    return haveQuery;
}

//}}}-------------------------------------------------------------------
//{{{ Errors

void CGLWindow::CheckForErrors (void)
{
    static const char c_ErrorText[] =
	"\0invalid enum"
	"\0invalid value"
	"\0invalid operation"
	"\0stack overflow"
	"\0stack underflow"
	"\0out of memory"
	"\0unknown GL error";
    unsigned e = glGetError(), etxtsz = sizeof(c_ErrorText);
    if (e == GL_NO_ERROR)
	return;
    while (glGetError() != GL_NO_ERROR) {}	// Clear all subsequent errors
    e = min (7u, e-(GL_INVALID_ENUM-1));	// Report only first error
    auto etxt = c_ErrorText;
    for (auto i = 0u; i < e; ++i)
	etxt = strnext(etxt, etxtsz);
    DTRACE ("GLError: %s\n", etxt);
    XError::emit (etxt);
}

//}}}-------------------------------------------------------------------

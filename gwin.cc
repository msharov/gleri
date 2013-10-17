// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gwin.h"
#include <sys/time.h>

//----------------------------------------------------------------------

CGLWindow::CGLWindow (iid_t iid, const SWinInfo& winfo, Window win, GLXContext ctx, CIConn* pconn)
: PRGLR(iid)
,_ctx(ctx,iid,win)
,_pendingFrame()
,_pconn (pconn)
,_color (0xffffffff)
,_syncEvent()
,_nextVSync (NotWaitingForVSync)
,_curShaderId (CGObject::NoObject)
,_curShader (G::GoidNull)
,_curBuffer (G::GoidNull)
,_curTexture (G::GoidNull)
,_curFont (G::GoidNull)
,_winfo (winfo)
{
    DTRACE ("[%x] Create: window %x, context %x\n", iid, win, ctx);
    _winfo.h = 0;	// Make invalid until explicit resize
    _query[query_FrameEnd] = 0;
    _syncEvent.type = CEvent::FrameSync;
    _syncEvent.key = 1000000000/60;
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
    _winfo.w = w; _winfo.h = h;
    Viewport (0, 0, w, h);
    PRGLR::Restate (_winfo);
}

void CGLWindow::Viewport (GLint x, GLint y, GLsizei w, GLsizei h) noexcept
{
    if (!w) w = _winfo.w;
    if (!h) h = _winfo.h;
    DTRACE ("[%x] Viewport %hux%hu+%hd+%hd\n", IId(), w,h,x,y);
    _viewport.x = x;
    _viewport.y = y;
    _viewport.w = w;
    _viewport.h = h;
    glViewport (x,_winfo.h-y-h,w,h);
    glScissor (x,_winfo.h-y-h,w,h);
    memset (_proj, 0, sizeof(_proj));
    _proj[0][0] = 2.f/w;
    _proj[1][1] = -2.f/h;
    _proj[3][3] = 1.f;
    Offset (0, 0);
}

void CGLWindow::Offset (GLint x, GLint y) noexcept
{
    DTRACE ("[%x] Offset %hd:%hd\n", IId(), x,y);
    _proj[3][0] = float(-(_viewport.w-2*x-1))/_viewport.w;	// 0.5 pixel center adjustment, 0.5*(2/w)=1/w
    _proj[3][1] = float(_viewport.h-2*y-1)/_viewport.h;
    UniformMatrix ("Transform", Proj());
}

void CGLWindow::Scale (float x, float y) noexcept
{
    DTRACE ("[%x] Scale %g:%g\n", IId(), x,y);
    _proj[0][0] = 2.f*x/_viewport.w;
    _proj[1][1] = -2.f*y/_viewport.h;
    UniformMatrix ("Transform", Proj());
}

uint64_t CGLWindow::DrawFrame (bstri cmdis, Display* dpy)
{
    if (_nextVSync != NotWaitingForVSync) {
	DTRACE ("[%x] Waiting for vsync: ", IId());
	bool bHaveQuery;
	for (unsigned n = 0; !(bHaveQuery = QueryResultAvailable(_query[query_FrameEnd])) && ++n < 64;)
	    usleep(256);
	if (bHaveQuery) {
	    uint64_t times[ArraySize(_query)];	// Query times are in ns
	    for (unsigned i = 0; i < ArraySize(_query); ++i)
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

	// Clear VAO slots. Need only to do it here because buffers can not be freed during drawing, so all slots remain valid
	glBindVertexArray (_vao[0]);
	for (GLuint i = 0; i < MAX_VAO_SLOTS; ++i)
	    glDisableVertexAttribArray (i);
	// Clear GL state remembered from the previous frame
	_curShader = _curBuffer = _curTexture = _curFont = G::GoidNull;

	// Parse the drawlist
	PDraw<bstri>::Parse (*this, cmdis);

	// End of frame swap and queries
	PostQuery (_query[query_RenderEnd]);
	glXSwapBuffers (dpy, Drawable());
	PostQuery (_query[query_FrameEnd]);

	CheckForErrors();
    }
    return (_nextVSync);
}

uint64_t CGLWindow::DrawFrameNoWait (bstri cmdis, Display* dpy)
{
    if (_nextVSync != NotWaitingForVSync) {
	_pendingFrame.assign (cmdis.ipos(), cmdis.end());
	return (_nextVSync);
    }
    return (DrawFrame (cmdis, dpy));
}

uint64_t CGLWindow::DrawPendingFrame (Display* dpy) noexcept
{
    return (DrawFrame (bstri (&*_pendingFrame.begin(), _pendingFrame.size()), dpy));
}

//----------------------------------------------------------------------
// Buffer

void CGLWindow::BindBuffer (const CBuffer& buf, G::EBufferType btype) noexcept
{
    if (Buffer() == buf.Id())
	return;
    DTRACE ("[%x] BindBuffer %x\n", IId(), buf.CId());
    SetBuffer (buf.CId());
    glBindBuffer (btype, buf.Id());
}

//----------------------------------------------------------------------
// Shader interface

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

void CGLWindow::Parameter (const char* varname, const CBuffer& buf, G::EType type, GLuint nels, GLuint offset, GLuint stride) noexcept
{
    Parameter (glGetAttribLocation (ShaderId(), varname), buf, type, nels, offset, stride);
}

void CGLWindow::Parameter (GLuint slot, const CBuffer& buf, G::EType type, GLuint nels, GLuint offset, GLuint stride) noexcept
{
    BindBuffer (buf, G::ARRAY_BUFFER);
    DTRACE ("[%x] Parameter %u set to %x, type %s[%u], +%u/%u\n", IId(), slot, buf.Id(), G::TypeName(type), nels, offset, stride);
    glEnableVertexAttribArray (slot);
    glVertexAttribPointer (slot, nels, type, GL_FALSE, stride, BufferOffset(offset));
}

void CGLWindow::Uniform4f (const char* varname, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const noexcept
{
    GLint slot = glGetUniformLocation (ShaderId(), varname);
    if (slot < 0) return;
    DTRACE ("[%x] Uniform4f %s = %g,%g,%g,%g\n", IId(), varname, x,y,z,w);
    glUniform4f (slot, x, y, z, w);
}

void CGLWindow::Uniform4iv (const char* varname, const GLint* v) const noexcept
{
    GLint slot = glGetUniformLocation (ShaderId(), varname);
    if (slot < 0) return;
    DTRACE ("[%x] Uniform4iv %s = %d,%d,%d,%d\n", IId(), varname, v[0],v[1],v[2],v[3]);
    glUniform4iv (slot, 4, v);
}

void CGLWindow::UniformMatrix (const char* varname, const GLfloat* mat) const noexcept
{
    GLint slot = glGetUniformLocation (ShaderId(), varname);
    if (slot < 0) return;
    DTRACE ("[%x] UniformMatrix %s =\n\t%g,%g,%g,%g\n\t%g,%g,%g,%g\n\t%g,%g,%g,%g\n\t%g,%g,%g,%g\n", IId(), varname, mat[0],mat[1],mat[2],mat[3], mat[4],mat[5],mat[6],mat[7], mat[8],mat[9],mat[10],mat[11], mat[12],mat[13],mat[14],mat[15]);
    glUniformMatrix4fv (slot, 1, GL_FALSE, mat);
}

void CGLWindow::UniformTexture (const char* varname, const CTexture& t, GLuint itex) noexcept
{
    if (Texture() == t.CId()) return;
    GLint slot = glGetUniformLocation (ShaderId(), varname);
    if (slot < 0) return;
    DTRACE ("[%x] UniformTexture %s = %x slot %u\n", IId(), varname, t.CId(), itex);
    glActiveTexture (GL_TEXTURE0+itex);
    glBindTexture (GL_TEXTURE_2D, t.Id());
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

void CGLWindow::Enable (G::EFeature f, uint16_t o) noexcept
{
    static const GLenum c_Features[G::CAP_N] =	// Parallel to G::EFeature
	{ GL_BLEND, GL_CULL_FACE, GL_DEPTH_CLAMP, GL_DEPTH_TEST, GL_MULTISAMPLE };
    if (f >= G::CAP_N)
	return;
    if (o)
	glEnable (c_Features[f]);
    else
	glDisable (c_Features[f]);
}

//----------------------------------------------------------------------
// Texture

void CGLWindow::Sprite (const CTexture& t, coord_t x, coord_t y)
{
    DTRACE ("[%x] Sprite %x at %d:%d\n", IId(), t.Id(), x,y);
    SetTextureShader();
    UniformTexture ("Texture", t);
    Uniform4f ("ImageRect", x, y, t.Width(), t.Height());
    Uniform4f ("SpriteRect", 0, 0, t.Width()-1, t.Height()-1);
    glDrawArrays (GL_POINTS, 0, 1);
}

void CGLWindow::Sprite (const CTexture& t, coord_t x, coord_t y, coord_t sx, coord_t sy, dim_t sw, dim_t sh)
{
    DTRACE ("[%x] Sprite %x at %d:%d, src %ux%u+%d+%d\n", IId(), t.Id(), x,y, sw,sh,sx,sy);
    SetTextureShader();
    UniformTexture ("Texture", t);
    Uniform4f ("ImageRect", x, y, t.Width(), t.Height());
    Uniform4f ("SpriteRect", sx, sy, sw-1, sh-1);
    glDrawArrays (GL_POINTS, 0, 1);
}

//----------------------------------------------------------------------
// Font

void CGLWindow::Text (coord_t x, coord_t y, const char* s)
{
    const CFont& f = (Font() == G::GoidNull ? _pconn->DefaultFont() : LookupFont(Font()));
    SetFontShader();

    DTRACE ("[%x] Text at %d:%d: '%s'\n", IId(), x,y,s);
    const unsigned nChars = strlen(s);
    struct SVertex { GLshort x,y,s,t; } v [nChars];
    const unsigned fw = f.LetterW(), fh = f.LetterH();
    for (unsigned i = 0, lx = x; i < nChars; ++i, lx+=fw) {
	v[i].x = lx;
	v[i].y = y;
	v[i].s = f.LetterX(s[i]);
	v[i].t = f.LetterY(s[i]);
    }
    GLuint buf;
    glGenBuffers (1, &buf);
    glBindBuffer (GL_ARRAY_BUFFER, buf);
    glBufferData (GL_ARRAY_BUFFER, sizeof(v), v, GL_STREAM_DRAW);
    glEnableVertexAttribArray (G::param_Vertex);
    glVertexAttribPointer (G::param_Vertex, 4, GL_SHORT, GL_FALSE, 0, nullptr);
    UniformTexture ("Texture", f);
    Uniform4f ("FontSize", fw-1,fh-1, 256,256);
    glDrawArrays (GL_POINTS, 0, nChars);
    glDisableVertexAttribArray (G::param_Vertex);
    glDeleteBuffers (1, &buf);
}

//----------------------------------------------------------------------
// Queries

inline void CGLWindow::PostQuery (GLuint q)
{
    glQueryCounter (q, GL_TIMESTAMP);
    QueryResultAvailable (q);
}

inline bool CGLWindow::QueryResultAvailable (GLuint q) const
{
    GLint haveQuery;
    glGetQueryObjectiv (q, GL_QUERY_RESULT_AVAILABLE, &haveQuery);
    return (haveQuery);
}

//----------------------------------------------------------------------
// Errors

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
    const char* etxt = c_ErrorText;
    for (unsigned i = 0; i < e; ++i)
	etxt = strnext(etxt, etxtsz);
    DTRACE ("GLError: %s\n", etxt);
    XError::emit (etxt);
}

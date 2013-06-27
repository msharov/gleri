// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gwin.h"
#include <sys/time.h>

//----------------------------------------------------------------------

CGLWindow::CGLWindow (iid_t iid, const SWinInfo& winfo, Window win, GLXContext ctx, CIConn* pconn)
: PRGLR(iid)
,_ctx(ctx,win)
,_shader()
,_texture()
,_font()
,_pak()
,_pendingFrame()
,_pconn (pconn)
,_color (0xffffffff)
,_syncEvent()
,_nextVSync (NotWaitingForVSync)
,_curShader (CGObject::NoObject)
,_curBuffer (CGObject::NoObject)
,_curTexture (CGObject::NoObject)
,_curFont (CGObject::NoObject)
,_winfo (winfo)
,_pendingFrameIId (0)
{
    DTRACE ("[%x] Create: window %x, context %x\n", iid, win, ctx);
    _winfo.h = 0;	// Make invalid until explicit resize
    _query[query_FrameEnd] = 0;
    _syncEvent.type = CEvent::FrameSync;
    _syncEvent.key = 1000000000/60;
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
    SetDefaultShader();
    glGenQueries (ArraySize(_query), _query);
}

void CGLWindow::Deactivate (void)
{
    _curShader = _curBuffer = _curTexture = _curFont = CGObject::NoObject;
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
    _proj[3][0] = -float(_viewport.w-2*x-1)/_viewport.w;
    _proj[3][1] = float(_viewport.h-2*y-1)/_viewport.h;
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
	_nextVSync = CApp::NowMS() + LastFrameTime()/1000000 + 1;	// Round up to avoid busywait above
	DTRACE ("[%x] Parsing drawlist\n", IId());
	PostQuery (_query[query_RenderBegin]);
	PDraw<bstri>::Parse (*this, cmdis);
	PostQuery (_query[query_RenderEnd]);
	glXSwapBuffers (dpy, Drawable());
	PostQuery (_query[query_FrameEnd]);
    }
    return (_nextVSync);
}

uint64_t CGLWindow::DrawFrameNoWait (bstri cmdis, Display* dpy, iid_t iid)
{
    if (_nextVSync != NotWaitingForVSync) {
	_pendingFrameIId = iid;
	_pendingFrame.assign (cmdis.ipos(), cmdis.end());
	return (_nextVSync);
    }
    return (DrawFrame (cmdis, dpy));
}

uint64_t CGLWindow::DrawPendingFrame (Display* dpy) noexcept
{
    uint64_t nf = 0;
    try {
	DrawFrame (bstri (&*_pendingFrame.begin(), _pendingFrame.size()), dpy);
    } catch (XError& e) {
	ForwardError ("Draw", e, -1, _pendingFrameIId);
    }
    _pendingFrame.clear();
    return (nf);
}

void CGLWindow::ForwardError (const char* cmdname, const XError& e, int fd, iid_t iid) noexcept
{
    PRGLR* pcli = this;
    try {
	PRGLR errbuf (iid);
	if (!pcli) {
	    errbuf.SetFd (fd);
	    pcli = &errbuf;
	}
	size_t bufsz = 16+strlen(cmdname)+2+strlen(e.what())+1;
	char buf [bufsz];
	snprintf (buf, bufsz, "%s: %s", cmdname, e.what());
	DTRACE ("[%x] Forwarding error: %s\n", pcli->IId(), buf);
	pcli->ForwardError (buf);
	pcli->WriteCmds();
    } catch (...) {}	// fd errors will be caught by poll
}

//----------------------------------------------------------------------
// Resource loader by enum

/*static*/ void CGLWindow::ShaderUnpack (const GLubyte* s, GLuint ssz, const char* shs[5]) noexcept
{
    bstri shis (s, ssz);
    for (unsigned i = 0; i < 5; ++i) {
	shs[i] = shis.read_strz();
	if (!*shs[i])
	    shs[i] = nullptr;
    }
}

GLuint CGLWindow::LoadResource (G::EResource dtype, G::EBufferHint hint, const GLubyte* d, GLuint dsz)
{
    GLuint sid = CGObject::NoObject;
    switch (dtype) {
	case G::EResource::DATAPAK:
	    sid = LoadDatapak (d, dsz);
	    break;
	case G::EResource::SHADER: {
	    const char* shs[5];
	    ShaderUnpack (d, dsz, shs);
	    sid = LoadShader (shs[0],shs[1],shs[2],shs[3],shs[4]);
	    } break;
	case G::EResource::TEXTURE:
	    sid = LoadTexture (d, dsz);
	    break;
	case G::EResource::FONT:
	    sid = LoadFont (d, dsz);
	    break;
	default:
	    BufferData (sid = CreateBuffer(G::EBufferType(dtype)), d, dsz, hint, G::EBufferType(dtype)); break;
	    break;
    }
    return (sid);
}

GLuint CGLWindow::LoadPakResource (G::EResource dtype, G::EBufferHint hint, GLuint pak, const char* filename, GLuint flnsz)
{
    if (dtype == G::EResource::SHADER) {
	const char* shs[5];
	ShaderUnpack ((const uint8_t*) filename, flnsz, shs);
	return (LoadShader (pak, shs[0],shs[1],shs[2],shs[3],shs[4]));
    } else {
	const CDatapak* p = Datapak (pak);
	if (!p) XError::emit ("RGL protocol error");
	GLuint fsz;
	const GLubyte* pf = p->File (filename, fsz);
	if (!pf) throw XError ("%s is not in the datapak", filename);
	return (LoadResource (dtype, hint, pf, fsz));
    }
}

void CGLWindow::FreeResource (G::EResource dtype, GLuint id)
{
    switch (dtype) {
	case G::EResource::DATAPAK:	FreeDatapak (id);	break;
	case G::EResource::SHADER:	FreeShader (id);	break;
	case G::EResource::TEXTURE:	FreeTexture (id);	break;
	case G::EResource::FONT:	FreeFont (id);		break;
	default:			FreeBuffer (id);	break;
    }
}

//----------------------------------------------------------------------
// Datapak

GLuint CGLWindow::LoadDatapak (const GLubyte* pi, GLuint isz)
{
    DTRACE ("[%x] LoadDatapak %u bytes\n", IId(), isz);
    GLuint osz = 0;
    GLubyte* po = CDatapak::DecompressBlock (pi, isz, osz);
    if (!po) XError::emit ("failed to decompress datapak");
    return (_pak.emplace (_pak.end(), ContextId(), po, osz)->Id());
}

GLuint CGLWindow::LoadDatapak (const char* filename)
{
    DTRACE ("[%x] LoadDatapak from file %s\n", IId(), filename);
    CMMFile f (filename);
    return (LoadDatapak (f.MMData(), f.MMSize()));
}

void CGLWindow::FreeDatapak (GLuint id)
{
    DTRACE ("[%x] FreeDatapak %x\n", IId(), id);
    EraseGObject (_pak, id);
}

const CDatapak* CGLWindow::Datapak (GLuint id) const
{
    return (FindGObject (_pak, id));
}

//----------------------------------------------------------------------
// Buffer

GLuint CGLWindow::CreateBuffer (G::EBufferType btype) noexcept
{
    DTRACE ("[%x] CreateBuffer type %u\n", IId(), btype);
    return (_buffer.emplace (_buffer.end(), ContextId(), btype)->Id());
}

void CGLWindow::FreeBuffer (GLuint buf) noexcept
{
    DTRACE ("[%x] FreeBuffer %x\n", IId(), buf);
    if (Buffer() == buf)
	SetBuffer (CGObject::NoObject);
    EraseGObject (_buffer, buf);
}

void CGLWindow::BindBuffer (GLuint id)
{
    if (Buffer() == id)
	return;
    const CBuffer* pbo = FindGObject (_buffer, id);
    if (!pbo) XError::emit ("no such buffer");
    BindBuffer (id, pbo->Type());
}

void CGLWindow::BindBuffer (GLuint id, G::EBufferType btype) noexcept
{
    if (Buffer() == id)
	return;
    DTRACE ("[%x] BindBuffer %x\n", IId(), id);
    SetBuffer (id);
    glBindBuffer (btype, id);
}

void CGLWindow::BufferData (GLuint buf, const void* data, GLuint dsz, G::EBufferHint mode, G::EBufferType btype)
{
    BindBuffer (buf, btype);
    DTRACE ("[%x] BufferData %u bytes into %x\n", IId(), dsz, buf);
    glBufferData (btype, dsz, data, mode);
}

void CGLWindow::BufferSubData (GLuint buf, const void* data, GLuint dsz, GLuint offset, G::EBufferType btype)
{
    BindBuffer (buf, btype);
    DTRACE ("[%x] BufferSubData %u bytes at %u into %x\n", IId(), dsz, offset, buf);
    glBufferSubData (btype, offset, dsz, data);
}

//----------------------------------------------------------------------
// Shader interface

GLuint CGLWindow::LoadShader (const char* v, const char* tc, const char* te, const char* g, const char* f)
{
    DTRACE ("[%x] LoadShader\n", IId());
    return (_shader.emplace (_shader.end(), ContextId(), CShader::Sources(v,tc,te,g,f))->Id());
}

GLuint CGLWindow::LoadShader (GLuint pak, const char* v, const char* tc, const char* te, const char* g, const char* f)
{
    DTRACE ("[%x] LoadShader from pak %x: %s,%s,%s,%s,%s\n", IId(), pak,v,tc,te,g,f);
    const CDatapak* ppak = Datapak(pak);
    if (!ppak) XError::emit ("RGL protocol error");
    return (_shader.emplace (_shader.end(), ContextId(), CShader::Sources(*ppak,v,tc,te,g,f))->Id());
}

void CGLWindow::FreeShader (GLuint sh) noexcept
{
    DTRACE ("[%x] FreeShader %x\n", IId(), sh);
    EraseGObject (_shader, sh);
}

void CGLWindow::Shader (GLuint id) noexcept
{
    if (Shader() == id)
	return;
    if (id == CGObject::NoObject)
	return;
    DTRACE ("[%x] SetShader %x\n", IId(), id);
    SetShader (id);
    glUseProgram (id);
    UniformMatrix ("Transform", Proj());
    Color (Color());
}

void CGLWindow::Parameter (const char* varname, GLuint buf, G::EType type, GLuint nels, GLuint offset, GLuint stride) noexcept
{
    Parameter (glGetAttribLocation (Shader(), varname), buf, type, nels, offset, stride);
}

void CGLWindow::Parameter (GLuint slot, GLuint buf, G::EType type, GLuint nels, GLuint offset, GLuint stride) noexcept
{
    BindBuffer (buf, G::ARRAY_BUFFER);
    if (slot >= 16) return;
    DTRACE ("[%x] Parameter %u set to %x, type %ux%u, offset %u, stride %u\n", IId(), slot, buf, type, nels, offset, stride);
    glEnableVertexAttribArray (slot);
    glVertexAttribPointer (slot, nels, type, GL_FALSE, stride, BufferOffset(offset));
}

void CGLWindow::Uniform4f (const char* varname, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const noexcept
{
    GLint slot = glGetUniformLocation (Shader(), varname);
    if (slot < 0) return;
    DTRACE ("[%x] Uniform4f %s = %g,%g,%g,%g\n", IId(), varname, x,y,z,w);
    glUniform4f (slot, x, y, z, w);
}

void CGLWindow::Uniform4iv (const char* varname, const GLint* v) const noexcept
{
    GLint slot = glGetUniformLocation (Shader(), varname);
    if (slot < 0) return;
    DTRACE ("[%x] Uniform4iv %s = %d,%d,%d,%d\n", IId(), varname, v[0],v[1],v[2],v[3]);
    glUniform4iv (slot, 4, v);
}

void CGLWindow::UniformMatrix (const char* varname, const GLfloat* mat) const noexcept
{
    GLint slot = glGetUniformLocation (Shader(), varname);
    if (slot < 0) return;
    DTRACE ("[%x] UniformMatrix %s =\n\t%g,%g,%g,%g\n\t%g,%g,%g,%g\n\t%g,%g,%g,%g\n\t%g,%g,%g,%g\n", IId(), varname, mat[0],mat[1],mat[2],mat[3], mat[4],mat[5],mat[6],mat[7], mat[8],mat[9],mat[10],mat[11], mat[12],mat[13],mat[14],mat[15]);
    glUniformMatrix4fv (slot, 1, GL_FALSE, mat);
}

void CGLWindow::UniformTexture (const char* varname, GLuint img, GLuint itex) noexcept
{
    if (Texture() == img) return;
    GLint slot = glGetUniformLocation (Shader(), varname);
    if (slot < 0) return;
    DTRACE ("[%x] UniformTexture %s = %x slot %u\n", IId(), varname, img, itex);
    glActiveTexture (GL_TEXTURE0+itex);
    glBindTexture (GL_TEXTURE_2D, img);
    SetTexture (img);
    glUniform1i (slot, itex);
}

namespace {
inline void UnpackColor (GLuint c, float& r, float& g, float& b, float& a)
{
    static const float convf = 1.f/255;
#if __x86_64__
    asm("movd	%4, %0\n\t"	// r(c000)
	"xorps	%1, %1\n\t"	// g(0000)
	"punpcklbw %1, %0\n\t"	// c 1->2 expand
	"shufps	$0, %5, %5\n\t"	// a(ffff)
	"punpcklwd %1, %0\n\t"	// c 2->4 expand
	"cvtdq2ps %0, %0\n\t"	// c to float r(rgba)
	"mulps	%5, %0\n\t"	// normalize to 0..1
	"movaps	%0, %1\n\t"	// r(rgba) g(rgba)
	"movaps	%0, %3\n\t"	// a(rgba)
	"movhlps %0, %2\n\t"	// b(ba..)
	"shufps	$1, %1, %1\n\t"	// g(g...)
	"shufps	$3, %3, %3"	// a(a...)
	:"=x"(r),"=x"(g),"=x"(b),"=x"(a):"r"(c),"3"(convf));
#else
    GLubyte rb, gb, bb, ab;
    vunpack4(c,rb,gb,bb,ab);
    r = rb*convf; g = gb*convf; b = bb*convf; a = ab*convf;
#endif
}
} // namespace

void CGLWindow::Color (GLuint c) noexcept
{
    SetColor(c);
    float r,g,b,a;
    UnpackColor (c,r,g,b,a);
    DTRACE ("[%x] Color 0x%08x\n", IId(), c);
    Uniform4f ("Color", r, g, b, a);
}

void CGLWindow::Clear (GLuint c) noexcept
{
    float r,g,b,a;
    UnpackColor (c,r,g,b,a);
    DTRACE ("[%x] Clear 0x%08x\n", IId(), c);
    glClearColor (r,g,b,a);
    glClear (GL_COLOR_BUFFER_BIT);
}

void CGLWindow::DrawCmdInit (void) noexcept
{
    if (_curShader == CGObject::NoObject || _curShader == _pconn->TextureShader() || _curShader == _pconn->FontShader())
	SetDefaultShader();
}

//----------------------------------------------------------------------
// Texture

GLuint CGLWindow::LoadTexture (const GLubyte* d, GLuint dsz)
{
    DTRACE ("[%x] LoadTexture %u bytes\n", IId(), dsz);
    _texture.emplace_back (ContextId(), d, dsz);
    return (_texture.back().Id());
}

GLuint CGLWindow::LoadTexture (const char* filename)
{
    CMMFile f (filename);
    DTRACE ("[%x] LoadTexture from file %s\n", IId(), filename);
    return (LoadTexture (f.MMData(), f.MMSize()));
}

void CGLWindow::FreeTexture (GLuint id)
{
    DTRACE ("[%x] FreeTexture %x\n", IId(), id);
    EraseGObject (_texture, id);
}

const CTexture* CGLWindow::Texture (GLuint id) const
{
    return (FindGObject (_texture, id));
}

void CGLWindow::Sprite (coord_t x, coord_t y, GLuint id)
{
    const CTexture* pimg = Texture(id);
    if (!pimg) return;
    DTRACE ("[%x] Sprite %x at %d:%d\n", IId(), id, x,y);
    SetTextureShader();
    UniformTexture ("Texture", pimg->Id());
    Uniform4f ("ImageRect", x, y, pimg->Width(), pimg->Height());
    Uniform4f ("SpriteRect", 0, 0, pimg->Width(), pimg->Height());
    glDrawArrays (GL_POINTS, 0, 1);
}

void CGLWindow::Sprite (coord_t x, coord_t y, GLuint id, coord_t sx, coord_t sy, dim_t sw, dim_t sh)
{
    const CTexture* pimg = Texture(id);
    if (!pimg) return;
    DTRACE ("[%x] Sprite %x at %d:%d, src %ux%u+%d+%d\n", IId(), id, x,y, sw,sh,sx,sy);
    SetTextureShader();
    UniformTexture ("Texture", pimg->Id());
    Uniform4f ("ImageRect", x, y, pimg->Width(), pimg->Height());
    Uniform4f ("SpriteRect", sx, sy, sw, sh);
    glDrawArrays (GL_POINTS, 0, 1);
}

//----------------------------------------------------------------------
// Font

GLuint CGLWindow::LoadFont (const GLubyte* p, GLuint psz)
{
    DTRACE ("[%x] LoadFont %u bytes\n", IId(), psz);
    _font.emplace_back (ContextId(), p, psz);
    SetFont (_font.back().Id());
    return (Font());
}

GLuint CGLWindow::LoadFont (const char* filename)
{
    DTRACE ("[%x] LoadFont from file %s\n", IId(), filename);
    CMMFile f (filename);
    GLuint sz = 0;
    GLubyte* p = CDatapak::DecompressBlock (f.MMData(), f.MMSize(), sz);
    if (!p) XError::emit ("RGL protocol error");
    GLuint id = LoadFont (p, sz);
    free (p);
    return (id);
}

GLuint CGLWindow::LoadFont (GLuint pak, const char* filename)
{
    DTRACE ("[%x] LoadFont from pak %x file %s\n", IId(), pak, filename);
    const CDatapak* p = Datapak (pak);
    if (!p) XError::emit ("RGL protocol error");
    GLuint fsz;
    const GLubyte* pf = p->File (filename, fsz);
    if (!pf) XError::emit ("RGL protocol error");
    return (LoadFont (pf, fsz));
}

void CGLWindow::FreeFont (GLuint id)
{
    DTRACE ("[%x] FreeFont %x\n", IId(), id);
    EraseGObject (_font, id);
}

const CFont* CGLWindow::Font (GLuint id) const noexcept
{
    return (FindGObject (_font, id));
}

void CGLWindow::Text (coord_t x, coord_t y, const char* s)
{
    const CFont* pfont = Font (Font());
    if (!pfont && !(pfont = _pconn->DefaultFont()))
	return;

    DTRACE ("[%x] Text at %d:%d: '%s'\n", IId(), x,y,s);
    const unsigned nChars = strlen(s);
    struct SVertex { GLshort x,y,s,t; } v [nChars];
    const unsigned fw = pfont->Width(), fh = pfont->Height();
    for (unsigned i = 0, lx = x; i < nChars; ++i, lx+=fw) {
	v[i].x = lx;
	v[i].y = y;
	v[i].s = pfont->LetterX(s[i]);
	v[i].t = pfont->LetterY(s[i]);
    }

    GLuint buf = CreateBuffer();
    BufferData (buf, v, sizeof(v), G::STATIC_DRAW);

    SetFontShader();
    UniformTexture ("Texture", pfont->Id());
    Uniform4f ("FontSize", fw,fh, 256,256);
    Parameter (G::TEXT_DATA, buf, G::SHORT, 4);

    glDrawArrays (GL_POINTS, 0, nChars);

    FreeBuffer (buf);

    glFlush();	// Bug in radeon driver overrides uniforms for all queued texts
}

//----------------------------------------------------------------------
// Queries

inline void CGLWindow::PostQuery (GLuint q)
{
    glQueryCounter (q, GL_TIMESTAMP);
    QueryResultAvailable (q);	// Probably a radeon bug: an unasked for query may be set to the first ask time
}

inline bool CGLWindow::QueryResultAvailable (GLuint q) const
{
    GLint haveQuery;
    glGetQueryObjectiv (q, GL_QUERY_RESULT_AVAILABLE, &haveQuery);
    return (haveQuery);
}

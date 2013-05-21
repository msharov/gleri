// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gcli.h"
#include <sys/time.h>

//----------------------------------------------------------------------

/*static*/ const CGLClient* CGLClient::s_RootClient = nullptr;

CGLClient::CGLClient (iid_t iid, Window win, GLXContext ctx)
: PRGLR(iid)
,_ctx(ctx,win)
,_cidmap()
,_shader()
,_texture()
,_font()
,_pak()
,_pendingFrame()
,_color (0xffffffff)
,_syncEvent()
,_nextVSync (NotWaitingForVSync)
,_curShader (CGObject::NoObject)
,_curBuffer (CGObject::NoObject)
,_curTexture (CGObject::NoObject)
,_curFont (CGObject::NoObject)
,_pendingFrameIId (0)
{
    memset (&_winfo, 0, sizeof(_winfo));
    if (!s_RootClient)
	s_RootClient = this;
    _query[query_FrameEnd] = 0;
    _syncEvent.type = CEvent::FrameSync;
    _syncEvent.key = 1000000000/60;
}

void CGLClient::Init (void)
{
    glEnable (GL_BLEND);
    glEnable (GL_CULL_FACE);
    glEnable (GL_SCISSOR_TEST);
    glDisable (GL_DEPTH_TEST);
    glDepthMask (GL_FALSE);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glXSwapIntervalSGI (1);
    if (s_RootClient == this)
	return;
    SetDefaultShader();
    glGenQueries (ArraySize(_query), _query);
}

void CGLClient::Resize (int16_t x, int16_t y, uint16_t w, uint16_t h) noexcept
{
    _winfo.x = x; _winfo.y = y;
    if (_winfo.w == w && _winfo.h == h)
	return;
    _winfo.w = w; _winfo.h = h;
    Viewport (0, 0, w, h);
    PRGLR::Restate (_winfo);
}

void CGLClient::Viewport (GLint x, GLint y, GLsizei w, GLsizei h) noexcept
{
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

void CGLClient::Offset (GLint x, GLint y) noexcept
{
    _proj[3][0] = -float(_viewport.w-2*x-1)/_viewport.w;
    _proj[3][1] = float(_viewport.h-2*y-1)/_viewport.h;
    UniformMatrix ("Transform", Proj());
}

uint64_t CGLClient::DrawFrame (bstri cmdis, Display* dpy)
{
    if (_nextVSync != NotWaitingForVSync) {
	while (!QueryResultAvailable(_query[query_FrameEnd]))
	    usleep(256);
	uint64_t times[ArraySize(_query)];	// Query times are in ns
	for (unsigned i = 0; i < ArraySize(_query); ++i)
	    glGetQueryObjectui64v (_query[i], GL_QUERY_RESULT, &times[i]);
	_syncEvent.time = times[query_RenderEnd] - times[query_RenderBegin];
	_syncEvent.key = times[query_FrameEnd] - times[query_RenderBegin];
	Event (_syncEvent);
	_nextVSync = NotWaitingForVSync;
    }
    if (cmdis.remaining()) {
	_nextVSync = CApp::NowMS() + LastFrameTime()/1000000 + 1;	// Round up to avoid busywait above
	PostQuery (_query[query_RenderBegin]);
	PDraw<bstri>::Parse (*this, cmdis);
	PostQuery (_query[query_RenderEnd]);
	glXSwapBuffers (dpy, Drawable());
	PostQuery (_query[query_FrameEnd]);
    }
    return (_nextVSync);
}

uint64_t CGLClient::DrawFrameNoWait (bstri cmdis, Display* dpy, iid_t iid)
{
    if (_nextVSync != NotWaitingForVSync) {
	_pendingFrameIId = iid;
	_pendingFrame.assign (cmdis.ipos(), cmdis.end());
	return (_nextVSync);
    }
    return (DrawFrame (cmdis, dpy));
}

uint64_t CGLClient::DrawPendingFrame (Display* dpy) noexcept
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

void CGLClient::ForwardError (const char* cmdname, const XError& e, int fd, iid_t iid) noexcept
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
	pcli->ForwardError (buf);
	pcli->WriteCmds();
    } catch (...) {}	// fd errors will be caught by poll
}

//----------------------------------------------------------------------
// Client-side id mapping

void CGLClient::MapId (uint32_t cid, GLuint sid) noexcept
{
    _cidmap.insert (SIdMap(cid,sid));
}

GLuint CGLClient::LookupId (uint32_t cid) const noexcept
{
    auto fi = _cidmap.lower_bound (SIdMap(cid,0));
    return (fi == _cidmap.end() || fi->_cid != cid ? UINT32_MAX : fi->_sid);
}

uint32_t CGLClient::LookupSid (GLuint sid) const noexcept
{
    for (const auto& i : _cidmap)
	if (i._sid == sid)
	    return (i._cid);
    return (UINT32_MAX);
}

void CGLClient::UnmapId (uint32_t cid) noexcept
{
    erase_if (_cidmap, [cid](const SIdMap& i) { return (i._cid == cid); });
}

//----------------------------------------------------------------------
// Resource loader by enum

/*static*/ void CGLClient::ShaderUnpack (const GLubyte* s, GLuint ssz, const char* shs[5]) noexcept
{
    bstri shis (s, ssz);
    for (unsigned i = 0; i < 5; ++i) {
	shs[i] = shis.read_strz();
	if (!*shs[i])
	    shs[i] = nullptr;
    }
}

GLuint CGLClient::LoadResource (G::EResource dtype, G::EBufferHint hint, const GLubyte* d, GLuint dsz)
{
    GLuint sid = UINT_MAX;
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

GLuint CGLClient::LoadPakResource (G::EResource dtype, G::EBufferHint hint, GLuint pak, const char* filename, GLuint flnsz)
{
    if (dtype == G::EResource::SHADER) {
	const char* shs[5];
	ShaderUnpack ((const uint8_t*) filename, flnsz, shs);
	return (LoadShader (pak, shs[0],shs[1],shs[2],shs[3],shs[4]));
    } else {
	const CDatapak* p = Datapak (pak);
	if (!p) Error();
	GLuint fsz;
	const GLubyte* pf = p->File (filename, fsz);
	if (!pf) Error();
	return (LoadResource (dtype, hint, pf, fsz));
    }
}

void CGLClient::FreeResource (G::EResource dtype, GLuint id)
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

GLuint CGLClient::LoadDatapak (const GLubyte* pi, GLuint isz)
{
    GLuint osz = 0;
    GLubyte* po = CDatapak::DecompressBlock (pi, isz, osz);
    if (!po) return (-1);
    return (_pak.emplace (_pak.end(), ContextId(), po, osz)->Id());
}

GLuint CGLClient::LoadDatapak (const char* filename)
{
    CMMFile f (filename);
    return (LoadDatapak (f.MMData(), f.MMSize()));
}

void CGLClient::FreeDatapak (GLuint id)
{
    EraseGObject (_pak, id);
}

const CDatapak* CGLClient::Datapak (GLuint id) const
{
    return (FindGObject (_pak, id));
}

//----------------------------------------------------------------------
// Buffer

GLuint CGLClient::CreateBuffer (G::EBufferType btype) noexcept
{
    return (_buffer.emplace (_buffer.end(), ContextId(), btype)->Id());
}

void CGLClient::FreeBuffer (GLuint buf) noexcept
{
    if (Buffer() == buf)
	SetBuffer (CGObject::NoObject);
    EraseGObject (_buffer, buf);
}

void CGLClient::BindBuffer (GLuint id)
{
    if (Buffer() == id)
	return;
    const CBuffer* pbo = FindGObject (_buffer, id);
    if (!pbo) Error();
    BindBuffer (id, pbo->Type());
}

void CGLClient::BindBuffer (GLuint id, G::EBufferType btype) noexcept
{
    if (Buffer() == id)
	return;
    SetBuffer (id);
    glBindBuffer (btype, id);
}

void CGLClient::BufferData (GLuint buf, const void* data, GLuint dsz, G::EBufferHint mode, G::EBufferType btype)
{
    BindBuffer (buf, btype);
    glBufferData (btype, dsz, data, mode);
}

void CGLClient::BufferSubData (GLuint buf, const void* data, GLuint dsz, GLuint offset, G::EBufferType btype)
{
    BindBuffer (buf, btype);
    glBufferSubData (btype, offset, dsz, data);
}

//----------------------------------------------------------------------
// Shader interface

GLuint CGLClient::LoadShader (const char* v, const char* tc, const char* te, const char* g, const char* f)
{
    _shader.emplace_back (ContextId(), CShader::Sources(v,tc,te,g,f));
    return (_shader.back().Id());
}

GLuint CGLClient::LoadShader (GLuint pak, const char* v, const char* tc, const char* te, const char* g, const char* f)
{
    const CDatapak* ppak = Datapak(pak);
    if (!ppak) return (CGObject::NoObject);
    _shader.emplace_back (ContextId(), CShader::Sources(*ppak,v,tc,te,g,f));
    return (_shader.back().Id());
}

void CGLClient::FreeShader (GLuint sh) noexcept
{
    EraseGObject (_shader, sh);
}

void CGLClient::Shader (GLuint id) noexcept
{
    if (Shader() == id)
	return;
    if (id == UINT_MAX)
	return;
    SetShader (id);
    glUseProgram (id);
    UniformMatrix ("Transform", Proj());
    Color (Color());
}

void CGLClient::Parameter (const char* varname, GLuint buf, G::EType type, GLuint nels, GLuint offset, GLuint stride) noexcept
{
    Parameter (glGetAttribLocation (Shader(), varname), buf, type, nels, offset, stride);
}

void CGLClient::Parameter (GLuint slot, GLuint buf, G::EType type, GLuint nels, GLuint offset, GLuint stride) noexcept
{
    BindBuffer (buf, G::ARRAY_BUFFER);
    if (slot >= 16) return;
    glEnableVertexAttribArray (slot);
    glVertexAttribPointer (slot, nels, type, GL_FALSE, stride, BufferOffset(offset));
}

void CGLClient::Uniform4f (const char* varname, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const noexcept
{
    GLint slot = glGetUniformLocation (Shader(), varname);
    if (slot < 0) return;
    glUniform4f (slot, x, y, z, w);
}

void CGLClient::Uniform4iv (const char* varname, const GLint* v) const noexcept
{
    GLint slot = glGetUniformLocation (Shader(), varname);
    if (slot < 0) return;
    glUniform4iv (slot, 4, v);
}

void CGLClient::UniformMatrix (const char* varname, const GLfloat* mat) const noexcept
{
    GLint slot = glGetUniformLocation (Shader(), varname);
    if (slot < 0) return;
    glUniformMatrix4fv (slot, 1, GL_FALSE, mat);
}

void CGLClient::UniformTexture (const char* varname, GLuint img, GLuint itex) noexcept
{
    if (Texture() == img) return;
    GLint slot = glGetUniformLocation (Shader(), varname);
    if (slot < 0) return;
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
    GLubyte rb = c, gb = c>>8, bb = c>>16, ab = c>>24;
    r = rb*convf; g = gb*convf; b = bb*convf; a = ab*convf;
#endif
}
} // namespace

void CGLClient::Color (GLuint c) noexcept
{
    SetColor(c);
    float r,g,b,a;
    UnpackColor (c,r,g,b,a);
    Uniform4f ("Color", r, g, b, a);
}

void CGLClient::Clear (GLuint c) noexcept
{
    float r,g,b,a;
    UnpackColor (c,r,g,b,a);
    glClearColor (r,g,b,a);
    glClear (GL_COLOR_BUFFER_BIT);
}

void CGLClient::DrawCmdInit (void) noexcept
{
    if (_curShader == s_RootClient->TextureShader() || _curShader == s_RootClient->FontShader())
	SetDefaultShader();
}

//----------------------------------------------------------------------
// Texture

GLuint CGLClient::LoadTexture (const GLubyte* d, GLuint dsz)
{
    _texture.emplace_back (ContextId(), d, dsz);
    return (_texture.back().Id());
}

GLuint CGLClient::LoadTexture (const char* filename)
{
    CMMFile f (filename);
    return (LoadTexture (f.MMData(), f.MMSize()));
}

void CGLClient::FreeTexture (GLuint id)
{
    EraseGObject (_texture, id);
}

const CTexture* CGLClient::Texture (GLuint id) const
{
    return (FindGObject (_texture, id));
}

void CGLClient::Sprite (coord_t x, coord_t y, GLuint id)
{
    const CTexture* pimg = Texture(id);
    if (!pimg) return;
    SetTextureShader();
    UniformTexture ("Texture", pimg->Id());
    Uniform4f ("ImageRect", x, y, pimg->Width(), pimg->Height());
    Uniform4f ("SpriteRect", 0, 0, pimg->Width(), pimg->Height());
    glDrawArrays (GL_POINTS, 0, 1);
}

void CGLClient::Sprite (coord_t x, coord_t y, GLuint id, coord_t sx, coord_t sy, dim_t sw, dim_t sh)
{
    const CTexture* pimg = Texture(id);
    if (!pimg) return;
    SetTextureShader();
    UniformTexture ("Texture", pimg->Id());
    Uniform4f ("ImageRect", x, y, pimg->Width(), pimg->Height());
    Uniform4f ("SpriteRect", sx, sy, sw, sh);
    glDrawArrays (GL_POINTS, 0, 1);
}

//----------------------------------------------------------------------
// Font

GLuint CGLClient::LoadFont (const GLubyte* p, GLuint psz)
{
    _font.emplace_back (ContextId(), p, psz);
    SetFont (_font.back().Id());
    return (Font());
}

GLuint CGLClient::LoadFont (const char* filename)
{
    CMMFile f (filename);
    GLuint sz = 0;
    GLubyte* p = CDatapak::DecompressBlock (f.MMData(), f.MMSize(), sz);
    if (!p) return (-1);
    GLuint id = LoadFont (p, sz);
    free (p);
    return (id);
}

GLuint CGLClient::LoadFont (GLuint pak, const char* filename)
{
    const CDatapak* p = Datapak (pak);
    if (!p) return (-1);
    GLuint fsz;
    const GLubyte* pf = p->File (filename, fsz);
    if (!pf) return (-1);
    return (LoadFont (pf, fsz));
}

void CGLClient::FreeFont (GLuint id)
{
    EraseGObject (_font, id);
}

const CFont* CGLClient::Font (GLuint id) const noexcept
{
    return (FindGObject (_font, id));
}

void CGLClient::Text (coord_t x, coord_t y, const char* s)
{
    const CFont* pfont = Font (Font());
    if (!pfont && !(pfont = s_RootClient->DefaultFont()))
	return;

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

inline void CGLClient::PostQuery (GLuint q)
{
    glQueryCounter (q, GL_TIMESTAMP);
    QueryResultAvailable (q);	// Probably a radeon bug: an unasked for query may be set to the first ask time
}

inline bool CGLClient::QueryResultAvailable (GLuint q) const
{
    GLint haveQuery;
    glGetQueryObjectiv (q, GL_QUERY_RESULT_AVAILABLE, &haveQuery);
    return (haveQuery);
}

#include "gcli.h"
#include "mmfile.h"

//----------------------------------------------------------------------

/*static*/ const CGLClient* CGLClient::s_RootClient = nullptr;

CGLClient::CGLClient (int fd, iid_t iid, Window win, GLXContext ctx)
: PRGLR(fd,iid)
,_ctx(ctx,win)
,_cidmap()
,_shader()
,_texture()
,_font()
,_pak()
,_color (0xffffffff)
,_curShader (CGObject::NoObject)
,_curBuffer (CGObject::NoObject)
,_curTexture (CGObject::NoObject)
,_curFont (CGObject::NoObject)
,_w(0)
,_h(0)
{
    if (!s_RootClient)
	s_RootClient = this;
}

void CGLClient::Init (void)
{
    glEnable (GL_BLEND);
    glEnable (GL_CULL_FACE);
    glDisable (GL_DEPTH_TEST);
    glDepthMask (GL_FALSE);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glXSwapIntervalSGI (1);
    if (s_RootClient == this)
	return;
    SetDefaultShader();
}

void CGLClient::Resize (uint16_t w, uint16_t h) noexcept
{
    if (_w == w && _h == h)
	return;
    _w = w; _h = h;

    glViewport (0, 0, w, h);
    memset (_proj, 0, sizeof(_proj));
    _proj[0][0] = 2.f/w;
    _proj[1][1] = -2.f/h;
    _proj[3][3] = 1.f;
    _proj[3][0] = -float(w-1)/w;
    _proj[3][1] = float(h-1)/h;
    UniformMatrix ("Transform", Proj());

    PRGLR::Resize (w, h);
}

void CGLClient::MapId (uint32_t cid, GLuint sid) noexcept
{
    _cidmap.insert (SIdMap(cid,sid));
}

GLuint CGLClient::LookupId (uint32_t cid) const noexcept
{
    auto fi = _cidmap.lower_bound (SIdMap(cid,0));
    return (fi == _cidmap.end() ? UINT32_MAX : fi->_sid);
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
// Datapak

GLuint CGLClient::LoadDatapak (const GLubyte* pi, GLuint isz)
{
    GLuint osz = 0;
    GLubyte* po = CMMFile::DecompressBlock (pi, isz, osz);
    if (!po) return (-1);
    _pak.emplace_back (ContextId(), po, osz);
    return (_pak.back().Id());
}

GLuint CGLClient::LoadDatapak (const char* filename)
{
    CMMFile f (filename);
    return (LoadDatapak (f.Data(), f.Size()));
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

GLuint CGLClient::CreateBuffer (void) noexcept
{
    GLuint id;
    glGenBuffers (1, &id);
    return (id);
}

void CGLClient::FreeBuffer (GLuint buf) noexcept
{
    if (Buffer() == buf)
	SetBuffer (CGObject::NoObject);
    glDeleteBuffers (1, &buf);
}

void CGLClient::BindBuffer (GLuint id) noexcept
{
    if (Buffer() == id)
	return;
    SetBuffer (id);
    glBindBuffer (GL_ARRAY_BUFFER, id);
}

void CGLClient::BufferData (GLuint buf, const void* data, GLuint dsz, GLushort mode, GLushort btype)
{
    BindBuffer (buf);
    glBufferData (btype, dsz, data, mode);
}

void CGLClient::BufferSubData (GLuint buf, const void* data, GLuint dsz, GLuint offset, GLushort btype)
{
    BindBuffer (buf);
    glBufferSubData (btype, offset, dsz, data);
}

//----------------------------------------------------------------------
// Shader interface

GLuint CGLClient::LoadShader (GLuint pak, const char* v, const char* tc, const char* te, const char* g, const char* f) noexcept
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

void CGLClient::Parameter (const char* varname, GLuint buf, GLenum type, GLuint nels, GLuint offset, GLuint stride) noexcept
{
    Parameter (glGetAttribLocation (Shader(), varname), buf, type, nels, offset, stride);
}

void CGLClient::Parameter (GLuint slot, GLuint buf, GLenum type, GLuint nels, GLuint offset, GLuint stride) noexcept
{
    BindBuffer (buf);
    if (slot >= 16) return;
    glEnableVertexAttribArray (slot);
    glVertexAttribPointer (slot, nels, type, GL_FALSE, stride, (const void*)(long) offset);
}

void CGLClient::Uniform4f (const char* varname, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const noexcept
{
    GLint slot = glGetUniformLocation (Shader(), varname);
    if (slot < 0) return;
    glUniform4f (slot, x, y, z, w);
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

void CGLClient::Primitive (GLenum mode, GLuint first, GLuint count) noexcept
{
    if (_curShader == s_RootClient->TextureShader() || _curShader == s_RootClient->FontShader())
	SetDefaultShader();
    glDrawArrays (mode,first,count);
}

//----------------------------------------------------------------------
// Texture

GLuint CGLClient::LoadTexture (const char* filename)
{
    CMMFile f (filename);
    _texture.emplace_back (ContextId(), f.Data(), f.Size());
    return (_texture.back().Id());
}

void CGLClient::FreeTexture (GLuint id)
{
    EraseGObject (_texture, id);
}

const CTexture* CGLClient::Texture (GLuint id) const
{
    return (FindGObject (_texture, id));
}

void CGLClient::Sprite (short x, short y, GLuint id)
{
    const CTexture* pimg = Texture(id);
    if (!pimg) return;
    SetTextureShader();
    UniformTexture ("Texture", pimg->Id());
    Uniform4f ("ImageRect", x, y, pimg->Width(), pimg->Height());
    Uniform4f ("SpriteRect", 0, 0, pimg->Width(), pimg->Height());
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
    GLubyte* p = CMMFile::DecompressBlock (f.Data(), f.Size(), sz);
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

void CGLClient::Text (int16_t x, int16_t y, const char* s)
{
    const CFont* pfont = Font (Font());
    if (!pfont && !(pfont = s_RootClient->DefaultFont()))
	return;

    const unsigned nChars = strlen(s);
    struct SVertex { int16_t x,y,s,t; } v [nChars];
    const unsigned fw = pfont->Width(), fh = pfont->Height();
    for (unsigned i = 0, lx = x; i < nChars; ++i, lx+=fw) {
	v[i].x = lx;
	v[i].y = y;
	v[i].s = pfont->LetterX(s[i]);
	v[i].t = pfont->LetterY(s[i]);
    }

    GLuint buf = CreateBuffer();
    BufferData (buf, v, sizeof(v), GL_STATIC_DRAW);

    SetFontShader();
    UniformTexture ("Texture", pfont->Id());
    Uniform4f ("FontSize", fw,fh, 256,256);
    Parameter (G::TEXT_DATA, buf, GL_SHORT, 4);

    glDrawArrays (GL_POINTS, 0, nChars);

    FreeBuffer (buf);

    glFlush();	// Bug in radeon driver overrides uniforms for all queued texts
}

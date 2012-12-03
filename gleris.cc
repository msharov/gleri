#include "gleris.h"
#include <GL/glext.h>
#include <GL/glxext.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include ".o/data/data.h"

//----------------------------------------------------------------------
//{{{ Error handling ---------------------------------------------------
namespace {

static char* _xlib_error = nullptr;
static int XlibErrorHandler (Display* dpy, XErrorEvent* ee)
{
    if (_xlib_error)	// Report only the first error
	return (0);
    char errortext [256];
    XGetErrorText (dpy, ee->error_code, errortext, sizeof(errortext));
    asprintf (&_xlib_error, "%lu.%hhu.%hhu: %s", ee->serial, ee->request_code, ee->minor_code, errortext);
    return (0);
}

static int XlibIOErrorHandler (Display*)
{
    printf ("Error: connection to X server abnormally terminated\n");
    exit (EXIT_FAILURE);
    return (0);
}

} // namespace

//}}}-------------------------------------------------------------------
//{{{ CGleris::CClient

void CGleris::CClient::MapId (uint32_t cid, GLuint sid) noexcept
{
    _cidmap.insert (SIdMap(cid,sid));
}

GLuint CGleris::CClient::LookupId (uint32_t cid) const noexcept
{
    auto fi = _cidmap.lower_bound (SIdMap(cid,0));
    return (fi == _cidmap.end() ? UINT32_MAX : fi->_sid);
}

uint32_t CGleris::CClient::LookupSid (GLuint sid) const noexcept
{
    for (const auto& i : _cidmap)
	if (i._sid == sid)
	    return (i._cid);
    return (UINT32_MAX);
}

void CGleris::CClient::UnmapId (uint32_t cid) noexcept
{
    for (auto i = _cidmap.begin(); i != _cidmap.end(); ++i)
	if (i->_cid == cid)
	    --(i = _cidmap.erase(i));
}

template <typename T>
inline void CGleris::CClient::RemoveIdsFrom (vector<T>& v)
{
    for (auto i = v.begin(); i < v.end(); ++i)
	if (LookupSid(i->Id()) != UINT32_MAX)
	    --(i = v.erase(i));
}

//}}}-------------------------------------------------------------------
//{{{ App core

CGleris::CGleris (void) noexcept
: CApp()
,_fbconfig (nullptr)
,_color (0xffffffff)
,_curShader (CGObject::NoObject)
,_curBuffer (CGObject::NoObject)
,_curTexture (CGObject::NoObject)
,_curContext (nullptr)
,_curFont (nullptr)
,_texture()
,_font()
,_shader()
,_pak()
,_cli()
,_icbuf()
,_dpy (nullptr)
,_visinfo (nullptr)
,_colormap (None)
,_screen (None)
,_rootWindow (None)
,_glversion (0)
,_options (0)
{
    XSetErrorHandler (XlibErrorHandler);
    XSetIOErrorHandler (XlibIOErrorHandler);
}

CGleris::~CGleris (void) noexcept
{
    for (auto& c : _cli)
	DestroyClient (c);
    XSync (_dpy, True);
    XCloseDisplay (_dpy);
}

inline void CGleris::OnArgs (argc_t argc, argv_t argv) noexcept
{
    if (argc > 1 && !strcmp(argv[1],"-s")) {
	SetOption (opt_SingleClient);
	WatchFd (STDIN_FILENO);
    }
}

//----------------------------------------------------------------------
// X and OpenGL interface

void CGleris::CheckForXlibErrors (void) const
{
    XSync (_dpy, False);
    if (_xlib_error) {
	throw XError (_xlib_error);
	free (_xlib_error);
	_xlib_error = nullptr;
    }
}

Window CGleris::CreateWindow (unsigned w, unsigned h) const
{
    XSetWindowAttributes swa;
    swa.colormap = _colormap;
    swa.background_pixmap = None;
    swa.border_pixel = BlackPixel (_dpy, _screen);
    swa.event_mask = StructureNotifyMask| ExposureMask| KeyPressMask| ButtonPressMask| PointerMotionMask;

    Window win = XCreateWindow (_dpy, _rootWindow, 0, 0, w, h, 0,
				_visinfo->depth, InputOutput, _visinfo->visual,
				CWBackPixmap| CWBorderPixel| CWColormap| CWEventMask, &swa);
    if (!win)
	throw XError ("failed to create window");
    return (win);
}

GLXContext CGleris::CreateContext (unsigned version) const
{
    int major = version>>4, minor = version&0xf;
    int context_attribs[] = {
	GLX_CONTEXT_MAJOR_VERSION_ARB,	major,
	GLX_CONTEXT_MINOR_VERSION_ARB,	minor,
	GLX_CONTEXT_FLAGS_ARB,		GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
	GLX_CONTEXT_PROFILE_MASK_ARB,	GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
	None
    };
    GLXContext ctx = glXCreateContextAttribsARB (_dpy, _fbconfig, _cli.empty() ? nullptr : _cli[0].ContextId(), True, context_attribs);
    if (!ctx)
	throw XError ("X server does not support OpenGL %d.%d", major, minor);
    if (!glXIsDirect (_dpy, ctx)) {
	glXDestroyContext (_dpy, ctx);
	throw XError ("direct rendering is not enabled");
    }
    return (ctx);
}

inline void CGleris::ActivateContext (const CContext& rctx) noexcept
{
    if (rctx.Context() != _curContext)
	glXMakeCurrent (_dpy, rctx.Drawable(), _curContext = rctx.Context());
}

void CGleris::DestroyContext (const CContext& rctx) noexcept
{
    if (rctx.Context() == _curContext)
	glXMakeCurrent (_dpy, None, _curContext = nullptr);
    glXDestroyContext (_dpy, rctx.Context());
    XDestroyWindow (_dpy, rctx.Drawable());
}

void CGleris::Init (argc_t argc, argv_t argv)
{
    OnArgs (argc, argv);
    //
    // Connect to X display and get server information
    //
    _dpy = XOpenDisplay (nullptr);
    if (!_dpy)
	throw XError ("could not open X display");
    WatchFd (ConnectionNumber(_dpy));

    int glx_major = 0, glx_minor = 0;
    if (!glXQueryVersion (_dpy, &glx_major, &glx_minor) || (glx_major<<4|glx_minor) < 0x14)
	throw XError ("X server does not support GLX 1.4");

    static const int fbconfattr[] = {
	GLX_DRAWABLE_TYPE,	GLX_WINDOW_BIT,
	GLX_X_VISUAL_TYPE,	GLX_TRUE_COLOR,
	GLX_RENDER_TYPE,	GLX_RGBA_BIT,
	GLX_CONFIG_CAVEAT,	GLX_NONE,
	GLX_DOUBLEBUFFER,	True,
	GLX_DEPTH_SIZE,		16,
	GLX_RED_SIZE,		8,
	GLX_GREEN_SIZE,		8,
	GLX_BLUE_SIZE,		8,
	GLX_ALPHA_SIZE,		8,
	GLX_NONE
    };
    int fbcount;
    GLXFBConfig* fbcs = glXChooseFBConfig (_dpy, DefaultScreen(_dpy), fbconfattr, &fbcount);
    if (!fbcs || !fbcount)
	throw XError ("no suitable visuals available");
    _fbconfig = fbcs[0];
    XFree (fbcs);

    _visinfo = glXGetVisualFromFBConfig (_dpy, _fbconfig);
    if (!_visinfo)
	throw XError ("no suitable visuals available");
    _screen = _visinfo->screen;
    _rootWindow = RootWindow(_dpy, _screen);
    //
    // Create global resources
    //
    _colormap = XCreateColormap(_dpy, _rootWindow, _visinfo->visual, AllocNone);

    // Create the root gl context (share root)
    Window rctxw = CreateWindow (1, 1);	// Temporary window to create the root gl context
    CheckForXlibErrors();
    GLXContext ctx = glXCreateNewContext (_dpy, _fbconfig, GLX_RGBA_TYPE, nullptr, True);
    if (!ctx)
	throw XError ("failed to create an OpenGL context");
    glXMakeCurrent (_dpy, rctxw, ctx);

    // The root context is needed to get the highest supported opengl version
    GLint major = 0, minor = 0;
    const char* verstr = (const char*) glGetString (GL_VERSION);
    if (verstr)
	major = atoi(verstr);
    if (major >= 3) {
	glGetIntegerv (GL_MAJOR_VERSION, &major);
	glGetIntegerv (GL_MINOR_VERSION, &minor);
    }
    if ((_glversion = (major<<4)+minor) < 0x33)
	_glversion = 0x33;

    // Now delete it and recreate with core profile for highest version
    glXMakeCurrent (_dpy, None, nullptr);
    glXDestroyContext (_dpy, ctx);
    _cli.emplace_back (0, rctxw, CreateContext(_glversion));

    // Activate the root context and load shared resources into it
    ActivateContext (_cli[0].Context());
    GLuint pak = LoadDatapak (File_resource, sizeof(File_resource));
    LoadFont (pak, "ter-d18b.psf");
    LoadShader (pak, "sh/flat_v.glsl", "sh/flat_f.glsl");
    LoadShader (pak, "sh/image_v.glsl", "sh/image_f.glsl");
    LoadShader (pak, "sh/font_v.glsl", "sh/font_f.glsl");
    FreeDatapak (pak);

    // Process X events queued so far
    OnXEvent();
}

void CGleris::OnXEvent (void)
{
    for (XEvent xev; XPending(_dpy);) {
	XNextEvent(_dpy,&xev);

	auto icli = _cli.begin();
	for (; icli < _cli.end(); ++icli)
	    if (icli->Drawable() == xev.xany.window)
		break;
	if (icli >= _cli.end())
	    break;

	if (xev.type == Expose)
	    icli->Draw();
	else if (xev.type == ConfigureNotify) {
	    ActivateContext (icli->Context());
	    OnResize (xev.xconfigure.width, xev.xconfigure.height);
	    icli->Resize (xev.xconfigure.width, xev.xconfigure.height);
	} else if (xev.type == KeyPress)
	    icli->Event (xev.xkey.keycode);
	icli->WriteToFd (STDIN_FILENO);
    }
}

void CGleris::OnFd (int fd)
{
    CApp::OnFd(fd);
    if (fd == ConnectionNumber(_dpy))
	OnXEvent();
    else if (fd == STDIN_FILENO) {
	_icbuf.ReadFromFd (STDIN_FILENO);
	PRGL::Parse (*this, _icbuf);
    }
}

void CGleris::OnFdError (int fd)
{
    CApp::OnFdError(fd);
    if (fd == ConnectionNumber(_dpy))
	throw XError ("X server connection terminated");
    else if (fd == STDIN_FILENO) {
	printf ("client disconnected\n");
	return (Quit());
    }
}

void CGleris::CreateClient (CClient::iid_t iid, uint16_t w, uint16_t h, uint16_t glversion)
{
    Window wid = CreateWindow (w, h);
    XMapWindow (_dpy, wid);
    CheckForXlibErrors();

    GLXContext ctx = CreateContext(glversion);
    _cli.emplace_back (iid, wid, ctx);

    // Activate the new context and set default parameters
    glXMakeCurrent (_dpy, wid, _curContext = ctx);

    glEnable (GL_BLEND);
    glEnable (GL_CULL_FACE);
    glDisable (GL_DEPTH_TEST);
    glDepthMask (GL_FALSE);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glXSwapIntervalSGI (1);

    OnXEvent();
}

void CGleris::DestroyClient (CClient& rc) noexcept
{
    ActivateContext (rc.Context());
    rc.RemoveIdsFrom (_texture);
    rc.RemoveIdsFrom (_font);
    rc.RemoveIdsFrom (_shader);
    rc.RemoveIdsFrom (_pak);
    DestroyContext (rc.Context());
}

CGleris::CClient* CGleris::ClientRecord (CClient::iid_t iid) noexcept
{
    for (auto i = _cli.begin(); i < _cli.end(); ++i) {
	if (i->IId() == iid) {
	    ActivateContext (i->Context());
	    return (&*i);
	}
    }
    return (nullptr);
}

void CGleris::ClientDraw (CClient& cli, bstri& cmdis)
{
    PDraw<bstri>::Parse (*this, cli, cmdis);
    glXSwapBuffers (_dpy, cli.Drawable());
}

void CGleris::OnResize (unsigned w, unsigned h)
{
    glViewport (0, 0, w, h);
    memset (_proj, 0, sizeof(_proj));
    _proj[0][0] = 2.f/w;
    _proj[1][1] = -2.f/h;
    _proj[3][3] = 1.f;
    _proj[3][0] = -float(w-1)/w;
    _proj[3][1] = float(h-1)/h;
}

//}}}-------------------------------------------------------------------
//{{{ Shader interface

GLuint CGleris::LoadShader (GLuint pak, const char* v, const char* tc, const char* te, const char* g, const char* f) noexcept
{
    const CDatapak* ppak = Datapak(pak);
    if (!ppak) return (CGObject::NoObject);
    _shader.emplace_back (_curContext, CShader::Sources(*ppak,v,tc,te,g,f));
    return (_shader.back().Id());
}

void CGleris::FreeShader (GLuint sh) noexcept
{
    for (auto i = _shader.begin(); i < _shader.end(); ++i)
	if (i->Id() == sh)
	    --(i = _shader.erase(i));
}

void CGleris::Shader (GLuint id) noexcept
{
    if (_curShader == id)
	return;
    if (id == UINT_MAX)
	return;
    _curShader = id;
    glUseProgram (id);
    UniformMatrix ("Transform", &_proj[0][0]);
    Color (_color);
}

void CGleris::Parameter (const char* varname, GLuint buf, GLenum type, GLuint size, GLuint offset, GLuint stride) noexcept
{
    Parameter (glGetAttribLocation (_curShader, varname), buf, type, size, offset, stride);
}

void CGleris::Parameter (GLuint slot, GLuint buf, GLenum type, GLuint size, GLuint offset, GLuint stride) noexcept
{
    BindBuffer (buf);
    if (slot >= 16) return;
    glEnableVertexAttribArray (slot);
    glVertexAttribPointer (slot, size, type, GL_FALSE, stride, (const void*)(long) offset);
}

void CGleris::Uniform4f (const char* varname, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const noexcept
{
    GLint slot = glGetUniformLocation (_curShader, varname);
    if (slot < 0) return;
    glUniform4f (slot, x, y, z, w);
}

void CGleris::UniformMatrix (const char* varname, const GLfloat* mat) const noexcept
{
    GLint slot = glGetUniformLocation (_curShader, varname);
    if (slot < 0) return;
    glUniformMatrix4fv (slot, 1, GL_FALSE, mat);
}

void CGleris::UniformTexture (const char* varname, GLuint img, GLuint itex) noexcept
{
    if (_curTexture == img) return;
    GLint slot = glGetUniformLocation (_curShader, varname);
    if (slot < 0) return;
    glActiveTexture (GL_TEXTURE0+itex);
    glBindTexture (GL_TEXTURE_2D, img);
    _curTexture = img;
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

void CGleris::Color (GLuint c) noexcept
{
    _color = c;
    float r,g,b,a;
    UnpackColor (c,r,g,b,a);
    Uniform4f ("Color", r, g, b, a);
}

void CGleris::Clear (GLuint c) noexcept
{
    float r,g,b,a;
    UnpackColor (c,r,g,b,a);
    glClearColor (r,g,b,a);
    glClear (GL_COLOR_BUFFER_BIT);
}

//}}}-------------------------------------------------------------------
//{{{ Buffer

GLuint CGleris::CreateBuffer (void) noexcept
{
    GLuint id;
    glGenBuffers (1, &id);
    return (id);
}

void CGleris::FreeBuffer (GLuint buf) noexcept
{
    if (_curBuffer == buf)
	_curBuffer = CGObject::NoObject;
    glDeleteBuffers (1, &buf);
}

void CGleris::BindBuffer (GLuint id) noexcept
{
    if (_curBuffer == id)
	return;
    _curBuffer = id;
    glBindBuffer (GL_ARRAY_BUFFER, id);
}

void CGleris::BufferData (GLuint buf, const void* data, GLuint size, GLushort mode, GLushort btype)
{
    BindBuffer (buf);
    glBufferData (btype, size, data, mode);
}

void CGleris::BufferSubData (GLuint buf, const void* data, GLuint size, GLuint offset, GLushort btype)
{
    BindBuffer (buf);
    glBufferSubData (btype, offset, size, data);
}

//}}}-------------------------------------------------------------------
//{{{ Datapak

GLuint CGleris::LoadDatapak (const GLubyte* pi, GLuint isz)
{
    GLuint osz = 0;
    GLubyte* po = DecompressBlock (pi, isz, osz);
    if (!po) return (-1);
    _pak.emplace_back (_curContext, po, osz);
    return (_pak.back().Id());
}

GLuint CGleris::LoadDatapak (const char* filename)
{
    CMMFile f (filename);
    return (LoadDatapak (f.Data(), f.Size()));
}

void CGleris::FreeDatapak (GLuint id)
{
    for (auto i = _pak.begin(); i < _pak.end(); ++i)
	if (i->Id() == id)
	    --(i = _pak.erase(i));
}

const CDatapak* CGleris::Datapak (GLuint id) const
{
    for (auto i = _pak.begin(); i < _pak.end(); ++i)
	if (i->Id() == id)
	    return (&*i);
    return (nullptr);
}

//}}}-------------------------------------------------------------------
//{{{ Texture

GLuint CGleris::LoadTexture (const char* filename)
{
    CMMFile f (filename);
    _texture.emplace_back (_curContext, f.Data(), f.Size());
    return (_texture.back().Id());
}

void CGleris::FreeTexture (GLuint id)
{
    for (auto i = _texture.begin(); i < _texture.end(); ++i)
	if (i->Id() == id)
	    --(i = _texture.erase(i));
}

const CTexture* CGleris::Texture (GLuint id) const
{
    for (auto i = _texture.begin(); i < _texture.end(); ++i)
	if (i->Id() == id)
	    return (&*i);
    return (nullptr);
}

void CGleris::Sprite (short x, short y, GLuint id)
{
    const CTexture* pimg = Texture(id);
    if (!pimg) return;
    const GLshort w = pimg->Width(), h = pimg->Height(), xw = x+w, yh = y+h;
    const GLshort v[16] = { x,y, x,yh, xw,y, xw,yh,  0,0, 0,TEXCOORD_ONE, TEXCOORD_ONE,0, TEXCOORD_ONE,TEXCOORD_ONE };

    GLuint buf = CreateBuffer();
    BufferData (buf, v, sizeof(v), GL_STATIC_DRAW);

    Shader (_shader[1].Id());
    Parameter (G::VERTEX, buf, GL_SHORT, 2, 0);
    Parameter (G::TEXTURE_COORD, buf, GL_SHORT, 2, sizeof(v)/2);
    UniformTexture ("Texture", pimg->Id());

    glDrawArrays (GL_TRIANGLE_STRIP, 0, 4);

    FreeBuffer (buf);
}

//}}}-------------------------------------------------------------------
//{{{ Font

GLuint CGleris::LoadFont (const GLubyte* p, GLuint psz)
{
    _font.emplace_back (_curContext, p, psz);
    _curFont = &_font.back();
    return (_curFont->Id());
}

GLuint CGleris::LoadFont (const char* filename)
{
    CMMFile f (filename);
    GLuint sz = 0;
    GLubyte* p = DecompressBlock (f.Data(), f.Size(), sz);
    if (!p) return (-1);
    GLuint id = LoadFont (p, sz);
    free (p);
    return (id);
}

GLuint CGleris::LoadFont (GLuint pak, const char* filename)
{
    const CDatapak* p = Datapak (pak);
    if (!p) return (-1);
    GLuint fsz;
    const GLubyte* pf = p->File (filename, fsz);
    if (!pf) return (-1);
    return (LoadFont (pf, fsz));
}

void CGleris::FreeFont (GLuint id)
{
    for (auto i = _font.begin(); i < _font.end(); ++i)
	if (i->Id() == id)
	    --(i = _font.erase(i));
}

void CGleris::SetFont (GLuint id)
{
    for (auto i = _font.begin(); i < _font.end(); ++i)
	if (i->Id() == id)
	    _curFont = &*i;
}

void CGleris::Text (int16_t x, int16_t y, const char* s)
{
    if (!_curFont) return;
    const unsigned nChars = strlen(s);
    struct SVertex {
	int16_t	x;
	int16_t	y;
	int16_t	s;
	int16_t	t;
    };
    SVertex* v = (SVertex*) alloca (nChars*4*sizeof(SVertex));
    GLint* first = (GLint*) alloca (nChars*sizeof(GLint));
    GLsizei* count = (GLsizei*) alloca (nChars*sizeof(GLsizei));
    const unsigned fw = _curFont->Width(), fh = _curFont->Height();
    for (unsigned i = 0; i < nChars; ++i, x+=fw) {
	unsigned fy = _curFont->LetterY(s[i]);
	unsigned fx = _curFont->LetterX(s[i]);
	const unsigned fscale = TEXCOORD_ONE/256;

	v[4*i+0].x = x;
	v[4*i+0].y = y;
	v[4*i+1].x = x;
	v[4*i+1].y = y+fh;
	v[4*i+2].x = x+fw;
	v[4*i+2].y = y;
	v[4*i+3].x = x+fw;
	v[4*i+3].y = y+fh;

	v[4*i+0].s = fx*fscale;
	v[4*i+0].t = fy*fscale;
	v[4*i+1].s = fx*fscale;
	v[4*i+1].t = (fy+fh)*fscale;
	v[4*i+2].s = (fx+fw)*fscale;
	v[4*i+2].t = fy*fscale;
	v[4*i+3].s = (fx+fw)*fscale;
	v[4*i+3].t = (fy+fh)*fscale;

	first[i] = i*4;
	count[i] = 4;
    }

    GLuint buf = CreateBuffer();
    BufferData (buf, v, nChars*4*sizeof(SVertex), GL_STATIC_DRAW);

    Shader (_shader[2].Id());
    UniformTexture ("Texture", _curFont->Id());
    Parameter (G::VERTEX, buf, GL_SHORT, 2, 0, sizeof(SVertex));
    Parameter (G::TEXTURE_COORD, buf, GL_SHORT, 2, sizeof(SVertex)/2, sizeof(SVertex));

    glMultiDrawArrays (GL_TRIANGLE_STRIP, first, count, nChars);

    FreeBuffer (buf);
}

//}}}-------------------------------------------------------------------

GLERI_APP (CGleris)

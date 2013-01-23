// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gleris.h"
#include ".o/data/data.h"

//----------------------------------------------------------------------

CGleris::CGleris (void) noexcept
: CApp()
,_fbconfig (nullptr)
,_curCli (nullptr)
,_cli()
,_icbuf (0)
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
	_icbuf.SetFd (STDIN_FILENO, true);
	WatchFd (STDIN_FILENO);
    }
}

//----------------------------------------------------------------------
// X and OpenGL interface

/*static*/ char* CGleris::_xlib_error = nullptr;

/*static*/ int CGleris::XlibErrorHandler (Display* dpy, XErrorEvent* ee) noexcept
{
    char errortext [256];
    XGetErrorText (dpy, ee->error_code, ArrayBlock(errortext));
    if (!_xlib_error)	// Report only the first error
	asprintf (&_xlib_error, "%lu.%hhu.%hhu: %s", ee->serial, ee->request_code, ee->minor_code, errortext);
    return (0);
}

/*static*/ int CGleris::XlibIOErrorHandler (Display*) noexcept
{
    fprintf (stderr, "Error: connection to X server abnormally terminated\n");
    exit (EXIT_FAILURE);
}

void CGleris::CheckForXlibErrors (void) const
{
    XSync (_dpy, False);
    if (_xlib_error)
	throw XError (true, _xlib_error);
}

Window CGleris::CreateWindow (const SWinInfo& winfo) const
{
    XSetWindowAttributes swa;
    swa.colormap = _colormap;
    swa.background_pixmap = None;
    swa.border_pixel = BlackPixel (_dpy, _screen);
    swa.event_mask = StructureNotifyMask| ExposureMask| KeyPressMask| ButtonPressMask| PointerMotionMask;

    Window win = XCreateWindow (_dpy, _rootWindow, winfo.x, winfo.y, winfo.w, winfo.h, 0,
				_visinfo->depth, InputOutput, _visinfo->visual,
				CWBackPixmap| CWBorderPixel| CWColormap| CWEventMask, &swa);
    if (!win)
	throw XError ("failed to create window");
    return (win);
}

inline void CGleris::ActivateClient (CGLClient& rcli) noexcept
{
    if (_curCli == &rcli)
	return;
    _curCli = &rcli;
    glXMakeCurrent (_dpy, rcli.Drawable(), rcli.ContextId());
}

void CGleris::Init (argc_t argc, argv_t argv)
{
    CApp::Init (argc, argv);
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
    static const SWinInfo rootinfo = { 0, 0, 1, 1, 0x33, 0x43, SWinInfo::wt_Normal, SWinInfo::wf_Hidden };
    Window rctxw = CreateWindow (rootinfo);	// Temporary window to create the root gl context
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
    XDestroyWindow (_dpy, rctxw);

    CreateClient (-1, 0, rootinfo);

    // Load shared resources into the root context
    GLuint pak = _curCli->LoadDatapak (ArrayBlock (File_resource));
    _curCli->LoadFont (pak, "ter-d18b.psf");
    _curCli->LoadShader (pak, "sh/flat_v.glsl", "sh/flat_f.glsl");
    _curCli->LoadShader (pak, "sh/image_v.glsl", "sh/image_g.glsl", "sh/image_f.glsl");
    _curCli->LoadShader (pak, "sh/font_v.glsl", "sh/image_g.glsl", "sh/font_f.glsl");
    _curCli->FreeDatapak (pak);
}

void CGleris::OnXEvent (void)
{
    for (XEvent xev; XPending(_dpy);) {
	XNextEvent(_dpy,&xev);

	CGLClient* icli = ClientRecordForWindow (xev.xany.window);
	if (!icli) break;

	if (xev.type == Expose)
	    icli->Draw();
	else if (xev.type == ConfigureNotify) {
	    ActivateClient (*icli);
	    icli->Resize (xev.xconfigure.x, xev.xconfigure.y, xev.xconfigure.width, xev.xconfigure.height);
	} else if (xev.type == KeyPress)
	    icli->Event (xev.xkey.keycode);
	icli->WriteCmds();
    }
}

void CGleris::OnFd (int fd)
{
    CApp::OnFd(fd);
    if (fd == ConnectionNumber(_dpy))
	OnXEvent();
    else if (fd == _icbuf.Fd()) {
	_icbuf.ReadCmds();
	PRGL::Parse (*this, _icbuf);
	for (auto& c : _cli)
	    c->WriteCmds();
    }
}

void CGleris::OnFdError (int fd)
{
    CApp::OnFdError(fd);
    if (fd == ConnectionNumber(_dpy))
	throw XError ("X server connection terminated");
    else if (fd == STDIN_FILENO) {
	for (auto i = _cli.begin(); i < _cli.end(); ++i) {
	    if ((*i)->Matches(fd)) {
		DestroyClient (*i);
		--(i = _cli.erase(i));
	    }
	}
	if (_cli.size() <= 1 && Option (opt_SingleClient))
	    Quit();
    }
}

void CGleris::CreateClient (int fd, iid_t iid, SWinInfo winfo)
{
    // Parse requested GL version, high byte max version, low byte min version
    uint8_t reqver = min(max(winfo.mingl,winfo.maxgl), _glversion);
    int major = reqver>>4, minor = reqver&0xf;
    if (reqver < winfo.mingl)
	throw XError ("X server does not support OpenGL %d.%d", major, minor);
    winfo.mingl = winfo.maxgl = reqver;

    // Create the window
    Window wid = CreateWindow (winfo);
    CheckForXlibErrors();

    // Create the OpenGL context
    int context_attribs[] = {
	GLX_CONTEXT_MAJOR_VERSION_ARB,	major,
	GLX_CONTEXT_MINOR_VERSION_ARB,	minor,
	GLX_CONTEXT_FLAGS_ARB,		GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
	GLX_CONTEXT_PROFILE_MASK_ARB,	GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
	None
    };
    GLXContext ctx = glXCreateContextAttribsARB (_dpy, _fbconfig, _cli.empty() ? nullptr : _cli[0]->ContextId(), True, context_attribs);
    if (!ctx)
	throw XError ("X server does not support OpenGL %d.%d", major, minor);
    if (!glXIsDirect (_dpy, ctx)) {
	glXDestroyContext (_dpy, ctx);
	XDestroyWindow (_dpy, wid);
	throw XError ("direct rendering is not enabled");
    }

    // Create client record
    _cli.push_back (new CGLClient (iid, wid, ctx));

    // Activate the new context and set default parameters
    if (!(winfo.flags & SWinInfo::wf_Hidden))
	XMapWindow (_dpy, wid);
    CGLClient& rcli = *_cli.back();
    rcli.SetFd (fd, _icbuf.CanPassFd());
    ActivateClient (rcli);
    rcli.Init();

    // Process events generated by above
    OnXEvent();
}

void CGleris::DestroyClient (CGLClient*& pc) noexcept
{
    ActivateClient (*pc);
    _curCli = nullptr;
    GLXContext ctx = pc->ContextId();
    Window w = pc->Drawable();
    delete pc;
    pc = nullptr;
    glXMakeCurrent (_dpy, None, nullptr);
    glXDestroyContext (_dpy, ctx);
    XDestroyWindow (_dpy, w);
}

CGLClient* CGleris::ClientRecord (int fd, iid_t iid) noexcept
{
    for (auto& icli : _cli) {
	if (icli->Matches (fd,iid)) {
	    ActivateClient (*icli);
	    return (icli);
	}
    }
    return (nullptr);
}

CGLClient* CGleris::ClientRecordForWindow (Window w) noexcept
{
    for (auto& icli : _cli)
	if (icli->Drawable() == w)
	    return (icli);
    return (nullptr);
}

void CGleris::ClientDraw (CGLClient& cli, bstri& cmdis)
{
    PDraw<bstri>::Parse (cli, cmdis);
    glXSwapBuffers (_dpy, cli.Drawable());
}

void CGleris::ForwardError (PRGLR* pcli, const char* cmdname, const XError& e, int fd, iid_t iid) const noexcept
{
    try {
	PRGLR errbuf (iid);
	if (!pcli) {
	    errbuf.SetFd (fd);
	    pcli = &errbuf;
	}
	size_t bufsz = strlen(cmdname)+2+strlen(e.what())+1;
	char buf [bufsz];
	snprintf (buf, bufsz, "%s: %s", cmdname, e.what());
	pcli->ForwardError (buf);
	pcli->WriteCmds();
    } catch (...) {}
}

GLERI_APP (CGleris)

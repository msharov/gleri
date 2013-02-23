// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gleris.h"
#include ".o/data/data.h"
#include <X11/XF86keysym.h>

//----------------------------------------------------------------------

/*static*/ char* CGleris::_xlib_error = nullptr;
/*static*/ char CGleris::s_SocketPath [c_SocketPathLen];

//----------------------------------------------------------------------

CGleris::CGleris (void) noexcept
: CApp()
,_fbconfig (nullptr)
,_curCli (nullptr)
,_cli()
,_iconn()
,_dpy (nullptr)
,_visinfo (nullptr)
,_colormap (None)
,_screen (None)
,_rootWindow (None)
,_nextiid (0)
,_localSocket()
,_tcpSocket()
,_glversion (0)
,_options (0)
{
    XSetErrorHandler (XlibErrorHandler);
    XSetIOErrorHandler (XlibIOErrorHandler);
    snprintf (ArrayBlock(s_SocketPath), GLERIS_SOCKET, getenv("HOME"));
}

CGleris::~CGleris (void) noexcept
{
    for (auto& c : _cli)
	DestroyClient (c);
    _cli.clear();
    if (_dpy) {
	XSync (_dpy, True);
	XCloseDisplay (_dpy);
    }
    if (_localSocket.IsOpen()) {
	_localSocket.ForceClose();
	unlink (s_SocketPath);
    }
}

void CGleris::OnArgs (argc_t argc, argv_t argv) noexcept
{
    for (;;) {
	switch (getopt(argc, argv, "?st")) {
	    case -1:	return;
	    case 's':	SetOption (opt_SingleClient); break;
	    case 't':	SetOption (opt_TCPSocket); break;
	    default:
		printf (
		    GLERIS_NAME " " GLERI_VERSTRING "\n"
		    "An OpenGL interface service\n\n"
		    "Usage:\t" GLERIS_NAME " [-st]\n\n"
		    "\t-s\tsingle client mode, command socket on stdin\n"
		    "\t-t\tcreate tcp socket on localhost:" PP_STRINGIFY_I(GLERIS_PORT) "+display\n");
		exit (EXIT_SUCCESS);
	}
    }
}

void CGleris::AddConnection (int fd, bool canPassFd)
{
    _iconn.emplace_back (GenIId());
    _iconn.back().SetFd (fd, canPassFd);
    WatchFd (fd);
}

void CGleris::RemoveConnection (int fd) noexcept
{
    for (auto i = _iconn.begin(); i < _iconn.end(); ++i) {
	if (i->Fd() != fd) continue;
	--(i = _iconn.erase(i));
	for (auto j = _cli.begin(); j < _cli.end(); ++j) {
	    if ((*j)->Matches(fd)) {
		DestroyClient (*j);
		--(j = _cli.erase(j));
	    }
	}
    }
    if (_iconn.empty() && Option (opt_SingleClient))
	Quit();
}

CCmdBuf* CGleris::LookupConnection (int fd) noexcept
{
    for (auto i = _iconn.begin(); i < _iconn.end(); ++i)
	if (i->Fd() == fd)
	    return (&*i);
    return (nullptr);
}

//----------------------------------------------------------------------
// X and OpenGL interface

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

/*static*/ void CGleris::Error (const char* m)
{
    throw XError (m);
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
	Error ("could not open X display");
    WatchFd (ConnectionNumber(_dpy));

    int glx_major = 0, glx_minor = 0;
    if (!glXQueryVersion (_dpy, &glx_major, &glx_minor) || (glx_major<<4|glx_minor) < 0x14)
	Error ("X server does not support GLX 1.4");

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
	Error ("no suitable visuals available");
    _fbconfig = fbcs[0];
    XFree (fbcs);

    _visinfo = glXGetVisualFromFBConfig (_dpy, _fbconfig);
    if (!_visinfo)
	Error ("no suitable visuals available");
    _screen = _visinfo->screen;
    _rootWindow = RootWindow(_dpy, _screen);
    //
    // Create global resources
    //
    _colormap = XCreateColormap(_dpy, _rootWindow, _visinfo->visual, AllocNone);

    // Create the root gl context (share root)
    static const SWinInfo rootinfo = { 0, 0, 1, 1, 0x33, 0x43, SWinInfo::wt_Normal, SWinInfo::wf_Hidden };
    Window rctxw = CreateWindow (rootinfo);	// Temporary window to create the root gl context
    GLXContext ctx = glXCreateNewContext (_dpy, _fbconfig, GLX_RGBA_TYPE, nullptr, True);
    if (!ctx)
	Error ("failed to create an OpenGL context");
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

    CreateClient (0, rootinfo);

    // Load shared resources into the root context
    GLuint pak = _curCli->LoadDatapak (ArrayBlock (File_resource));
    _curCli->LoadFont (pak, "ter-d18b.psf");
    _curCli->LoadShader (pak, "sh/flat_v.glsl", "sh/flat_f.glsl");
    _curCli->LoadShader (pak, "sh/image_v.glsl", "sh/image_g.glsl", "sh/image_f.glsl");
    _curCli->LoadShader (pak, "sh/font_v.glsl", "sh/image_g.glsl", "sh/font_f.glsl");
    _curCli->FreeDatapak (pak);

    // Start listening on server sockets
    if (Option (opt_SingleClient))
	AddConnection (STDIN_FILENO, true);
    else {
	_localSocket.Bind (s_SocketPath, GLERIS_LISTEN_QUEUE_SIZE);
	WatchFd (_localSocket.Fd());
	if (Option (opt_TCPSocket)) {
	    _tcpSocket.Bind (INADDR_LOOPBACK, GLERIS_PORT, GLERIS_LISTEN_QUEUE_SIZE);
	    WatchFd (_tcpSocket.Fd());
	}
    }
}

Window CGleris::CreateWindow (const SWinInfo& winfo)
{
    XSetWindowAttributes swa;
    swa.colormap = _colormap;
    swa.background_pixmap = None;
    swa.border_pixel = BlackPixel (_dpy, _screen);
    swa.event_mask =
	StructureNotifyMask| ExposureMask| KeyPressMask| KeyReleaseMask|
	ButtonPressMask| ButtonReleaseMask| PointerMotionMask| KeymapStateMask|
	VisibilityChangeMask| FocusChangeMask| PropertyChangeMask;

    Window win = XCreateWindow (_dpy, _rootWindow, winfo.x, winfo.y, winfo.w, winfo.h, 0,
				_visinfo->depth, InputOutput, _visinfo->visual,
				CWBackPixmap| CWBorderPixel| CWColormap| CWEventMask, &swa);
    if (!win)
	Error ("failed to create window");
    XSync (_dpy, False);
    OnXEvent();
    return (win);
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
	} else if (xev.type == KeyPress || xev.type == KeyRelease)
	    icli->Event (EventFromXKey (xev.xkey));
	else if (xev.type == ButtonPress || xev.type == ButtonRelease)
	    icli->Event (EventFromButton (xev.xbutton));
	else if (xev.type == MotionNotify)
	    icli->Event (EventFromMotion (xev.xmotion));
	else if (xev.type == MappingNotify)
	    XRefreshKeyboardMapping (&xev.xmapping);
    }
    if (_xlib_error)
	throw XError (true, _xlib_error);
}

/*static*/ uint32_t CGleris::ModsFromXState (uint32_t state) noexcept
{
    static const uint8_t c_Modmap[] = {
	ShiftMapIndex,	ModShiftShift,
	ControlMapIndex,ModCtrlShift,
	Mod1MapIndex,	ModAltShift,
	Mod4MapIndex,	ModBannerShift,
	8,		ModLeftShift,
	9,		ModMiddleShift,
	10,		ModRightShift
    };
    uint32_t mods = 0;
    for (unsigned i = 0; i < ArraySize(c_Modmap); i+=2)
	if (state & (1u << c_Modmap[i]))
	    mods |= (1u << c_Modmap[i+1]);
    return (mods);
}

/*static*/ inline CEvent CGleris::EventFromXKey (const XKeyEvent& xev) noexcept
{
    // Lookup keysym and char equivalent
    char keybuf [8];
    KeySym ksym;
    XComposeStatus kmods;
    int bufused = XLookupString (const_cast<XKeyEvent*>(&xev), ArrayBlock(keybuf), &ksym, &kmods);

    // Convert X-specific ranges to unicode
    enum : uint32_t {
	XK_Prefix		= 0xff00,
	XK_Offset		= XK_Prefix - Key::XKBase,
	XF86XK_Prefix		= 0x1008FF00,
	XF86XK_Offset		= XF86XK_Prefix - Key::XFKSBase,
	XK_Unicode_Prefix	= 0x01000000
    };
    if (ksym >= 0xff00 && ksym <= 0xffff)
	ksym -= XK_Offset;
    else if (ksym >= XF86XK_Prefix)
	ksym -= XF86XK_Offset;
    else if (ksym >= XK_Unicode_Prefix)
	ksym -= XK_Unicode_Prefix;

    #include "xkeymap.h"	// Defines c_Keymap and c_Modmap

    // Map KeySyms to CEvent Key enum
    uint32_t ekey = 0;
    for (unsigned i = 0; i < ArraySize(c_Keymap); i+=2)
	if (c_Keymap[i] == ksym)
	    ekey = c_Keymap[i+1];
    if (!ekey && bufused > 0 && (ksym >= ' ' && ksym <= '~')) {
	ekey = keybuf[0];
	if (ekey < ' ')
	    ekey += 'a'-1;
    }

    // Map modifiers to Mod constants
    ekey |= ModsFromXState (xev.state);
    // Remove Shift mod from uppercase letters
    if ((ekey & KMod::Shift) && uint16_t(ekey) >= 'A' && uint16_t(ekey) <= 'Z')
	ekey &= ~KMod::Shift;

    // Return the event
    CEvent e;
    e.key = ekey;
    e.x = xev.x;
    e.y = xev.y;
    e.type = (xev.type == KeyRelease) ? CEvent::KeyUp : CEvent::KeyDown;
    return (e);
}

/*static*/ inline CEvent CGleris::EventFromButton (const XButtonEvent& xev) noexcept
{
    CEvent e;
    e.key = xev.button| ModsFromXState(xev.state);
    e.x = xev.x;
    e.y = xev.y;
    e.type = (xev.type == ButtonRelease) ? CEvent::ButtonUp : CEvent::ButtonDown;
    return (e);
}

/*static*/ inline CEvent CGleris::EventFromMotion (const XMotionEvent& xev) noexcept
{
    CEvent e;
    e.key = ModsFromXState(xev.state);
    e.x = xev.x;
    e.y = xev.y;
    e.type = CEvent::Motion;
    return (e);
}

void CGleris::OnFd (int fd)
{
    CApp::OnFd(fd);
    CCmdBuf* pic;
    if (fd == _localSocket.Fd() || fd == _tcpSocket.Fd()) {
	int cfd = accept4 (fd, nullptr, nullptr, SOCK_NONBLOCK| SOCK_CLOEXEC);
	if (cfd < 0)
	    Error ("accept");
	AddConnection (cfd, fd == _localSocket.Fd());
    } else if ((pic = LookupConnection(fd))) {
	pic->ReadCmds();
	pic->ProcessMessages (*this, PRGL::Parse<CGleris>);
    }
    OnXEvent();
    for (auto c : _cli)
	try { c->WriteCmds(); } catch (...) {}	// fd errors will be caught by poll
}

void CGleris::OnFdError (int fd)
{
    CApp::OnFdError(fd);
    if (fd == ConnectionNumber(_dpy))
	Error ("X server connection terminated");
    else
	RemoveConnection (fd);
    OnXEvent();
}

void CGleris::OnTimer (uint64_t tms)
{
    CApp::OnTimer (tms);
    for (auto c : _cli)
	if (c->NextFrameTime() == tms)
	    WaitForTime (c->DrawPendingFrame (_dpy));
    OnXEvent();
    for (auto c : _cli)
	try { c->WriteCmds(); } catch (...) {}	// fd errors will be caught by poll
}

//----------------------------------------------------------------------
// Client records, selection and forwarding

void CGleris::CreateClient (iid_t iid, SWinInfo winfo, const CCmdBuf* piconn)
{
    // Parse requested GL version, high byte max version, low byte min version
    uint8_t reqver = min(max(winfo.mingl,winfo.maxgl), _glversion);
    int major = reqver>>4, minor = reqver&0xf;
    if (reqver < winfo.mingl)
	throw XError ("X server does not support OpenGL %d.%d", major, minor);
    winfo.mingl = winfo.maxgl = reqver;

    // Create the window
    Window wid = CreateWindow (winfo);

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
	Error ("direct rendering is not enabled");
    }

    // Create client record
    _cli.push_back (new CGLClient (iid, wid, ctx));

    // Activate the new context and set default parameters
    if (!(winfo.flags & SWinInfo::wf_Hidden))
	XMapWindow (_dpy, wid);
    CGLClient& rcli = *_cli.back();
    if (piconn)
	rcli.SetFd (piconn->Fd(), piconn->CanPassFd());
    ActivateClient (rcli);
    rcli.Init();
}

inline void CGleris::ActivateClient (CGLClient& rcli) noexcept
{
    if (_curCli == &rcli)
	return;
    _curCli = &rcli;
    glXMakeCurrent (_dpy, rcli.Drawable(), rcli.ContextId());
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

void CGleris::ClientDraw (CGLClient& cli, bstri cmdis)
{
    WaitForTime (cli.DrawFrameNoWait (cmdis, _dpy));
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
    } catch (...) {}	// fd errors will be caught by poll
}

GLERI_APP (CGleris)

// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gleris.h"
#include <X11/XF86keysym.h>

//----------------------------------------------------------------------

/*static*/ char* CGleris::_xlib_error = nullptr;
/*static*/ char CGleris::s_SocketPath [c_SocketPathLen];

//----------------------------------------------------------------------

CGleris::CGleris (void) noexcept
: CApp()
,_fbconfig (nullptr)
,_curCli (nullptr)
,_win()
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
    DTRACE ("gleris " GLERI_VERSTRING " started\n");
    syslog (LOG_INFO, "gleris " GLERI_VERSTRING " started");
    XSetErrorHandler (XlibErrorHandler);
    XSetIOErrorHandler (XlibIOErrorHandler);
    snprintf (ArrayBlock(s_SocketPath), GLERIS_SOCKET, getenv("HOME"));
}

CGleris::~CGleris (void) noexcept
{
    DTRACE ("gleris " GLERI_VERSTRING " exiting\n");
    if (_localSocket.IsOpen()) {
	DTRACE ("Closing gleris socket\n");
	_localSocket.ForceClose();
	unlink (s_SocketPath);
    }
    for (auto& c : _win)
	DestroyClient (c);
    _win.clear();
    if (_dpy) {
	DTRACE ("Flushing X connection\n");
	XFlush (_dpy);
	// Do not close the connection here to allow OpenGL to cleanup
    }
}

void CGleris::OnArgs (argc_t argc, argv_t argv) noexcept
{
    for (;;) {
	switch (getopt(argc, argv, "?std")) {
	    case -1:	return;
	    case 's':	SetOption (opt_SingleClient); break;
	    case 't':	SetOption (opt_TCPSocket); break;
	#ifndef NDEBUG
	    case 'd':	{ extern bool g_bDebugTrace; g_bDebugTrace = true; } break;
	#endif
	    default:
		printf (
		    GLERIS_NAME " " GLERI_VERSTRING "\n"
		    "An OpenGL interface service\n\n"
		    "Usage:\t" GLERIS_NAME
		#ifndef NDEBUG
		    " [-dst]\n\n"
		    "\t-d\toutput debugging information to stdout\n"
		#else
		    " [-st]\n\n"
		#endif
		    "\t-s\tsingle client mode, command socket on stdin\n"
		    "\t-t\tcreate tcp socket on localhost:" PP_STRINGIFY_I(GLERIS_PORT) "+display\n"
		);
		exit (EXIT_SUCCESS);
	}
    }
}

void CGleris::AddConnection (int fd, bool canPassFd)
{
    DTRACE("Incoming connection on %d, %s pass fds\n", fd, canPassFd ? "can" : "can't");
    _iconn.push_back (new CIConn (GenIId(), fd, canPassFd));
    WatchFd (fd);
}

void CGleris::RemoveConnection (int fd) noexcept
{
    DTRACE("Removing connection on %d\n", fd);
    for (auto i = _iconn.begin(); i < _iconn.end(); ++i) {
	if ((*i)->Fd() != fd) continue;
	for (auto j = _win.begin(); j < _win.end(); ++j) {
	    if ((*j)->Matches(fd)) {
		DestroyClient (*j);
		--(j = _win.erase(j));
	    }
	}
	delete (*i);
	--(i = _iconn.erase(i));
    }
    if (_iconn.empty() && Option (opt_SingleClient)) {
	DTRACE("Last connection terminated in single-client mode; quitting\n");
	Quit();
    }
}

CCmdBuf* CGleris::LookupConnection (int fd) noexcept
{
    for (auto i = _iconn.begin(); i < _iconn.end(); ++i)
	if ((*i)->Fd() == fd)
	    return (*i);
    return (nullptr);
}

//----------------------------------------------------------------------
// X and OpenGL interface

/*static*/ int CGleris::XlibErrorHandler (Display* dpy, XErrorEvent* ee) noexcept
{
    char errortext [256];
    XGetErrorText (dpy, ee->error_code, ArrayBlock(errortext));
    if (!_xlib_error)	// If multiple errors arrive, the first is likely the important one
	asprintf (&_xlib_error, "%lu.%hhu.%hhu: %s", ee->serial, ee->request_code, ee->minor_code, errortext);
    return (0);
}

/*static*/ int CGleris::XlibIOErrorHandler (Display*) noexcept
{
    syslog (LOG_ERR, "X server connection terminated");
    Instance().OnXlibIOError();
    exit (EXIT_FAILURE);
}

void CGleris::Init (argc_t argc, argv_t argv)
{
    CApp::Init (argc, argv, LOG_DAEMON, LOG_CONS);
    OnArgs (argc, argv);
    //
    // Connect to X display and get server information
    //
    _dpy = XOpenDisplay (nullptr);
    if (!_dpy)
	XError::emit ("could not open X display");
    WatchFd (ConnectionNumber(_dpy));

    GetAtoms();

    int glx_major = 0, glx_minor = 0;
    if (!glXQueryVersion (_dpy, &glx_major, &glx_minor) || (glx_major<<4|glx_minor) < 0x14)
	XError::emit ("X server does not support GLX 1.4");
    DTRACE("Opened X server connection. GLX %d.%d available\n", glx_major, glx_minor);

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
	XError::emit ("no suitable visuals available");
    _fbconfig = fbcs[0];
    XFree (fbcs);

    _visinfo = glXGetVisualFromFBConfig (_dpy, _fbconfig);
    if (!_visinfo)
	XError::emit ("no suitable visuals available");
    _screen = _visinfo->screen;
    _rootWindow = RootWindow(_dpy, _screen);
    //
    // Create global resources
    //
    _colormap = XCreateColormap(_dpy, _rootWindow, _visinfo->visual, AllocNone);

    // Create the root gl context (share root)
    static const SWinInfo rootinfo = { 0, 0, 1, 1, 0, 0x33, 0x43, 0, SWinInfo::type_Normal, SWinInfo::state_Hidden, SWinInfo::flag_None };
    Window rctxw = CreateWindow (rootinfo);	// Temporary window to create the root gl context
    GLXContext ctx = glXCreateNewContext (_dpy, _fbconfig, GLX_RGBA_TYPE, nullptr, True);
    if (!ctx)
	XError::emit ("failed to create an OpenGL context");
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
    CIConn::LoadDefaultResources (_curCli);

    // Start listening on server sockets
    if (Option (opt_SingleClient)) {
	DTRACE ("Single client mode. Listening on stdin.\n");
	AddConnection (STDIN_FILENO, true);
    } else {
	if (0 == access (s_SocketPath, F_OK))
	    throw XError ("gleris is already running on socket %s", s_SocketPath);
	DTRACE ("Listening on local socket %s\n", s_SocketPath);
	_localSocket.Bind (s_SocketPath, GLERIS_LISTEN_QUEUE_SIZE);
	WatchFd (_localSocket.Fd());
	if (Option (opt_TCPSocket)) {
	    DTRACE ("Listening on TCP socket localhost:%hu\n", GLERIS_PORT);
	    _tcpSocket.Bind (INADDR_LOOPBACK, GLERIS_PORT, GLERIS_LISTEN_QUEUE_SIZE);
	    WatchFd (_tcpSocket.Fd());
	}
    }
}

void CGleris::GetAtoms (void) noexcept
{
    //{{{ c_AtomNames
    static const char c_AtomNames[] =
	"\0ATOM"
	"\0_NET_WM_STATE"
	"\0_NET_WM_STATE_MODAL"
	"\0_NET_WM_STATE_DEMANDS_ATTENTION"
	"\0_NET_WM_STATE_FOCUSED"
	"\0_NET_WM_STATE_STICKY"
	"\0_NET_WM_STATE_SKIP_TASKBAR"
	"\0_NET_WM_STATE_SKIP_PAGER"
	"\0_NET_WM_STATE_ABOVE"
	"\0_NET_WM_STATE_BELOW"
	"\0_NET_WM_STATE_MAXIMIZED_HORZ"
	"\0_NET_WM_STATE_MAXIMIZED_VERT"
	"\0_NET_WM_STATE_HIDDEN"
	"\0_NET_WM_STATE_FULLSCREEN"
	"\0_NET_WM_STATE_FULLSCREEN_EXCLUSIVE"
	"\0_NET_WM_WINDOW_TYPE"
	"\0_NET_WM_WINDOW_TYPE_NORMAL"
	"\0_NET_WM_WINDOW_TYPE_DESKTOP"
	"\0_NET_WM_WINDOW_TYPE_DOCK"
	"\0_NET_WM_WINDOW_TYPE_DIALOG"
	"\0_NET_WM_WINDOW_TYPE_TOOLBAR"
	"\0_NET_WM_WINDOW_TYPE_UTILITY"
	"\0_NET_WM_WINDOW_TYPE_MENU"
	"\0_NET_WM_WINDOW_TYPE_POPUP_MENU"
	"\0_NET_WM_WINDOW_TYPE_DROPDOWN_MENU"
	"\0_NET_WM_WINDOW_TYPE_COMBO"
	"\0_NET_WM_WINDOW_TYPE_NOTIFICATION"
	"\0_NET_WM_WINDOW_TYPE_TOOLTIP"
	"\0_NET_WM_WINDOW_TYPE_SPLASH"
	"\0_NET_WM_WINDOW_TYPE_DND";
    //}}}
    const char* ana [a_Last], *an = c_AtomNames;
    for (unsigned i = 0, anl = sizeof(c_AtomNames); i < a_Last; ++i)
	ana[i] = an = strnext(an,anl);
    if (!XInternAtoms (_dpy, const_cast<char**>(ana), a_Last, false, _atoms))
	XError::emit ("failed to get server strings");
}

unsigned CGleris::WinStateAtoms (const SWinInfo& winfo, uint32_t a[16]) const noexcept
{
    unsigned ac = 0;
    if (winfo.wstate & SWinInfo::state_MaximizedX)
	a[ac++] = _atoms[a_NET_WM_STATE_MAXIMIZED_HORZ];
    if (winfo.wstate & SWinInfo::state_MaximizedY)
	a[ac++] = _atoms[a_NET_WM_STATE_MAXIMIZED_VERT];
    if (winfo.wstate == SWinInfo::state_Hidden)
	a[ac++] = _atoms[a_NET_WM_STATE_HIDDEN];
    if (winfo.wstate == SWinInfo::state_Fullscreen || winfo.wstate == SWinInfo::state_Gamescreen)
	a[ac++] = _atoms[a_NET_WM_STATE_FULLSCREEN];
    if (winfo.wstate == SWinInfo::state_Gamescreen)
	a[ac++] = _atoms[a_NET_WM_STATE_FULLSCREEN_EXCLUSIVE];
    for (unsigned f = 0; f < 8; ++f)
	if (winfo.flags & (1<<f))
	    a[ac++] = _atoms[a_NET_WM_STATE+1+f];
    return (ac);
}

Window CGleris::CreateWindow (const SWinInfo& winfo)
{
    XSetWindowAttributes swa;
    swa.colormap = _colormap;
    swa.background_pixmap = None;
    swa.backing_store = NotUseful;
    swa.border_pixel = BlackPixel (_dpy, _screen);
    swa.event_mask =
	StructureNotifyMask| ExposureMask| KeyPressMask| KeyReleaseMask|
	ButtonPressMask| ButtonReleaseMask| PointerMotionMask|
	VisibilityChangeMask| FocusChangeMask| PropertyChangeMask;
    swa.save_under = swa.override_redirect = winfo.Decoless();

    Window win = XCreateWindow (_dpy, _rootWindow, winfo.x, winfo.y, winfo.w, winfo.h, 0,
				_visinfo->depth, InputOutput, _visinfo->visual,
				CWBackPixmap| CWBackingStore| CWOverrideRedirect|
				CWSaveUnder| CWBorderPixel| CWColormap| CWEventMask, &swa);
    DTRACE ("Created window %x, %ux%u+%d+%d\n", win, winfo.w,winfo.h, winfo.x,winfo.y);
    if (!win)
	XError::emit ("failed to create window");
    XSync (_dpy, False);
    OnXEvent();
    return (win);
}

/*static*/ void CGleris::DTRACE_EventType (const XEvent& e) noexcept
{
#ifndef NDEBUG
    //{{{ c_EventNames
    static const char c_EventNames[] =
	"Error"
	"\0Reply"
	"\0KeyPress"
	"\0KeyRelease"
	"\0ButtonPress"
	"\0ButtonRelease"
	"\0MotionNotify"
	"\0EnterNotify"
	"\0LeaveNotify"
	"\0FocusIn"
	"\0FocusOut"
	"\0KeymapNotify"
	"\0Expose"
	"\0GraphicsExpose"
	"\0NoExpose"
	"\0VisibilityNotify"
	"\0CreateNotify"
	"\0DestroyNotify"
	"\0UnmapNotify"
	"\0MapNotify"
	"\0MapRequest"
	"\0ReparentNotify"
	"\0ConfigureNotify"
	"\0ConfigureRequest"
	"\0GravityNotify"
	"\0ResizeRequest"
	"\0CirculateNotify"
	"\0CirculateRequest"
	"\0PropertyNotify"
	"\0SelectionClear"
	"\0SelectionRequest"
	"\0SelectionNotify"
	"\0ColormapNotify"
	"\0ClientMessage"
	"\0MappingNotify"
	"\0GenericEvent"
	"\0Invalid";
    //}}}
    const char* en = c_EventNames;
    int t = min(e.type,LASTEvent);
    unsigned enl = sizeof(c_EventNames);
    for (int i = 0; i < t; ++i)
	en = strnext(en,enl);
    DTRACE ("Received X event %s for window %x\n", en, e.xany.window);
#else
    DTRACE ("Received X event type %u, for window %x\n", e.type, e.xany.window);
#endif
}

void CGleris::OnXEvent (void)
{
    for (XEvent xev; XPending(_dpy);) {
	XNextEvent(_dpy,&xev);
	DTRACE_EventType (xev);

	if (xev.type == MappingNotify || xev.type == KeymapNotify) {
	    DTRACE ("Keyboard mapping updated\n");
	    XRefreshKeyboardMapping (&xev.xmapping);
	    continue;
	}

	CGLWindow* icli = ClientRecordForWindow (xev.xany.window);
	if (!icli) {
	    DTRACE ("No window associated with this event\n");
	    continue;
	}

	if (xev.type == Expose)
	    icli->Draw();
	else if (xev.type == ConfigureNotify) {
	    ActivateClient (*icli);
	    icli->Resize (xev.xconfigure.x, xev.xconfigure.y, xev.xconfigure.width, xev.xconfigure.height);
	} else if (xev.type == KeyPress || xev.type == KeyRelease) {
	    DTRACE ("[%x] Receive keypress %u\n", icli->IId(), xev.xkey.keycode);
	    icli->Event (EventFromXKey (xev.xkey));
	} else if (xev.type == ButtonPress || xev.type == ButtonRelease) {
	    DTRACE ("[%x] Receive button press %u at %u:%u for %x\n", icli->IId(), xev.xbutton.button, xev.xbutton.x, xev.xbutton.y);
	    icli->Event (EventFromButton (xev.xbutton));
	} else if (xev.type == MotionNotify) {
	    DTRACE ("[%x] Receive motion event to %u:%u\n", icli->IId(), xev.xmotion.x, xev.xmotion.y);
	    icli->Event (EventFromMotion (xev.xmotion));
	} else if (xev.type == MapNotify) {
	    DTRACE ("[%x] Receive map notification\n", icli->IId());
	    if (icli->WinInfo().Decoless()) {	// override-redirect windows do not automatically get focus
		DTRACE ("[%x] Requesting input focus for window %x\n", icli->IId(), icli->Drawable());
		XSetInputFocus (_dpy, icli->Drawable(), RevertToParent, CurrentTime);
	    }
	} else
	    DTRACE ("[%x] Unhandled event %u\n", icli->IId(), xev.type);
    }
    for (auto c : _win)
	try { c->WriteCmds(); } catch (...) {}	// fd errors will be caught by poll
    if (_xlib_error)
	throw XError (true, _xlib_error);
}

/*static*/ uint32_t CGleris::ModsFromXState (uint32_t state) noexcept
{
    static const uint8_t c_Modmap[] = {
	ShiftMapIndex,	KMod::ShiftShift,
	ControlMapIndex,KMod::CtrlShift,
	Mod1MapIndex,	KMod::AltShift,
	Mod4MapIndex,	KMod::BannerShift,
	8,		KMod::LeftShift,
	9,		KMod::MiddleShift,
	10,		KMod::RightShift
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
	    XError::emit ("accept");
	AddConnection (cfd, fd == _localSocket.Fd());
    } else if ((pic = LookupConnection(fd))) {
	pic->ReadCmds();
	pic->ProcessMessages (*this, PRGL::Parse<CGleris>);
    }
    OnXEvent();
}

void CGleris::OnFdError (int fd)
{
    CApp::OnFdError(fd);
    if (fd == ConnectionNumber(_dpy))
	XError::emit ("X server connection terminated");
    else
	RemoveConnection (fd);
    OnXEvent();
}

void CGleris::OnTimer (uint64_t tms)
{
    CApp::OnTimer (tms);
    for (auto c : _win) {
	if (c->NextFrameTime() == tms) {
	    ActivateClient (*c);
	    WaitForTime (c->DrawPendingFrame (_dpy));
	}
    }
    OnXEvent();
}

//----------------------------------------------------------------------
// Client records, selection and forwarding

void CGleris::CreateClient (iid_t iid, SWinInfo winfo, CCmdBuf* piconn)
{
    // Parse requested GL version, high byte max version, low byte min version
    uint8_t reqver = min(max(winfo.mingl,winfo.maxgl), _glversion);
    int major = reqver>>4, minor = reqver&0xf;
    DTRACE ("Creating client window, opengl version %d.%d\n", major, minor);
    if (reqver < winfo.mingl)
	throw XError ("X server does not support OpenGL %d.%d", major, minor);
    winfo.mingl = 0x33; winfo.maxgl = reqver;

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
    GLXContext ctx = glXCreateContextAttribsARB (_dpy, _fbconfig, _win.empty() ? nullptr : _win[0]->ContextId(), True, context_attribs);
    if (!ctx)
	throw XError ("X server does not support OpenGL %d.%d", major, minor);
    if (!glXIsDirect (_dpy, ctx)) {
	glXDestroyContext (_dpy, ctx);
	XDestroyWindow (_dpy, wid);
	XError::emit ("direct rendering is not enabled");
    }

    // Create client record
    _win.push_back (new CGLWindow (iid, winfo, wid, ctx, static_cast<CIConn*>(piconn)));

    // Activate the new context and set default parameters
    CGLWindow& rcli = *_win.back();
    if (piconn)
	rcli.SetFd (piconn->Fd(), piconn->CanPassFd());
    ActivateClient (rcli);
    if (_win.size() > 1)	// The root client has no state
	rcli.Init();

    // Set additional window attributes and map if not hidden
    if (winfo.Parented()) {
	for (const auto w : _win) {
	    if (w->Matches (rcli.Fd(), winfo.parent)) {
		DTRACE ("[%x] Setting parent window of %x to %x\n", rcli.IId(), wid, w->Drawable());
		XSetTransientForHint (_dpy, wid, w->Drawable());
	    }
	}
    }
    if (winfo.Decoless())	// override-redirect windows do not receive configure events
	rcli.Resize (winfo.x, winfo.y, winfo.w, winfo.h);
    XChangeProperty (_dpy, wid, _atoms[a_NET_WM_WINDOW_TYPE], _atoms[a_ATOM], 32, PropModeReplace,
		     (unsigned char*) &_atoms[a_NET_WM_WINDOW_TYPE+1+winfo.wtype], 1);
    uint32_t wsa [16];
    unsigned nwsa = WinStateAtoms (winfo, wsa);
    if (nwsa)
	XChangeProperty (_dpy, wid, _atoms[a_NET_WM_STATE], _atoms[a_ATOM], 32, PropModeReplace, (unsigned char*) wsa, nwsa);
    if (winfo.wstate != SWinInfo::state_Hidden)
	XMapWindow (_dpy, wid);

    // Check for X errors in all of the above
    XSync (_dpy, False);
    OnXEvent();
}

inline void CGleris::ActivateClient (CGLWindow& rcli) noexcept
{
    if (_curCli == &rcli)
	return;
    DTRACE ("Activate client window %x, context %x\n", rcli.Drawable(), rcli.ContextId());
    if (_curCli)
	_curCli->Deactivate();
    _curCli = &rcli;
    glXMakeCurrent (_dpy, rcli.Drawable(), rcli.ContextId());
}

void CGleris::CloseClient (CGLWindow* pcli) noexcept
{
    auto ic = find (_win.begin(), _win.end(), pcli);
    if (ic < _win.end()) {
	DestroyClient (*ic);
	_win.erase (ic);
    }
}

void CGleris::DestroyClient (CGLWindow*& pc) noexcept
{
    if (_dpy) {
	DTRACE ("Destroying client window %x, context %x\n", pc->Drawable(), pc->ContextId());
	if (_curCli == pc) {
	    glXMakeCurrent (_dpy, None, nullptr);
	    _curCli = nullptr;
	}
	glXDestroyContext (_dpy, pc->ContextId());
	XDestroyWindow (_dpy, pc->Drawable());
    }
    delete pc;
    pc = nullptr;
}

CGLWindow* CGleris::ClientRecord (int fd, iid_t iid) noexcept
{
    for (auto& icli : _win) {
	if (icli->Matches (fd,iid)) {
	    ActivateClient (*icli);
	    return (icli);
	}
    }
    return (nullptr);
}

CGLWindow* CGleris::ClientRecordForWindow (Window w) noexcept
{
    for (auto& icli : _win)
	if (icli->Drawable() == w)
	    return (icli);
    return (nullptr);
}

void CGleris::ClientDraw (CGLWindow& cli, bstri cmdis, iid_t iid)
{
    WaitForTime (cli.DrawFrameNoWait (cmdis, _dpy, iid));
}

GLERI_APP (CGleris)

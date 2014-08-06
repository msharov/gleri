// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gleris.h"
#include <X11/XF86keysym.h>
#include <X11/Xauth.h>

//----------------------------------------------------------------------

/*static*/ char* CGleris::_xlib_error = nullptr;
/*static*/ char CGleris::s_SocketPath [c_SocketPathLen];

//----------------------------------------------------------------------

CGleris::CGleris (void) noexcept
: CApp()
,_curCli (nullptr)
,_win()
,_iconn()
,_dpy (nullptr)
,_rootWindow (None)
,_nextiid (0)
,_localSocket()
,_tcpSocket()
,_glversion (0)
,_options (0)
,_fbconfig()
{
    DTRACE ("gleris " GLERI_VERSTRING " started\n");
    syslog (LOG_INFO, "gleris " GLERI_VERSTRING " started");
    XSetErrorHandler (XlibErrorHandler);
    XSetIOErrorHandler (XlibIOErrorHandler);
    memset (_xauth, 0, sizeof(_xauth));
}

CGleris::~CGleris (void) noexcept
{
    DTRACE ("gleris " GLERI_VERSTRING " exiting\n");
    if (_localSocket.IsOpen()) {
	DTRACE ("Closing gleris socket\n");
	_localSocket.ForceClose();
	unlink (s_SocketPath);
    }
    for (auto w = _win.end(); w-- > _win.begin();)
	DestroyClient (*w);
    _win.clear();
    for (auto& c : _iconn)
	delete c;
    _iconn.clear();
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
    if (fd >= 0)
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
    if (_iconn.size() <= 1 && (Option (opt_SingleClient) || Option (opt_SystemdActivated))) {
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

void CGleris::Authenticate (CCmdBuf& cmdbuf, uint32_t pid, uint32_t screen, const char* hostname, const SDataBlock& argv, const SDataBlock& xauth)
{
    CIConn& pconn = static_cast<CIConn&>(cmdbuf);
    if (!xauth._p || xauth._sz != ArraySize(_xauth) || 0 != memcmp(_xauth, xauth._p, ArraySize(_xauth)))
	XError::emit ("invalid xauth token");
    pconn.SetAuthenticated();
    pconn.SetPid (pid);
    pconn.SetScreen (screen);
    pconn.SetHostname (hostname);
    pconn.SetArgv (argv);
}

void CGleris::ForwardError (const CCmd::SMsgHeader& h, const XError& e, int fd) noexcept
{
    ForwardError (h.Cmdname(), e, fd, h.iid);
    DTRACE ("Failing command (hsz=0x%x,sz=0x%x):\n", h.hsz,h.sz);
    bstri msgstrm (h.Msgstrm());
    DHEXDUMP (msgstrm.ipos()-h.hsz,h.Msgsize());
}

void CGleris::ForwardError (const char* cmdname, const XError& e, int fd, iid_t iid) noexcept
{
    try {
	PRGLR* pcli = ClientRecord (fd, iid);
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
	CloseClient (static_cast<CGLWindow*>(pcli));
    } catch (...) {}	// fd errors will be caught by poll
}

void CGleris::OnExport (const char*, int fd)
{
    PRGLR exbuf (0);
    exbuf.SetFd (fd);
    exbuf.Export ("RGL");
    exbuf.WriteCmds();
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
    // Cache xauth information
    //
    const char* xdispstr = getenv("GLERI_DISPLAY");
    if (!xdispstr) xdispstr = getenv("DISPLAY");
    if (!xdispstr) xdispstr = ":0";
    SXDisplay dinfo;
    ParseXDisplay (xdispstr, dinfo);
    if (XAUTH_DATA_LEN != GetXauthData (dinfo, _xauth))
	DTRACE ("Error: failed to load xauth key for %s:%hu.%hu; defaulting to zero\n", dinfo.host, dinfo.display, dinfo.screen);
    else {
	DTRACE ("Successfully loaded xauth key for %s:%hu.%hu\n", dinfo.host, dinfo.display, dinfo.screen);
	DHEXDUMP (ArrayBlock(_xauth));
    }
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
    //
    // Get fbconfigs and visuals
    //
    static const int fbconfattr[] = {
	GLX_DRAWABLE_TYPE,	GLX_WINDOW_BIT,
	GLX_DOUBLEBUFFER,	True,
	GLX_RED_SIZE,		8,
	GLX_GREEN_SIZE,		8,
	GLX_BLUE_SIZE,		8,
	GLX_DEPTH_SIZE,		16,
	0
    };
    int fbcount;
    GLXFBConfig* fbcs = glXChooseFBConfig (_dpy, DefaultScreen(_dpy), fbconfattr, &fbcount);
    if (!fbcs || !fbcount)
	XError::emit ("no suitable visuals available");
    DTRACE("%d fbconfigs available\n", fbcount);
    for (int i = 0, ss = 0, sn = 0; i < fbcount; ++i) {
	glXGetFBConfigAttrib (_dpy, fbcs[i], GLX_SAMPLES, &sn);
	if (ss <= (int) ArraySize(_fbconfig) && !_fbconfig[ss] && sn >= ((!!ss)<<ss)) {
	    _fbconfig[ss++] = fbcs[i];
	    #ifndef NDEBUG
		int id = 0; glXGetFBConfigAttrib (_dpy, fbcs[i], GLX_VISUAL_ID, &id);
		DTRACE("FBConfig %x: MSAA %d\n", id, sn);
	    #endif
	}
    }
    for (unsigned i = 0; i < ArraySize(_fbconfig); ++i)	// Check for unavailable MSAA configs
	if (!_fbconfig[i])				// [0] is always valid at this point, so propagate
	    _fbconfig[i] = _fbconfig[i-1];
    XFree (fbcs);

    for (unsigned i = 0; i < ArraySize(_visinfo); ++i)
	if (!(_visinfo[i] = glXGetVisualFromFBConfig (_dpy, _fbconfig[i])))
	    XError::emit ("no suitable visuals available");
    _rootWindow = RootWindow(_dpy, _visinfo[0]->screen);
    //
    // Create global resources
    //
    for (unsigned i = 0; i < ArraySize(_colormap); ++i)
	_colormap[i] = XCreateColormap(_dpy, _rootWindow, _visinfo[i]->visual, AllocNone);

    // Create the root gl context (share root)
    static const WinInfo rootinfo (0, 0, 1, 1, 0, 0x33, 0x43, WinInfo::MSAA_OFF, WinInfo::type_Normal, WinInfo::state_Hidden, WinInfo::flag_None);
    Window rctxw = CreateWindow (rootinfo);	// Temporary window to create the root gl context
    GLXContext ctx = glXCreateNewContext (_dpy, _fbconfig[0], GLX_RGBA_TYPE, nullptr, True);
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

    AddConnection (-1, false);			// Dummy connection object for the share root window
    uint32_t mypid = getpid();
    char hostname [HOST_NAME_MAX];
    gethostname (ArrayBlock(hostname));
    Authenticate (*_iconn.back(), mypid, 0, hostname, CCmd::SDataBlock(argv[0],strlen(argv[0])), CCmd::SDataBlock(_xauth,sizeof(_xauth)));
    CreateClient (0, rootinfo, "gleris", _iconn.back());
    _iconn.back()->LoadDefaultResources (_curCli);

    // Set WM properties on the root context's window
    Xutf8SetWMProperties (_dpy, _curCli->Drawable(), GLERIS_NAME, nullptr, const_cast<char**>(argv), argc, nullptr, nullptr, nullptr);
    XChangeProperty (_dpy, _curCli->Drawable(), _atoms[a_WM_CLIENT_MACHINE], _atoms[a_STRING], 8, PropModeReplace, (unsigned char*) hostname, strlen(hostname));
    XChangeProperty (_dpy, _curCli->Drawable(), _atoms[a_NET_WM_PID], _atoms[a_CARDINAL], 32, PropModeReplace, (unsigned char*) &mypid, 1);

    // Start listening on server sockets
    unsigned nSystemdFds;
    if (Option (opt_SingleClient)) {
	DTRACE ("Single client mode. Listening on stdin.\n");
	AddConnection (STDIN_FILENO, true);
    } else if ((nSystemdFds = CFile::SystemdFdsAvailable())) {
	for (unsigned fd = CFile::SD_LISTEN_FDS_START; fd < nSystemdFds + CFile::SD_LISTEN_FDS_START; ++fd)
	    if (!_localSocket.BindSystemdFd (fd, PF_LOCAL) && !_tcpSocket.BindSystemdFd (fd, PF_INET))
		XError::emit ("invalid socket passed in by systemd");
	SetOption (opt_SystemdActivated);
	if (_localSocket.IsOpen())
	    WatchFd (_localSocket.Fd());
	if (_tcpSocket.IsOpen())
	    WatchFd (_tcpSocket.Fd());
    } else {
	const char* sockfmt = GLERIS_XDG_SOCKET;
	const char* sockdir = getenv ("XDG_RUNTIME_DIR");
	if (!sockdir) {
	    sockfmt = GLERIS_SOCKET;
	    sockdir = getenv("HOME");
	}
	snprintf (ArrayBlock(s_SocketPath), sockfmt, sockdir, dinfo.display);
	if (0 == access (s_SocketPath, F_OK))
	    throw XError ("gleris is already running on socket %s", s_SocketPath);
	DTRACE ("Listening on local socket %s\n", s_SocketPath);
	_localSocket.Bind (s_SocketPath, GLERIS_LISTEN_QUEUE_SIZE);
	WatchFd (_localSocket.Fd());
	if (Option (opt_TCPSocket)) {
	    DTRACE ("Listening on TCP socket localhost:%hu\n", GLERIS_PORT+dinfo.display);
	    _tcpSocket.Bind (INADDR_LOOPBACK, GLERIS_PORT+dinfo.display, GLERIS_LISTEN_QUEUE_SIZE);
	    WatchFd (_tcpSocket.Fd());
	}
    }
}

void CGleris::GetAtoms (void) noexcept
{
    //{{{ c_AtomNames
    static const char c_AtomNames[] =
	"\0ATOM"
	"\0STRING"
	"\0CARDINAL"
	"\0WM_CLIENT_MACHINE"
	"\0WM_COMMAND"
	"\0WM_PROTOCOLS"
	"\0WM_DELETE_WINDOW"
	"\0_NET_WM_PING"
	"\0_NET_WM_SYNC_REQUEST"
	"\0_NET_WM_PID"
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

unsigned CGleris::WinStateAtoms (const WinInfo& winfo, uint32_t a[16]) const noexcept
{
    unsigned ac = 0;
    if (winfo.wstate & WinInfo::state_MaximizedX)
	a[ac++] = _atoms[a_NET_WM_STATE_MAXIMIZED_HORZ];
    if (winfo.wstate & WinInfo::state_MaximizedY)
	a[ac++] = _atoms[a_NET_WM_STATE_MAXIMIZED_VERT];
    if (winfo.wstate == WinInfo::state_Hidden)
	a[ac++] = _atoms[a_NET_WM_STATE_HIDDEN];
    if (winfo.wstate == WinInfo::state_Fullscreen || winfo.wstate == WinInfo::state_Gamescreen)
	a[ac++] = _atoms[a_NET_WM_STATE_FULLSCREEN];
    if (winfo.wstate == WinInfo::state_Gamescreen)
	a[ac++] = _atoms[a_NET_WM_STATE_FULLSCREEN_EXCLUSIVE];
    for (unsigned f = 0; f < 8; ++f)
	if (winfo.flags & (1<<f))
	    a[ac++] = _atoms[a_NET_WM_STATE+1+f];
    return (ac);
}

Window CGleris::CreateWindow (const WinInfo& winfo)
{
    XSetWindowAttributes swa;
    swa.colormap = _colormap[winfo.aa];
    swa.background_pixmap = None;
    swa.backing_store = NotUseful;
    swa.border_pixel = BlackPixel (_dpy, _visinfo[winfo.aa]->screen);
    swa.event_mask =
	StructureNotifyMask| ExposureMask| KeyPressMask| KeyReleaseMask|
	ButtonPressMask| ButtonReleaseMask| PointerMotionMask| FocusChangeMask;
    swa.save_under = swa.override_redirect = winfo.IsDecoless();

    Window win = XCreateWindow (_dpy, _rootWindow, winfo.x, winfo.y, winfo.w, winfo.h, 0,
				_visinfo[winfo.aa]->depth, InputOutput, _visinfo[winfo.aa]->visual,
				CWBackPixmap| CWBackingStore| CWOverrideRedirect|
				CWSaveUnder| CWBorderPixel| CWColormap| CWEventMask, &swa);
    DTRACE ("Created window %x, %ux%u+%d+%d\n", win, winfo.w,winfo.h, winfo.x,winfo.y);
    if (!win)
	XError::emit ("failed to create window");
    XSync (_dpy, False);
    if (_xlib_error)
	throw XError (true, _xlib_error);
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
	    try {
		ActivateClient (*icli);
		icli->Resize (xev.xconfigure.x, xev.xconfigure.y, xev.xconfigure.width, xev.xconfigure.height);
	    } catch (XError& e) {
		DTRACE ("[%x] GL error while resizing: %s\n", e.what());
	    }
	} else if (xev.type == KeyPress || xev.type == KeyRelease) {
	    DTRACE ("[%x] Receive keypress %u\n", icli->IId(), xev.xkey.keycode);
	    icli->Event (EventFromXKey (xev.xkey));
	} else if (xev.type == ButtonPress || xev.type == ButtonRelease) {
	    DTRACE ("[%x] Receive button press %x at %d:%d\n", icli->IId(), xev.xbutton.button, xev.xbutton.x, xev.xbutton.y);
	    icli->Event (EventFromButton (xev.xbutton));
	} else if (xev.type == MotionNotify) {
	    DTRACE ("[%x] Receive motion event to %d:%d\n", icli->IId(), xev.xmotion.x, xev.xmotion.y);
	    icli->Event (EventFromMotion (xev.xmotion));
	} else if (xev.type == FocusIn || xev.type == FocusOut) {
	    bool bFocusIn = (xev.type == FocusIn);
	    DTRACE ("[%x] %s focus\n", icli->IId(), bFocusIn ? "Receive" : "Lose");
	    icli->Event ((CEvent){0,bFocusIn,0,CEvent::Focus,0});
	} else if (xev.type == MapNotify) {
	    DTRACE ("[%x] Receive map notification\n", icli->IId());
	    if (icli->Info().IsPopupMenu()) {	// override-redirect windows do not automatically get focus
		DTRACE ("[%x] Requesting input focus for popup menu %x\n", icli->IId(), icli->Drawable());
		XSetInputFocus (_dpy, icli->Drawable(), RevertToParent, CurrentTime);
		if (GrabSuccess != XGrabPointer (_dpy, icli->Drawable(), false,	// false = restrict events to target
						 ButtonPressMask| ButtonReleaseMask| PointerMotionMask,
						 GrabModeAsync, GrabModeAsync, None, None, CurrentTime)) {
		    DTRACE ("[%x] Pointer grab failed\n", icli->IId());
		    icli->Event ((CEvent){0,0,0,CEvent::Close,0});
		}
	    }
	} else if (xev.type == DestroyNotify) {
	    DTRACE ("[%x] Receive destroy notification\n", icli->IId());
	    icli->SetDrawable (None);
	    icli->Event ((CEvent){0,0,0,CEvent::Destroy,0});
	    try { icli->WriteCmds(); } catch (...) {};	// If this fails, the client is already disconnected
	    foreach (auto,c,_win) {
		if (*c == icli) {
		    DestroyClient (*c);
		    --(c = _win.erase(c));
		}
	    }
	} else if (xev.type == ClientMessage) {
	    if ((Atom) xev.xclient.data.l[0] == _atoms[a_WM_DELETE_WINDOW]) {
		DTRACE ("[%x] WM delete window\n", icli->IId());
		icli->Event ((CEvent){0,0,0,CEvent::Close,0});
	    } else if ((Atom) xev.xclient.data.l[0] == _atoms[a_NET_WM_PING]) {
		DTRACE ("[%x] WM ping\n", icli->IId());
		icli->Event ((CEvent){0,0,0,CEvent::Ping,(uint32_t)xev.xclient.data.l[1]});
	    #ifndef NDEBUG
	    } else {
		DTRACE ("[%x] Unknown WM_PROTOCOLS message %s\n", icli->IId(), XGetAtomName(_dpy, xev.xclient.data.l[0]));
	    #endif
	    }
	} else
	    DTRACE ("[%x] Unhandled event %u\n", icli->IId(), xev.type);
    }
    for (auto c : _win)
	try { c->WriteCmds(); } catch (...) {}	// fd errors will be caught by poll
    if (_xlib_error) {
	DTRACE ("Xlib error: %s\n", _xlib_error);
	syslog (LOG_ERR, "Xlib error: %s", _xlib_error);
	_xlib_error = nullptr;
    }
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
    e.key = xev.button| (ModsFromXState(xev.state) & (KMod::Shift| KMod::Ctrl| KMod::Alt));
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
	pic->ProcessMessages<PRGL> (*this);
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
	    try {
		DTRACE ("[%x] Rendering queued frame after vsync\n", c->IId());
		ActivateClient (*c);
		WaitForTime (c->DrawPendingFrame (_dpy));
	    } catch (XError& e) {
		DTRACE ("[%x] Queued frame generated error: %s\n", c->IId(), e.what());
		ForwardError ("Draw", e, c->Fd(), c->IId());
	    }
	    c->ClearPendingFrame();
	}
    }
    OnXEvent();
}

//----------------------------------------------------------------------
// Client records, selection and forwarding

CGLWindow* CGleris::CreateClient (iid_t iid, WinInfo winfo, const char* title, CCmdBuf* piconn)
{
    CIConn* pconn = static_cast<CIConn*>(piconn);
    if (pconn && !pconn->Authenticated())
	XError::emit ("unauthenticated connection can not open windows");
    // Parse requested GL version, high byte max version, low byte min version
    uint8_t reqver = min(max(winfo.mingl,winfo.maxgl), _glversion);
    int major = reqver>>4, minor = reqver&0xf;
    DTRACE ("Creating client window, opengl version %d.%d\n", major, minor);
    if (reqver < winfo.mingl)
	throw XError ("X server does not support OpenGL %d.%d", major, minor);
    winfo.mingl = 0x33; winfo.maxgl = reqver;
    if (winfo.aa > WinInfo::MSAA_MAX)
	winfo.aa = WinInfo::MSAA_MAX;

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
    GLXContext ctx = glXCreateContextAttribsARB (_dpy, _fbconfig[winfo.aa], _win.empty() ? nullptr : _win[0]->ContextId(), True, context_attribs);
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
    if (winfo.IsParented()) {
	for (const auto w : _win) {
	    if (w->Matches (rcli.Fd(), winfo.parent)) {
		DTRACE ("[%x] Setting parent window of %x to %x\n", rcli.IId(), wid, w->Drawable());
		XSetTransientForHint (_dpy, wid, w->Drawable());
	    }
	}
    }
    if (winfo.IsDecoless())	// override-redirect windows do not receive configure events
	rcli.Resize (winfo.x, winfo.y, winfo.w, winfo.h);
    XSizeHints szhints;
    szhints.flags = PBaseSize;
    szhints.base_width = winfo.w;
    szhints.base_height = winfo.h;
    if (winfo.wtype != WinInfo::type_Normal) {	// Normal windows are resizable, the rest are not
	szhints.flags |= PMinSize| PMaxSize;
	szhints.min_width = winfo.w;
	szhints.max_width = winfo.w;
	szhints.min_height = winfo.h;
	szhints.max_height = winfo.h;
    }
    XSetWMNormalHints (_dpy, wid, &szhints);
    if (title)
	XStoreName (_dpy, wid, title);
    if (pconn) {
	if (!pconn->Argv().empty())
	    XChangeProperty (_dpy, wid, _atoms[a_WM_COMMAND], _atoms[a_STRING], 8, PropModeReplace,
			     (const unsigned char*) &pconn->Argv()[0], pconn->Argv().size());
	if (!pconn->Hostname().empty())
	    XChangeProperty (_dpy, wid, _atoms[a_WM_CLIENT_MACHINE], _atoms[a_STRING], 8, PropModeReplace,
			     (const unsigned char*) pconn->Hostname().c_str(), pconn->Hostname().size());
	if (pconn->Pid())
	    XChangeProperty (_dpy, wid, _atoms[a_NET_WM_PID], _atoms[a_CARDINAL], 32, PropModeReplace,
			     (const unsigned char*) pconn->PidPtr(), 1);
    }
    XChangeProperty (_dpy, wid, _atoms[a_WM_PROTOCOLS], _atoms[a_ATOM], 32, PropModeReplace,
		     (const unsigned char*) &_atoms[a_WM_DELETE_WINDOW], 2);
    XChangeProperty (_dpy, wid, _atoms[a_NET_WM_WINDOW_TYPE], _atoms[a_ATOM], 32, PropModeReplace,
		     (const unsigned char*) &_atoms[a_NET_WM_WINDOW_TYPE+1+winfo.wtype], 1);
    uint32_t wsa [16];
    unsigned nwsa = WinStateAtoms (winfo, wsa);
    if (nwsa)
	XChangeProperty (_dpy, wid, _atoms[a_NET_WM_STATE], _atoms[a_ATOM], 32, PropModeReplace, (const unsigned char*) wsa, nwsa);
    if (winfo.wstate != WinInfo::state_Hidden)
	XMapWindow (_dpy, wid);

    // Check for X errors in all of the above
    XSync (_dpy, False);
    if (_xlib_error)
	throw XError (true, _xlib_error);
    return (_curCli);
}

inline void CGleris::ActivateClient (CGLWindow& rcli) noexcept
{
    if (_curCli == &rcli)
	return;
    if (_curCli) {
	DTRACE ("Deactivate client window %x, context %x\n", _curCli->Drawable(), _curCli->ContextId());
	_curCli->Deactivate();
	_curCli = nullptr;
    }
    DTRACE ("Activate client window %x, context %x\n", rcli.Drawable(), rcli.ContextId());
    glXMakeCurrent (_dpy, rcli.Drawable(), rcli.ContextId());
    _curCli = &rcli;
    rcli.Activate();
}

void CGleris::CloseClient (CGLWindow* pcli) noexcept
{
    if (pcli->Drawable() != None) {
	for (auto c = _win.begin(), p = _win.end(); c < _win.end(); ++c) {
	    if (*c == pcli)	// To break dependency loops only
		p = c;		// look for children after parent
	    else if (p < c && (*c)->Info().parent == pcli->IId())
		CloseClient (*c);
	}
	DTRACE ("Destroying client window %x, context %x\n", pcli->Drawable(), pcli->ContextId());
	XDestroyWindow (_dpy, pcli->Drawable());
    }
}

void CGleris::DestroyClient (CGLWindow*& pc) noexcept
{
    if (_dpy) {
	DTRACE ("Erasing client with window %x, context %x\n", pc->Drawable(), pc->ContextId());
	_curCli = nullptr;		// Whenever any window dies, ALL GL contexts become detached
	// Activate root context to delete resources
	// The client window is dead at this point, so can't activate the client context directly.
	ActivateClient (*_win[0]);
	pc->FreeResources();
	glXMakeCurrent (_dpy, None, nullptr);
	glXDestroyContext (_dpy, pc->ContextId());
	CloseClient (pc);
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

void CGleris::ClientDraw (CGLWindow& cli, G::goid_t fbid, bstri cmdis)
{
    if (fbid != G::default_Framebuffer)
	cli.ParseDrawlist (fbid, cmdis);
    else
	WaitForTime (cli.DrawFrameNoWait (cmdis, _dpy));
}

void CGleris::ClientEvent (const CGLWindow& cli, const CEvent& e)
{
    if (e.type == CEvent::Ping) {
	XEvent xev;
	memset (&xev, 0, sizeof(xev));
	xev.xclient.type = ClientMessage;
	xev.xclient.window = _rootWindow;
	xev.xclient.message_type = _atoms[a_WM_PROTOCOLS];
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = _atoms[a_NET_WM_PING];
	xev.xclient.data.l[1] = e.time;
	xev.xclient.data.l[2] = cli.Drawable();
	XSendEvent (_dpy, cli.Drawable(), false, SubstructureRedirectMask| SubstructureNotifyMask, &xev);
    }
}

GLERI_APP (CGleris)

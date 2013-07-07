// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "gleri.h"
#include "gwin.h"

//----------------------------------------------------------------------

class CGleris : public CApp {
			CGleris (void) noexcept;
public:
    enum EOption {
	opt_SingleClient,
	opt_TCPSocket,
	opt_Last
    };
public:
    static CGleris&	Instance (void) noexcept { static CGleris app; return (app); }
    virtual		~CGleris (void) noexcept;
    void		Init (argc_t argc, argv_t argv);
    inline bool		Option (EOption o) const	{ return (_options & (1<<o)); }
private:
    enum { c_SocketPathLen = sizeof(sockaddr_un::sun_path) };
    typedef CGLWindow::iid_t	iid_t;
    typedef PRGL::SWinInfo	SWinInfo;
    typedef const SWinInfo&	rcwininfo_t;
    typedef CCmd::SDataBlock	SDataBlock;
    //{{{ EAtom
    enum EAtom : unsigned {
	a_ATOM,
	a_STRING,
	a_CARDINAL,
	a_WM_CLIENT_MACHINE,
	a_WM_COMMAND,
	a_NET_WM_PID,
	a_NET_WM_STATE,
	a_NET_WM_STATE_MODAL,
	a_NET_WM_STATE_DEMANDS_ATTENTION,
	a_NET_WM_STATE_FOCUSED,
	a_NET_WM_STATE_STICKY,
	a_NET_WM_STATE_SKIP_TASKBAR,
	a_NET_WM_STATE_SKIP_PAGER,
	a_NET_WM_STATE_ABOVE,
	a_NET_WM_STATE_BELOW,
	a_NET_WM_STATE_MAXIMIZED_HORZ,
	a_NET_WM_STATE_MAXIMIZED_VERT,
	a_NET_WM_STATE_HIDDEN,
	a_NET_WM_STATE_FULLSCREEN,
	a_NET_WM_STATE_FULLSCREEN_EXCLUSIVE,
	a_NET_WM_WINDOW_TYPE,
	a_NET_WM_WINDOW_TYPE_NORMAL,
	a_NET_WM_WINDOW_TYPE_DESKTOP,
	a_NET_WM_WINDOW_TYPE_DOCK,
	a_NET_WM_WINDOW_TYPE_DIALOG,
	a_NET_WM_WINDOW_TYPE_TOOLBAR,
	a_NET_WM_WINDOW_TYPE_UTILITY,
	a_NET_WM_WINDOW_TYPE_MENU,
	a_NET_WM_WINDOW_TYPE_POPUP_MENU,
	a_NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
	a_NET_WM_WINDOW_TYPE_COMBO,
	a_NET_WM_WINDOW_TYPE_NOTIFICATION,
	a_NET_WM_WINDOW_TYPE_TOOLTIP,
	a_NET_WM_WINDOW_TYPE_SPLASH,
	a_NET_WM_WINDOW_TYPE_DND,
	a_Last
    };
    enum EWMSTATEAction : uint32_t {
	_NET_WM_STATE_REMOVE,
	_NET_WM_STATE_ADD,
	_NET_WM_STATE_TOGGLE
    };
    enum EWMSTATESource : uint32_t {
	_NET_WM_STATE_SOURCE_UNSPECIFIED,
	_NET_WM_STATE_SOURCE_APPLICATION,
	_NET_WM_STATE_SOURCE_USER
    };
    //}}}
public:
			// Client id translation
    CGLWindow*		ClientRecord (int fd, iid_t iid) noexcept;
    CGLWindow*		ClientRecordForWindow (Window w) noexcept;
    void		Authenticate (CCmdBuf& cmdbuf, uint32_t pid, const char* hostname, const SDataBlock& argv, const SDataBlock& xauth);
    void		CreateClient (iid_t iid, SWinInfo winfo, const char* title = nullptr, CCmdBuf* piconn = nullptr);
    void		CloseClient (CGLWindow* pcli) noexcept;
    void		ClientDraw (CGLWindow& cli, bstri cmdis, iid_t iid);
private:
    inline void		OnArgs (argc_t argc, argv_t argv) noexcept;
    Window		CreateWindow (rcwininfo_t winfo);
    inline void		AddConnection (int fd, bool canPassFd = false);
    void		RemoveConnection (int fd) noexcept;
    inline CCmdBuf*	LookupConnection (int fd) noexcept;
    inline void		ActivateClient (CGLWindow& rcli) noexcept;
    void		DestroyClient (CGLWindow*& pcli) noexcept;
    inline void		SetOption (EOption o)	{ _options |= (1<<o); }
    inline iid_t	GenIId (void)		{ return (++_nextiid); }
    inline void		GetAtoms (void) noexcept;
    unsigned		WinStateAtoms (const SWinInfo& winfo, uint32_t a[16]) const noexcept;
    void		OnXEvent (void);
    static uint32_t	ModsFromXState (uint32_t state) noexcept;
   static inline CEvent	EventFromXKey (const XKeyEvent& xev) noexcept;
   static inline CEvent	EventFromButton (const XButtonEvent& xev) noexcept;
   static inline CEvent	EventFromMotion (const XMotionEvent& xev) noexcept;
    virtual void	OnFd (int fd);
    virtual void	OnFdError (int fd);
    virtual void	OnTimer (uint64_t tms);
    static int		XlibErrorHandler (Display* dpy, XErrorEvent* ee) noexcept;
    static int		XlibIOErrorHandler (Display*) noexcept NORETURN;
    inline void		OnXlibIOError (void) { _dpy = nullptr; }
    static inline void	DTRACE_EventType (const XEvent& e) noexcept;
private:
    GLXFBConfig		_fbconfig;
    CGLWindow*		_curCli;
    vector<CGLWindow*>	_win;
    vector<CIConn*>	_iconn;
    Display*		_dpy;
    XVisualInfo*	_visinfo;
    Colormap		_colormap;
    XID			_screen;
    Window		_rootWindow;
    Atom		_atoms [a_Last];
    iid_t		_nextiid;
    CFile		_localSocket;
    CFile		_tcpSocket;
    uint8_t		_glversion;
    uint8_t		_options;
    char		_xauth [XAUTH_DATA_LEN];
    static char*	_xlib_error;
    static char		s_SocketPath [c_SocketPathLen];
};

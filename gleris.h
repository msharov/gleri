// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "gleri.h"
#include "gcli.h"

//----------------------------------------------------------------------

class CGleris : public CApp {
			CGleris (void) noexcept;
public:
    static CGleris&	Instance (void) noexcept { static CGleris app; return (app); }
    virtual		~CGleris (void) noexcept;
    void		Init (argc_t argc, argv_t argv);
protected:
    enum EOption {
	opt_SingleClient,
	opt_TCPSocket,
	opt_Last
    };
    enum { c_SocketPathLen = sizeof(sockaddr_un::sun_path) };
    typedef CGLClient::iid_t	iid_t;
    typedef PRGL::SWinInfo	SWinInfo;
    typedef const SWinInfo&	rcwininfo_t;
public:
    inline bool		Option (EOption o) const	{ return (_options & (1<<o)); }
			// Client id translation
    CGLClient*		ClientRecord (int fd, iid_t iid) noexcept;
    CGLClient*		ClientRecordForWindow (Window w) noexcept;
    void		CreateClient (iid_t iid, SWinInfo winfo, const CCmdBuf* piconn = nullptr);
    void		CloseClient (CGLClient* pcli) noexcept;
    void		ClientDraw (CGLClient& cli, bstri cmdis);
    void		ForwardError (PRGLR* pcli, const char* cmdname, const XError& e, int fd, iid_t iid) const noexcept;
private:
    inline void		OnArgs (argc_t argc, argv_t argv) noexcept;
    Window		CreateWindow (rcwininfo_t winfo);
    inline void		AddConnection (int fd, bool canPassFd = false);
    void		RemoveConnection (int fd) noexcept;
    inline CCmdBuf*	LookupConnection (int fd) noexcept;
    inline void		ActivateClient (CGLClient& rcli) noexcept;
    void		DestroyClient (CGLClient*& pcli) noexcept;
    inline void		SetOption (EOption o)	{ _options |= (1<<o); }
    inline iid_t	GenIId (void)		{ return (++_nextiid); }
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
    static void		Error (const char* m) NORETURN;
private:
    GLXFBConfig		_fbconfig;
    CGLClient*		_curCli;
    vector<CGLClient*>	_cli;
    vector<CCmdBuf>	_iconn;
    Display*		_dpy;
    XVisualInfo*	_visinfo;
    Colormap		_colormap;
    XID			_screen;
    Window		_rootWindow;
    iid_t		_nextiid;
    CFile		_localSocket;
    CFile		_tcpSocket;
    uint8_t		_glversion;
    uint8_t		_options;
    static char*	_xlib_error;
    static char		s_SocketPath [c_SocketPathLen];
};

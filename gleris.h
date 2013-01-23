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
    static CGleris&	Instance (void) { static CGleris app; return (app); }
    virtual		~CGleris (void) noexcept;
    void		Init (argc_t argc, argv_t argv);
protected:
    enum EOption {
	opt_SingleClient,
	opt_Last
    };
    typedef CGLClient::iid_t	iid_t;
    typedef PRGL::SWinInfo	SWinInfo;
    typedef const SWinInfo&	rcwininfo_t;
public:
    inline bool		Option (EOption o) const	{ return (_options & (1<<o)); }
			// Client id translation
    CGLClient*		ClientRecord (int fd, iid_t iid) noexcept;
    CGLClient*		ClientRecordForWindow (Window w) noexcept;
    void		CreateClient (int fd, iid_t iid, SWinInfo winfo);
    void		ClientDraw (CGLClient& cli, bstri& cmdis);
    void		ForwardError (PRGLR* pcli, const char* cmdname, const XError& e, int fd, iid_t iid) const noexcept;
private:
    inline void		OnArgs (argc_t argc, argv_t argv) noexcept;
    void		CheckForXlibErrors (void) const;
    Window		CreateWindow (rcwininfo_t winfo) const;
    inline void		ActivateClient (CGLClient& rcli) noexcept;
    void		DestroyClient (CGLClient*& pcli) noexcept;
    inline void		SetOption (EOption o)	{ _options |= (1<<o); }
    void		OnXEvent (void);
    virtual void	OnFd (int fd);
    virtual void	OnFdError (int fd);
    static int		XlibErrorHandler (Display* dpy, XErrorEvent* ee) noexcept;
    static int		XlibIOErrorHandler (Display*) noexcept NORETURN;
private:
    GLXFBConfig		_fbconfig;
    CGLClient*		_curCli;
    vector<CGLClient*>	_cli;
    CCmdBuf		_icbuf;
    Display*		_dpy;
    XVisualInfo*	_visinfo;
    Colormap		_colormap;
    XID			_screen;
    Window		_rootWindow;
    uint8_t		_glversion;
    uint8_t		_options;
    static char*	_xlib_error;
};

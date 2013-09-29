// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "app.h"
#include "window.h"

class CGLApp : public CApp {
public:
    virtual			~CGLApp (void) noexcept;
    void			Init (argc_t argc, argv_t argv);
    CWindow*			ClientRecord (int fd, CWindow::wid_t wid);
    static inline CGLApp&	Instance (void)		{ return (static_cast<CGLApp&>(CApp::Instance())); }
    template <typename WC, typename... A>
    inline WC*			CreateWindow (A... a)	{ WC* w = new WC (GenWId(), a...); OpenWindow(w); return (w); }
    inline void			ForwardError (const CCmd::SMsgHeader&, const XError& e, int) const { throw e; }
    inline void			OnExport (const char*, int) {}
    inline void			SendUICommand (const char* cmd)	{ SendUIEvent (CEvent::Command, cmd); }
    inline void			SendUIChange (const char* cmd)	{ SendUIEvent (CEvent::UIChange, cmd); }
protected:
    inline			CGLApp (void);
    virtual void		OnFd (int fd);
    virtual void		OnFdError (int fd);
    virtual void		OnTimer (uint64_t tms);
private:
    inline CWindow::wid_t	GenWId (void)		{ return (++_nextwid); }
    void			ConnectToServer (void);
    static int			LaunchServer (void);
    void			OpenWindow (CWindow* w);
    void			FinishWindowProcessing (void);
    void			SendUIEvent (CEvent::EType et, const char* cmd);
private:
    vector<CWindow*>		_wins;
    CCmdBuf			_srvbuf;
    CFile			_srvsock;
    CWindow::wid_t		_nextwid= 0;
    uint16_t			_screen	= 0;
    char			_xauth [XAUTH_DATA_LEN];
    argc_t			_argc	= 0;
    argv_t			_argv	= nullptr;
};

//----------------------------------------------------------------------

inline CGLApp::CGLApp (void)
: CApp()
,_wins()
,_srvbuf(0)
,_srvsock()
{
    memset (_xauth, 0, sizeof(_xauth));
}

// Here because CApp is needed
inline void CWindow::WaitForTime (uint64_t tms) const
{
    CApp::Instance().WaitForTime (tms);
}
inline uint64_t CWindow::NowMS (void) const noexcept
{
    return (CApp::NowMS());
}

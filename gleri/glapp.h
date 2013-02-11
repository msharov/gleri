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
protected:
    inline			CGLApp (void);
    virtual void		OnFd (int fd);
    virtual void		OnFdError (int fd);
    virtual void		OnTimer (uint64_t tms);
    template <typename WC>
    inline void			CreateWindow (void)	{ OpenWindow (new WC (GenWId())); }
private:
    inline CWindow::wid_t	GenWId (void)		{ return (++_nextwid); }
    void			ConnectToServer (void);
    static int			LaunchServer (void);
    void			OpenWindow (CWindow* w);
private:
    vector<CWindow*>		_wins;
    CCmdBuf			_srvbuf;
    CFile			_srvsock;
    CWindow::wid_t		_nextwid;
};

//----------------------------------------------------------------------

inline CGLApp::CGLApp (void)
: CApp()
,_wins()
,_srvbuf(0)
,_srvsock()
,_nextwid(0)
{
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

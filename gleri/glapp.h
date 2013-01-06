#pragma once
#include "app.h"
#include "window.h"

class CGLApp : public CApp {
public:
    virtual			~CGLApp (void) noexcept;
    void			Init (argc_t argc, argv_t argv);
protected:
    inline			CGLApp (void);
    virtual void		OnFd (int fd);
    virtual void		OnFdError (int fd);
    template <typename WC>
    inline void			CreateWindow (void)	{ OpenWindow (new WC (_srvsock, 44)); }
private:
    int				LaunchServer (void) noexcept;
    void			OpenWindow (CWindow* w);
private:
    vector<CWindow*>		_wins;
    CCmdBuf			_srvbuf;
    int				_srvsock;
    pid_t			_srvpid;
};

//----------------------------------------------------------------------

inline CGLApp::CGLApp (void)
: CApp()
,_wins()
,_srvbuf(-1,0)
,_srvsock(-1)
,_srvpid(0)
{
}

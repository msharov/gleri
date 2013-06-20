// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "rglp.h"
#include "event.h"

class CWindow : protected PRGL {
public:
    typedef iid_t		wid_t;
    typedef uint32_t		key_t;
    typedef PRGL::coord_t	coord_t;
    typedef PRGL::dim_t		dim_t;
    typedef PRGL::color_t	color_t;
    typedef PRGL::SWinInfo	SWinInfo;
    typedef const SWinInfo&	rcwininfo_t;
    enum { NotWaitingForVSync = UINT64_MAX };
public:
    inline explicit	CWindow (wid_t wid) noexcept;
    inline virtual	~CWindow (void)			{ }
    inline virtual void	OnExpose (void)			{ Draw(); }
    inline virtual void	OnInit (void)			{ }
    virtual void	OnTimer (uint64_t tms);
    inline virtual void	OnVSync (void)			{ if (_drawPending) Draw(); }
    inline void		OnRestate (rcwininfo_t wi)	{ _info = wi; OnResize (wi.w, wi.h); }
    inline virtual void	OnResize (dim_t, dim_t)		{ }
    virtual void	OnError (const char* m);
    virtual void	OnEvent (const CEvent& e);
    inline virtual void	Draw (void)			{ }
    inline void		WriteCmds (void)		{ PRGL::WriteCmds(); }
    inline void		SetFd (int fd, bool pfd=false)	{ PRGL::SetFd(fd, pfd); }
    inline bool		Matches (int fd, iid_t iid)const{ return (PRGL::Matches(fd,iid)); }
    inline bool		Matches (int fd) const		{ return (PRGL::Matches(fd)); }
    inline void		PostClose (void)		{ PRGL::Close(); }
protected:
    inline rcwininfo_t	Info (void) const		{ return (_info); }
    void		Close (void);
    inline uint32_t	LastRenderTimeNS (void) const	{ return (_fsync.time); }
    inline uint32_t	RefreshTimeNS (void) const	{ return (_fsync.key); }
    inline virtual void	OnKey (key_t)			{ }
    inline virtual void	OnKeyUp (key_t)			{ }
    inline virtual void	OnButton (key_t, coord_t, coord_t)	{ }
    inline virtual void	OnButtonUp (key_t, coord_t, coord_t)	{ }
    inline virtual void	OnMotion (coord_t, coord_t, key_t)	{ }
    template <typename W>
    inline void		DrawT (const W& w);
    template <typename Drw>
    inline void		OnDraw (Drw&) const		{ }
    inline void		WaitForTime (uint64_t tms)const;// Body in glapp.h (because CApp needed)
    bool		WaitingForVSync (void);
    inline uint64_t	NowMS (void) const noexcept;
private:
    SWinInfo		_info;
    CEvent		_fsync;
    uint64_t		_nextVSync;
    bool		_drawPending;
};

//----------------------------------------------------------------------

inline CWindow::CWindow (wid_t wid) noexcept
: PRGL(wid)
,_fsync()
,_nextVSync (NotWaitingForVSync)
,_drawPending (false)
{
    memset (&_info,0,sizeof(_info));
    _fsync.key = 1000000000/60;
}

template <typename W>
inline void CWindow::DrawT (const W& w)
{
    if (WaitingForVSync())
	return;
    PDraw<bstrs> drws;
    w.OnDraw (drws);
    auto drww = PRGL::Draw (drws.size());
    w.OnDraw (drww);
}

//----------------------------------------------------------------------

#define ONDRAWDECL				\
    virtual void Draw (void);			\
    template <typename Drw>			\
    inline void

#define ONDRAWIMPL(W)				\
    void W::Draw (void) { DrawT (*this); }	\
    template <typename Drw>			\
    inline void W

//----------------------------------------------------------------------

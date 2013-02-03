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
    typedef PRGL::SWinInfo	SWinInfo;
    typedef const SWinInfo&	rcwininfo_t;
public:
    inline explicit	CWindow (wid_t wid) noexcept	: PRGL(wid) { memset (&_info,0,sizeof(_info)); }
    inline rcwininfo_t	Info (void) const		{ return (_info); }
    inline virtual void	OnExpose (void)			{ Draw(); }
    inline virtual void	OnInit (void)			{ }
    inline void		OnRestate (rcwininfo_t wi)	{ _info = wi; OnResize (wi.w, wi.h); }
    inline virtual void	OnResize (uint16_t, uint16_t)	{ }
    virtual void	OnError (const char* m);
    virtual void	OnEvent (const CEvent& e);
    inline virtual void	OnKey (uint32_t)		{ }
    inline virtual void	OnKeyUp (uint32_t)		{ }
    inline virtual void	OnButton (uint32_t, int16_t, int16_t)		{ }
    inline virtual void	OnButtonUp (uint32_t, int16_t, int16_t)		{ }
    inline virtual void	OnMotion (int16_t, int16_t, uint32_t)		{ }
    inline virtual void	Draw (void)			{ }
    template <typename Drw>
    inline void		OnDraw (Drw&) const		{ }
    inline void		WriteCmds (void)		{ PRGL::WriteCmds(); }
    inline void		SetFd (int fd, bool pfd=false)	{ PRGL::SetFd(fd, pfd); }
private:
    SWinInfo		_info;
};

//----------------------------------------------------------------------

#define ONDRAWDECL				\
    virtual void Draw (void);			\
    template <typename Drw>			\
    inline void

#define ONDRAWIMPL(W)				\
    void W::Draw (void) {			\
	const W& w = *this;			\
	PDraw<bstrs> drws;			\
	w.OnDraw (drws);			\
	auto drww = PRGL::Draw (drws.size());	\
	w.OnDraw (drww);			\
    }						\
    template <typename Drw>			\
    inline void W

//----------------------------------------------------------------------

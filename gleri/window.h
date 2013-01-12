// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "rglp.h"

class CWindow : protected PRGL {
public:
    typedef iid_t	wid_t;
public:
    inline explicit	CWindow (wid_t wid)		: PRGL(wid) { }
    inline virtual void	OnExpose (void)			{ Draw(); }
    inline virtual void	OnInit (void)			{ }
    inline virtual void	OnResize (uint16_t, uint16_t)	{ }
    inline virtual void	OnEvent (uint32_t)		{ }
    inline virtual void	OnError (const char* m)		{ throw XError (m); }
    inline virtual void	Draw (void)			{ }
    template <typename Drw>
    inline void		OnDraw (Drw&) const		{ }
    inline void		WriteCmds (void)		{ PRGL::WriteCmds(); }
    inline void		SetFd (int fd, bool pfd=false)	{ PRGL::SetFd(fd, pfd); }
};

//----------------------------------------------------------------------

#define ONDRAWDECL				\
    virtual void Draw (void);			\
    template <typename Drw>			\
    inline void

#define ONDRAWIMPL(W)				\
    void W::Draw (void) {			\
	PDraw<bstrs> drws; OnDraw (drws);	\
	auto drww = PRGL::Draw (drws.size());	\
	OnDraw (drww);				\
    }						\
    template <typename Drw>			\
    inline void W

//----------------------------------------------------------------------

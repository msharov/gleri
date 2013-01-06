#pragma once
#include "rglp.h"

class CWindow : protected PRGL {
public:
    typedef CCmd::iid_t	wid_t;
public:
    inline explicit	CWindow (int fd, wid_t wid)	: PRGL(fd,wid) { }
    inline virtual void	OnExpose (void)			{ Draw(); }
    inline virtual void	OnInit (void)			{ }
    inline virtual void	OnResize (uint16_t, uint16_t)	{ }
    inline virtual void	OnEvent (uint32_t)		{ }
    inline virtual void	Draw (void)			{ }
    template <typename Drw>
    inline void		OnDraw (Drw&) const		{ }
    inline void		WriteCmds (void)		{ PRGL::WriteCmds(); }
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

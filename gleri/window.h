// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "rglp.h"
#include "event.h"

class CWindow : protected PRGL {
public:
    using PRGL::iid_t;
    using PRGL::coord_t;
    using PRGL::dim_t;
    using PRGL::color_t;
    using PRGL::WinInfo;
    typedef uint32_t		key_t;
    typedef const WinInfo&	rcwininfo_t;
    enum { NotWaitingForVSync = UINT64_MAX };
public:
    inline explicit	CWindow (iid_t wid) noexcept;
    inline virtual	~CWindow (void)			{ }
    inline void		Export (const char* ol)		{ PRGL::Export (ol); }
    inline void		Authenticate (uint32_t argc, char* const* argv, const char* hostname, uint32_t pid, uint32_t screen, const void* ad, uint32_t adsz)	{ PRGL::Authenticate(argc,argv,hostname,pid,screen,ad,adsz); }
    inline virtual void	OnExpose (void)			{ Draw(); }
    inline virtual void	OnInit (void)			{ }
    virtual void	OnTimer (uint64_t tms);
    inline virtual void	OnVSync (void)			{ if (_drawPending) Draw(); }
    inline void		OnRestate (rcwininfo_t wi)	{ _info = wi; OnResize (wi.w, wi.h); }
    inline virtual void	OnResize (dim_t, dim_t)		{ }
    virtual void	OnError (const char* m)		{ XError::emit (m); }
    virtual void	OnEvent (const CEvent& e);
    inline void		OnSaveFramebufferData (goid_t id, const char* filename, const SDataBlock& d);
    inline virtual void	OnSaveFramebuffer (goid_t, CFile&)	{ }
    inline virtual void	Draw (void)				{ }
    inline void		OnResourceInfo (goid_t id, uint16_t type, const SDataBlock& d);
    inline virtual void	OnTextureInfo (goid_t, const G::Texture::Header&)	{ }
    inline virtual void	OnFontInfo (goid_t, G::Font::Info&)			{ }
    inline void		WriteCmds (void)		{ if (!_closePending) PRGL::WriteCmds(); }
    inline iid_t	IId (void) const		{ return (PRGL::IId()); }
    inline void		SetFd (int fd, bool pfd=false)	{ PRGL::SetFd(fd, pfd); }
    inline bool		Matches (int fd, iid_t iid)const{ return (PRGL::Matches(fd,iid)); }
    inline bool		Matches (int fd) const		{ return (PRGL::Matches(fd)); }
    void		Close (void);
    inline bool		DestroyPending (void) const	{ return (_destroyPending); }
    inline void		Destroy (void)			{ _destroyPending = true; }
protected:
    inline rcwininfo_t	Info (void) const		{ return (_info); }
    inline uint32_t	LastRenderTimeNS (void) const	{ return (_vsync.time); }
    inline uint32_t	RefreshTimeNS (void) const	{ return (_vsync.key); }
    inline virtual void	OnFocus (bool)				{ }
    inline virtual void	OnVisibility (Visibility::State)	{ }
    inline virtual void	OnKey (key_t)				{ }
    inline virtual void	OnKeyUp (key_t)				{ }
    inline virtual void	OnButton (key_t, coord_t, coord_t)	{ }
    inline virtual void	OnButtonUp (key_t, coord_t, coord_t)	{ }
    inline virtual void	OnMotion (coord_t, coord_t, key_t)	{ }
    inline virtual void	OnCrossing (bool, coord_t, coord_t, key_t) { }
    inline virtual void	OnCommand (const char*)			{ }
    inline virtual void	OnUIChanged (const char*)		{ }
    inline virtual void	OnUIAccepted (const char*)		{ }
    template <typename W>
    inline void		DrawT (const W& w);
    template <typename Drw>
    inline void		OnDraw (Drw&) const		{ }
    inline void		WaitForTime (uint64_t tms)const;// Body in glapp.h (because CApp needed)
    bool		WaitingForVSync (void);
    inline uint64_t	NowMS (void) const noexcept;
private:
    WinInfo		_info;
    CEvent		_vsync;
    uint64_t		_nextVSync;
    bool		_drawPending;
    bool		_closePending;
    bool		_destroyPending;
};

//----------------------------------------------------------------------

inline CWindow::CWindow (iid_t wid) noexcept
: PRGL(wid)
,_vsync()
,_nextVSync (NotWaitingForVSync)
,_drawPending (false)
,_closePending (false)
,_destroyPending (false)
{
    memset (&_info,0,sizeof(_info));
    _vsync.key = 1000000000/60;
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

inline void CWindow::OnSaveFramebufferData (goid_t id, const char* filename, const SDataBlock& d)
{
    CFile f (filename, O_WRONLY| O_CREAT| O_TRUNC| O_CLOEXEC, 0600);
    f.Write (d._p, d._sz);
    OnSaveFramebuffer (id, f);
}

inline void CWindow::OnResourceInfo (goid_t id, uint16_t type, const SDataBlock& d)
{
    bstri is (bstri::const_pointer(d._p), d._sz);
    EResource rtype = EResource(type);
    if (rtype == EResource::FONT) {
	G::Font::Info fi;
	is >> fi;
	OnFontInfo (id, fi);
    } else if (rtype >= EResource::_TEXTURE_FIRST && rtype <= EResource::_TEXTURE_LAST) {
	G::Texture::Header h;
	is >> h;
	OnTextureInfo (id, h);
    }
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

#define DRAWFBDECL(Name)			\
    void Draw##Name (goid_t fbid);		\
    template <typename Drw>			\
    inline void OnDraw##Name (Drw& drw)

#define DRAWFBIMPL(W,Name)			\
    void W::Draw##Name (goid_t fbid) {		\
	PDraw<bstrs> drws;			\
	OnDraw##Name (drws);			\
	auto drww = PRGL::Draw (drws.size(),fbid);\
	OnDraw##Name (drww);			\
    }						\
    template <typename Drw>			\
    inline void W::OnDraw##Name (Drw& drw)

//----------------------------------------------------------------------

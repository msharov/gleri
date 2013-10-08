// This file is part of the GLERI project
//
// Copyright (c) 2013 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "window.h"

class CWidget {
public:
    typedef PRGL::iid_t		iid_t;
    typedef PRGL::draww_t	draww_t;
    typedef PRGL::SWinInfo	SWinInfo;
    typedef PRGL::goid_t	goid_t;
    typedef PRGL::coord_t	coord_t;
    typedef PRGL::dim_t		dim_t;
    typedef PRGL::color_t	color_t;
    typedef PRGL::pfontinfo_t	pfontinfo_t;
    typedef CWindow::key_t	key_t;
    enum EFlags {
	f_Focused
    };
    struct SSize {
	dim_t w,h;
	inline constexpr SSize (void) :w(0),h(0) {}
	inline constexpr SSize (dim_t nw, dim_t nh) : w(nw),h(nh) {}
    };
public:
    inline		CWidget (PRGL* prgl)		:_prgl(prgl),_x(0),_y(0),_w(0),_h(0),_flags(0) {}
    virtual inline	~CWidget (void)			{}
    virtual void	OnEvent (const CEvent& e);
    virtual void	OnResize (dim_t w, dim_t h)	{ _w = w; _h = h; }
    template <typename Drw>
    inline void		OnDraw (Drw&) const		{}
    virtual void	OnFocus (bool b) noexcept	{ SetFlag (f_Focused, b); }
    virtual void	Draw (PDraw<bstrs>& drw) const = 0;
    virtual void	Draw (PDraw<bstro>& drw) const = 0;
    virtual SSize	OnMeasure (void) const = 0;
    inline bool		Flag (EFlags f) const		{ return (_flags & (1<<f)); }
    inline void		SetFlag (EFlags f, bool v=true)	{ if (v) _flags |= (1<<f); else _flags &= ~(1<<f); }
    inline bool		Encloses (coord_t x, coord_t y) const	{ return (dim_t(x-_x) < _w && dim_t(y-_y) < _h); };
			// PRGL forwards
    inline iid_t	IId (void) const		{ return (_prgl->IId()); }
    inline pfontinfo_t	Font (void) const		{ return (_prgl->Font()); }
    inline pfontinfo_t	Font (goid_t id) const		{ return (_prgl->Font(id)); }
    inline void		Close (void)			{ ((CWindow*)_prgl)->Close(); }	// Hacky, but don't want to give direct PRGL access in window
    inline void		Event (const CEvent& e)		{ _prgl->Event(e); }
    inline goid_t	BufferData (const void* data, uint32_t dsz, G::EBufferHint hint = G::STATIC_DRAW, G::EBufferType btype = G::ARRAY_BUFFER)	{ return (_prgl->BufferData(data,dsz,hint,btype)); }
    inline goid_t	BufferData (const char* f, G::EBufferHint hint = G::STATIC_DRAW, G::EBufferType btype = G::ARRAY_BUFFER)			{ return (_prgl->BufferData(f,hint,btype)); }
    inline goid_t	BufferData (goid_t pak, const char* f, G::EBufferHint hint = G::STATIC_DRAW, G::EBufferType btype = G::ARRAY_BUFFER)		{ return (_prgl->BufferData(pak,f,hint,btype)); }
    inline void		BufferSubData (goid_t id, const void* data, uint32_t dsz, uint32_t offset = 0, G::EBufferHint hint = G::STATIC_DRAW, G::EBufferType btype = G::ARRAY_BUFFER)	{ _prgl->BufferSubData(id,data,dsz,offset,hint,btype); }
    inline void		FreeBuffer (goid_t id)				{ _prgl->FreeBuffer(id); }
    inline goid_t	LoadDatapak (const void* d, uint32_t dsz)	{ return (_prgl->LoadDatapak(d,dsz)); }
    inline goid_t	LoadDatapak (const char* f)			{ return (_prgl->LoadDatapak(f)); }
    inline goid_t	LoadDatapak (goid_t pak, const char* f)		{ return (_prgl->LoadDatapak(pak,f)); }
    inline void		FreeDatapak (goid_t id)				{ _prgl->FreeDatapak(id); }
    inline goid_t	LoadTexture (const void* d, uint32_t dsz, G::Pixel::Fmt storeas = G::Pixel::RGBA)	{ return (_prgl->LoadTexture(d,dsz,storeas)); }
    inline goid_t	LoadTexture (const char* f, G::Pixel::Fmt storeas = G::Pixel::RGBA)			{ return (_prgl->LoadTexture(f,storeas)); }
    inline goid_t	LoadTexture (goid_t pak, const char* f, G::Pixel::Fmt storeas = G::Pixel::RGBA)		{ return (_prgl->LoadTexture(pak,f,storeas)); }
    inline void		FreeTexture (goid_t id)				{ _prgl->FreeTexture(id); }
    inline goid_t	LoadFont (const void* d, uint32_t dsz)		{ return (_prgl->LoadFont(d,dsz)); }
    inline goid_t	LoadFont (const char* f)			{ return (_prgl->LoadFont(f)); }
    inline goid_t	LoadFont (goid_t pak, const char* f)		{ return (_prgl->LoadFont(pak,f)); }
    inline void		FreeFont (goid_t id)				{ _prgl->FreeFont(id); }
    inline goid_t	LoadShader (const char* v, const char* tc, const char* te, const char* g, const char* f)	{ return (_prgl->LoadShader(v,tc,te,g,f)); }
    inline goid_t	LoadShader (const char* v, const char* tc, const char* te, const char* f)			{ return (_prgl->LoadShader(v,tc,te,f)); }
    inline goid_t	LoadShader (const char* v, const char* g, const char* f)					{ return (_prgl->LoadShader(v,g,f)); }
    inline goid_t	LoadShader (const char* v, const char* f)							{ return (_prgl->LoadShader(v,f)); }
    inline goid_t	LoadShader (goid_t pak, const char* v, const char* tc, const char* te, const char* g, const char* f)	{ return (_prgl->LoadShader(pak,v,tc,te,g,f)); }
    inline goid_t	LoadShader (goid_t pak, const char* v, const char* tc, const char* te, const char* f)		{ return (_prgl->LoadShader(pak,v,tc,te,f)); }
    inline goid_t	LoadShader (goid_t pak, const char* v, const char* g, const char* f)				{ return (_prgl->LoadShader(pak,v,g,f)); }
    inline goid_t	LoadShader (goid_t pak, const char* v, const char* f)						{ return (_prgl->LoadShader(pak,v,f)); }
    inline void		FreeShader (goid_t id)										{ _prgl->FreeShader(id); }
    template <typename W, typename... Args>
    inline W*		CreateSubWidget (Args... args)	{ return (new W (_prgl, args...)); }
protected:
    inline virtual void	OnKey (key_t)				{ }
    inline virtual void	OnKeyUp (key_t)				{ }
    inline virtual void	OnButton (key_t, coord_t, coord_t)	{ }
    inline virtual void	OnButtonUp (key_t, coord_t, coord_t)	{ }
    inline virtual void	OnMotion (coord_t, coord_t, key_t)	{ }
    inline virtual void	OnCommand (const char*)			{ }
    inline virtual void	OnUIChange (const char*)		{ }
private:
    PRGL*		_prgl;
public:
    coord_t		_x,_y;
    dim_t		_w,_h;
private:
    uint16_t		_flags;
};

//----------------------------------------------------------------------

#define ONWIGDRAWDECL	\
    virtual void	Draw (PDraw<bstrs>& drw) const override;	\
    virtual void	Draw (PDraw<bstro>& drw) const override;	\
    template <typename Drw> inline void

#define ONWIGDRAWIMPL(W)\
    void W::Draw (PDraw<bstrs>& drw) const { OnDraw (drw); }	\
    void W::Draw (PDraw<bstro>& drw) const { OnDraw (drw); }	\
    template <typename Drw> inline void W

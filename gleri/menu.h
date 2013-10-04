// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "glapp.h"
#include "packbox.h"

class CMenuEntry : public CWidget {
public:
    inline		CMenuEntry (PRGL* prgl, const char* text, const char* id, const char* accel = "")
			    : CWidget(prgl),_text(text),_id(id),_accel(accel),_backrect() {}
    ONWIGDRAWDECL	OnDraw (Drw& drw) const;
    virtual SSize	OnMeasure (void) const;
    virtual void	OnResize (dim_t w, dim_t h);
    virtual void	OnKey (key_t key);
    virtual void	OnButtonUp (key_t b, coord_t x, coord_t y);
private:
    const char*		_text;
    const char*		_id;
    const char*		_accel;
    goid_t		_backrect;
};

//----------------------------------------------------------------------

class CPopupMenu : public CWindow {
public:
    static inline CPopupMenu*	Create (iid_t parent, coord_t x, coord_t y, const char* mdef)		{ return (CGLApp::Instance().CreateWindow<CPopupMenu>(parent,x,y,mdef)); }
    inline explicit	CPopupMenu (iid_t wid, iid_t parent, coord_t x, coord_t y, const char* mdef)	: CWindow(wid), _parent(parent),_border(0),_x(x),_y(y),_mdef(mdef),_items(this) {}
    virtual void	OnInit (void);
    virtual void	OnResize (dim_t w, dim_t h);
    ONDRAWDECL		OnDraw (Drw& drw) const;
    virtual void	OnEvent (const CEvent& e);
protected:
    virtual void	OnKey (key_t key);
    virtual void	OnMotion (coord_t x, coord_t y, key_t b);
private:
    iid_t		_parent;
    goid_t		_border;
    coord_t		_x,_y;
    const char*		_mdef;
    CPackbox		_items;
};

//----------------------------------------------------------------------

#define BEGIN_MENU(name)	static const char name[] =
#define MENUITEM(text,accel,id)	"m\0" text "\0" accel "\0" id "\0"
#define END_MENU		;

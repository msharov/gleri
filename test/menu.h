// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "../gleri.h"

class CPopupMenu : public CWindow {
public:
    static inline CPopupMenu*	Create (wid_t parent, coord_t x, coord_t y)	{ return (CGLApp::Instance().CreateWindow<CPopupMenu>(parent,x,y)); }
    inline explicit	CPopupMenu (wid_t wid, wid_t parent, coord_t x, coord_t y)	: CWindow(wid), _parent(parent),_x(x),_y(y) {}
    virtual void	OnInit (void);
    virtual void	OnResize (dim_t w, dim_t h);
    virtual void	OnKey (key_t key);
    virtual void	OnButton (key_t b, coord_t x, coord_t y);
    ONDRAWDECL		OnDraw (Drw& drw) const;
private:
    wid_t		_parent;
    coord_t		_x,_y;
};

// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "../gleri.h"

class CTestWindow : public CWindow {
public:
    explicit		CTestWindow (iid_t wid);
    virtual void	OnInit (void);
    virtual void	OnResize (dim_t w, dim_t h);
    virtual void	OnTimer (uint64_t tms);
    ONDRAWDECL		OnDraw (Drw& drw) const;
protected:
    virtual void	OnKey (key_t key);
    virtual void	OnButton (key_t b, coord_t x, coord_t y);
    virtual void	OnCommand (const char* cmd);
private:
    goid_t		_vbuf;
    goid_t		_cbuf;
    goid_t		_gradShader;
    goid_t		_walk;
    goid_t		_cat;
    goid_t		_smalldepth;
    goid_t		_smallcol;
    goid_t		_smallfb;
    coord_t		_wx;
    coord_t		_wy;
    coord_t		_wsx;
    coord_t		_wsy;
    uint64_t		_wtimer;
    char		_hellomsg [48];
};

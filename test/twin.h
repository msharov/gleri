// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "../gleri.h"

class CTestWindow : public CWindow {
public:
    explicit		CTestWindow (iid_t wid);
    virtual void	OnInit (void) override;
    virtual void	OnResize (dim_t w, dim_t h) override;
    virtual void	OnTimer (uint64_t tms) override;
    ONDRAWDECL		OnDraw (Drw& drw) const;
			DRAWFBDECL(Offscreen);
protected:
    virtual void	OnKey (key_t key) override;
    virtual void	OnButton (key_t b, coord_t x, coord_t y) override;
    virtual void	OnButtonUp (key_t b, coord_t x, coord_t y) override;
    virtual void	OnMotion (coord_t x, coord_t y, key_t b) override;
    virtual void	OnCommand (const char* cmd) override;
    virtual void	OnClipboardData (G::Clipboard ci, G::ClipboardFmt fmt, const char* d) override;
    virtual void	OnClipboardOp (ClipboardOp op, G::Clipboard ci, G::ClipboardFmt fmt) override;
private:
    goid_t		_vbuf;
    goid_t		_cbuf;
    goid_t		_gradShader;
    goid_t		_walk;
    goid_t		_cat;
    goid_t		_smalldepth;
    goid_t		_smallcol;
    goid_t		_smallfb;
    goid_t		_ofscrdepth;
    goid_t		_ofscrcol;
    goid_t		_ofscrfb;
    goid_t		_vwfont;
    goid_t		_selrectbuf;
    coord_t		_wx;
    coord_t		_wy;
    coord_t		_wsx;
    coord_t		_wsy;
    uint64_t		_wtimer;
    char		_hellomsg [48];
    const char*		_screenshot;
    coord_t		_selrectpts [4][2];
};

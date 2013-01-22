// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include <gleri.h>

class CTestWindow : public CWindow {
public:
    inline explicit	CTestWindow (wid_t wid)	: CWindow(wid) { }
    virtual void	OnInit (void);
    virtual void	OnResize (uint16_t w, uint16_t h);
    virtual void	OnEvent (uint32_t key);
    ONDRAWDECL		OnDraw (Drw& drw) const;
private:
    uint32_t		_vbuf;
    uint32_t		_walk;
    uint32_t		_gradShader;
};

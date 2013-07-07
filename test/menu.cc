// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "menu.h"

void CPopupMenu::OnInit (void)
{
    CWindow::OnInit();
    Open ("Menu", (SWinInfo){ _x,_y,100,80,0,0x33,0,0,SWinInfo::type_PopupMenu,SWinInfo::state_Normal,SWinInfo::flag_None });
}

void CPopupMenu::OnResize (dim_t w, dim_t h)
{
    CWindow::OnResize (w,h);
}

void CPopupMenu::OnKey (key_t key)
{
    CWindow::OnKey (key);
    if (key == 'q' || key == Key::Escape)
	Close();
}

void CPopupMenu::OnButton (key_t b, coord_t x, coord_t y)
{
    CWindow::OnButton (b,x,y);
    if (b == 1)
	Close();
}

ONDRAWIMPL(CPopupMenu)::OnDraw (Drw& drw) const
{
    CWindow::OnDraw (drw);
    drw.Clear (RGB(32,32,32));
    drw.Color (128,128,128);
    drw.Text (10, 10, "Entry 1");
    drw.Text (10, 30, "Entry 2");
    drw.Text (10, 50, "Entry 3");
}

// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "menu.h"

//----------------------------------------------------------------------

CMenuEntry::SSize CMenuEntry::OnMeasure (void) const
{
    const auto& f = *Font();
    return SSize (f.Width() + f.Width(_text), f.Height() + f.Height()/2);
}

void CMenuEntry::OnResize (dim_t w, dim_t h)
{
    CWidget::OnResize (w, h);
    coord_t rdata[] = { VGEN_TSRECT (0,0,w,h) };
    if (!_backrect)
	_backrect = BufferData (G::ARRAY_BUFFER, rdata, sizeof(rdata));
    else
	BufferSubData (_backrect, rdata, sizeof(rdata));
}

ONWIGDRAWIMPL(CMenuEntry)::OnDraw (Drw& drw) const
{
    CWidget::OnDraw (drw);
    if (!_backrect)
	return;
    drw.VertexPointer (_backrect);
    if (Flag (f_Focused)) {
	drw.Color (48,48,48);
	drw.TriangleStrip (0, 4);
    }
    drw.Color (140,140,140);
    drw.Text (Font()->Width()/2, Font()->Height()/4, _text);
}

void CMenuEntry::OnKey (key_t key)
{
    CWidget::OnKey (key);
    if (key == KMod::Alt+_accel[0] || (key == Key::Enter && Flag(f_Focused))) {
	CGLApp::Instance().SendUICommand (_id);
	Close();
    }
}

void CMenuEntry::OnButtonUp (key_t b, coord_t x, coord_t y)
{
    CWidget::OnButtonUp (b, x, y);
    if (Flag(f_Focused) && (b == Button::Left || b == Button::Right) && x > 1 && y > 1) {
	CGLApp::Instance().SendUICommand (_id);
	Close();
    }
}

//----------------------------------------------------------------------

void CPopupMenu::OnInit (void)
{
    CWindow::OnInit();

    for (const char *i = _mdef; *i;) {
	auto mtype = i;
	auto sz = UINT_MAX;
	auto mtext = strnext (mtype, sz);
	auto maccel = strnext (mtext, sz);
	auto mid = strnext (maccel, sz);
	i = strnext (mid, sz);
	if (*mtype == 'm')
	    _items.emplace_back<CMenuEntry> (mtext, mid, maccel);
    }
    _items.SetFocus (0);

    CWidget::SSize isz = _items.OnMeasure();
    Open ("Menu", WinInfo(_x,_y,isz.w,isz.h,_parent,0x33,0,WinInfo::MSAA_OFF,WinInfo::type_PopupMenu));
}

void CPopupMenu::OnResize (dim_t w, dim_t h)
{
    CWindow::OnResize (w,h);
    _items.OnResize (w, h);
    coord_t rw = w-1, rh = h-1,
	    borderpts[] = { 0,rh, rw,rh, rw,0,  rw,0, 0,0, 0,rh };
    if (_border)
	BufferSubData (_border, borderpts, sizeof(borderpts));
    else
	_border = BufferData (G::ARRAY_BUFFER, borderpts, sizeof(borderpts));
}

void CPopupMenu::OnEvent (const CEvent& e)
{
    CWindow::OnEvent (e);
    _items.OnEvent (e);
}

void CPopupMenu::OnKey (key_t key)
{
    CWindow::OnKey (key);
    if (key == Key::Up)
	_items.SetFocus (_items.Focus()-1);
    else if (key == Key::Down)
	_items.SetFocus (_items.Focus()+1);
    Draw();
}

void CPopupMenu::OnMotion (coord_t x, coord_t y, key_t b)
{
    CWindow::OnMotion (x, y, b);
    auto t = _items.FindEnclosing (x,y);
    if (t < _items.end() && !t->Flag (CWidget::f_Focused)) {
	_items.SetFocus (t-_items.begin());
	Draw();
    }
}

ONDRAWIMPL(CPopupMenu)::OnDraw (Drw& drw) const
{
    CWindow::OnDraw (drw);
    drw.Clear (RGB(32,32,32));
    _items.Draw (drw);
    drw.VertexPointer (_border);
    drw.Color (RGB(0,0,0));
    drw.LineStrip (0, 3);
    drw.Color (RGB(64,64,64));
    drw.LineStrip (3, 3);
}

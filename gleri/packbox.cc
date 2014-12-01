// This file is part of the GLERI project
//
// Copyright (c) 2013 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "packbox.h"

//----------------------------------------------------------------------

CPackbox::~CPackbox (void)
{
    clear();
}

CPackbox::iterator CPackbox::erase (iterator f, iterator l) noexcept
{
    auto wf = f.base(), wl = l.base();
    for (auto i = wf; i < wl; ++i)
	delete *i;
    return _wigv.erase (f.base(), l.base());
}

ONWIGDRAWIMPL(CPackbox)::OnDraw (Drw& drw) const
{
    for (const auto w : _wigv) {
	drw.Viewport (_x+w->_x, _y+w->_y, w->_w, w->_h);
	w->Draw (drw);
    }
    drw.ResetViewport();
}

CPackbox::SSize CPackbox::OnMeasure (void) const
{
    SSize sz;
    for (auto i : _wigv) {
	SSize is = i->OnMeasure();
	sz.w = max (sz.w, is.w);
	sz.h += is.h;
    }
    return sz;
}

void CPackbox::OnResize (dim_t w, dim_t h)
{
    CWidget::OnResize (w, h);
    coord_t y = 0;
    for (auto i : _wigv) {
	i->_x = 0;
	i->_y = y;
	i->OnResize (w, i->OnMeasure().h);
	y += i->_h;
    }
}

CPackbox::iterator CPackbox::FindEnclosing (coord_t x, coord_t y) noexcept
{
    foreach (iterator, i, *this)
	if (i->Encloses (x, y))
	    return i;
    return end();
}

CPackbox::size_type CPackbox::Focus (void) const noexcept
{
    for (auto i = 0u; i < size(); ++i)
	if (_wigv[i]->Flag (f_Focused))
	    return i;
    return UINT16_MAX;
}

void CPackbox::SetFocus (size_type f) noexcept
{
    auto oldfocus = Focus();
    if (f >= size() || f == oldfocus)
	return;
    if (oldfocus < size())
	_wigv[oldfocus]->OnFocus (false);
    _wigv[f]->OnFocus (true);
}

void CPackbox::OnEvent (const CEvent& e)
{
    CWidget::OnEvent (e);
    for (auto i : _wigv) {
	if ((e.type >= CEvent::ButtonDown && e.type <= CEvent::Motion) && !i->Encloses (e.x, e.y))
	    continue;
	i->OnEvent (e);
    }
}

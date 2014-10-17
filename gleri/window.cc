// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "window.h"
#include "glapp.h"

void CWindow::Close (void)
{
    PRGL::Close();
    PRGL::WriteCmds();
    _closePending = true;	// Prevents further writes
}

void CWindow::OnEvent (const CEvent& e)
{
    switch (e.type) {
	case CEvent::Destroy:		Destroy();			break;
	case CEvent::Close:		Close();			break;
	case CEvent::Ping:		Event (e);			break;
	case CEvent::VSync:		_vsync = e;			break;
	case CEvent::Focus:		OnFocus (e.key);		break;
	case CEvent::Visibility:	OnVisibility (Visibility::State(e.key)); break;
	case CEvent::KeyDown:		OnKey (e.key);			break;
	case CEvent::KeyUp:		OnKeyUp (e.key);		break;
	case CEvent::ButtonDown:	OnButton (e.key, e.x, e.y);	break;
	case CEvent::ButtonUp:		OnButtonUp (e.key, e.x, e.y);	break;
	case CEvent::Motion:		OnMotion (e.x, e.y, e.key);	break;
	case CEvent::Crossing:		OnCrossing (e.time, e.x, e.y, e.key); break;
	case CEvent::Command:		OnCommand (e.CommandName());	break;
	case CEvent::UIChanged:		OnUIChanged (e.CommandName());	break;
	case CEvent::UIAccepted:	OnUIAccepted (e.CommandName());	break;
    }
}

void CWindow::OnTimer (uint64_t tms)
{
    if (tms != _nextVSync)
	return;
    _nextVSync = NotWaitingForVSync;
    OnVSync();
}

bool CWindow::WaitingForVSync (void)
{
    if (_nextVSync != NotWaitingForVSync)
	return (_drawPending = true);
    WaitForTime (_nextVSync = NowMS() + RefreshTimeNS()/1000000 + 1);
    return (_drawPending = false);
}

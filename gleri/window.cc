// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "window.h"

void CWindow::OnEvent (const CEvent& e)
{
    switch (e.type) {
	case CEvent::KeyDown:		OnKey (e.key);			break;
	case CEvent::KeyUp:		OnKeyUp (e.key);		break;
	case CEvent::ButtonDown:	OnButton (e.key, e.x, e.y);	break;
	case CEvent::ButtonUp:		OnButtonUp (e.key, e.x, e.y);	break;
	case CEvent::Motion:		OnMotion (e.x, e.y, e.key);	break;
    }
}

void CWindow::OnError (const char* m)
{
    throw XError (m);
}

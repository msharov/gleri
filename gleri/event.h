// This file is part of the GLERI project
//
// Copyright (c) 2013 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "config.h"

class CEvent {
public:
    uint32_t	key;
    int16_t	x;
    int16_t	y;
    enum EType : uint32_t {
	KeyDown,
	KeyUp,
	ButtonDown,
	ButtonUp,
	Motion,
	FrameSync
    }		type;
    uint32_t	time;
public:
    inline	CEvent (void) noexcept { memset (this, 0, sizeof(*this)); }
};

enum : uint32_t {
    KeyModShift = 24,
    ModShiftShift = KeyModShift,
    ModCtrlShift,
    ModAltShift,
    ModBannerShift,
    ModLeftShift,
    ModMiddleShift,
    ModRightShift,
    ModMask = (UINT32_MAX<<KeyModShift),
    KeyMask = ~ModMask,
    UnicodePrivateRegionStart = 0xe000
};

namespace Key {
    enum : uint32_t {
	Null, Menu, PageUp, Copy, Break,
	Insert, Delete, Pause, Backspace, Tab,
	Enter, Redo, PageDown, Home, Alt,
	Shift, Ctrl, CapsLock, NumLock, ScrollLock,
	SysReq, Banner, Paste, Close, Cut,
	End, Undo, Escape, Right, Left,
	Up, Down,
	Space,
	Other = UnicodePrivateRegionStart,
	Back = Other, Calculator, Center, Documents, Eject,
	Explorer, Favorites, Find, Forward, Help,
	Hibernate, History, LogOff, Mail, Mute,
	New, Open, Options, Play, PowerDown,
	Print, Refresh, Save, ScreenSaver, Spell,
	Stop, VolumeDown, VolumeUp, WWW, WheelButton,
	ZoomIn, ZoomOut,
	F0, F1, F2, F3, F4,
	F5, F6, F7, F8, F9,
	F10, F11, F12, F13, F14,
	F15, F16, F17, F18, F19,
	F20, F21, F22, F23, F24,
	XKBase = Other+0x100,
	XFKSBase = XKBase+0x200
    };
} // namespace Key

namespace Button {
    enum : uint32_t {
	Left, Middle, Right, Wheel, WheelUp,
	WheelDown, Back, Forward, Search
    };
} // namespace Button

namespace KMod {
    enum : uint32_t {
	Shift	= (1<<ModShiftShift),
	Ctrl	= (1<<ModCtrlShift),
	Alt	= (1<<ModAltShift),
	Banner	= (1<<ModBannerShift),
	Left	= (1<<ModLeftShift),
	Middle	= (1<<ModMiddleShift),
	Right	= (1<<ModRightShift)
    };
} // namespace KMod

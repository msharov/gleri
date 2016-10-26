// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "cmd.h"
#include "event.h"

class PRGLR : private CCmdBuf {
public:
    using CCmdBuf::iid_t;
    using WinInfo	= G::WinInfo;
    using goid_t	= G::goid_t;
    using coord_t	= G::coord_t;
    using dim_t		= G::dim_t;
    using color_t	= G::color_t;
    using rcwininfo_t	= const WinInfo&;
    enum : uint32_t { c_ObjectName = vpack4('R','G','L','R') };
private:
    enum class ECmd : cmd_t {
	Restate,
	Draw,
	Event,
	SaveFB,
	SaveFBData,
	ResInfo,
	ClipboardData,
	NCmds
    };
public:
    inline explicit		PRGLR (iid_t iid) noexcept	: CCmdBuf(iid) {}
    inline void			Restate (rcwininfo_t winfo)	{ Cmd(ECmd::Restate,winfo); }
    inline void			Draw (void)			{ Cmd(ECmd::Draw); }
    inline void			Event (const CEvent& e)		{ Cmd(ECmd::Event,e); }
    void			SaveFB (goid_t id, const char* filename, CFile& f);
    template <typename RInfo>
    inline void			ResourceInfo (goid_t id, uint16_t type, const RInfo& ri);
    inline void			ClipboardData (const char* v, G::Clipboard c = G::Clipboard::PRIMARY, G::ClipboardFmt fmt = G::ClipboardFmt::UTF8_STRING);
    inline void			ClipboardEvent (ClipboardOp op, G::Clipboard ci = G::Clipboard::PRIMARY, G::ClipboardFmt fmt = G::ClipboardFmt::UTF8_STRING);
    inline void			ForwardError (const char* m)	{ CCmdBuf::ForwardError(m); }
    inline void			Export (const char* ol)		{ CCmdBuf::Export (ol); }
    inline void			WriteCmds (void)		{ CCmdBuf::WriteCmds(); }
    inline void			SetFd (int fd, bool pfd=false)	{ CCmdBuf::SetFd(fd,pfd); }
				// Reading interface
    template <typename F>
    static inline void		Parse (F& f, const SMsgHeader& h, CCmdBuf& cmdbuf);
    inline iid_t		IId (void) const		{ return CCmdBuf::IId(); }
    inline int			Fd (void) const			{ return CCmdBuf::Fd(); }
    inline bool			Matches (int fd, iid_t iid)const{ return Fd() == fd && IId() == iid; }
    inline bool			Matches (int fd) const		{ return Fd() == fd; }
protected:
    inline bool			CanPassFd (void) const		{ return CCmdBuf::CanPassFd(); }
private:
    template <typename... Arg>
    inline void			Cmd (ECmd cmd, const Arg&... args);
    template <typename... Arg>
    inline void			CmdU (ECmd cmd, size_type unwritten, const Arg&... args);
    bstro			CreateCmd (ECmd cmd, size_type sz, size_type unwritten = 0) noexcept;
    static inline const char*	LookupCmdName (ECmd cmd, size_type& sz) noexcept;
    static ECmd			LookupCmd (const char* name, size_type bleft) noexcept;
private:
    static const char		_cmdNames[];
};

//{{{ Inline bodies ----------------------------------------------------

template <typename... Arg>
void PRGLR::Cmd (ECmd cmd, const Arg&... args)
{
    bstrs ss;
    variadic_arg_size (ss, args...);
    bstro os = CreateCmd (cmd, ss.size());
    variadic_arg_write (os, args...);
}

template <typename... Arg>
void PRGLR::CmdU (ECmd cmd, size_type unwritten, const Arg&... args)
{
    bstrs ss;
    variadic_arg_size (ss, args...);
    bstro os = CreateCmd (cmd, ss.size()+unwritten, unwritten);
    variadic_arg_write (os, args...);
}

template <typename RInfo>
void PRGLR::ResourceInfo (goid_t id, uint16_t type, const RInfo& ri)
{
    bstrs ss;
    ss << ri;
    Cmd (ECmd::ResInfo, id, type, uint16_t(0), ss.size(), ri);
}

void PRGLR::ClipboardData (const char* v, G::Clipboard c, G::ClipboardFmt fmt)
    { Cmd (ECmd::ClipboardData, c, fmt, v); }
void PRGLR::ClipboardEvent (ClipboardOp op, G::Clipboard ci, G::ClipboardFmt fmt)
    { Event (CEvent (CEvent::Clipboard, unsigned(op), unsigned(ci), unsigned(fmt))); }

//}}}-------------------------------------------------------------------
//{{{ Read parser

template <typename F>
void PRGLR::Parse (F& f, const SMsgHeader& h, CCmdBuf& cmdbuf) // static
{
    auto cmdis (h.Msgstrm());
    auto clir = f.ClientRecord (cmdbuf.Fd(), h.iid);
    if (!clir)
	return;
    switch (LookupCmd (h.Cmdname(), h.hsz)) {
	case ECmd::Restate:	{ WinInfo winfo; Args(cmdis,winfo); clir->OnRestate(winfo); } break;
	case ECmd::Draw:	clir->OnExpose(); break;
	case ECmd::Event:	{ CEvent e; Args(cmdis,e); clir->OnEvent(e); } break;
	case ECmd::SaveFB:	{ goid_t id; uint32_t r, fd;
				    Args(cmdis,id,r,fd);
				    CFile of (fd);
				    clir->OnSaveFramebuffer (id, of);
				} break;
	case ECmd::SaveFBData:	{ goid_t id; const char* filename; uint32_t tsz,toff; SDataBlock d;
				    Args(cmdis,id,filename,tsz,toff,d);
				    clir->OnSaveFramebufferData (id,filename,d);
				} break;
	case ECmd::ResInfo:	{ goid_t id; uint16_t type, r1; SDataBlock d;
				    Args(cmdis,id,type,r1,d);
				    clir->OnResourceInfo (id,type,d);
				} break;
	case ECmd::ClipboardData: { G::Clipboard ci; G::ClipboardFmt fmt; const char* v = nullptr;
				    Args(cmdis,ci,fmt,v);
				    clir->OnClipboardData (ci,fmt,v);
				} break;
	default:		XError::emit ("invalid protocol command");
    }
}

//}}}-------------------------------------------------------------------

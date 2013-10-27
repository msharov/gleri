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
    typedef G::WinInfo		WinInfo;
    typedef G::goid_t		goid_t;
    typedef G::coord_t		coord_t;
    typedef G::dim_t		dim_t;
    typedef G::color_t		color_t;
    typedef const WinInfo&	rcwininfo_t;
    enum : uint32_t { c_ObjectName = vpack4('R','G','L','R') };
private:
    enum class ECmd : cmd_t {
	Restate,
	Draw,
	Event,
	NCmds
    };
public:
    inline explicit		PRGLR (iid_t iid) noexcept	: CCmdBuf(iid) {}
    inline void			Restate (rcwininfo_t winfo)	{ Cmd(ECmd::Restate,winfo); }
    inline void			Draw (void)			{ Cmd(ECmd::Draw); }
    inline void			Event (const CEvent& e)		{ Cmd(ECmd::Event,e); }
    inline void			ForwardError (const char* m)	{ CCmdBuf::ForwardError(m); }
    inline void			Export (const char* ol)		{ CCmdBuf::Export (ol); }
    inline void			WriteCmds (void)		{ CCmdBuf::WriteCmds(); }
    inline void			SetFd (int fd, bool pfd=false)	{ CCmdBuf::SetFd(fd,pfd); }
				// Reading interface
    template <typename F>
    static inline void		Parse (F& f, const SMsgHeader& h, CCmdBuf& cmdbuf);
    inline iid_t		IId (void) const		{ return (CCmdBuf::IId()); }
    inline int			Fd (void) const			{ return (CCmdBuf::Fd()); }
    inline bool			Matches (int fd, iid_t iid)const{ return (Fd() == fd && IId() == iid); }
    inline bool			Matches (int fd) const		{ return (Fd() == fd); }
private:
    template <typename... Arg>
    inline void			Cmd (ECmd cmd, const Arg&... args);
    bstro			CreateCmd (ECmd cmd, size_type sz) noexcept;
    static inline const char*	LookupCmdName (ECmd cmd, size_type& sz) noexcept;
    static ECmd			LookupCmd (const char* name, size_type bleft) noexcept;
private:
    static const char		_cmdNames[];
};

//{{{ Inline bodies ----------------------------------------------------

template <typename... Arg>
inline void PRGLR::Cmd (ECmd cmd, const Arg&... args)
{
    bstrs ss;
    variadic_arg_size (ss, args...);
    bstro os = CreateCmd (cmd, ss.size());
    variadic_arg_write (os, args...);
}

//}}}-------------------------------------------------------------------
//{{{ Read parser

template <typename F>
/*static*/ inline void PRGLR::Parse (F& f, const SMsgHeader& h, CCmdBuf& cmdbuf)
{
    bstri cmdis (h.Msgstrm());
    auto clir = f.ClientRecord (cmdbuf.Fd(), h.iid);
    if (!clir)
	return;
    switch (LookupCmd (h.Cmdname(), h.hsz)) {
	case ECmd::Restate:	{ WinInfo winfo; Args(cmdis,winfo); clir->OnRestate(winfo); } break;
	case ECmd::Draw:	clir->OnExpose(); break;
	case ECmd::Event:	{ CEvent e; Args(cmdis,e); clir->OnEvent(e); } break;
	default:		XError::emit ("invalid protocol command");
    }
}

//}}}-------------------------------------------------------------------

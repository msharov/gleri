// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "rglp.h"
#include "event.h"

class PRGLR : private CCmdBuf {
public:
    typedef CCmdBuf::iid_t	iid_t;
    typedef PRGL::SWinInfo	SWinInfo;
    typedef PRGL::goid_t	goid_t;
    typedef PRGL::coord_t	coord_t;
    typedef PRGL::dim_t		dim_t;
    typedef PRGL::color_t	color_t;
    typedef const SWinInfo&	rcwininfo_t;
private:
    enum class ECmd : cmd_t {
	Error,
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
    inline void			ForwardError (const char* m)	{ Cmd(ECmd::Error,m); }
    inline void			WriteCmds (void)		{ CCmdBuf::WriteCmds(); }
    inline void			SetFd (int fd, bool pfd=false)	{ CCmdBuf::SetFd(fd,pfd); }
				// Reading interface
    template <typename F>
    static inline void		Parse (F& f, const SMsgHeader& h, const char* cmdname, CCmdBuf& cmdbuf, bstri& is, bstri cmdis);
    inline bool			Matches (int fd, iid_t iid)const{ return (Fd() == fd && IId() == iid); }
    inline bool			Matches (int fd) const		{ return (Fd() == fd); }
    static inline void		Error (void)			{ CCmdBuf::Error(); }
private:
    template <typename... Arg>
    inline void			Cmd (ECmd cmd, const Arg&... args);
    template <typename... Arg>
    static inline void		Args (bstri& is, Arg&... args);
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

template <typename... Arg>
/*static*/ inline void PRGLR::Args (bstri& is, Arg&... args)
{
    bstrs ss; variadic_arg_size (ss, args...);	// Size of args
    if (is.remaining() < ss.size())		// Have the whole thing?
	Error();				//  sz may be != ss.size if args has a string
    variadic_arg_read (is, args...);		// Read args
}

//}}}-------------------------------------------------------------------
//{{{ Read parser

template <typename F>
/*static*/ inline void PRGLR::Parse (F& f, const SMsgHeader& h, const char* cmdname, CCmdBuf& cmdbuf, bstri&, bstri cmdis)
{
    auto clir = f.ClientRecord (cmdbuf.Fd(), h.iid);
    if (h.objname != RGLObject || !clir)	// Not for me
	Error();
    switch (LookupCmd (cmdname, h.hsz)) {
	case ECmd::Error:	{ const char* m = nullptr; Args(cmdis,m); clir->OnError(m); } break;
	case ECmd::Restate:	{ SWinInfo winfo; Args(cmdis,winfo); clir->OnRestate(winfo); } break;
	case ECmd::Draw:	clir->OnExpose(); break;
	case ECmd::Event:	{ CEvent e; Args(cmdis,e); clir->OnEvent(e); } break;
	default:		Error();
    }
}

//}}}-------------------------------------------------------------------

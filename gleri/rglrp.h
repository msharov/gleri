// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "cmd.h"

class PRGLR : private CCmdBuf {
public:
    typedef CCmdBuf::iid_t	iid_t;
private:
    enum class ECmd : cmd_t {
	Init,
	Resize,
	Draw,
	Event,
	NCmds
    };
public:
    inline explicit		PRGLR (iid_t iid)		: CCmdBuf(iid) {}
    inline void			Init (void)			{ Cmd(ECmd::Init); }
    inline void			Resize (uint16_t w, uint16_t h)	{ Cmd(ECmd::Resize,w,h); }
    inline void			Draw (void)			{ Cmd(ECmd::Draw); }
    inline void			Event (uint32_t key)		{ Cmd(ECmd::Event,key); }
    inline void			WriteCmds (void)		{ CCmdBuf::WriteCmds(); }
    inline void			SetFd (int fd, bool pfd=false)	{ CCmdBuf::SetFd(fd,pfd); }
				// Reading interface
    template <typename F>
    static inline void		Parse (F& f, CCmdBuf& cmdbuf);
    inline bool			Matches (int fd, iid_t iid)	{ return (Fd() == fd && IId() == iid); }
    static inline void		Error (void)			{ CCmdBuf::Error(); }
private:
    template <typename... Arg>
    inline void			Cmd (ECmd cmd, const Arg&... args);
    bstro			CreateCmd (ECmd cmd, size_type sz) noexcept;
    static inline const char*	LookupCmdName (ECmd cmd, size_type& sz) noexcept;
    static ECmd			LookupCmd (const char* name, size_type bleft) noexcept;
private:
    static const char		_cmdNames[];
};

//----------------------------------------------------------------------

template <typename... Arg>
inline void PRGLR::Cmd (ECmd cmd, const Arg&... args)
{
    bstrs ss;
    variadic_arg_size (ss, args...);
    bstro os = CreateCmd (cmd, ss.size());
    variadic_arg_write (os, args...);
}

template <typename F>
/*static*/ inline void PRGLR::Parse (F& f, CCmdBuf& cmdbuf)
{
    size_type sz; iid_t iid; uint16_t fdoffset; uint8_t hsz; uint32_t objn;	// All commands start with these
    const size_type chsz = sizeof(sz)+sizeof(iid)+sizeof(fdoffset)+sizeof(hsz)+sizeof(objn);

    bstri is = cmdbuf.BeginRead();

    while (is.remaining() > chsz) {	// While have commands
	auto ihdr = is.ipos();		// Save header start for return
	is >> sz >> iid >> fdoffset >> hsz >> objn;
	if (is.remaining() < (hsz-=chsz)+sz) {
	    is.iseek (ihdr);		// Restart at header
	    break;
	}
	if (objn != RGLObject)		// Not for me
	    Error();

	bstri cmdis (is.ipos()+hsz, sz);	// Command data stream
	const char* cmdname = (const char*) is.ipos();
	is.skip (hsz+sz);			// Skip to next command

	switch (LookupCmd (cmdname, hsz)) {
	    case ECmd::Init:	f.OnInit(); break;
	    case ECmd::Resize:	{ uint16_t w,h; cmdis >> w >> h; f.OnResize(w,h); } break;
	    case ECmd::Draw:	f.OnExpose(); break;
	    case ECmd::Event:	{ uint32_t key; cmdis >> key; f.OnEvent(key); } break;
	    default: Error();
	}
    }
    cmdbuf.EndRead(is);
}

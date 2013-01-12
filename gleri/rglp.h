// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "cmd.h"
#include "drawp.h"
#include <sys/mman.h>

class PRGL : private CCmdBuf {
public:
    typedef CCmdBuf::iid_t	iid_t;
    typedef PDraw<bstro>	draww_t;
private:
    enum class ECmd : cmd_t {
	Open,
	Draw,
	LoadResource,
	LoadFile,
	FreeResource,
	BufferSubData,
	NCmds,
    };
public:
    inline			PRGL (iid_t iid)		: CCmdBuf(iid),_nextid(0) {}
				// Command writing
    inline void			Open (uint16_t w, uint16_t h, uint32_t glver = 0x33)	{ Cmd(ECmd::Open,w,h,glver); }
    inline draww_t		Draw (size_type sz);
    inline uint32_t		CreateBuffer (void)		{ return (GenId()); }
    inline void			BufferData (uint32_t id, const void* data, uint32_t dsz, G::EBufferHint hint = G::STATIC_DRAW, G::EBufferType btype = G::ARRAY_BUFFER);
    inline void			BufferSubData (uint32_t id, const void* data, uint32_t dsz, uint16_t offset = 0, G::EBufferType btype = G::ARRAY_BUFFER);
    inline void			FreeBuffer (uint32_t id);
    inline uint32_t		CreateTexture (void)		{ return (GenId()); }
    void			LoadTexture (uint32_t id, const char* f);
    inline void			FreeTexture (uint32_t id);
    inline void			WriteCmds (void)		{ CCmdBuf::WriteCmds(); }
    inline void			SetFd (int fd, bool passFd)	{ CCmdBuf::SetFd(fd, passFd); }
				// Buffer reading for serialization
    template <typename F>
    static inline void		Parse (F& f, CCmdBuf& cmdbuf);
    static inline void		Error (void)			{ CCmdBuf::Error(); }
private:
    template <typename... Arg>
    inline void			Cmd (ECmd cmd, const Arg&... args);
    bstro			CreateCmd (ECmd cmd, size_type sz) noexcept;
    inline uint32_t		GenId (void)			{ return (++_nextid); }
    static inline const char*	LookupCmdName (ECmd cmd, size_type& sz) noexcept;
    static ECmd			LookupCmd (const char* name, size_type bleft) noexcept;
				// Generic loader interface
    inline void			LoadResource (uint32_t id, const void* data, uint32_t dsz, G::EResource dtype, G::EBufferHint hint = G::STATIC_DRAW);
    inline void			FreeResource (uint32_t id, G::EResource dtype);
private:
    uint32_t			_nextid;
    static const char		_cmdNames[];
};

template <typename... Arg>
inline void PRGL::Cmd (ECmd cmd, const Arg&... args)
{
    bstrs ss;
    variadic_arg_size (ss, args...);
    bstro os = CreateCmd (cmd, ss.size());
    variadic_arg_write (os, args...);
}

inline PRGL::draww_t PRGL::Draw (size_type sz)
    { bstro os = CreateCmd (ECmd::Draw,sz+sizeof(size_type)); os << sz; return (draww_t(os)); }
inline void PRGL::BufferSubData (uint32_t id, const void* data, uint32_t dsz, uint16_t offset, G::EBufferType btype)
    { Cmd (ECmd::BufferSubData, id, btype, offset, SDataBlock (data, dsz)); }
inline void PRGL::LoadResource (uint32_t id, const void* data, uint32_t dsz, G::EResource dtype, G::EBufferHint hint)
    { Cmd (ECmd::LoadResource, id, dtype, hint, SDataBlock (data, dsz)); }
inline void PRGL::FreeResource (uint32_t id, G::EResource dtype)
    { Cmd (ECmd::FreeResource, id, dtype); }

inline void PRGL::BufferData (uint32_t id, const void* data, uint32_t dsz, G::EBufferHint hint, G::EBufferType btype)
    { LoadResource (id, data, dsz, G::EResource(btype-unsigned(G::ARRAY_BUFFER)), hint); }
inline void PRGL::FreeBuffer (uint32_t id)
    { FreeResource (id, G::EResource::VERTEX_ARRAY); }
inline void PRGL::FreeTexture (uint32_t id)
    { FreeResource (id, G::EResource::TEXTURE); }

template <typename F>
/*static*/ inline void PRGL::Parse (F& f, CCmdBuf& cmdbuf)
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
	auto clir = f.ClientRecord(cmdbuf.Fd(), iid);
	try {
	    bstri cmdis (is.ipos()+hsz, sz);	// Command data stream
	    const char* cmdname = (const char*) is.ipos();
	    is.skip (hsz+sz);			// Skip to next command

	    ECmd cmd = LookupCmd (cmdname, hsz);
	    if (objn != RGLObject || (!clir ^ (cmd == ECmd::Open)))
		Error();

	    switch (cmd) {
		case ECmd::Open: {
		    uint16_t w,h; uint32_t glver;
		    if (cmdis.remaining() < sizeof(w)+sizeof(h)+sizeof(glver)) Error();
		    cmdis >> w >> h >> glver;
		    f.CreateClient (cmdbuf.Fd(), iid, w, h, glver);
		    } break;
		case ECmd::Draw: {
		    size_type dlsz; cmdis >> dlsz;
		    if (cmdis.remaining() < dlsz) Error();
		    f.ClientDraw (*clir,cmdis);
		    } break;
		case ECmd::LoadResource: {
		    uint32_t id, dsz; G::EBufferHint hint; G::EResource dtype;
		    if (cmdis.remaining() < sizeof(id)+sizeof(dtype)+sizeof(hint)+sizeof(dsz)) Error();
		    cmdis >> id >> dtype >> hint >> dsz;
		    if (cmdis.remaining() < dsz) Error();
		    uint32_t sid = clir->LookupId (id);
		    if (sid != UINT32_MAX)
			clir->FreeResource (dtype, sid);
		    sid = clir->LoadResource (dtype, hint, cmdis.ipos(), dsz);
		    clir->MapId (id, sid);
		    } break;
		case ECmd::LoadFile: {
		    uint32_t id, dsz; G::EBufferHint hint; G::EResource dtype;
		    if (cmdis.remaining() < sizeof(id)+sizeof(dtype)+sizeof(hint)+sizeof(dsz)) Error();
		    cmdis >> id >> dtype >> hint >> dsz;
		    bstri dfis = cmdbuf.ReceiveFileOpen (is);
		    if (cmdbuf.ReceiveComplete()) {
			uint32_t sid = clir->LookupId (id);
			if (sid != UINT32_MAX)
			    clir->FreeResource (dtype, sid);
			sid = clir->LoadResource (dtype, hint, dfis.ipos(), dfis.remaining());
			clir->MapId (id, sid);
		    }
		    cmdbuf.ReceiveFileClose();
		    } break;
		case ECmd::FreeResource: {
		    uint32_t id; G::EResource dtype;
		    if (cmdis.remaining() < sizeof(id)+sizeof(dtype)) Error();
		    cmdis >> id >> dtype;
		    clir->FreeResource (dtype, clir->LookupId(id));
		    clir->UnmapId (id);
		    } break;
		case ECmd::BufferSubData: {
		    uint32_t id, dsz; uint16_t offset, btype;
		    if (cmdis.remaining() < sizeof(id)+sizeof(btype)+sizeof(offset)+sizeof(dsz)) Error();
		    cmdis >> id >> btype >> offset >> dsz;
		    if (cmdis.remaining() < dsz) Error();
		    clir->BufferSubData (clir->LookupId(id), cmdis.ipos(), dsz, offset, btype);
		    } break;
		default:
		    break;
	    }
	} catch (XError& e) {
	    f.ForwardError (clir, e, cmdbuf.Fd(), iid);
	}
    }
    cmdbuf.EndRead(is);
}

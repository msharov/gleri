// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "cmd.h"
#include "drawp.h"

class PRGL : private CCmdBuf {
public:
    typedef CCmdBuf::iid_t	iid_t;
    typedef PDraw<bstro>	draww_t;
    struct SWinInfo {
	int16_t		x;
	int16_t		y;
	uint16_t	w;
	uint16_t	h;
	uint8_t		mingl;
	uint8_t		maxgl;
	enum EWinType : uint8_t {
	    wt_Normal,
	    wt_Dialog,
	    wt_Menu,
	    wt_Tray
	}		state;
	enum EWinFlag {
	    wf_Hidden		= 1<<0,
	    wf_MaximizedX	= 1<<1,
	    wf_MaximizedY	= 1<<2,
	    wf_Maximized	= wf_MaximizedX| wf_MaximizedY,
	    wf_Fullscreen	= 1<<3,
	    wf_Gamescreen	= 1<<4
	};
	uint8_t		flags;
    };
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
    inline explicit		PRGL (iid_t iid) noexcept	: CCmdBuf(iid),_nextid(0) {}
				// Command writing
    inline void			WriteCmds (void)		{ CCmdBuf::WriteCmds(); }
    inline void			SetFd (int fd, bool passFd)	{ CCmdBuf::SetFd(fd, passFd); }
				// Commands
    inline void			Open (const SWinInfo& winfo)	{ Cmd(ECmd::Open,winfo); }
    inline void			Open (uint16_t w, uint16_t h, uint8_t glver = 0x33)	{ SWinInfo winfo = { 0,0,w,h,glver,0,SWinInfo::wt_Normal,0 }; Open(winfo); }
    inline draww_t		Draw (size_type sz);
    inline uint32_t		BufferData (const void* data, uint32_t dsz, G::EBufferHint hint = G::STATIC_DRAW, G::EBufferType btype = G::ARRAY_BUFFER);
    inline void			BufferSubData (uint32_t id, const void* data, uint32_t dsz, uint16_t offset = 0, G::EBufferType btype = G::ARRAY_BUFFER);
    inline void			FreeBuffer (uint32_t id);
    uint32_t			LoadTexture (const char* f);
    inline void			FreeTexture (uint32_t id);
    inline uint32_t		LoadShader (const char* v, const char* tc, const char* te, const char* g, const char* f);
    inline uint32_t		LoadShader (const char* v, const char* tc, const char* te, const char* f);
    inline uint32_t		LoadShader (const char* v, const char* g, const char* f);
    inline uint32_t		LoadShader (const char* v, const char* f);
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
    inline uint32_t		LoadResource (G::EResource dtype, const void* data, uint32_t dsz, G::EBufferHint hint = G::STATIC_DRAW);
    inline void			FreeResource (uint32_t id, G::EResource dtype);
private:
    uint32_t			_nextid;
    static const char		_cmdNames[];
};

//{{{ Inline bodies ----------------------------------------------------

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
inline uint32_t PRGL::LoadResource (G::EResource dtype, const void* data, uint32_t dsz, G::EBufferHint hint)
    { uint32_t id = GenId(); Cmd (ECmd::LoadResource, id, dtype, hint, SDataBlock (data, dsz)); return (id); }
inline void PRGL::FreeResource (uint32_t id, G::EResource dtype)
    { Cmd (ECmd::FreeResource, id, dtype); }

inline uint32_t PRGL::BufferData (const void* data, uint32_t dsz, G::EBufferHint hint, G::EBufferType btype)
    { return (LoadResource (G::EResource(btype-unsigned(G::ARRAY_BUFFER)), data, dsz, hint)); }
inline void PRGL::FreeBuffer (uint32_t id)
    { FreeResource (id, G::EResource::VERTEX_ARRAY); }
inline void PRGL::FreeTexture (uint32_t id)
    { FreeResource (id, G::EResource::TEXTURE); }
inline uint32_t PRGL::LoadShader (const char* v, const char* tc, const char* te, const char* g, const char* f)
{
    // Inline LoadResource call here because the compiler knows all about
    // the passed in strings and can crush this code very well
    uint32_t id = GenId();
    G::EResource dtype = G::EResource::SHADER;
    G::EBufferHint hint = G::STATIC_DRAW;
    bstrs ss;
    const uint32_t shdsz = strlen(v)+1+strlen(tc)+1+strlen(te)+1+strlen(g)+1+strlen(f)+1;
    ss << id << dtype << hint << shdsz;
    ss.write_strz (v); ss.write_strz (tc); ss.write_strz (te); ss.write_strz (g); ss.write_strz (f);
    bstro os = CreateCmd (ECmd::LoadResource, ss.size());
    os << id << dtype << hint << shdsz;
    os.write_strz (v); os.write_strz (tc); os.write_strz (te); os.write_strz (g); os.write_strz (f);
    return (id);
}
inline uint32_t PRGL::LoadShader (const char* v, const char* tc, const char* te, const char* f)
    { return (LoadShader (v, tc, te, "", f)); }
inline uint32_t PRGL::LoadShader (const char* v, const char* g, const char* f)
    { return (LoadShader (v, "", "", g, f)); }
inline uint32_t PRGL::LoadShader (const char* v, const char* f)
    { return (LoadShader (v, "", "", "", f)); }

//}}}-------------------------------------------------------------------
//{{{ The read parser

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
	const char* cmdname = "protocol";
	bstri cmdis (is.ipos()+hsz, sz);	// Command data stream
	try {
	    cmdname = (const char*) is.ipos();
	    is.skip (hsz+sz);			// Skip to next command

	    ECmd cmd = LookupCmd (cmdname, hsz);
	    if (objn != RGLObject || (!clir ^ (cmd == ECmd::Open)))
		Error();

	    switch (cmd) {
		case ECmd::Open: {
		    SWinInfo winfo;
		    if (cmdis.remaining() < sizeof(SWinInfo)) Error();
		    cmdis >> winfo;
		    f.CreateClient (iid, winfo, &cmdbuf);
		    } break;
		case ECmd::Draw: {
		    size_type dlsz; cmdis >> dlsz;
		    if (cmdis.remaining() < dlsz) Error();
		    cmdis = bstri(cmdis.ipos(), dlsz);
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
		    Error();
		    break;
	    }
	} catch (XError& e) {
	    f.ForwardError (clir, cmdname, e, cmdbuf.Fd(), iid);
	    #ifndef NDEBUG
	        hsz = 8+Align(hsz,8);
		printf ("Failing command (hsz=0x%x,sz=0x%x):\n", hsz,sz); fflush(stdout);
		hexdump (ihdr, hsz+sz);
		printf ("Error at offset 0x%lx:\n", cmdis.ipos()-(is.ipos()-sz)); fflush(stdout);
		hexdump (cmdis.ipos(), cmdis.remaining());
	    #endif
	}
    }
    cmdbuf.EndRead(is);
}
//}}}-------------------------------------------------------------------

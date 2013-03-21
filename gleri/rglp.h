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
    typedef draww_t::goid_t	goid_t;
    typedef draww_t::coord_t	coord_t;
    typedef draww_t::dim_t	dim_t;
    typedef draww_t::color_t	color_t;
    struct SWinInfo {
	coord_t		x,y;
	dim_t		w,h;
	uint8_t		mingl,maxgl;
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
    enum : uint32_t { RGLObject = RGBA('R','G','L',0) };
    enum class ECmd : cmd_t {
	Open,
	Close,
	Draw,
	LoadResource,
	LoadFile,
	FreeResource,
	BufferSubData,
	NCmds,
    };
public:
    inline explicit		PRGL (iid_t iid) noexcept	: CCmdBuf(iid),_nextid(0) {}
    inline bool			Matches (int fd, iid_t iid)const{ return (Fd() == fd && IId() == iid); }
    inline bool			Matches (int fd) const		{ return (Fd() == fd); }
				// Command writing
    inline void			WriteCmds (void)		{ CCmdBuf::WriteCmds(); }
    inline void			SetFd (int fd, bool passFd)	{ CCmdBuf::SetFd(fd, passFd); }
				// Commands
    inline void			Open (const SWinInfo& winfo)	{ Cmd(ECmd::Open,winfo); }
    inline void			Open (dim_t w, dim_t h, uint8_t glver = 0x33)	{ SWinInfo winfo = { 0,0,w,h,glver,0,SWinInfo::wt_Normal,0 }; Open(winfo); }
    inline void			Close (void)			{ Cmd(ECmd::Close); }
    inline draww_t		Draw (size_type sz);
    inline goid_t		BufferData (const void* data, uint32_t dsz, G::EBufferHint hint = G::STATIC_DRAW, G::EBufferType btype = G::ARRAY_BUFFER);
    inline void			BufferSubData (goid_t id, const void* data, uint32_t dsz, uint32_t offset = 0, G::EBufferType btype = G::ARRAY_BUFFER, G::EBufferHint hint = G::STATIC_DRAW);
    inline void			FreeBuffer (goid_t id);
    goid_t			LoadTexture (const char* f);
    inline void			FreeTexture (goid_t id);
    inline goid_t		LoadShader (const char* v, const char* tc, const char* te, const char* g, const char* f);
    inline goid_t		LoadShader (const char* v, const char* tc, const char* te, const char* f);
    inline goid_t		LoadShader (const char* v, const char* g, const char* f);
    inline goid_t		LoadShader (const char* v, const char* f);
    inline void			FreeShader (goid_t id);
				// Buffer reading for serialization
    template <typename F>
    static inline void		Parse (F& f, const SMsgHeader& h, const char* cmdname, CCmdBuf& cmdbuf, bstri& is, bstri cmdis);
    static inline void		Error (void)			{ CCmdBuf::Error(); }
private:
    template <typename... Arg>
    inline void			Cmd (ECmd cmd, const Arg&... args);
    template <typename... Arg>
    static inline void		Args (bstri& is, Arg&... args);
    bstro			CreateCmd (ECmd cmd, size_type sz) noexcept;
    inline goid_t		GenId (void)			{ return (++_nextid); }
    static inline const char*	LookupCmdName (ECmd cmd, size_type& sz) noexcept;
    static ECmd			LookupCmd (const char* name, size_type bleft) noexcept;
				// Generic loader interface
    inline goid_t		LoadResource (G::EResource dtype, const void* data, uint32_t dsz, G::EBufferHint hint = G::STATIC_DRAW);
    inline void			FreeResource (goid_t id, G::EResource dtype);
private:
    goid_t			_nextid;
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
inline void PRGL::BufferSubData (goid_t id, const void* data, uint32_t dsz, uint32_t offset, G::EBufferType btype, G::EBufferHint hint)
    { Cmd (ECmd::BufferSubData, id, btype, hint, offset, SDataBlock (data, dsz)); }
inline PRGL::goid_t PRGL::LoadResource (G::EResource dtype, const void* data, uint32_t dsz, G::EBufferHint hint)
    { goid_t id = GenId(); Cmd (ECmd::LoadResource, id, dtype, hint, SDataBlock (data, dsz)); return (id); }
inline void PRGL::FreeResource (goid_t id, G::EResource dtype)
    { Cmd (ECmd::FreeResource, id, dtype); }

inline PRGL::goid_t PRGL::BufferData (const void* data, uint32_t dsz, G::EBufferHint hint, G::EBufferType btype)
    { return (LoadResource (G::EResource(btype-unsigned(G::ARRAY_BUFFER)), data, dsz, hint)); }
inline void PRGL::FreeBuffer (goid_t id)
    { FreeResource (id, G::EResource::VERTEX_ARRAY); }
inline void PRGL::FreeTexture (goid_t id)
    { FreeResource (id, G::EResource::TEXTURE); }
inline PRGL::goid_t PRGL::LoadShader (const char* v, const char* tc, const char* te, const char* g, const char* f)
{
    // Inline LoadResource call here because the compiler knows all about
    // the passed in strings and can crush this code very well
    goid_t id = GenId();
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
inline PRGL::goid_t PRGL::LoadShader (const char* v, const char* tc, const char* te, const char* f)
    { return (LoadShader (v, tc, te, "", f)); }
inline PRGL::goid_t PRGL::LoadShader (const char* v, const char* g, const char* f)
    { return (LoadShader (v, "", "", g, f)); }
inline PRGL::goid_t PRGL::LoadShader (const char* v, const char* f)
    { return (LoadShader (v, "", "", "", f)); }
inline void PRGL::FreeShader (goid_t id)
    { FreeResource (id, G::EResource::SHADER); }

//}}}-------------------------------------------------------------------
//{{{ The read parser

template <typename... Arg>
/*static*/ inline void PRGL::Args (bstri& is, Arg&... args)
{
    bstrs ss; variadic_arg_size (ss, args...);	// Size of args
    if (is.remaining() < ss.size())		// Have the whole thing?
	Error();				//  sz may be != ss.size if args has a string
    variadic_arg_read (is, args...);		// Read args
}

template <typename F>
/*static*/ inline void PRGL::Parse (F& f, const SMsgHeader& h, const char* cmdname, CCmdBuf& cmdbuf, bstri& is, bstri cmdis)
{
    auto clir = f.ClientRecord(cmdbuf.Fd(), h.iid);
    try {
	ECmd cmd = LookupCmd (cmdname, h.hsz);
	if (h.objname != RGLObject || (!clir ^ (cmd == ECmd::Open)))
	    Error();

	switch (cmd) {
	    case ECmd::Open: {
		SWinInfo winfo;
		Args (cmdis, winfo);
		f.CreateClient (h.iid, winfo, &cmdbuf);
		} break;
	    case ECmd::Close:
		f.CloseClient (clir);
		break;
	    case ECmd::Draw: {
		SDataBlock b;
		Args (cmdis, b);
		f.ClientDraw (*clir, bstri ((bstri::const_pointer) b._p, b._sz));
		} break;
	    case ECmd::LoadResource: {
		goid_t id; G::EBufferHint hint; G::EResource dtype; SDataBlock d;
		Args (cmdis, id, dtype, hint, d);
		uint32_t sid = clir->LookupId (id);
		if (sid != UINT32_MAX)
		    clir->FreeResource (dtype, sid);
		sid = clir->LoadResource (dtype, hint, (const uint8_t*) d._p, d._sz);
		clir->MapId (id, sid);
		} break;
	    case ECmd::LoadFile: {
		goid_t id; uint32_t dsz; G::EBufferHint hint; G::EResource dtype;
		Args (cmdis, id, dtype, hint, dsz);
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
		goid_t id; G::EResource dtype;
		Args (cmdis, id, dtype);
		clir->FreeResource (dtype, clir->LookupId(id));
		clir->UnmapId (id);
		} break;
	    case ECmd::BufferSubData: {
		goid_t id; uint32_t offset; G::EBufferType btype; G::EBufferHint hint; SDataBlock d;
		Args (cmdis, id, btype, hint, offset, d);
		clir->BufferSubData (clir->LookupId(id), (const uint8_t*) d._p, d._sz, offset, btype);
		} break;
	    default:
		Error();
		break;
	}
    } catch (XError& e) {
	f.ForwardError (clir, cmdname, e, cmdbuf.Fd(), h.iid);
	#ifndef NDEBUG
	    uint16_t hsz = sizeof(SMsgHeader)+h.hsz;
	    printf ("Failing command (hsz=0x%x,sz=0x%x):\n", hsz,h.sz); fflush(stdout);
	    hexdump (is.ipos()-(hsz+h.sz), hsz+h.sz);
	    printf ("Error at offset 0x%lx:\n", cmdis.ipos()-(is.ipos()-h.sz)); fflush(stdout);
	    hexdump (cmdis.ipos(), cmdis.remaining());
	#endif
    }
}

//}}}-------------------------------------------------------------------

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
    enum : goid_t { GoidNull = numeric_limits<goid_t>::max() };
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
    enum : uint32_t { RGLObject = vpack4('R','G','L',0) };
    enum class ECmd : cmd_t {
	Open,
	Close,
	Draw,
	LoadData,
	LoadFile,
	LoadPakFile,
	FreeResource,
	BufferSubData,
	NCmds,
    };
    struct SShader {
	inline SShader (const char* v, const char* tc, const char* te, const char* g, const char* f)
	    :_v(v),_tc(tc),_te(te),_g(g),_f(f),_sz(strlen(v)+1+strlen(tc)+1+strlen(te)+1+strlen(g)+1+strlen(f)+1) {}
	template <typename Stm>
	inline void write (Stm& os) const
	    { os << _sz; os.write_strz (_v); os.write_strz (_tc); os.write_strz (_te); os.write_strz (_g); os.write_strz (_f); }
    private:
	const char *_v, *_tc, *_te, *_g, *_f;
	uint32_t _sz;
    };
public:
    inline explicit		PRGL (iid_t iid) noexcept	: CCmdBuf(iid),_nextid(0) {}
    inline iid_t		IId (void) const		{ return (CCmdBuf::IId()); }
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
    inline goid_t		BufferData (const char* f, G::EBufferHint hint = G::STATIC_DRAW, G::EBufferType btype = G::ARRAY_BUFFER);
    inline goid_t		BufferData (goid_t pak, const char* f, G::EBufferHint hint = G::STATIC_DRAW, G::EBufferType btype = G::ARRAY_BUFFER);
    inline void			BufferSubData (goid_t id, const void* data, uint32_t dsz, uint32_t offset = 0, G::EBufferType btype = G::ARRAY_BUFFER, G::EBufferHint hint = G::STATIC_DRAW);
    inline void			FreeBuffer (goid_t id);
    inline goid_t		LoadDatapak (const void* d, uint32_t dsz);
    inline goid_t		LoadDatapak (const char* f);
    inline goid_t		LoadDatapak (goid_t pak, const char* f);
    inline void			FreeDatapak (goid_t id);
    inline goid_t		LoadTexture (const void* d, uint32_t dsz);
    inline goid_t		LoadTexture (const char* f);
    inline goid_t		LoadTexture (goid_t pak, const char* f);
    inline void			FreeTexture (goid_t id);
    inline goid_t		LoadFont (const void* d, uint32_t dsz);
    inline goid_t		LoadFont (const char* f);
    inline goid_t		LoadFont (goid_t pak, const char* f);
    inline void			FreeFont (goid_t id);
    inline goid_t		LoadShader (const char* v, const char* tc, const char* te, const char* g, const char* f);
    inline goid_t		LoadShader (const char* v, const char* tc, const char* te, const char* f);
    inline goid_t		LoadShader (const char* v, const char* g, const char* f);
    inline goid_t		LoadShader (const char* v, const char* f);
    inline goid_t		LoadShader (goid_t pak, const char* v, const char* tc, const char* te, const char* g, const char* f);
    inline goid_t		LoadShader (goid_t pak, const char* v, const char* tc, const char* te, const char* f);
    inline goid_t		LoadShader (goid_t pak, const char* v, const char* g, const char* f);
    inline goid_t		LoadShader (goid_t pak, const char* v, const char* f);
    inline void			FreeShader (goid_t id);
				// Buffer reading for serialization
    template <typename F>
    static inline void		Parse (F& f, const SMsgHeader& h, const char* cmdname, CCmdBuf& cmdbuf, bstri cmdis);
    static inline void		Error (void)			{ CCmdBuf::Error(); }
private:
    template <typename... Arg>
    inline void			Cmd (ECmd cmd, const Arg&... args);
    template <typename... Arg>
    inline void			CmdU (ECmd cmd, size_type unwritten, const Arg&... args);
    template <typename... Arg>
    static inline void		Args (bstri& is, Arg&... args);
    bstro			CreateCmd (ECmd cmd, size_type sz, size_type unwritten = 0) noexcept;
    inline goid_t		GenId (void)			{ return (++_nextid); }
    static inline const char*	LookupCmdName (ECmd cmd, size_type& sz) noexcept;
    static ECmd			LookupCmd (const char* name, size_type bleft) noexcept;
				// Generic loader interface
    inline goid_t		LoadData (G::EResource dtype, const void* data, uint32_t dsz, G::EBufferHint hint = G::STATIC_DRAW);
    inline goid_t		LoadPakFile (G::EResource dtype, goid_t pak, const char* filename, G::EBufferHint hint = G::STATIC_DRAW);
    goid_t			LoadFile (G::EResource dtype, const char* filename, G::EBufferHint hint = G::STATIC_DRAW);
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

template <typename... Arg>
inline void PRGL::CmdU (ECmd cmd, size_type unwritten, const Arg&... args)
{
    bstrs ss;
    variadic_arg_size (ss, args...);
    bstro os = CreateCmd (cmd, ss.size()+unwritten, unwritten);
    variadic_arg_write (os, args...);
}

inline PRGL::draww_t PRGL::Draw (size_type sz)
    { bstro os = CreateCmd (ECmd::Draw,sz+sizeof(size_type)); os << sz; return (draww_t(os)); }
inline PRGL::goid_t PRGL::LoadData (G::EResource dtype, const void* data, uint32_t dsz, G::EBufferHint hint)
    { goid_t id = GenId(); Cmd (ECmd::LoadData, id, dtype, hint, dsz, uint32_t(0), SDataBlock (data, dsz)); return (id); }
inline PRGL::goid_t PRGL::LoadPakFile (G::EResource dtype, goid_t pak, const char* filename, G::EBufferHint hint)
    { goid_t id = GenId(); Cmd (ECmd::LoadPakFile, id, dtype, hint, pak, filename); return (id); }
inline void PRGL::FreeResource (goid_t id, G::EResource dtype)
    { Cmd (ECmd::FreeResource, id, dtype); }

inline PRGL::goid_t PRGL::BufferData (const void* data, uint32_t dsz, G::EBufferHint hint, G::EBufferType btype)
    { return (LoadData (G::EResource(btype), data, dsz, hint)); }
inline PRGL::goid_t PRGL::BufferData (const char* f, G::EBufferHint hint, G::EBufferType btype)
    { return (LoadFile (G::EResource(btype), f, hint)); }
inline PRGL::goid_t PRGL::BufferData (goid_t pak, const char* f, G::EBufferHint hint, G::EBufferType btype)
    { return (LoadPakFile (G::EResource(btype), pak, f, hint)); }
inline void PRGL::BufferSubData (goid_t id, const void* data, uint32_t dsz, uint32_t offset, G::EBufferType btype, G::EBufferHint hint)
    { Cmd (ECmd::BufferSubData, id, btype, hint, offset, SDataBlock (data, dsz)); }
inline void PRGL::FreeBuffer (goid_t id)
    { FreeResource (id, G::EResource::BUFFER_VERTEX); }

inline PRGL::goid_t PRGL::LoadDatapak (const void* d, uint32_t dsz)
    { return (LoadData (G::EResource::DATAPAK, d, dsz)); }
inline PRGL::goid_t PRGL::LoadDatapak (const char* f)
    { return (LoadFile (G::EResource::DATAPAK, f)); }
inline PRGL::goid_t PRGL::LoadDatapak (goid_t pak, const char* f)
    { return (LoadPakFile (G::EResource::DATAPAK, pak, f)); }
inline void PRGL::FreeDatapak (goid_t id)
    { FreeResource (id, G::EResource::DATAPAK); }

inline PRGL::goid_t PRGL::LoadTexture (const void* d, uint32_t dsz)
    { return (LoadData (G::EResource::TEXTURE, d, dsz)); }
inline PRGL::goid_t PRGL::LoadTexture (const char* filename)
    { return (LoadFile (G::EResource::TEXTURE, filename)); }
inline PRGL::goid_t PRGL::LoadTexture (goid_t pak, const char* f)
    { return (LoadPakFile (G::EResource::TEXTURE, pak, f)); }
inline void PRGL::FreeTexture (goid_t id)
    { FreeResource (id, G::EResource::TEXTURE); }

inline PRGL::goid_t PRGL::LoadFont (const void* d, uint32_t dsz)
    { return (LoadData (G::EResource::FONT, d, dsz)); }
inline PRGL::goid_t PRGL::LoadFont (const char* f)
    { return (LoadFile (G::EResource::FONT, f)); }
inline PRGL::goid_t PRGL::LoadFont (goid_t pak, const char* f)
    { return (LoadPakFile (G::EResource::FONT, pak, f)); }
inline void PRGL::FreeFont (goid_t id)
    { FreeResource (id, G::EResource::FONT); }

inline PRGL::goid_t PRGL::LoadShader (const char* v, const char* tc, const char* te, const char* g, const char* f)
    { goid_t id = GenId(); Cmd (ECmd::LoadData, id, G::EResource::SHADER, G::STATIC_DRAW, uint32_t(0), uint32_t(0), SShader(v,tc,te,g,f)); return (id); }
inline PRGL::goid_t PRGL::LoadShader (const char* v, const char* tc, const char* te, const char* f)
    { return (LoadShader (v, tc, te, "", f)); }
inline PRGL::goid_t PRGL::LoadShader (const char* v, const char* g, const char* f)
    { return (LoadShader (v, "", "", g, f)); }
inline PRGL::goid_t PRGL::LoadShader (const char* v, const char* f)
    { return (LoadShader (v, "", "", "", f)); }
inline PRGL::goid_t PRGL::LoadShader (goid_t pak, const char* v, const char* tc, const char* te, const char* g, const char* f)
    { goid_t id = GenId(); Cmd (ECmd::LoadPakFile, id, G::EResource::SHADER, G::STATIC_DRAW, pak, SShader(v,tc,te,g,f)); return (id); }
inline PRGL::goid_t PRGL::LoadShader (goid_t pak, const char* v, const char* tc, const char* te, const char* f)
    { return (LoadShader (pak, v, tc, te, "", f)); }
inline PRGL::goid_t PRGL::LoadShader (goid_t pak, const char* v, const char* g, const char* f)
    { return (LoadShader (pak, v, "", "", g, f)); }
inline PRGL::goid_t PRGL::LoadShader (goid_t pak, const char* v, const char* f)
    { return (LoadShader (pak, v, "", "", "", f)); }
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
/*static*/ inline void PRGL::Parse (F& f, const SMsgHeader& h, const char* cmdname, CCmdBuf& cmdbuf, bstri cmdis)
{
    #ifndef NDEBUG
	auto icmdstart = cmdis.ipos();
    #endif
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
		f.ClientDraw (*clir, bstri ((bstri::const_pointer) b._p, b._sz), h.iid);
		} break;
	    case ECmd::LoadData: {
		goid_t id; G::EBufferHint hint; G::EResource dtype; uint32_t tsz, toff; SDataBlock d;
		Args (cmdis, id, dtype, hint, tsz, toff, d);
		uint32_t sid = clir->LookupId (id);
		if (sid != GoidNull)
		    clir->FreeResource (dtype, sid);
		sid = clir->LoadResource (dtype, hint, (const uint8_t*) d._p, d._sz);
		if (sid == GoidNull)
		    throw XError ("failed to load resource from data");
		clir->MapId (id, sid);
		} break;
	    case ECmd::LoadPakFile: {
		goid_t id,pak; const char* filename = nullptr; G::EResource dtype; G::EBufferHint hint;
		Args (cmdis, id, dtype, hint, pak, filename);
		uint32_t sid = clir->LookupId (id), flnsz = cmdis.ipos()-(const uint8_t*)filename;
		if (sid != GoidNull)
		    clir->FreeResource (dtype, sid);
		sid = clir->LoadPakResource (dtype, hint, clir->LookupId(pak), filename, flnsz);
		if (sid == GoidNull)
		    throw XError ("failed to load datapak resource %s", filename);
		clir->MapId (id, sid);
		} break;
	    case ECmd::LoadFile: {
		goid_t id; G::EBufferHint hint; G::EResource dtype; int fd;
		Args (cmdis, id, dtype, hint, fd);
		CMMFile recvf (fd);
		bstri dfis (recvf.MMData(), recvf.MMSize());
		uint32_t sid = clir->LookupId (id);
		if (sid != GoidNull)
		    clir->FreeResource (dtype, sid);
		sid = clir->LoadResource (dtype, hint, dfis.ipos(), dfis.remaining());
		if (sid == GoidNull)
		    throw XError ("failed to load resource from file");
		clir->MapId (id, sid);
		} break;
	    case ECmd::FreeResource: {
		goid_t id; G::EResource dtype;
		Args (cmdis, id, dtype);
		clir->FreeResource (dtype, clir->LookupId(id));
		clir->UnmapId (id);
		} break;
	    case ECmd::BufferSubData: {
		goid_t id; G::EBufferType btype; G::EBufferHint hint; uint32_t offset; SDataBlock d;
		Args (cmdis, id, btype, hint, offset, d);
		clir->BufferSubData (clir->LookupId(id), (const uint8_t*) d._p, d._sz, offset, btype);
		} break;
	    default:
		Error();
		break;
	}
    } catch (XError& e) {
	clir->ForwardError (cmdname, e, cmdbuf.Fd(), h.iid);	// ok if clir == nullptr
	#ifndef NDEBUG
	if (isatty(STDIN_FILENO)) {
	    printf ("Failing command (hsz=0x%x,sz=0x%x,errorat=0x%lx):\n", h.hsz,h.sz, cmdis.ipos()-icmdstart); fflush(stdout);
	    hexdump (icmdstart-h.hsz, h.hsz+h.sz);
	}
	#endif
	f.CloseClient (clir);
    }
}

//}}}-------------------------------------------------------------------

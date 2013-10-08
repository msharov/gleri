// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "cmd.h"
#include "drawp.h"
#include "event.h"

class PRGL : private CCmdBuf {
public:
    using CCmdBuf::iid_t;
    typedef PDraw<bstro>	draww_t;
    typedef G::SWinInfo		SWinInfo;
    typedef G::goid_t		goid_t;
    typedef G::coord_t		coord_t;
    typedef G::dim_t		dim_t;
    typedef G::color_t		color_t;
    enum : uint32_t { c_ObjectName = vpack4('R','G','L',0) };
    typedef const G::SFontInfo*	pfontinfo_t;
private:
    enum class ECmd : cmd_t {
	Auth,
	Open,
	Close,
	Draw,
	Event,
	LoadData,
	LoadFile,
	LoadPakFile,
	FreeResource,
	BufferSubData,
	TexParameter,
	NCmds,
    };
    //{{{ Serialization helper objects: SShader, SArgv
    struct SShader {
	inline SShader (const char* v, const char* tc, const char* te, const char* g, const char* f)
	    :_v(v),_tc(tc),_te(te),_g(g),_f(f),_sz(strlen(v)+1+strlen(tc)+1+strlen(te)+1+strlen(g)+1+strlen(f)+1) {}
	template <typename Stm>
	inline void write (Stm& os) const {
	    os << _sz;
	    os.write_strz (_v);
	    os.write_strz (_tc);
	    os.write_strz (_te);
	    os.write_strz (_g);
	    os.write_strz (_f);
	    os.skipalign(4);
	}
    private:
	const char *_v, *_tc, *_te, *_g, *_f;
	uint32_t _sz;
    };
    struct SArgv {
	inline SArgv (uint32_t argc, char* const* argv):_argv(argv),_argc(argc) {}
	template <typename Stm>
	inline void write (Stm& os) const {
	    uint32_t* pstrsz = (uint32_t*) os.ipos();
	    os.skip(4);	// Writing as a concatenated string (ay)
	    for (uint32_t i = 0; i < _argc; ++i)
		os.write_strz (_argv[i]);
	    if (Stm::is_writing)
		*pstrsz = os.ipos()-(typename Stm::pointer)pstrsz-4;
	    os.skipalign (4);
	}
    private:
	char* const* _argv;
	uint32_t _argc;
    };
    //}}}
public:
    inline explicit		PRGL (iid_t iid) noexcept	: CCmdBuf(iid),_lastid(iid<<16) {}
    inline iid_t		IId (void) const		{ return (CCmdBuf::IId()); }
    inline bool			Matches (int fd, iid_t iid)const{ return (Fd() == fd && IId() == iid); }
    inline bool			Matches (int fd) const		{ return (Fd() == fd); }
    inline pfontinfo_t		Font (void) const		{ static constexpr G::SFontInfo s_DefaultFontInfo = {10,18}; return (&s_DefaultFontInfo); }
    inline pfontinfo_t		Font (goid_t) const		{ return (nullptr); }
				// Command writing
    inline void			WriteCmds (void)		{ CCmdBuf::WriteCmds(); }
    inline void			SetFd (int fd, bool passFd)	{ CCmdBuf::SetFd(fd, passFd); }
				// Commands
    inline void			Export (const char* ol)		{ CCmdBuf::Export (ol); }
    inline void			Authenticate (uint32_t argc, char* const* argv, const char* hostname, uint32_t pid, const void* ad, uint32_t adsz)	{ Cmd (ECmd::Auth, SArgv(argc,argv), hostname, pid, SDataBlock(ad,adsz)); }
    inline void			Open (const char* title, const SWinInfo& winfo)			{ Cmd(ECmd::Open,winfo,title); }
    inline void			Open (const char* title, dim_t w, dim_t h, uint8_t mingl = 0x33, uint8_t maxgl = 0, SWinInfo::EMSAA aa = SWinInfo::MSAA_OFF)	{ Open (title, (SWinInfo){ 0,0,w,h,0,mingl,maxgl,aa,SWinInfo::type_Normal,SWinInfo::state_Normal,SWinInfo::flag_None }); }
    inline void			Close (void)			{ Cmd(ECmd::Close); }
    inline draww_t		Draw (size_type sz);
    inline void			Event (const CEvent& e)		{ Cmd(ECmd::Event,e); }
    inline goid_t		BufferData (const void* data, uint32_t dsz, G::EBufferHint hint = G::STATIC_DRAW, G::EBufferType btype = G::ARRAY_BUFFER);
    inline goid_t		BufferData (const char* f, G::EBufferHint hint = G::STATIC_DRAW, G::EBufferType btype = G::ARRAY_BUFFER);
    inline goid_t		BufferData (goid_t pak, const char* f, G::EBufferHint hint = G::STATIC_DRAW, G::EBufferType btype = G::ARRAY_BUFFER);
    inline void			BufferSubData (goid_t id, const void* data, uint32_t dsz, uint32_t offset = 0, G::EBufferHint hint = G::STATIC_DRAW, G::EBufferType btype = G::ARRAY_BUFFER);
    inline void			FreeBuffer (goid_t id);
    inline goid_t		LoadDatapak (const void* d, uint32_t dsz);
    inline goid_t		LoadDatapak (const char* f);
    inline goid_t		LoadDatapak (goid_t pak, const char* f);
    inline void			FreeDatapak (goid_t id);
    inline goid_t		LoadTexture (const void* d, uint32_t dsz, G::Pixel::Fmt storeas = G::Pixel::RGBA);
    inline goid_t		LoadTexture (const char* f, G::Pixel::Fmt storeas = G::Pixel::RGBA);
    inline goid_t		LoadTexture (goid_t pak, const char* f, G::Pixel::Fmt storeas = G::Pixel::RGBA);
    inline void			FreeTexture (goid_t id);
    inline void			TexParameter (G::Texture::Type t, G::Texture::Parameter p, int v)	{ Cmd(ECmd::TexParameter,t,p,v); }
    inline void			TexParameter (G::Texture::Parameter p, int v)				{ TexParameter (G::Texture::TEXTURE_2D,p,v); }
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
    static inline void		Parse (F& f, const SMsgHeader& h, CCmdBuf& cmdbuf);
private:
    template <typename... Arg>
    inline void			Cmd (ECmd cmd, const Arg&... args);
    template <typename... Arg>
    inline void			CmdU (ECmd cmd, size_type unwritten, const Arg&... args);
    bstro			CreateCmd (ECmd cmd, size_type sz, size_type unwritten = 0) noexcept;
    inline goid_t		GenId (void)			{ return (++_lastid); }
    static inline const char*	LookupCmdName (ECmd cmd, size_type& sz) noexcept;
    static ECmd			LookupCmd (const char* name, size_type bleft) noexcept;
				// Generic loader interface
    inline goid_t		LoadData (G::EResource dtype, const void* data, uint32_t dsz, uint16_t hint);
    inline goid_t		LoadPakFile (G::EResource dtype, goid_t pak, const char* filename, uint16_t hint);
    goid_t			LoadFile (G::EResource dtype, const char* filename, uint16_t hint);
    inline void			FreeResource (goid_t id, G::EResource dtype);
private:
    goid_t			_lastid;
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
inline PRGL::goid_t PRGL::LoadData (G::EResource dtype, const void* data, uint32_t dsz, uint16_t hint)
    { goid_t id = GenId(); Cmd (ECmd::LoadData, id, dtype, hint, dsz, uint32_t(0), SDataBlock (data, dsz)); return (id); }
inline PRGL::goid_t PRGL::LoadPakFile (G::EResource dtype, goid_t pak, const char* filename, uint16_t hint)
    { goid_t id = GenId(); Cmd (ECmd::LoadPakFile, id, dtype, hint, pak, filename); return (id); }
inline void PRGL::FreeResource (goid_t id, G::EResource dtype)
    { Cmd (ECmd::FreeResource, id, dtype); if (id==_lastid) --_lastid; }

inline PRGL::goid_t PRGL::BufferData (const void* data, uint32_t dsz, G::EBufferHint hint, G::EBufferType btype)
    { return (LoadData (G::EResource(btype), data, dsz, hint)); }
inline PRGL::goid_t PRGL::BufferData (const char* f, G::EBufferHint hint, G::EBufferType btype)
    { return (LoadFile (G::EResource(btype), f, hint)); }
inline PRGL::goid_t PRGL::BufferData (goid_t pak, const char* f, G::EBufferHint hint, G::EBufferType btype)
    { return (LoadPakFile (G::EResource(btype), pak, f, hint)); }
inline void PRGL::BufferSubData (goid_t id, const void* data, uint32_t dsz, uint32_t offset, G::EBufferHint hint, G::EBufferType btype)
    { Cmd (ECmd::BufferSubData, id, btype, hint, offset, SDataBlock (data, dsz)); }
inline void PRGL::FreeBuffer (goid_t id)
    { FreeResource (id, G::EResource::BUFFER_VERTEX); }

inline PRGL::goid_t PRGL::LoadDatapak (const void* d, uint32_t dsz)
    { return (LoadData (G::EResource::DATAPAK, d, dsz, 0)); }
inline PRGL::goid_t PRGL::LoadDatapak (const char* f)
    { return (LoadFile (G::EResource::DATAPAK, f, 0)); }
inline PRGL::goid_t PRGL::LoadDatapak (goid_t pak, const char* f)
    { return (LoadPakFile (G::EResource::DATAPAK, pak, f, 0)); }
inline void PRGL::FreeDatapak (goid_t id)
    { FreeResource (id, G::EResource::DATAPAK); }

inline PRGL::goid_t PRGL::LoadTexture (const void* d, uint32_t dsz, G::Pixel::Fmt storeas)
    { return (LoadData (G::EResource::TEXTURE, d, dsz, storeas)); }
inline PRGL::goid_t PRGL::LoadTexture (const char* filename, G::Pixel::Fmt storeas)
    { return (LoadFile (G::EResource::TEXTURE, filename, storeas)); }
inline PRGL::goid_t PRGL::LoadTexture (goid_t pak, const char* f, G::Pixel::Fmt storeas)
    { return (LoadPakFile (G::EResource::TEXTURE, pak, f, storeas)); }
inline void PRGL::FreeTexture (goid_t id)
    { FreeResource (id, G::EResource::TEXTURE); }

inline PRGL::goid_t PRGL::LoadFont (const void* d, uint32_t dsz)
    { return (LoadData (G::EResource::FONT, d, dsz, 0)); }
inline PRGL::goid_t PRGL::LoadFont (const char* f)
    { return (LoadFile (G::EResource::FONT, f, 0)); }
inline PRGL::goid_t PRGL::LoadFont (goid_t pak, const char* f)
    { return (LoadPakFile (G::EResource::FONT, pak, f, 0)); }
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

template <typename F>
/*static*/ inline void PRGL::Parse (F& f, const SMsgHeader& h, CCmdBuf& cmdbuf)
{
    bstri cmdis (h.Msgstrm());
    auto clir = f.ClientRecord(cmdbuf.Fd(), h.iid);
    ECmd cmd = LookupCmd (h.Cmdname(), h.hsz);
    if (!clir ^ (cmd == ECmd::Open || cmd == ECmd::Auth))
	return (f.OnNoClient (h));

    switch (cmd) {
	case ECmd::Auth: {
	    SDataBlock argv,b;
	    const char* hostname = nullptr;
	    uint32_t pid;
	    Args (cmdis, argv, hostname, pid, b);
	    f.Authenticate (cmdbuf, pid, hostname, argv, b);
	    } break;
	case ECmd::Open: {
	    SWinInfo winfo;
	    const char* title = nullptr;
	    Args (cmdis, winfo, title);
	    clir = f.CreateClient (h.iid, winfo, title, &cmdbuf);
	    } break;
	case ECmd::Close:
	    f.CloseClient (clir);
	    break;
	case ECmd::Draw: {
	    SDataBlock b;
	    Args (cmdis, b);
	    f.ClientDraw (*clir, bstri ((bstri::const_pointer) b._p, b._sz));
	    } break;
	case ECmd::Event: {
	    CEvent e;
	    Args (cmdis, e);
	    f.ClientEvent (*clir, e);
	    } break;
	case ECmd::LoadData:
	case ECmd::LoadPakFile:
	case ECmd::LoadFile: {
	    goid_t id; G::EResource dtype; uint16_t hint;
	    Args (cmdis, id, dtype, hint);
	    clir->VerifyFreeId (id);
	    uint32_t sid;
	    if (cmd == ECmd::LoadPakFile) {
		goid_t pak; const char* filename = nullptr;
		Args (cmdis, pak, filename);
		uint32_t flnsz = cmdis.ipos()-(const uint8_t*)filename;
		sid = clir->LoadPakResource (dtype, hint, clir->LookupId(pak), filename, flnsz);
		if (sid == G::GoidNull)
		    throw XError ("failed to load datapak resource %s", filename);
	    } else if (cmd == ECmd::LoadData) {
		uint32_t tsz, toff; SDataBlock d;
		Args (cmdis, tsz, toff, d);
		sid = clir->LoadResource (dtype, hint, (const uint8_t*) d._p, d._sz);
		if (sid == G::GoidNull)
		    XError::emit ("failed to load resource from data");
	    } else {
		int fd;
		Args (cmdis, fd);
		CMMFile recvf (fd);
		bstri dfis (recvf.MMData(), recvf.MMSize());
		sid = clir->LoadResource (dtype, hint, dfis.ipos(), dfis.remaining());
		if (sid == G::GoidNull)
		    XError::emit ("failed to load resource from file");
	    }
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
	case ECmd::TexParameter: {
	    G::Texture::Type t; G::Texture::Parameter p; int v;
	    Args (cmdis, t,p,v);
	    clir->TexParameter (t,p,v);
	    } break;
	default:
	    XError::emit ("invalid protocol command");
	    break;
    }
    #ifndef NDEBUG
    if (clir)
	clir->CheckForErrors();
    #endif
}

//}}}-------------------------------------------------------------------

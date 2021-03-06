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
    using CCmdBuf::SDataBlock;
    using draww_t	= PDraw<bstro>;
    using WinInfo	= G::WinInfo;
    using goid_t	= G::goid_t;
    using coord_t	= G::coord_t;
    using dim_t	= G::dim_t;
    using color_t	= G::color_t;
    using pfontinfo_t	= const G::Font::Info*;
    enum : uint32_t { c_ObjectName = vpack4('R','G','L',0) };
    enum { default_FontSize = 20 };
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
	SetCursor,
	GetClipboard,
	SetClipboard,
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
	    os.align (4);
	}
    private:
	const char *_v, *_tc, *_te, *_g, *_f;
	uint32_t _sz;
    };
    struct SArgv {
	inline SArgv (uint32_t argc, char* const* argv):_argv(argv),_argc(argc) {}
	template <typename Stm>
	inline void write (Stm& os) const {
	    auto pstrsz = (uint32_t*) os.ipos();
	    os.skip(4);	// Writing as a concatenated string (ay)
	    for (auto i = 0u; i < _argc; ++i)
		os.write_strz (_argv[i]);
	    if (Stm::is_writing)
		*pstrsz = os.ipos()-(typename Stm::pointer)pstrsz-4;
	    os.align (4);
	}
    private:
	char* const* _argv;
	uint32_t _argc;
    };
    //}}}
    //{{{ EResource enum
public:
    enum class EResource : uint16_t {
	DATAPAK,
	FRAMEBUFFER,
	SHADER,
	FONT,
	_BUFFER_FIRST = 0x20,
	BUFFER_VERTEX = _BUFFER_FIRST,
	BUFFER_INDEX,
	BUFFER_PIXEL_PACK,
	BUFFER_PIXEL_UNPACK,
	BUFFER_ATOMIC_COUNTER,
	BUFFER_COPY_READ,
	BUFFER_COPY_WRITE,
	BUFFER_DISPATCH_INDIRECT,
	BUFFER_DRAW_INDIRECT,
	BUFFER_SHADER_STORAGE,
	BUFFER_TEXTURE,
	BUFFER_TRANSFORM_FEEDBACK,
	BUFFER_UNIFORM,
	_BUFFER_LAST = BUFFER_UNIFORM,
	_TEXTURE_FIRST = _BUFFER_FIRST + 0x20,
	TEXTURE_1D = _TEXTURE_FIRST,
	TEXTURE_2D,
	TEXTURE_2D_MULTISAMPLE,
	TEXTURE_RECTANGLE,
	TEXTURE_1D_ARRAY,
	TEXTURE_CUBE_MAP,
	TEXTURE_CUBE_MAP_ARRAY,
	TEXTURE_3D,
	TEXTURE_2D_ARRAY,
	TEXTURE_2D_MULTISAMPLE_ARRAY,
	TEXTURE_SAMPLER,
	_TEXTURE_LAST = TEXTURE_SAMPLER,
	_N_RESOURCE_TYPES,
	_N_BUFFER_TYPES = _BUFFER_LAST-_BUFFER_FIRST+1,
	_N_TEXTURE_TYPES = _TEXTURE_LAST-_TEXTURE_FIRST+1
    };
    inline static EResource	ResourceFromBufferType (G::BufferType btype)	{ return EResource(uint16_t(EResource::_BUFFER_FIRST)+btype); }
    inline static G::BufferType	BufferTypeFromResource (EResource r)		{ return G::BufferType(uint16_t(r)-uint16_t(EResource::_BUFFER_FIRST)); }
    inline static EResource	ResourceFromTextureType (G::TextureType ttype)	{ return EResource(uint16_t(EResource::_TEXTURE_FIRST)+ttype); }
   inline static G::TextureType	TextureTypeFromResource (EResource r)		{ return G::TextureType(uint16_t(r)-uint16_t(EResource::_TEXTURE_FIRST)); }
    //}}}
public:
    inline explicit		PRGL (iid_t iid) noexcept	: CCmdBuf(iid),_lastid(iid<<16) {}
    inline iid_t		IId (void) const		{ return CCmdBuf::IId(); }
    inline bool			Matches (int fd, iid_t iid)const{ return Fd() == fd && IId() == iid; }
    inline bool			Matches (int fd) const		{ return Fd() == fd; }
    auto			Font (void) const		{ static constexpr const G::Font::FixedInfo s_DefaultFontInfo (10,18); return &s_DefaultFontInfo; }
    inline pfontinfo_t		Font (goid_t) const		{ return nullptr; }
				// Command writing
    inline void			WriteCmds (void)		{ CCmdBuf::WriteCmds(); }
    inline void			SetFd (int fd, bool passFd)	{ CCmdBuf::SetFd(fd, passFd); }
				// Commands
    inline void			Export (const char* ol)		{ CCmdBuf::Export (ol); }
    inline void			Authenticate (uint32_t argc, char* const* argv, const char* hostname, uint32_t pid, uint32_t screen, const void* ad, uint32_t adsz)	{ Cmd (ECmd::Auth, SArgv(argc,argv), hostname, pid, screen, SDataBlock(ad,adsz)); }
    inline void			Open (const char* title, const WinInfo& winfo)			{ Cmd(ECmd::Open,winfo,title); }
    inline void			Open (const char* title, dim_t w, dim_t h, uint8_t mingl = 0x33, uint8_t maxgl = 0, WinInfo::MSAA aa = WinInfo::MSAA_OFF)	{ Open (title, WinInfo(0,0,w,h,0,mingl,maxgl,aa)); }
    inline void			Close (void)			{ Cmd(ECmd::Close); }
    inline draww_t		Draw (size_type sz, goid_t fbid = G::default_Framebuffer);
    inline void			Event (const CEvent& e)		{ Cmd(ECmd::Event,e); }
    inline goid_t		BufferData (G::BufferType bt, const void* data, uint32_t dsz, G::BufferHint hint = G::STATIC_DRAW);
    inline goid_t		BufferData (G::BufferType bt, const char* f, G::BufferHint hint = G::STATIC_DRAW);
    inline goid_t		BufferData (goid_t pak, G::BufferType bt, const char* f, G::BufferHint hint = G::STATIC_DRAW);
    inline void			BufferSubData (goid_t id, const void* data, uint32_t dsz, uint32_t offset = 0);
    inline void			FreeBuffer (goid_t id);
    inline goid_t		LoadDatapak (const void* d, uint32_t dsz);
    inline goid_t		LoadDatapak (const char* f);
    inline goid_t		LoadDatapak (goid_t pak, const char* f);
    inline void			FreeDatapak (goid_t id);
    inline goid_t		LoadTexture (G::TextureType ttype, const void* d, uint32_t dsz, G::Pixel::Fmt storeas = G::Pixel::RGBA);
    inline goid_t		LoadTexture (G::TextureType ttype, const char* f, G::Pixel::Fmt storeas = G::Pixel::RGBA);
    inline goid_t		LoadTexture (goid_t pak, G::TextureType ttype, const char* f, G::Pixel::Fmt storeas = G::Pixel::RGBA);
    goid_t			CreateTexture (G::TextureType tt, dim_t w, dim_t h, dim_t d = 0, G::Pixel::Fmt fmt = G::Pixel::RGB, G::Pixel::Comp comp = G::Pixel::UNSIGNED_BYTE);
    inline goid_t		CreateDepthTexture (dim_t w, dim_t h)	{ return CreateTexture (G::TEXTURE_2D, w, h, 0, G::Pixel::DEPTH_COMPONENT, G::Pixel::FLOAT); }
    inline void			FreeTexture (goid_t id);
    inline void			TexParameter (G::TextureType t, G::Texture::Parameter p, int v)	{ Cmd(ECmd::TexParameter,t,p,v); }
    inline void			TexParameter (G::Texture::Parameter p, int v)			{ TexParameter (G::TEXTURE_2D,p,v); }
    inline void			SetCursor (G::Cursor c)						{ Cmd(ECmd::SetCursor,c); }
    inline void			GetClipboard (G::Clipboard c = G::Clipboard::PRIMARY, G::ClipboardFmt fmt = G::ClipboardFmt::UTF8_STRING);
    inline void			SetClipboard (const char* v, G::Clipboard c = G::Clipboard::PRIMARY, G::ClipboardFmt fmt = G::ClipboardFmt::UTF8_STRING) __attribute__((nonnull));
    inline goid_t		CreateFramebuffer (const G::FramebufferComponent* pa, unsigned na);
    inline goid_t		CreateFramebuffer (std::initializer_list<G::FramebufferComponent> fbc);
    inline goid_t		CreateFramebuffer (goid_t depthbuffer, goid_t colorbuffer);
    inline void			FreeFramebuffer (goid_t id);
    inline goid_t		LoadFont (const void* d, uint32_t dsz, uint8_t fontSize = default_FontSize);
    inline goid_t		LoadFont (const char* f, uint8_t fontSize = default_FontSize);
    inline goid_t		LoadFont (goid_t pak, const char* f, uint8_t fontSize = default_FontSize);
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
    inline goid_t		GenId (void)			{ return ++_lastid; }
    static inline const char*	LookupCmdName (ECmd cmd, size_type& sz) noexcept;
    static ECmd			LookupCmd (const char* name, size_type bleft) noexcept;
				// Generic loader interface
    inline goid_t		LoadData (EResource dtype, const void* data, uint32_t dsz, uint16_t hint);
    inline goid_t		LoadPakFile (EResource dtype, goid_t pak, const char* filename, uint16_t hint);
    goid_t			LoadFile (EResource dtype, const char* filename, uint16_t hint);
    inline void			FreeResource (goid_t id, EResource dtype);
private:
    goid_t			_lastid;
    static const char		_cmdNames[];
};

//{{{ Inline bodies ----------------------------------------------------

template <typename... Arg>
void PRGL::Cmd (ECmd cmd, const Arg&... args)
{
    bstrs ss;
    variadic_arg_size (ss, args...);
    bstro os = CreateCmd (cmd, ss.size());
    variadic_arg_write (os, args...);
}

template <typename... Arg>
void PRGL::CmdU (ECmd cmd, size_type unwritten, const Arg&... args)
{
    bstrs ss;
    variadic_arg_size (ss, args...);
    bstro os = CreateCmd (cmd, ss.size()+unwritten, unwritten);
    variadic_arg_write (os, args...);
}

PRGL::draww_t PRGL::Draw (size_type sz, goid_t fbid)
    { auto os = CreateCmd (ECmd::Draw,sz+sizeof(fbid)+sizeof(sz)); os << fbid << sz; return draww_t(os); }
PRGL::goid_t PRGL::LoadData (EResource dtype, const void* data, uint32_t dsz, uint16_t hint)
    { auto id = GenId(); Cmd (ECmd::LoadData, id, dtype, hint, dsz, uint32_t(0), SDataBlock (data, dsz)); return id; }
PRGL::goid_t PRGL::LoadPakFile (EResource dtype, goid_t pak, const char* filename, uint16_t hint)
    { auto id = GenId(); Cmd (ECmd::LoadPakFile, id, dtype, hint, pak, filename); return id; }
void PRGL::FreeResource (goid_t id, EResource dtype)
    { Cmd (ECmd::FreeResource, id, dtype); if (id==_lastid) --_lastid; }

PRGL::goid_t PRGL::BufferData (G::BufferType bt, const void* data, uint32_t dsz, G::BufferHint hint)
    { return LoadData (ResourceFromBufferType(bt), data, dsz, hint); }
PRGL::goid_t PRGL::BufferData (G::BufferType bt, const char* f, G::BufferHint hint)
    { return LoadFile (ResourceFromBufferType(bt), f, hint); }
PRGL::goid_t PRGL::BufferData (goid_t pak, G::BufferType bt, const char* f, G::BufferHint hint)
    { return LoadPakFile (ResourceFromBufferType(bt), pak, f, hint); }
void PRGL::BufferSubData (goid_t id, const void* data, uint32_t dsz, uint32_t offset)
    { Cmd (ECmd::BufferSubData, id, offset, SDataBlock (data, dsz)); }
void PRGL::FreeBuffer (goid_t id)
    { FreeResource (id, EResource::BUFFER_VERTEX); }

PRGL::goid_t PRGL::LoadDatapak (const void* d, uint32_t dsz)
    { return LoadData (EResource::DATAPAK, d, dsz, 0); }
PRGL::goid_t PRGL::LoadDatapak (const char* f)
    { return LoadFile (EResource::DATAPAK, f, 0); }
PRGL::goid_t PRGL::LoadDatapak (goid_t pak, const char* f)
    { return LoadPakFile (EResource::DATAPAK, pak, f, 0); }
void PRGL::FreeDatapak (goid_t id)
    { FreeResource (id, EResource::DATAPAK); }

PRGL::goid_t PRGL::LoadTexture (G::TextureType tt, const void* d, uint32_t dsz, G::Pixel::Fmt storeas)
    { return LoadData (ResourceFromTextureType(tt), d, dsz, storeas); }
PRGL::goid_t PRGL::LoadTexture (G::TextureType tt, const char* filename, G::Pixel::Fmt storeas)
    { return LoadFile (ResourceFromTextureType(tt), filename, storeas); }
PRGL::goid_t PRGL::LoadTexture (goid_t pak, G::TextureType tt, const char* f, G::Pixel::Fmt storeas)
    { return LoadPakFile (ResourceFromTextureType(tt), pak, f, storeas); }
void PRGL::FreeTexture (goid_t id)
    { FreeResource (id, EResource::TEXTURE_2D); }

void PRGL::GetClipboard (G::Clipboard c, G::ClipboardFmt fmt)
    { Cmd(ECmd::GetClipboard,c,fmt); }
void PRGL::SetClipboard (const char* v, G::Clipboard c, G::ClipboardFmt fmt)
    { Cmd(ECmd::SetClipboard,c,1u,fmt,v); }
PRGL::goid_t PRGL::CreateFramebuffer (const G::FramebufferComponent* pa, unsigned na)
    { return LoadData (EResource::FRAMEBUFFER, pa, na*sizeof(G::FramebufferComponent), 0); }
PRGL::goid_t PRGL::CreateFramebuffer (std::initializer_list<G::FramebufferComponent> fbc)
    { return CreateFramebuffer (fbc.begin(), fbc.size()); }
PRGL::goid_t PRGL::CreateFramebuffer (goid_t depthbuffer, goid_t colorbuffer)
    { return CreateFramebuffer (
		{{G::FRAMEBUFFER, G::DEPTH_ATTACHMENT, G::TEXTURE_2D, 0, depthbuffer},
		 {G::FRAMEBUFFER, G::COLOR_ATTACHMENT0, G::TEXTURE_2D, 0, colorbuffer}}); }
void PRGL::FreeFramebuffer (goid_t id)
    { FreeResource (id, EResource::FRAMEBUFFER); }

PRGL::goid_t PRGL::LoadFont (const void* d, uint32_t dsz, uint8_t fontSize)
    { return LoadData (EResource::FONT, d, dsz, fontSize); }
PRGL::goid_t PRGL::LoadFont (const char* f, uint8_t fontSize)
    { return LoadFile (EResource::FONT, f, fontSize); }
PRGL::goid_t PRGL::LoadFont (goid_t pak, const char* f, uint8_t fontSize)
    { return LoadPakFile (EResource::FONT, pak, f, fontSize); }
void PRGL::FreeFont (goid_t id)
    { FreeResource (id, EResource::FONT); }

PRGL::goid_t PRGL::LoadShader (const char* v, const char* tc, const char* te, const char* g, const char* f)
    { auto id = GenId(); Cmd (ECmd::LoadData, id, EResource::SHADER, G::STATIC_DRAW, uint32_t(0), uint32_t(0), SShader(v,tc,te,g,f)); return id; }
PRGL::goid_t PRGL::LoadShader (const char* v, const char* tc, const char* te, const char* f)
    { return LoadShader (v, tc, te, "", f); }
PRGL::goid_t PRGL::LoadShader (const char* v, const char* g, const char* f)
    { return LoadShader (v, "", "", g, f); }
PRGL::goid_t PRGL::LoadShader (const char* v, const char* f)
    { return LoadShader (v, "", "", "", f); }
PRGL::goid_t PRGL::LoadShader (goid_t pak, const char* v, const char* tc, const char* te, const char* g, const char* f)
    { auto id = GenId(); Cmd (ECmd::LoadPakFile, id, EResource::SHADER, G::STATIC_DRAW, pak, SShader(v,tc,te,g,f)); return id; }
PRGL::goid_t PRGL::LoadShader (goid_t pak, const char* v, const char* tc, const char* te, const char* f)
    { return LoadShader (pak, v, tc, te, "", f); }
PRGL::goid_t PRGL::LoadShader (goid_t pak, const char* v, const char* g, const char* f)
    { return LoadShader (pak, v, "", "", g, f); }
PRGL::goid_t PRGL::LoadShader (goid_t pak, const char* v, const char* f)
    { return LoadShader (pak, v, "", "", "", f); }
void PRGL::FreeShader (goid_t id)
    { FreeResource (id, EResource::SHADER); }

//}}}-------------------------------------------------------------------
//{{{ The read parser

template <typename F>
void PRGL::Parse (F& f, const SMsgHeader& h, CCmdBuf& cmdbuf) // static
{
    auto cmdis (h.Msgstrm());
    auto clir = f.ClientRecord(cmdbuf.Fd(), h.iid);
    auto cmd = LookupCmd (h.Cmdname(), h.hsz);
    if ((clir && cmd == ECmd::Auth) || (!clir && cmd != ECmd::Open && cmd != ECmd::Auth))
	return f.OnNoClient (h);

    switch (cmd) {
	case ECmd::Auth: {
	    SDataBlock argv,b;
	    const char* hostname = nullptr;
	    uint32_t pid,screen;
	    Args (cmdis, argv, hostname, pid, screen, b);
	    f.Authenticate (cmdbuf, pid, screen, hostname, argv, b);
	    } break;
	case ECmd::Open: {
	    WinInfo winfo;
	    const char* title = nullptr;
	    Args (cmdis, winfo, title);
	    if (clir)
		f.ResizeClient (*clir, winfo, title);
	    else
		clir = f.CreateClient (h.iid, winfo, title, &cmdbuf);
	    } break;
	case ECmd::Close:
	    f.CloseClient (clir);
	    break;
	case ECmd::Draw: {
	    goid_t fbid; SDataBlock b;
	    Args (cmdis, fbid, b);
	    f.ClientDraw (*clir, fbid, bstri ((bstri::const_pointer) b._p, b._sz));
	    } break;
	case ECmd::Event: {
	    CEvent e;
	    Args (cmdis, e);
	    f.ClientEvent (*clir, e);
	    } break;
	case ECmd::LoadData:
	case ECmd::LoadPakFile:
	case ECmd::LoadFile: {
	    goid_t id; EResource dtype; uint16_t hint;
	    Args (cmdis, id, dtype, hint);
	    clir->VerifyFreeId (id);
	    if (cmd == ECmd::LoadPakFile) {
		goid_t pakid; const char* filename = nullptr;
		Args (cmdis, pakid, filename);
		uint32_t flnsz = cmdis.ipos()-(const uint8_t*)filename;
		clir->LoadPakResource (id, dtype, hint, clir->LookupDatapak(pakid), filename, flnsz);
	    } else if (cmd == ECmd::LoadData) {
		uint32_t tsz, toff; SDataBlock d;
		Args (cmdis, tsz, toff, d);
		clir->LoadResource (id, dtype, hint, (const uint8_t*) d._p, d._sz);
	    } else {
		int fd;
		Args (cmdis, fd);
		CMMFile recvf (fd);
		bstri dfis (recvf.MMData(), recvf.MMSize());
		clir->LoadResource (id, dtype, hint, dfis.ipos(), dfis.remaining());
	    }
	    } break;
	case ECmd::FreeResource: {
	    goid_t id; EResource dtype;
	    Args (cmdis, id, dtype);
	    clir->FreeResource (id, dtype);
	    } break;
	case ECmd::BufferSubData: {
	    goid_t id; uint32_t offset; SDataBlock d;
	    Args (cmdis, id, offset, d);
	    clir->BufferSubData (clir->LookupBuffer(id), (const uint8_t*) d._p, d._sz, offset);
	    } break;
	case ECmd::TexParameter: {
	    G::TextureType t; G::Texture::Parameter p; int v;
	    Args (cmdis, t,p,v);
	    clir->TexParameter (t,p,v);
	    } break;
	case ECmd::SetCursor: {
	    G::Cursor c;
	    Args (cmdis, c);
	    f.SetClientCursor (*clir, c);
	    } break;
	case ECmd::GetClipboard: {
	    G::Clipboard ci; G::ClipboardFmt fmt;
	    Args (cmdis, ci, fmt);
	    f.ClientGetClipboard (*clir, ci, fmt);
	    } break;
	case ECmd::SetClipboard: {
	    G::Clipboard ci; G::ClipboardFmt fmt; const char* d; uint32_t nfmts = 0;
	    Args (cmdis, ci, nfmts, fmt, d);
	    f.ClientSetClipboard (*clir, ci, fmt, d);
	    } break;
	default:
	    XError::emit ("invalid protocol command");
	    break;
    }
    if (clir)
	clir->CheckForErrors();
}

//}}}-------------------------------------------------------------------

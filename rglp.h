#pragma once
#include "app.h"
#include "bstr.h"
#include <stdlib.h>
#include <stdio.h>

//{{{ G constants ------------------------------------------------------
namespace G {

//{{{2 RGB combiners ---------------------------------------------------

inline constexpr uint32_t RGBA (uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    { return ((a<<24)|(b<<16)|(g<<8)|r); }
inline static uint32_t ARGB (uint32_t c)
{
#if __x86_64__
    if (!__builtin_constant_p(c)) asm("rol\t$8,%0":"+r"(c)); else
#endif
	c = (c<<8)|(c>>24);
    return (__builtin_bswap32(c));
}
inline constexpr uint32_t RGB (uint8_t r, uint8_t g, uint8_t b)
    { return (RGBA(r,g,b,UINT8_MAX)); }
inline static uint32_t RGB (uint32_t c)
    { return (ARGB((UINT8_MAX<<24)|c)); }

//}}}2------------------------------------------------------------------
//{{{2 OpenGL constants

enum EType : uint16_t {
    BYTE = 0x1400,
    UNSIGNED_BYTE,
    SHORT,
    UNSIGNED_SHORT,
    INT,
    UNSIGNED_INT,
    FLOAT,
    BYTES2,
    BYTES3,
    BYTES4,
    DOUBLE
};

enum EBufferType : uint16_t {
    ARRAY_BUFFER = 0x8892,
    ELEMENT_ARRAY_BUFFER
};

enum EPrimitive : uint32_t {
    POINTS,
    LINES,
    LINE_LOOP,
    LINE_STRIP,
    TRIANGLES,
    TRIANGLE_STRIP,
    TRIANGLE_FAN
};

enum EBufferHint : uint16_t {
    STREAM_DRAW = 0x88E0,
    STREAM_READ,
    STREAM_COPY,
    STATIC_DRAW = 0x88E4,
    STATIC_READ,
    STATIC_COPY,
    DYNAMIC_DRAW = 0x88E8,
    DYNAMIC_READ,
    DYNAMIC_COPY
};

enum EStdParameter : uint8_t {
    VERTEX,
    TEXTURE_COORD
};

//}}}2------------------------------------------------------------------

} // namespace G
//}}}-------------------------------------------------------------------
//{{{ Utility functions
namespace {

/// \brief Rounds \p n up to be divisible by \p grain
template <typename T>
inline constexpr T Align (T n, size_t grain)
    { return ((n+(grain-1))-(n+(grain-1))%grain); }

} // namespace
//}}}-------------------------------------------------------------------
//{{{ CCmd

class CCmd {
public:
    typedef uint32_t		size_type;
    typedef uint8_t		value_type;
    typedef uint16_t		iid_t;
    typedef value_type*		pointer;
    typedef const value_type*	const_pointer;
protected:
    enum : uint32_t { RGLObject = G::RGBA('R','G','L',0) };
    enum : unsigned { InvalidCmd = UINT_MAX };
protected:
    //{{{2 Variadic helpers

    static inline bstrs& variadic_arg_size (bstrs& ss)
	{ return (ss); }
    template <typename A, typename... Arg>
    static inline bstrs& variadic_arg_size (bstrs& ss, const A& a, const Arg&... args)
	{ ss << a; return (variadic_arg_size (ss, args...)); }

    static inline bstri& variadic_arg_read (bstri& is)
	{ return (is); }
    template <typename A, typename... Arg>
    static inline bstri& variadic_arg_read (bstri& is, A& a, Arg&... args)
	{ is >> a; return (variadic_arg_read (is, args...)); }

    template <typename Stm>
    static inline Stm& variadic_arg_write (Stm& os)
	{ return (os); }
    template <typename Stm, typename A, typename... Arg>
    static inline Stm& variadic_arg_write (Stm& os, const A& a, const Arg&... args)
	{ os << a; return (variadic_arg_write (os, args...)); }

    //}}}2
public:
    //{{{2 Data block
    struct SDataBlock {
	const void*	_p;
	size_type	_sz;
	inline		SDataBlock (const void* p, size_type sz) :_p(p),_sz(sz) {}
    };
    //}}}2
};

inline bstrs& operator<< (bstrs& ss, const CCmd::SDataBlock& b) { ss << b._sz; ss.write(b._p,b._sz); ss.align(sizeof(b._sz)); return(ss); }
inline bstro& operator<< (bstro& os, const CCmd::SDataBlock& b) { os << b._sz; os.write(b._p,b._sz); os.align(sizeof(b._sz)); return(os); }

//}}}-------------------------------------------------------------------
//{{{ CCmdBuf

class CCmdBuf : public CCmd {
public:
    inline explicit		CCmdBuf (iid_t iid = 0)	:_buf(nullptr),_sz(0),_used(0),_iid(iid) {}
    inline			~CCmdBuf (void)		{ if(_buf) free(_buf); }
    inline iid_t		IId (void) const	{ return (_iid); }
    inline size_type		size (void) const	{ return (_used); }
    inline size_type		capacity (void) const	{ return (_sz); }
    void			ReadFromFd (int fd) noexcept;
    void			WriteToFd (int fd) noexcept;
    inline bstri		BeginRead (void) const	{ return (bstri(_buf,_used)); }
    inline void			EndRead (const bstri& is)	{ EndRead(is.ipos()); }
protected:
    bstro			CreateCmd (const char* m, size_type msz, size_type sz) noexcept;
    static inline const char*	LookupCmdName (unsigned cmd, size_type& sz, const char* cmdnames, size_type cleft) noexcept;
    static unsigned		LookupCmd (const char* name, size_type bleft, const char* cmdnames, size_type cleft) noexcept;
    static inline void		Error (void)		{ throw XError ("protocol error"); }
private:
    static inline const char*	nextname (const char* n, size_type& sz) noexcept;
    static inline bool		namecmp (const void* s1, const void* s2, size_type n) noexcept;
    inline size_type		nextcapacity (size_type v) const noexcept;
    inline pointer		addspace (size_type need) noexcept;
    inline size_type		remaining (void) const	{ return (_sz-_used); }
    inline pointer		begin (void)		{ return (_buf); }
    inline pointer		end (void)		{ return (begin()+size()); }
    void			EndRead (bstri::const_pointer p) noexcept;
private:
    pointer			_buf;
    size_type			_sz;
    size_type			_used;
    iid_t			_iid;
};

//}}}-------------------------------------------------------------------
//{{{ PDraw

template <typename Stm>
class PDraw : public CCmd {
private:
    enum class ECmd : uint16_t {
	DefaultShader,
	Clear,
	Shader,
	Parameter,
	Primitive,
	Color,
	Text,
	Sprite,
	NCmds
    };
public:
    inline		PDraw (void)		:_os() {}
    inline explicit	PDraw (const Stm& os)	:_os(os) {}
    inline size_type	size (void) const	{ return (_os.size()); }
			// Base drawing commands. See PDrawR reading equivalents below.
    inline void		DefaultShader (void)					{ Cmd (ECmd::DefaultShader); }
    inline void		Color (uint32_t c)					{ Cmd (ECmd::Color, c); }
    inline void		Clear (uint32_t c = 0)					{ Cmd (ECmd::Clear, c); }
    inline void		Shader (uint32_t id)					{ Cmd (ECmd::Shader, id); }
    inline void		Text (int16_t x, int16_t y, const char* s)		{ Cmd (ECmd::Text, x, y, s); }
    inline void		Primitive (G::EPrimitive type, uint32_t start, uint32_t sz)	{ Cmd (ECmd::Primitive, type, start, sz); }
    inline void		Sprite (int16_t x, int16_t y, uint32_t s)		{ Cmd (ECmd::Sprite, x, y, s); }
    inline void		Parameter (uint8_t slot, uint32_t buf, G::EType type = G::SHORT, uint8_t sz = 2, uint32_t offset = 0, uint32_t stride = 0)	{ Cmd (ECmd::Parameter, buf, type, slot, sz, offset, stride); }
			// Forwarding drawing commands
    inline void		Color (uint8_t r, uint8_t g, uint8_t b, uint8_t a = UINT8_MAX)	{ Color(G::RGBA(r,g,b,a)); }
    inline void		Points (uint32_t start, uint32_t sz)			{ Primitive (G::POINTS, start, sz); }
    inline void		Lines (uint32_t start, uint32_t sz)			{ Primitive (G::LINES, start, sz); }
    inline void		LineLoop (uint32_t start, uint32_t sz)			{ Primitive (G::LINE_LOOP, start, sz); }
    inline void		LineStrip (uint32_t start, uint32_t sz)			{ Primitive (G::LINE_STRIP, start, sz); }
    inline void		Triangles (uint32_t start, uint32_t sz)			{ Primitive (G::TRIANGLES, start, sz); }
    inline void		TriangleStrip (uint32_t start, uint32_t sz)		{ Primitive (G::TRIANGLE_STRIP, start, sz); }
    inline void		TriangleFan (uint32_t start, uint32_t sz)		{ Primitive (G::TRIANGLE_FAN, start, sz); }
    inline void		VertexPointer (uint32_t buf, G::EType type = G::SHORT, uint8_t sz = 2, uint32_t offset = 0, uint32_t stride = 0) { Parameter (G::VERTEX, buf, type, sz, offset, stride); }
    inline void		TexCoordPointer (uint32_t buf, G::EType type = G::SHORT, uint8_t sz = 2, uint32_t offset = 0, uint32_t stride = 0) { Parameter (G::TEXTURE_COORD, buf, type, sz, offset, stride); }
			// Reading interface
    template <typename F, typename CLIR>
    static inline void	Parse (F& f, const CLIR& clir, Stm& is);
private:
    template <typename... Arg>
    inline void		Cmd (ECmd cmd, const Arg&... args);
    template <typename... Arg>
    static inline void	Args (Stm& is, Arg&... args);
    constexpr uint32_t	Header (ECmd cmd, uint16_t sz) const	{ return ((uint32_t(sz)<<16)|uint32_t(cmd)); }
    static inline void	Error (void)				{ throw XError ("RGL parse error"); }
private:
    Stm			_os;
};

//{{{2 PDraw implementation ---------------------------------------------

template <typename Stm>
template <typename... Arg>
inline void PDraw<Stm>::Cmd (ECmd cmd, const Arg&... args)
{
    bstrs ss;
    variadic_arg_size (ss, args...);
    variadic_arg_write (_os, Header(cmd,ss.size()), args...);
}

template <typename Stm>
template <typename... Arg>
/*static*/ inline void PDraw<Stm>::Args (Stm& is, Arg&... args)
{
    bstrs ss; variadic_arg_size (ss, args...);	// Size of args
    uint16_t sz; is >> sz;			// Written size
    if (sz < ss.size() || is.remaining() < sz)	// Have the whole thing?
	Error();				//  sz may be != ss.size if args has a string
    bstri cmdis (is.ipos(),sz);			// Command stream with sz limit
    variadic_arg_read (cmdis, args...);		// Read args
    if (cmdis.ipos() != cmdis.end()) Error();	// Verify that sz is valid
    is.skip (sz);				// Update the parent stream
}

template <typename Stm>
template <typename F, typename CLIR>
/*static*/ inline void PDraw<Stm>::Parse (F& f, const CLIR& clir, Stm& is)
{
    while (is.remaining() >= 8) {
	ECmd cmd; is >> cmd;
	switch (cmd) {
	    case ECmd::DefaultShader: f.DefaultShader(); break;
	    case ECmd::Color: { uint32_t c; Args(is,c); f.Color(c); } break;
	    case ECmd::Clear: { uint32_t c; Args(is,c); f.Clear(c); } break;
	    case ECmd::Shader: { uint32_t id; Args(is,id); f.Shader(clir.LookupId(id)); } break;
	    case ECmd::Text: { uint16_t x,y; const char* s = nullptr; Args(is,x,y,s); if (s) f.Text(x,y,s); } break;
	    case ECmd::Sprite: { uint16_t x,y; uint32_t s; Args(is,x,y,s); f.Sprite(x,y,clir.LookupId(s)); } break;
	    case ECmd::Primitive: { uint32_t t,s,z; Args(is,t,s,z); f.Primitive(t,s,z); } break;
	    case ECmd::Parameter: {
		uint32_t buf, offset, stride; uint16_t type; uint8_t slot, size;
		Args(is,buf,type,slot,size,offset,stride);
		f.Parameter (slot, clir.LookupId(buf), type, size, offset, stride);
	    } break;
	    default: Error();
	}
    }
}

//}}}2------------------------------------------------------------------

#define ONDRAWDECL			\
    void		Draw (void);	\
    template <typename Drw>		\
    inline void

#define ONDRAWIMPL			\
    template <typename Drw>		\
    inline void

//}}}-------------------------------------------------------------------
//{{{ PRGL

class PRGL : public CCmdBuf {
public:
    typedef PDraw<bstro>	draww_t;
private:
    enum class ECmd : uint32_t {
	Open,
	Draw,
	BufferData,
	BufferSubData,
	FreeBuffer,
	LoadTexture,
	FreeTexture,
	NCmds,
    };
public:
    inline explicit		PRGL (iid_t iid)		:CCmdBuf(iid),_nextid(0) {}
				// Command writing
    inline void			Open (uint16_t w, uint16_t h, uint32_t glver = 0x33)	{ Cmd(ECmd::Open,w,h,glver); }
    inline draww_t		Draw (size_type sz)		{ return (draww_t(CreateCmd (ECmd::Draw,sz))); }
    inline uint32_t		CreateBuffer (void)		{ return (GenId()); }
    inline void			BufferData (uint32_t id, const void* data, uint32_t dsz, G::EBufferHint hint = G::STATIC_DRAW, G::EBufferType btype = G::ARRAY_BUFFER);
    inline void			BufferSubData (uint32_t id, const void* data, uint32_t dsz, uint16_t offset = 0, G::EBufferType btype = G::ARRAY_BUFFER);
    inline void			FreeBuffer (uint32_t id)	{ Cmd(ECmd::FreeBuffer,id); }
    inline uint32_t		CreateTexture (void)		{ return (GenId()); }
    inline void			LoadTexture (uint32_t id, const char* f)	{ Cmd(ECmd::LoadTexture,id,f); }
    inline void			FreeTexture (uint32_t id)	{ Cmd(ECmd::FreeTexture,id); }
				// Buffer reading for serialization
    inline bstri		BeginRead (void)		{ return (CCmdBuf::BeginRead()); }
    inline void			EndRead (const bstri& is)	{ CCmdBuf::EndRead(is); }
    template <typename F>
    static inline void		Parse (F& f, CCmdBuf& cmdbuf);
private:
    template <typename... Arg>
    inline void			Cmd (ECmd cmd, const Arg&... args);
    bstro			CreateCmd (ECmd cmd, size_type sz) noexcept;
    inline uint32_t		GenId (void)			{ return (++_nextid); }
    static inline const char*	LookupCmdName (ECmd cmd, size_type& sz) noexcept;
    static ECmd			LookupCmd (const char* name, size_type bleft) noexcept;
private:
    uint32_t			_nextid;
    static const char		_cmdNames[];
};

class PRGLR : public CCmdBuf {
private:
    enum class ECmd : uint32_t {
	Init,
	Resize,
	Draw,
	Event,
	NCmds
    };
public:
    inline explicit		PRGLR (iid_t iid)		:CCmdBuf(iid) {}
    inline void			Init (void)			{ Cmd(ECmd::Init); }
    inline void			Resize (uint16_t w, uint16_t h)	{ Cmd(ECmd::Resize,w,h); }
    inline void			Draw (void)			{ Cmd(ECmd::Draw); }
    inline void			Event (uint32_t key)		{ Cmd(ECmd::Event,key); }
    template <typename F>
    static inline void		Parse (F& f, CCmdBuf& cmdbuf);
private:
    template <typename... Arg>
    inline void			Cmd (ECmd cmd, const Arg&... args);
    bstro			CreateCmd (ECmd cmd, size_type sz) noexcept;
    static inline const char*	LookupCmdName (ECmd cmd, size_type& sz) noexcept;
    static ECmd			LookupCmd (const char* name, size_type bleft) noexcept;
private:
    static const char		_cmdNames[];
};

//{{{2 PRGL implementation ----------------------------------------------

template <typename... Arg>
inline void PRGL::Cmd (ECmd cmd, const Arg&... args)
{
    bstrs ss;
    variadic_arg_size (ss, args...);
    bstro os = CreateCmd (cmd, ss.size());
    variadic_arg_write (os, args...);
}

inline void PRGL::BufferData (uint32_t id, const void* data, uint32_t dsz, G::EBufferHint hint, G::EBufferType btype)
    { Cmd (ECmd::BufferData, id, btype, hint, SDataBlock (data, dsz)); }
inline void PRGL::BufferSubData (uint32_t id, const void* data, uint32_t dsz, uint16_t offset, G::EBufferType btype)
    { Cmd (ECmd::BufferSubData, id, btype, offset, SDataBlock (data, dsz)); }

template <typename F>
/*static*/ inline void PRGL::Parse (F& f, CCmdBuf& cmdbuf)
{
    size_type sz; iid_t iid; uint8_t hsz; uint32_t objn;	// All commands start with these
    const size_type chsz = sizeof(sz)+sizeof(iid)+sizeof(hsz)+sizeof(objn);

    bstri is = cmdbuf.BeginRead();

    while (is.remaining() > chsz) {	// While have commands
	auto ihdr = is.ipos();		// Save header start for return
	is >> sz >> iid >> hsz >> objn;
	if (is.remaining() < (hsz-=chsz)+sz) {
	    is.iseek (ihdr);		// Restart at header
	    break;
	}
	auto clir = f.ClientRecord(iid);
	if (objn != RGLObject)	// Not for me
	    Error();

	bstri cmdis (is.ipos()+hsz, sz);	// Command data stream
	const char* cmdname = (const char*) is.ipos();
	is.skip (hsz+sz);			// Skip to next command

	ECmd cmd = LookupCmd (cmdname, hsz);
	if (!clir ^ (cmd == ECmd::Open))
	    Error();

	switch (cmd) {
	    case ECmd::Open: {
		uint16_t w,h; uint32_t glver;
		cmdis >> w >> h >> glver;
		f.CreateClient (iid, w, h, glver);
		} break;
	    case ECmd::Draw:
		f.ClientDraw (*clir,cmdis);
		break;
	    case ECmd::BufferData: {
		uint32_t id, dsz; uint16_t btype, hint;
		if (cmdis.remaining() < 12) Error();
		cmdis >> id >> btype >> hint >> dsz;
		if (cmdis.remaining() < dsz) Error();
		uint32_t sid = clir->LookupId (id);
		if (sid == UINT32_MAX) {
		    sid = f.CreateBuffer();
		    clir->MapId (id, sid);
		}
		f.BufferData (sid, cmdis.ipos(), dsz, hint, btype);
		} break;
	    case ECmd::BufferSubData: {
		uint32_t id, dsz; uint16_t offset, btype;
		if (cmdis.remaining() < 12) Error();
		cmdis >> id >> btype >> offset >> dsz;
		if (cmdis.remaining() < dsz) Error();
		f.BufferSubData (clir->LookupId(id), cmdis.ipos(), dsz, offset, btype);
		} break;
	    case ECmd::FreeBuffer: {
		uint32_t id;
		if (cmdis.remaining() < 4) Error();
		cmdis >> id;
		f.FreeBuffer(clir->LookupId(id));
		clir->UnmapId(id);
		} break;
	    case ECmd::LoadTexture: {
		uint32_t id;
		const char* tfn;
		if (cmdis.remaining() < 8) Error();
		cmdis >> id >> tfn;
		if (!tfn) Error();
		clir->MapId(id,f.LoadTexture(tfn));
		} break;
	    case ECmd::FreeTexture: {
		uint32_t id;
		if (cmdis.remaining() < 4) Error();
		cmdis >> id;
		f.FreeTexture(clir->LookupId(id));
		clir->UnmapId(id);
		} break;
	    default:
		Error();
	}
    }
    cmdbuf.EndRead(is);
}

//}}}2------------------------------------------------------------------
//{{{2 PRGLR implementation

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
    size_type sz; iid_t iid; uint8_t hsz; uint32_t objn;	// All commands start with these
    const size_type chsz = sizeof(sz)+sizeof(iid)+sizeof(hsz)+sizeof(objn);

    bstri is = cmdbuf.BeginRead();

    while (is.remaining() > chsz) {	// While have commands
	auto ihdr = is.ipos();		// Save header start for return
	is >> sz >> iid >> hsz >> objn;
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

//}}}2------------------------------------------------------------------
//}}}-------------------------------------------------------------------

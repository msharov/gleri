#pragma once
#include "cmd.h"

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
    inline void		Color (uint8_t r, uint8_t g, uint8_t b, uint8_t a = UINT8_MAX)	{ Color(RGBA(r,g,b,a)); }
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

//----------------------------------------------------------------------

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

//----------------------------------------------------------------------

#define ONDRAWDECL			\
    void		Draw (void);	\
    template <typename Drw>		\
    inline void

#define ONDRAWIMPL			\
    template <typename Drw>		\
    inline void

//----------------------------------------------------------------------

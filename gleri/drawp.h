// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "cmd.h"

template <typename Stm>
class PDraw : public CCmd {
public:
    enum {
	is_sizing = Stm::is_sizing,
	is_reading = Stm::is_reading,
	is_writing = Stm::is_writing
    };
    template <typename T, unsigned N> struct ArrayArg {
	inline ArrayArg (const T* v = nullptr):_v(v) {}
	const T* _v;
    };
private:
    enum class ECmd : uint16_t {
	DefaultShader,
	Clear,
	Shader,
	Parameter,
	Uniformf,
	Uniformi,
	Uniformm,
	Uniformt,
	Shape,
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
    inline void		Shape (G::EShape type, uint32_t start, uint32_t sz)	{ Cmd (ECmd::Shape, type, start, sz); }
    inline void		Sprite (int16_t x, int16_t y, uint32_t s)		{ Cmd (ECmd::Sprite, x, y, s); }
    inline void		Parameter (uint8_t slot, uint32_t buf, G::EType type = G::SHORT, uint8_t sz = 2, uint32_t offset = 0, uint32_t stride = 0)	{ Cmd (ECmd::Parameter, buf, type, slot, sz, offset, stride); }
    inline void		Uniform (const char* name, float x, float y, float z, float w)		{ Cmd (ECmd::Uniformf, name, x,y,z,w); }
    inline void		Uniformi (const char* name, int x, int y, int z, int w)			{ Cmd (ECmd::Uniformi, name, x,y,z,w); }
    inline void		Uniformv (const char* name, const float* v);
    inline void		Uniformv (const char* name, const int* v);
    inline void		Texture (const char* name, uint32_t id, uint32_t slot = 0)		{ Cmd (ECmd::Uniformt, name, id, slot); }
    inline void		Matrix (const char* name, const float* m);
			// Forwarding drawing commands
    inline void		Color (uint8_t r, uint8_t g, uint8_t b, uint8_t a = UINT8_MAX)	{ Color(RGBA(r,g,b,a)); }
    inline void		Points (uint32_t start, uint32_t sz)			{ Shape (G::POINTS, start, sz); }
    inline void		Lines (uint32_t start, uint32_t sz)			{ Shape (G::LINES, start, sz); }
    inline void		LineLoop (uint32_t start, uint32_t sz)			{ Shape (G::LINE_LOOP, start, sz); }
    inline void		LineStrip (uint32_t start, uint32_t sz)			{ Shape (G::LINE_STRIP, start, sz); }
    inline void		Triangles (uint32_t start, uint32_t sz)			{ Shape (G::TRIANGLES, start, sz); }
    inline void		TriangleStrip (uint32_t start, uint32_t sz)		{ Shape (G::TRIANGLE_STRIP, start, sz); }
    inline void		TriangleFan (uint32_t start, uint32_t sz)		{ Shape (G::TRIANGLE_FAN, start, sz); }
    inline void		VertexPointer (uint32_t buf, G::EType type = G::SHORT, uint8_t sz = 2, uint32_t offset = 0, uint32_t stride = 0) { Parameter (G::VERTEX, buf, type, sz, offset, stride); }
    inline void		TexCoordPointer (uint32_t buf, G::EType type = G::SHORT, uint8_t sz = 2, uint32_t offset = 0, uint32_t stride = 0) { Parameter (G::TEXTURE_COORD, buf, type, sz, offset, stride); }
			// Reading interface
    template <typename F>
    static inline void	Parse (F& f, Stm& is);
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

//{{{ Inline bodies ----------------------------------------------------

template <typename Stm>
template <typename... Arg>
inline void PDraw<Stm>::Cmd (ECmd cmd, const Arg&... args)
{
    bstrs ss;
    variadic_arg_size (ss, args...);
    variadic_arg_write (_os, Header(cmd,ss.size()), args...);
}

template <typename Stm, typename T, unsigned N>
inline Stm& operator<< (Stm& stm, const typename PDraw<Stm>::template ArrayArg<T,N>& aa)
    { stm.write (aa._v, N*sizeof(T)); return (stm); }
template <typename T, unsigned N>
inline bstri& operator>> (bstri& stm, typename PDraw<bstri>::ArrayArg<T,N>& aa)
    { aa._v = stm.iptr<T>(); stm.skip (N*sizeof(T)); return (stm); }

template <typename Stm>
inline void PDraw<Stm>::Uniformv (const char* name, const float* v)
    { Cmd (ECmd::Uniformf, name, ArrayArg<float,4>(v)); }
template <typename Stm>
inline void PDraw<Stm>::Uniformv (const char* name, const int* v)
    { Cmd (ECmd::Uniformf, name, ArrayArg<int,4>(v)); }
template <typename Stm>
inline void PDraw<Stm>::Matrix (const char* name, const float* m)
    { Cmd (ECmd::Uniformm, name, ArrayArg<float,16>(m)); }

//}}}-------------------------------------------------------------------
//{{{ Parser

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
template <typename F>
/*static*/ inline void PDraw<Stm>::Parse (F& f, Stm& is)
{
    while (is.remaining() >= sizeof(ECmd)) {
	ECmd cmd; is >> cmd;
	switch (cmd) {
	    case ECmd::DefaultShader: Args(is); f.SetDefaultShader(); break;
	    case ECmd::Color: { uint32_t c; Args(is,c); f.Color(c); } break;
	    case ECmd::Clear: { uint32_t c; Args(is,c); f.Clear(c); } break;
	    case ECmd::Shader: { uint32_t id; Args(is,id); f.Shader(f.LookupId(id)); } break;
	    case ECmd::Text: { uint16_t x,y; const char* s = nullptr; Args(is,x,y,s); if (s) f.Text(x,y,s); } break;
	    case ECmd::Sprite: { uint16_t x,y; uint32_t s; Args(is,x,y,s); f.Sprite(x,y,f.LookupId(s)); } break;
	    case ECmd::Shape: { uint32_t t,s,z; Args(is,t,s,z); f.Shape(t,s,z); } break;
	    case ECmd::Parameter: {
		uint32_t buf, offset, stride; uint16_t type; uint8_t slot, size;
		Args(is,buf,type,slot,size,offset,stride);
		f.Parameter (slot, f.LookupId(buf), type, size, offset, stride);
	    } break;
	    case ECmd::Uniformf: { const char* name = nullptr; ArrayArg<float,4> uv; Args(is,name,uv); f.Uniform4fv (name, uv._v); } break;
	    case ECmd::Uniformi: { const char* name = nullptr; ArrayArg<int,4> uv; Args(is,name,uv); f.Uniform4iv (name, uv._v); } break;
	    case ECmd::Uniformm: { const char* name = nullptr; ArrayArg<float,16> uv; Args(is,name,uv); f.UniformMatrix (name, uv._v); } break;
	    case ECmd::Uniformt: { const char* name = nullptr; uint32_t id,slot; Args (is,name,id,slot); f.UniformTexture (name, id, slot); } break;
	    default: Error();
	}
    }
}

//}}}-------------------------------------------------------------------

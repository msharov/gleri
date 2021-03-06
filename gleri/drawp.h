// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "cmd.h"

class PDrawBase : public CCmd {
public:
    using goid_t	= G::goid_t;
    using coord_t	= G::coord_t;
    using dim_t		= G::dim_t;
    using color_t	= G::color_t;
    struct Range {
	uint32_t	offset;
	uint32_t	size;
    };
    using rangevec_t	= vector<Range>;
protected:
    template <typename T, unsigned N> struct ArrayArg {
	inline constexpr ArrayArg (const T* v = nullptr) :_v(v) {}
	template <typename AAStm>
	inline void write (AAStm& os) const { os.write (_v, N*sizeof(T)); }
	inline void read (bstri& is) { _v = is.iptr<T>(); is.skip (N*sizeof(T)); }
	const T* _v;
    };
    enum class ECmd : uint16_t {
	Clear,
	Viewport,
	Offset,
	Scale,
	Enable,
	Color,
	Text,
	Image,
	Sprite,
	Shader,
	BindBuffer,
	BindFramebuffer,
	BindFramebufferComponent,
	BindFont,
	Parameter,
	Uniformf,
	Uniformi,
	Uniformm,
	Uniformt,
	DrawArrays,
	DrawArraysIndirect,
	DrawArraysInstanced,
	DrawElements,
	DrawElementsIndirect,
	DrawElementsInstanced,
	DrawRangeElements,
	MultiDrawArrays,
	MultiDrawArraysIndirect,
	MultiDrawElements,
	MultiDrawElementsIndirect,
	SaveFramebuffer,
	InstancingDivisor,
	PatchVertices,
	PointSize,
	NCmds
    };
};

template <typename Stm>
class PDraw : public PDrawBase {
public:
    enum {
	is_sizing = Stm::is_sizing,
	is_reading = Stm::is_reading,
	is_writing = Stm::is_writing
    };
public:
    inline		PDraw (void)		:_os() {}
    inline explicit	PDraw (const Stm& os)	:_os(os) {}
    inline size_type	size (void) const	{ return _os.size(); }
			// Base drawing commands. See PDrawR reading equivalents below.
    inline void		Clear (color_t c = 0)					{ Cmd (ECmd::Clear, c); }
    inline void		Viewport (coord_t x, coord_t y, dim_t w, dim_t h)	{ Cmd (ECmd::Viewport, x,y,w,h); }
    inline void		ResetViewport (void)					{ Viewport (0,0,0,0); }
    inline void		Offset (coord_t x, coord_t y)				{ Cmd (ECmd::Offset, x,y); }
    inline void		Scale (float x, float y)				{ Cmd (ECmd::Scale, x,y); }
    inline void		Color (color_t c)					{ Cmd (ECmd::Color, c); }
    inline void		Enable (G::Feature f)					{ Cmd (ECmd::Enable, f, uint16_t(1)); }
    inline void		Disable (G::Feature f)					{ Cmd (ECmd::Enable, f, uint16_t(0)); }
    inline void		Text (coord_t x, coord_t y, const char* s)		{ Cmd (ECmd::Text, x, y, s); }
    inline void		Image (coord_t x, coord_t y, goid_t s)			{ Cmd (ECmd::Image, x, y, s); }
    inline void		Sprite (coord_t x, coord_t y, goid_t s, coord_t sx, coord_t sy, dim_t sw, dim_t sh)	{ Cmd (ECmd::Sprite,x,y,s,sx,sy,sw,sh); }
    inline void		Shader (goid_t id)					{ Cmd (ECmd::Shader, id); }
    inline void		DefaultShader (void)					{ Shader (G::default_FlatShader); }
    inline void		Buffer (goid_t id)					{ Cmd (ECmd::BindBuffer, id); }
    inline void		Framebuffer (goid_t id, G::FramebufferType bindas = G::FRAMEBUFFER)	{ Cmd (ECmd::BindFramebuffer, id, uint32_t(bindas)); }
    inline void		DefaultFramebuffer (void)						{ Framebuffer (G::default_Framebuffer, G::FRAMEBUFFER); }
    inline void		FramebufferComponent (goid_t id, const G::FramebufferComponent c)	{ Cmd (ECmd::BindFramebufferComponent, id, c); }
    inline void		FramebufferComponent (goid_t id, goid_t texid)	{ FramebufferComponent (id, G::FramebufferComponent (G::FRAMEBUFFER, G::COLOR_ATTACHMENT0, G::TEXTURE_2D, 0, texid)); }
    inline void		SaveFramebuffer (coord_t x, coord_t y, dim_t w, dim_t h, const char* filename, G::Texture::Format fmt, uint8_t quality = 100)
			    { Cmd (ECmd::SaveFramebuffer, x,y,w,h, filename, fmt,quality, G::Pixel::RGB, G::Pixel::UNSIGNED_BYTE, uint16_t(0)); }
    inline void		Font (goid_t f)					{ Cmd (ECmd::BindFont, f); }
    inline void		Parameter (uint32_t slot, goid_t buf, G::Type type = G::SHORT, uint8_t sz = 2, uint32_t offset = 0, uint16_t stride = 0)	{ Cmd (ECmd::Parameter, uint32_t(1), slot, buf, uint8_t(type-G::Type_BASE), sz, stride, offset); }
    inline void		Parameter (const char* slot, goid_t buf, G::Type type = G::SHORT, uint8_t sz = 2, uint32_t offset = 0, uint16_t stride = 0)	{ Cmd (ECmd::Parameter, slot, buf, uint8_t(type-G::Type_BASE), sz, stride, offset); }
    inline void		InstancingDivisor (uint16_t slot, uint16_t divisor)	{ Cmd (ECmd::InstancingDivisor, slot,divisor); }
    inline void		PatchVertices (uint32_t nv)				{ Cmd (ECmd::PatchVertices, nv); }
    inline void		PointSize (float ps)					{ Cmd (ECmd::PointSize, ps); }
    inline void		Uniform (const char* name, float x, float y, float z, float w)	{ Cmd (ECmd::Uniformf, name, x,y,z,w); }
    inline void		Uniformi (const char* name, int x, int y, int z, int w)	{ Cmd (ECmd::Uniformi, name, x,y,z,w); }
    inline void		Uniformv (const char* name, const float* v)		{ Cmd (ECmd::Uniformf, name, ArrayArg<float,4>(v)); }
    inline void		Uniformv (const char* name, const int* v)		{ Cmd (ECmd::Uniformf, name, ArrayArg<int,4>(v)); }
    inline void		Texture (const char* name, goid_t id, uint32_t slot=0)	{ Cmd (ECmd::Uniformt, name, id, slot); }
    inline void		Matrix (const char* name, const float* m)		{ Cmd (ECmd::Uniformm, name, ArrayArg<float,16>(m)); }
			// Various drawing methods
    inline void		DrawArrays (G::Shape type, uint32_t start, uint32_t sz)	{ Cmd (ECmd::DrawArrays, type, start, sz); }
    inline void		DrawArraysIndirect (G::Shape type, uint32_t bufoffset = 0)
			    { Cmd (ECmd::DrawArraysIndirect, type, bufoffset); }
    inline void		DrawArraysInstanced (G::Shape type, uint32_t start, uint32_t sz, uint32_t nInstances, uint32_t baseInstance = 0)
			    { Cmd (ECmd::DrawArraysInstanced, type, start, sz, nInstances, baseInstance); }
    inline void		DrawElements (G::Shape type, uint16_t n, G::Type itype = G::UNSIGNED_SHORT, uint32_t offset = 0, uint32_t baseVertex = 0)
			    { Cmd (ECmd::DrawElements, type, n, itype, offset, baseVertex); }
    inline void		DrawElementsIndirect (G::Shape type, G::Type itype = G::UNSIGNED_SHORT, uint16_t bufoffset = 0)
			    { Cmd (ECmd::DrawElementsIndirect, type, itype, bufoffset); }
    inline void		DrawElementsInstanced (G::Shape type, uint16_t n, uint32_t nInstances, G::Type itype = G::UNSIGNED_SHORT, uint32_t offset = 0, uint32_t baseVertex = 0, uint32_t baseInstance = 0)
			    { Cmd (ECmd::DrawElementsInstanced, type, n, itype, nInstances, offset, baseVertex, baseInstance); }
    inline void		DrawRangeElements (G::Shape type, uint16_t minel, uint16_t maxel, uint16_t n, G::Type itype = G::UNSIGNED_SHORT, uint32_t offset = 0, uint32_t baseVertex = 0)
			    { Cmd (ECmd::DrawRangeElements, type, n, itype, minel, maxel, offset, baseVertex); }
    inline void		MultiDrawArrays (G::Shape type, const rangevec_t& r)
			    { Cmd (ECmd::MultiDrawArrays, type, r); }
    inline void		MultiDrawArraysIndirect (G::Shape type, uint32_t n, uint32_t stride = 0, uint32_t bufoffset = 0)
			    { Cmd (ECmd::MultiDrawArraysIndirect, type, n, stride, bufoffset); }
    inline void		MultiDrawElements (G::Shape type, const rangevec_t& r, G::Type itype = G::UNSIGNED_SHORT)
			    { Cmd (ECmd::MultiDrawElements, type, itype, uint16_t(0), r); }
    inline void		MultiDrawElementsIndirect (G::Shape type, uint32_t n, G::Type itype = G::UNSIGNED_SHORT, uint16_t stride = 0, uint32_t bufoffset = 0)
			    { Cmd (ECmd::MultiDrawElementsIndirect, type, itype, stride, n, bufoffset); }
			// Forwarding drawing commands
    template <typename... Args>
    inline void		Textf (coord_t x, coord_t y, const char* fmt, Args... args)	{ char buf[256]; snprintf (ArrayBlock(buf), fmt, args...); Text (x,y,buf); }
    inline void		Color (uint8_t r, uint8_t g, uint8_t b, uint8_t a = UINT8_MAX)	{ Color(RGBA(r,g,b,a)); }
    inline void		Points (uint32_t start, uint32_t sz)			{ DrawArrays (G::POINTS, start, sz); }
    inline void		Lines (uint32_t start, uint32_t sz)			{ DrawArrays (G::LINES, start, sz); }
    inline void		LineLoop (uint32_t start, uint32_t sz)			{ DrawArrays (G::LINE_LOOP, start, sz); }
    inline void		LineStrip (uint32_t start, uint32_t sz)			{ DrawArrays (G::LINE_STRIP, start, sz); }
    inline void		Triangles (uint32_t start, uint32_t sz)			{ DrawArrays (G::TRIANGLES, start, sz); }
    inline void		TriangleStrip (uint32_t start, uint32_t sz)		{ DrawArrays (G::TRIANGLE_STRIP, start, sz); }
    inline void		TriangleFan (uint32_t start, uint32_t sz)		{ DrawArrays (G::TRIANGLE_FAN, start, sz); }
    inline void		VertexPointer (goid_t buf, G::Type type = G::SHORT, uint8_t sz = 2, uint32_t offset = 0, uint16_t stride = 0) { Parameter (G::param_Vertex, buf, type, sz, offset, stride); }
    inline void		ColorPointer (goid_t buf, G::Type type = G::UNSIGNED_BYTE, uint8_t sz = 4, uint32_t offset = 0, uint16_t stride = 0) { Parameter (G::param_Color, buf, type, sz, offset, stride); }
    inline void		TexCoordPointer (goid_t buf, G::Type type = G::SHORT, uint8_t sz = 2, uint32_t offset = 0, uint16_t stride = 0) { Parameter (G::param_TexCoord, buf, type, sz, offset, stride); }
    inline void		Screenshot (const char* filename)			{ SaveFramebuffer (0,0,0,0,filename,G::Texture::Format::JPEG); }
			// Reading interface
    template <typename F>
    static inline void	Parse (F& f, Stm& is);
private:
    template <typename... Arg>
    inline void		Cmd (ECmd cmd, const Arg&... args);
    template <typename... Arg>
    static inline void	Args (Stm& is, Arg&... args);
    constexpr uint32_t	Header (ECmd cmd, uint16_t sz) const	{ return vpack4(uint16_t(cmd),sz); }
private:
    Stm			_os;
};

//{{{ Inline bodies ----------------------------------------------------

template <typename Stm>
template <typename... Arg>
void PDraw<Stm>::Cmd (ECmd cmd, const Arg&... args)
{
    bstrs ss;
    variadic_arg_size (ss, args...);
    assert (!(ss.size()%4) && "All PDraw commands must end 4-byte aligned");
    variadic_arg_write (_os, Header(cmd,ss.size()), args...);
}

//}}}-------------------------------------------------------------------
//{{{ Parser

template <typename Stm>
template <typename... Arg>
void PDraw<Stm>::Args (Stm& is, Arg&... args) // static
{
    bstrs ss; variadic_arg_size (ss, args...);	// Size of args
    uint16_t sz; is >> sz;			// Written size
    if (sz < ss.size() || is.remaining() < sz)	// Have the whole thing?
	XError::emit ("drawlist parse error");	//  sz may be != ss.size if args has a string
    bstri cmdis (is.ipos(),sz);			// Command stream with sz limit
    variadic_arg_read (cmdis, args...);		// Read args
    if (cmdis.ipos() != cmdis.end())
	XError::emit ("drawlist parse error");	// Verify that sz is valid
    is.skip (sz);				// Update the parent stream
}

template <typename Stm>
template <typename F>
void PDraw<Stm>::Parse (F& f, Stm& is) // static
{
    while (is.remaining() >= sizeof(ECmd)) {
	ECmd cmd; is >> cmd;
	if (cmd >= ECmd::BindBuffer && cmd <= ECmd::MultiDrawArraysIndirect)
	    f.DrawCmdInit();
	switch (cmd) {
	    case ECmd::Clear: { color_t c; Args(is,c); f.Clear(c); } break;
	    case ECmd::Viewport: { coord_t x,y; dim_t w,h; Args(is,x,y,w,h); f.Viewport(x,y,w,h); } break;
	    case ECmd::Color: { color_t c; Args(is,c); f.Color(c); } break;
	    case ECmd::Offset: { coord_t x,y; Args(is,x,y); f.Offset(x,y); } break;
	    case ECmd::Scale: { float x,y; Args(is,x,y); f.Scale(x,y); } break;
	    case ECmd::Enable: { G::Feature feat; uint16_t o; Args(is,feat,o); f.Enable(feat,o); } break;
	    case ECmd::Text: { coord_t x,y; const char* s = nullptr; Args(is,x,y,s); if (s) f.Text(x,y,s); } break;
	    case ECmd::Image: { coord_t x,y; goid_t s; Args(is,x,y,s); f.Sprite(f.LookupTexture(s),x,y); } break;
	    case ECmd::Sprite: { coord_t x,y,sx,sy; dim_t sw,sh; goid_t s; Args(is,x,y,s,sx,sy,sw,sh); f.Sprite(f.LookupTexture(s),x,y,sx,sy,sw,sh); } break;
	    case ECmd::Shader: { goid_t id; Args(is,id); f.Shader(f.LookupShader(id)); } break;
	    case ECmd::BindBuffer: { goid_t id; Args(is,id); f.BindBuffer(f.LookupBuffer(id)); } break;
	    case ECmd::BindFramebuffer: { goid_t id; uint32_t bindas; Args(is,id,bindas); f.BindFramebuffer(f.LookupFramebuffer(id),G::FramebufferType(bindas)); } break;
	    case ECmd::BindFramebufferComponent: { goid_t id; G::FramebufferComponent c; Args(is,id,c); f.BindFramebufferComponent(f.LookupFramebuffer(id),c); } break;
	    case ECmd::BindFont: { goid_t fid; Args(is,fid); f.BindFont(fid); } break;
	    case ECmd::Parameter: {
		goid_t buf; uint32_t offset; uint16_t stride; uint8_t type, size; const char* slot = nullptr;
		Args(is,slot,buf,type,size,stride,offset);
		f.Parameter (slot, f.LookupBuffer(buf), G::Type(G::Type_BASE+type), size, offset, stride);
		} break;
	    case ECmd::Uniformf: { const char* name = nullptr; ArrayArg<float,4> uv; Args(is,name,uv); f.Uniform4fv (name, uv._v); } break;
	    case ECmd::Uniformi: { const char* name = nullptr; ArrayArg<int,4> uv; Args(is,name,uv); f.Uniform4iv (name, uv._v); } break;
	    case ECmd::Uniformm: { const char* name = nullptr; ArrayArg<float,16> uv; Args(is,name,uv); f.UniformMatrix (name, uv._v); } break;
	    case ECmd::Uniformt: { const char* name = nullptr; goid_t id,slot; Args (is,name,id,slot); f.UniformTexture (name, f.LookupTexture(id), slot); } break;
	    case ECmd::DrawArrays: { G::Shape t; uint32_t s,z; Args(is,t,s,z); f.DrawArrays(t,s,z); } break;
	    case ECmd::DrawArraysIndirect: { G::Shape t; uint32_t offset; Args(is,t,offset); f.DrawArraysIndirect (t, offset); } break;
	    case ECmd::DrawArraysInstanced:
		{ G::Shape t; uint32_t s,z,ni,bi; Args(is,t,s,z,ni,bi); f.DrawArraysInstanced(t,s,z,ni,bi); } break;
	    case ECmd::DrawElements:
		{ G::Shape t; uint16_t n; G::Type it; uint32_t o,bv; Args(is,t,n,it,o,bv); f.DrawElements(t,n,it,o,bv); } break;
	    case ECmd::DrawElementsIndirect:
		{ G::Shape t; G::Type it; uint32_t o; Args(is,t,it,o); f.DrawElementsIndirect(t,it,o); } break;
	    case ECmd::DrawElementsInstanced:
		{ G::Shape t; uint16_t n; G::Type it; uint32_t ni,o,bi,bv; Args(is,t,n,it,ni,o,bv,bi); f.DrawElementsInstanced(t,n,ni,it,o,bv,bi); } break;
	    case ECmd::DrawRangeElements:
		{ G::Shape t; uint16_t n,minv,maxv; G::Type it; uint32_t o,bv; Args(is,t,n,it,minv,maxv,o,bv); f.DrawRangeElements(t,minv,maxv,n,it,o,bv); } break;
	    case ECmd::MultiDrawArrays:
		{ G::Shape t; rangevec_t r; Args(is,t,r); f.MultiDrawArrays(t,r); } break;
	    case ECmd::MultiDrawArraysIndirect:
		{ G::Shape t; uint32_t n,stride,offset; Args(is,t,n,stride,offset); f.MultiDrawArraysIndirect (t,n,stride,offset); } break;
	    case ECmd::MultiDrawElements:
		{ G::Shape t; G::Type it; uint16_t n; rangevec_t r; Args(is,t,it,n,r); f.MultiDrawElements(t,r,it); } break;
	    case ECmd::MultiDrawElementsIndirect:
		{ G::Shape t; G::Type it; uint16_t stride; uint32_t n,o; Args(is,t,it,stride,n,o); f.MultiDrawElementsIndirect(t,it,n,stride,o); } break;
	    case ECmd::SaveFramebuffer: {
		coord_t x,y; dim_t w,h; const char* filename = nullptr; G::Texture::Format fmt; uint8_t quality; uint16_t pfmt,pcomp,resv;
		Args (is,x,y,w,h,filename,fmt,quality,pfmt,pcomp,resv);
		f.SaveFramebuffer (x,y,w,h,filename,fmt,quality);
		} break;
	    case ECmd::InstancingDivisor:
		{ uint16_t slot,divisor; Args(is,slot,divisor); f.SetInstancingDivisor(slot,divisor); } break;
	    case ECmd::PatchVertices:
		{ uint32_t nv; Args(is,nv); f.SetPatchVertices(nv); } break;
	    case ECmd::PointSize:
		{ float ps; Args(is,ps); f.SetPointSize(ps); } break;
	    default: XError::emit ("drawlist parse error");
	}
	#ifndef NDEBUG
	    f.CheckForErrors();
	#endif
    }
}

//}}}-------------------------------------------------------------------

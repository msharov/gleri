// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "iconn.h"
#include "goshad.h"
#include "gotex.h"
#include "gofont.h"

class CGLWindow : public PRGLR {
private:
    enum EStdQuery {
	query_RenderBegin,
	query_RenderEnd,
	query_FrameEnd,
	NStdQueries
    };
    enum { NotWaitingForVSync = CApp::NoTimer };
    enum { MAX_VAO_SLOTS = 16 };
    typedef float		matrix4f_t[4][4];
    typedef PRGL::SWinInfo	SWinInfo;
public:
				CGLWindow (iid_t iid, const SWinInfo& winfo, Window win, GLXContext ctx, CIConn* pconn);
				~CGLWindow (void) noexcept;
    void			Init (void);
    void			Deactivate (void);
    inline const CContext&	Context (void) const		{ return (_ctx); }
    inline GLXContext		ContextId (void) const		{ return (_ctx.Context()); }
    inline Window		Drawable (void) const		{ return (_ctx.Drawable()); }
    inline const SWinInfo&	WinInfo (void) const		{ return (_winfo); }
    void			Resize (coord_t x, coord_t y, dim_t w, dim_t h) noexcept;
    uint64_t			DrawFrame (bstri cmdis, Display* dpy);
    uint64_t			DrawFrameNoWait (bstri cmdis, Display* dpy);
    uint64_t			DrawPendingFrame (Display* dpy) noexcept;
    inline void			ClearPendingFrame (void)	{ _pendingFrame.clear(); }
    uint64_t			NextFrameTime (void) const	{ return (_nextVSync); }
				// Client-side id map, forwarded to the connection object
    inline void			MapId (goid_t cid, GLuint sid) noexcept	{ _pconn->MapId (cid, sid); }
    inline GLuint		LookupId (goid_t cid) const noexcept	{ return (_pconn->LookupId (cid)); }
    inline void			UnmapId (goid_t cid) noexcept		{ _pconn->UnmapId (cid); }
				// State variables
    inline const float*		Proj (void) const		{ return (&_proj[0][0]); }
    inline GLuint		Color (void) const		{ return (_color); }
    inline void			SetColor (GLuint c)		{ _color = c; }
    inline GLuint		Shader (void) const		{ return (_curShader); }
    inline void			SetShader (GLuint s)		{ _curShader = s; }
    inline GLuint		Buffer (void) const		{ return (_curBuffer); }
    inline void			SetBuffer (GLuint b)		{ _curBuffer = b; }
    inline GLuint		Texture (void) const		{ return (_curTexture); }
    inline void			SetTexture (GLuint t)		{ _curTexture = t; }
    inline GLuint		Font (void) const		{ return (_curFont); }
    inline void			SetFont (GLuint f)		{ _curFont = f; }
    inline GLuint		LastRenderTime (void) const	{ return (_syncEvent.time); }
    inline GLuint		LastFrameTime (void) const	{ return (_syncEvent.key); }
				// Resource loader by enum
    GLuint			LoadResource (G::EResource dtype, G::EBufferHint hint, const GLubyte* d, GLuint dsz);
    GLuint			LoadPakResource (G::EResource dtype, G::EBufferHint hint, GLuint pak, const char* filename, GLuint flnsz);
    void			FreeResource (G::EResource dtype, GLuint id);
				// Datapak
    GLuint			LoadDatapak (const char* filename);
    GLuint			LoadDatapak (const GLubyte* p, GLuint psz);
    void			FreeDatapak (GLuint id);
    const CDatapak*		Datapak (GLuint id) const;
				// Buffer
    GLuint			CreateBuffer (G::EBufferType btype = G::ARRAY_BUFFER) noexcept;
    void			BindBuffer (GLuint id);
    void			BindBuffer (GLuint id, G::EBufferType btype) noexcept;
    void			FreeBuffer (GLuint buf) noexcept;
    void			BufferSubData (GLuint buf, const void* data, GLuint size, GLuint offset = 0, G::EBufferType btype = G::ARRAY_BUFFER);
    void			BufferData (GLuint buf, const void* data, GLuint size, G::EBufferHint mode = G::STATIC_DRAW, G::EBufferType btype = G::ARRAY_BUFFER);
				// Shader
    GLuint			LoadShader (const char* v, const char* tc, const char* te, const char* g, const char* f);
    GLuint			LoadShader (GLuint pak, const char* v, const char* tc, const char* te, const char* g, const char* f);
    inline GLuint		LoadShader (GLuint pak, const char* v, const char* tc, const char* te, const char* f) noexcept	{ return (LoadShader(pak,v,tc,te,nullptr,f)); }
    inline GLuint		LoadShader (GLuint pak, const char* v, const char* g, const char* f) noexcept	{ return (LoadShader(pak,v,nullptr,nullptr,g,f)); }
    inline GLuint		LoadShader (GLuint pak, const char* v, const char* f) noexcept	{ return (LoadShader(pak,v,nullptr,nullptr,nullptr,f)); }
    void			FreeShader (GLuint sh) noexcept;
    inline void			SetDefaultShader (void) noexcept	{ Shader (_pconn->DefaultShader()); }
    inline void			SetTextureShader (void) noexcept	{ Shader (_pconn->TextureShader()); }
    inline void			SetFontShader (void) noexcept		{ Shader (_pconn->FontShader()); }
    void			Shader (GLuint id) noexcept;
    void			Parameter (const char* name, GLuint buf, G::EType type = G::SHORT, GLuint size = 2, GLuint offset = 0, GLuint stride = 0) noexcept;
    void			Parameter (GLuint slot, GLuint buf, G::EType type = G::SHORT, GLuint size = 2, GLuint offset = 0, GLuint stride = 0) noexcept;
    void			Uniform4f (const char* varname, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const noexcept;
    inline void			Uniform4fv (const char* varname, const GLfloat* v) const noexcept	{ Uniform4f(varname,v[0],v[1],v[2],v[3]); }
    void			Uniform4iv (const char* varname, const GLint* v) const noexcept;
    void			UniformMatrix (const char* varname, const GLfloat* mat) const noexcept;
    void			UniformTexture (const char* varname, GLuint img, GLuint itex = 0) noexcept;
    void			Color (GLuint c) noexcept;
    inline void			Color (GLubyte r, GLubyte g, GLubyte b, GLubyte a =255)	{ Color (RGBA(r,g,b,a)); }
    void			Clear (GLuint c) noexcept;
    void			Viewport (GLint x, GLint y, GLsizei w, GLsizei h) noexcept;
    void			Offset (GLint x, GLint y) noexcept;
    void			Enable (G::EFeature f, uint16_t o) noexcept;
				//{{{ DrawArrays and friends, inlined
    void			DrawCmdInit (void) noexcept;
    void			DrawArrays (G::EShape mode, GLuint first, GLuint count) noexcept {
				    DTRACE ("[%x] DrawArrays %s: %u vertices from %u\n", IId(), G::ShapeName(mode), count, first);
				    glDrawArrays (mode,first,count);
				}
    void			DrawArraysIndirect (G::EShape type, uint32_t offset = 0) noexcept {
				    DTRACE ("[%x] DrawArraysIndirect %s: offset %u\n", IId(), G::ShapeName(type), offset);
				    glDrawArraysIndirect (type, BufferOffset(offset));
				}
    void			DrawArraysInstanced (G::EShape type, uint32_t start, uint32_t sz, uint32_t nInstances, uint32_t baseInstance = 0) noexcept {
				    DTRACE ("[%x] DrawArraysInstanced %s: %u vertices from %u, %u instances, base %u\n", IId(), G::ShapeName(type), sz, start, nInstances, baseInstance);
				    if (baseInstance)
					glDrawArraysInstancedBaseInstance (type, start, sz, nInstances, baseInstance);
				    else
					glDrawArraysInstanced (type, start, sz, nInstances);
				}
    void			DrawElements (G::EShape type, uint16_t n, G::EType itype = G::UNSIGNED_SHORT, uint32_t offset = 0, uint32_t baseVertex = 0) noexcept {
				    DTRACE ("[%x] DrawElements %s: %u indexes from %u, basevertex %u, type %s\n", IId(), G::ShapeName(type), n, offset, baseVertex, G::TypeName(itype));
				    if (baseVertex)
					glDrawElementsBaseVertex (type, n, itype, BufferOffset(offset), baseVertex);
				    else
					glDrawElements (type, n, itype, BufferOffset(offset));
				}
    void			DrawElementsIndirect (G::EShape type, G::EType itype = G::UNSIGNED_SHORT, uint16_t offset = 0) noexcept {
				    DTRACE ("[%x] DrawElementsIndirect %s: offset %u\n", IId(), G::ShapeName(type), offset);
				    glDrawElementsIndirect (type, itype, BufferOffset(offset));
				}
    void			DrawElementsInstanced (G::EShape type, uint16_t n, uint32_t nInstances, G::EType itype = G::UNSIGNED_SHORT, uint32_t offset = 0, uint32_t baseVertex = 0, uint32_t baseInstance = 0) noexcept {
				    DTRACE ("[%x] DrawElementsInstanced %s: %u indexes from %u, basevertex %u, %u instances, base %u\n", IId(), G::ShapeName(type), n, offset, baseVertex, nInstances, baseInstance);
				    if (baseVertex && baseInstance)
					glDrawElementsInstancedBaseVertexBaseInstance (type, n, itype, BufferOffset(offset), nInstances, baseVertex, baseInstance);
				    else if (baseVertex)
					glDrawElementsInstancedBaseVertex (type, n, itype, BufferOffset(offset), nInstances, baseVertex);
				    else if (baseInstance)
					glDrawElementsInstancedBaseInstance (type, n, itype, BufferOffset(offset), nInstances, baseInstance);
				    else
					glDrawElementsInstanced (type, n, itype, BufferOffset(offset), nInstances);
				}
    void			DrawRangeElements (G::EShape type, uint16_t minel, uint16_t maxel, uint16_t n, G::EType itype = G::UNSIGNED_SHORT, uint32_t offset = 0, uint32_t baseVertex = 0) noexcept {
				    DTRACE ("[%x] DrawRangeElements %s: %u vertices from %u, basevertex %u, range %hu-%hu, type %s\n", IId(), G::ShapeName(type), n, offset, baseVertex, minel, maxel, G::TypeName(itype));
				    if (baseVertex)
					glDrawRangeElementsBaseVertex (type, minel, maxel, n, itype, BufferOffset(offset), baseVertex);
				    else
					glDrawRangeElements (type, minel, maxel, n, itype, BufferOffset(offset));
				}
				//}}}
				// Texture
    GLuint			LoadTexture (const GLubyte* d, GLuint dsz);
    GLuint			LoadTexture (const char* filename);
    void			FreeTexture (GLuint id);
    const CTexture*		Texture (GLuint id) const;
    void			Sprite (coord_t x, coord_t y, GLuint id);
    void			Sprite (coord_t x, coord_t y, GLuint id, coord_t sx, coord_t sy, dim_t sw, dim_t sh);
				// Font
    GLuint			LoadFont (const char* filename);
    GLuint			LoadFont (GLuint pak, const char* filename);
    GLuint			LoadFont (const GLubyte* p, GLuint psz);
    void			FreeFont (GLuint id);
    const CFont*		Font (GLuint id) const noexcept;
    void			Text (coord_t x, coord_t y, const char* s);
private:
    static inline const void*	BufferOffset (unsigned o)	{ return ((const void*)(uintptr_t(o))); }
    static void			ShaderUnpack (const GLubyte* s, GLuint ssz, const char* shs[5]) noexcept;
				// Queries
    inline void			PostQuery (GLuint q);
    inline bool			QueryResultAvailable (GLuint q) const;
private:
    CContext			_ctx;
    vector<CBuffer>		_buffer;
    vector<CShader>		_shader;
    vector<CTexture>		_texture;
    vector<CFont>		_font;
    vector<CDatapak>		_pak;
    vector<GLubyte>		_pendingFrame;
    CIConn*			_pconn;
    matrix4f_t			_proj;
    GLuint			_color;
    GLuint			_query[NStdQueries];
    GLuint			_vao[2];
    CEvent			_syncEvent;
    uint64_t			_nextVSync;
    GLuint			_curShader;
    GLuint			_curBuffer;
    GLuint			_curTexture;
    GLuint			_curFont;
    SWinInfo			_winfo;
    struct { coord_t x,y,w,h; }	_viewport;
};

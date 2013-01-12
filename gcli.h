// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "config.h"
#include "gleri.h"
#include "goshad.h"
#include "gotex.h"
#include "gofont.h"
#include "gopak.h"

class CGLClient : public PRGLR {
private:
    struct SIdMap {
	union {
	    struct {
		GLuint		_sid;
		uint32_t	_cid;
	    };
	    uint64_t	_key;
	};
	inline		SIdMap (uint32_t c, uint32_t s)		:_sid(s),_cid(c) {}
	inline bool	operator< (const SIdMap& v) const	{ return (_key < v._key); }
	inline bool	operator== (const SIdMap& v) const	{ return (_key == v._key); }
    };
    typedef float		matrix4f_t[4][4];
public:
				CGLClient (iid_t iid, Window win, GLXContext ctx);
    void			Init (void);
    inline const CContext&	Context (void) const		{ return (_ctx); }
    inline GLXContext		ContextId (void) const		{ return (_ctx.Context()); }
    inline Window		Drawable (void) const		{ return (_ctx.Drawable()); }
    void			Resize (uint16_t w, uint16_t h) noexcept;
    void			MapId (uint32_t cid, GLuint sid) noexcept;
    GLuint			LookupId (uint32_t cid) const noexcept;
    uint32_t			LookupSid (GLuint sid) const noexcept;
    void			UnmapId (uint32_t cid) noexcept;
				// State variables
    inline const float*		Proj (void) const	{ return (&_proj[0][0]); }
    inline GLuint		Color (void) const	{ return (_color); }
    inline void			SetColor (GLuint c)	{ _color = c; }
    inline GLuint		Shader (void) const	{ return (_curShader); }
    inline void			SetShader (GLuint s)	{ _curShader = s; }
    inline GLuint		Buffer (void) const	{ return (_curBuffer); }
    inline void			SetBuffer (GLuint b)	{ _curBuffer = b; }
    inline GLuint		Texture (void) const	{ return (_curTexture); }
    inline void			SetTexture (GLuint t)	{ _curTexture = t; }
    inline GLuint		Font (void) const	{ return (_curFont); }
    inline void			SetFont (GLuint f)	{ _curFont = f; }
				// Resource loader by enum
    GLuint			LoadResource (G::EResource dtype, G::EBufferHint hint, const GLubyte* d, GLuint dsz);
    void			FreeResource (G::EResource dtype, GLuint id);
				// Datapak
    GLuint			LoadDatapak (const char* filename);
    GLuint			LoadDatapak (const GLubyte* p, GLuint psz);
    void			FreeDatapak (GLuint id);
    const CDatapak*		Datapak (GLuint id) const;
				// Buffer
    GLuint			CreateBuffer (void) noexcept;
    void			BindBuffer (GLuint id) noexcept;
    void			FreeBuffer (GLuint buf) noexcept;
    void			BufferSubData (GLuint buf, const void* data, GLuint size, GLuint offset = 0, GLushort btype = GL_ARRAY_BUFFER);
    void			BufferData (GLuint buf, const void* data, GLuint size, GLushort mode = GL_STATIC_DRAW, GLushort btype = GL_ARRAY_BUFFER);
				// Shader
    GLuint			LoadShader (const char* v, const char* tc, const char* te, const char* g, const char* f) noexcept;
    GLuint			LoadShader (GLuint pak, const char* v, const char* tc, const char* te, const char* g, const char* f) noexcept;
    inline GLuint		LoadShader (GLuint pak, const char* v, const char* tc, const char* te, const char* f) noexcept	{ return (LoadShader(pak,v,tc,te,nullptr,f)); }
    inline GLuint		LoadShader (GLuint pak, const char* v, const char* g, const char* f) noexcept	{ return (LoadShader(pak,v,nullptr,nullptr,g,f)); }
    inline GLuint		LoadShader (GLuint pak, const char* v, const char* f) noexcept	{ return (LoadShader(pak,v,nullptr,nullptr,nullptr,f)); }
    void			FreeShader (GLuint sh) noexcept;
    inline void			SetDefaultShader (void) noexcept	{ assert (s_RootClient); Shader (s_RootClient->DefaultShader()); }
    inline void			SetTextureShader (void) noexcept	{ assert (s_RootClient); Shader (s_RootClient->TextureShader()); }
    inline void			SetFontShader (void) noexcept		{ assert (s_RootClient); Shader (s_RootClient->FontShader()); }
    void			Shader (GLuint id) noexcept;
    void			Parameter (const char* name, GLuint buf, GLenum type = GL_SHORT, GLuint size = 2, GLuint offset = 0, GLuint stride = 0) noexcept;
    void			Parameter (GLuint slot, GLuint buf, GLenum type = GL_SHORT, GLuint size = 2, GLuint offset = 0, GLuint stride = 0) noexcept;
    void			Uniform4f (const char* varname, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const noexcept;
    void			UniformMatrix (const char* varname, const GLfloat* mat) const noexcept;
    void			UniformTexture (const char* varname, GLuint img, GLuint itex = 0) noexcept;
    void			Color (GLuint c) noexcept;
    inline void			Color (GLubyte r, GLubyte g, GLubyte b, GLubyte a =255)	{ Color (RGBA(r,g,b,a)); }
    void			Clear (GLuint c) noexcept;
    void			Primitive (GLenum mode, GLuint first, GLuint count) noexcept;
				// Texture
    GLuint			LoadTexture (const GLubyte* d, GLuint dsz);
    GLuint			LoadTexture (const char* filename);
    void			FreeTexture (GLuint id);
    const CTexture*		Texture (GLuint id) const;
    void			Sprite (short x, short y, GLuint id);
				// Font
    GLuint			LoadFont (const char* filename);
    GLuint			LoadFont (GLuint pak, const char* filename);
    GLuint			LoadFont (const GLubyte* p, GLuint psz);
    void			FreeFont (GLuint id);
    const CFont*		Font (GLuint id) const noexcept;
    void			Text (int16_t x, int16_t y, const char* s);
private:
				// Shared resources
    inline GLuint		DefaultShader (void) const	{ assert (s_RootClient == this && _shader.size() > 0); return (_shader[0].Id()); }
    inline GLuint		TextureShader (void) const	{ assert (s_RootClient == this && _shader.size() > 1); return (_shader[1].Id()); }
    inline GLuint		FontShader (void) const		{ assert (s_RootClient == this && _shader.size() > 2); return (_shader[2].Id()); }
    inline const CFont*		DefaultFont (void) const	{ assert (s_RootClient == this && _font.size() > 0);   return (&_font[0]); }
private:
    CContext			_ctx;
    set<SIdMap>			_cidmap;
    vector<CShader>		_shader;
    vector<CTexture>		_texture;
    vector<CFont>		_font;
    vector<CDatapak>		_pak;
    matrix4f_t			_proj;
    GLuint			_color;
    GLuint			_curShader;
    GLuint			_curBuffer;
    GLuint			_curTexture;
    GLuint			_curFont;
    uint16_t			_w;
    uint16_t			_h;
    static const CGLClient*	s_RootClient;
};

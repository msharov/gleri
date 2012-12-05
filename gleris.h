#pragma once
#include "gleri.h"
#include "gob.h"

//----------------------------------------------------------------------

class CGleris : public CApp {
			CGleris (void) noexcept;
public:
    static CGleris&	Instance (void) { static CGleris app; return (app); }
    virtual		~CGleris (void) noexcept;
    void		Init (argc_t argc, argv_t argv);
protected:
    enum EOption {
	opt_SingleClient,
	opt_Last
    };
    enum : GLshort { TEXCOORD_ONE = (1<<14) };
    typedef float	matrix4f_t[4][4];
public:
    class CClient : public PRGLR {
    private:
	struct SIdMap {
	    union {
		struct {
		    GLuint	_sid;
		    uint32_t	_cid;
		};
		uint64_t	_key;
	    };
	    inline		SIdMap (uint32_t c, uint32_t s)		:_sid(s),_cid(c) {}
	    inline bool	operator< (const SIdMap& v) const	{ return (_key < v._key); }
	    inline bool	operator== (const SIdMap& v) const	{ return (_key == v._key); }
	};
    public:
	inline explicit		CClient (int fd, iid_t iid, Window win, GLXContext ctx) : PRGLR(fd,iid),_ctx(ctx,win),_cidmap(),_w(0),_h(0) {}
	inline const CContext&	Context (void) const				{ return (_ctx); }
	inline GLXContext	ContextId (void) const				{ return (_ctx.Context()); }
	inline Window		Drawable (void) const				{ return (_ctx.Drawable()); }
	void			Resize (uint16_t w, uint16_t h) noexcept;
	void			MapId (uint32_t cid, GLuint sid) noexcept;
	GLuint			LookupId (uint32_t cid) const noexcept;
	uint32_t		LookupSid (GLuint sid) const noexcept;
	void			UnmapId (uint32_t cid) noexcept;
	template <typename T>
	inline void		RemoveIdsFrom (vector<T>& v);
    private:
	CContext	_ctx;
	set<SIdMap>	_cidmap;
	uint16_t	_w;
	uint16_t	_h;
    };
public:
    inline bool		Option (EOption o) const	{ return (_options & (1<<o)); }
    void		OnResize (unsigned w, unsigned h);
			// Client id translation
    CClient*		ClientRecord (int fd, CClient::iid_t iid) noexcept;
    CClient*		ClientRecordForWindow (Window w) noexcept;
    void		CreateClient (int fd, CClient::iid_t iid, uint16_t w, uint16_t h, uint16_t glversion, bool hidden = false);
    void		ClientDraw (CClient& cli, bstri& cmdis);
			// Output interface
    inline void		Primitive (GLenum mode, GLuint first, GLuint count) const noexcept	{ glDrawArrays(mode,first,count); }
			// Shader
    GLuint		LoadShader (GLuint pak, const char* v, const char* tc, const char* te, const char* g, const char* f) noexcept;
    inline GLuint	LoadShader (GLuint pak, const char* v, const char* tc, const char* te, const char* f) noexcept	{ return (LoadShader(pak,v,tc,te,nullptr,f)); }
    inline GLuint	LoadShader (GLuint pak, const char* v, const char* g, const char* f) noexcept	{ return (LoadShader(pak,v,nullptr,nullptr,g,f)); }
    inline GLuint	LoadShader (GLuint pak, const char* v, const char* f) noexcept	{ return (LoadShader(pak,v,nullptr,nullptr,nullptr,f)); }
    void		FreeShader (GLuint sh) noexcept;
    inline void		DefaultShader (void) noexcept	{ Shader (_shader[0].Id()); }
    void		Shader (GLuint id) noexcept;
    void		Parameter (const char* name, GLuint buf, GLenum type = GL_SHORT, GLuint size = 2, GLuint offset = 0, GLuint stride = 0) noexcept;
    void		Parameter (GLuint slot, GLuint buf, GLenum type = GL_SHORT, GLuint size = 2, GLuint offset = 0, GLuint stride = 0) noexcept;
    void		Uniform4f (const char* varname, GLfloat x, GLfloat y, GLfloat z, GLfloat w) const noexcept;
    void		UniformMatrix (const char* varname, const GLfloat* mat) const noexcept;
    void		UniformTexture (const char* varname, GLuint img, GLuint itex = 0) noexcept;
    void		Color (GLuint c) noexcept;
    inline void		Color (GLubyte r, GLubyte g, GLubyte b, GLubyte a =255)	{ Color (RGBA(r,g,b,a)); }
    void		Clear (GLuint c) noexcept;
			// Buffer
    GLuint		CreateBuffer (void) noexcept;
    void		BindBuffer (GLuint id) noexcept;
    void		FreeBuffer (GLuint buf) noexcept;
    void		BufferSubData (GLuint buf, const void* data, GLuint size, GLuint offset = 0, GLushort btype = GL_ARRAY_BUFFER);
    void		BufferData (GLuint buf, const void* data, GLuint size, GLushort mode = GL_STATIC_DRAW, GLushort btype = GL_ARRAY_BUFFER);
			// Datapak
    GLuint		LoadDatapak (const char* filename);
    GLuint		LoadDatapak (const GLubyte* p, GLuint psz);
    void		FreeDatapak (GLuint id);
    const CDatapak*	Datapak (GLuint id) const;
			// Texture
    GLuint		LoadTexture (const char* filename);
    void		FreeTexture (GLuint id);
    const CTexture*	Texture (GLuint id) const;
    void		Sprite (short x, short y, GLuint id);
			// Font
    GLuint		LoadFont (const char* filename);
    GLuint		LoadFont (GLuint pak, const char* filename);
    GLuint		LoadFont (const GLubyte* p, GLuint psz);
    void		FreeFont (GLuint id);
    void		SetFont (GLuint id);
    void		Text (int16_t x, int16_t y, const char* s);
private:
    inline void		OnArgs (argc_t argc, argv_t argv) noexcept;
    void		CheckForXlibErrors (void) const;
    Window		CreateWindow (unsigned w, unsigned h) const;
    inline void		ActivateClient (const CClient& rcli) noexcept;
    void		DestroyClient (CClient& rcli) noexcept;
    inline void		SetOption (EOption o)	{ _options |= (1<<o); }
    void		OnXEvent (void);
    virtual void	OnFd (int fd);
    virtual void	OnFdError (int fd);
    static int		XlibErrorHandler (Display* dpy, XErrorEvent* ee) noexcept;
    static int		XlibIOErrorHandler (Display*) noexcept NORETURN;
private:
    GLXFBConfig		_fbconfig;
    matrix4f_t		_proj;
    GLuint		_color;
    GLuint		_curShader;
    GLuint		_curBuffer;
    GLuint		_curTexture;
    GLXContext		_curContext;
    const CFont*	_curFont;
    CClient*		_curCli;
    vector<CTexture>	_texture;
    vector<CFont>	_font;
    vector<CShader>	_shader;
    vector<CDatapak>	_pak;
    vector<CClient>	_cli;
    CCmdBuf		_icbuf;
    Display*		_dpy;
    XVisualInfo*	_visinfo;
    Colormap		_colormap;
    XID			_screen;
    Window		_rootWindow;
    unsigned short	_glversion;
    uint8_t		_options;
    static char*	_xlib_error;
};

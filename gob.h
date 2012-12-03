#pragma once
#include "config.h"
#include <GL/gl.h>
#include <GL/glx.h>
#include <limits.h>

//----------------------------------------------------------------------

class CMMFile {
public:
			CMMFile (void)			:_fd(-1),_sz(0),_p(nullptr) {}
    inline explicit	CMMFile (const char* filename)	{ Open (filename); }
    inline		~CMMFile (void)			{ Close(); }
    void		Open (const char* filename);
    void		Close (void);
  inline const GLubyte*	Data (void) const		{ return (_p); }
    inline GLuint	Size (void) const		{ return (_sz); }
private:
    int			_fd;
    GLuint		_sz;
    GLubyte*		_p;
};

GLubyte* DecompressBlock (const GLubyte* p, GLuint isz, GLuint& osz);

//----------------------------------------------------------------------

class CGObject {
public:
    enum : GLuint { NoObject = UINT_MAX };
public:
    inline		CGObject (GLXContext ctx, GLuint id)	:_ctx(ctx),_id(id) {}
    inline explicit	CGObject (CGObject&& v)			:_ctx(v._ctx),_id(v._id) { v._ctx = nullptr; v._id = NoObject; }
    inline CGObject&	operator= (CGObject&& v)		{ swap(_ctx, v._ctx); swap(_id,v._id); return (*this); }
    inline GLXContext	Context (void) const			{ return (_ctx); }
    inline GLuint	Id (void) const				{ return (_id); }
private:
    GLXContext		_ctx;
    GLuint		_id;
};

//----------------------------------------------------------------------

class CContext : public CGObject {
public:
    inline		CContext (GLXContext ctx, Window win)	: CGObject(ctx, win) {}
    Window		Drawable (void) const			{ return (Id()); }
};

//----------------------------------------------------------------------

class CDatapak : public CGObject {
public:
			CDatapak (GLXContext ctx, GLubyte* p, GLuint psz) noexcept;
    inline explicit	CDatapak (CDatapak&& v)	: CGObject(move(v)), _sz(v._sz), _p(v._p) { v._sz = 0; v._p = nullptr; }
    inline CDatapak&	operator= (CDatapak&& v){ CGObject::operator= (move(v)); swap(_sz,v._sz); swap(_p,v._p); return (*this); }
			~CDatapak (void) noexcept;
    const GLubyte*	File (const char* filename, GLuint& sz) const noexcept;
    inline GLuint	Size (void) const	{ return (_sz); }
private:
    inline GLuint	GenId (void) const	{ GLuint id; glGenBuffers (1, &id); return (id); }
private:
    GLuint		_sz;
    GLubyte*		_p;
};

//----------------------------------------------------------------------

class CShader : public CGObject {
public:
    //{{{ Source pointer aggregators
    class Sources {
    public:
	enum EStage {
	    shader_Vertex,
	    shader_TesselationControl,
	    shader_TesselationEvaluation,
	    shader_Geometry,
	    shader_Fragment,
	    shader_NStages
	};
    private:
	union { struct {
	const char*	_vertex;
	const char*	_tcontrol;
	const char*	_teval;
	const char*	_geometry;
	const char*	_fragment;
	};
	const char*	_stage [shader_NStages];
	};
	union { struct {
	GLint		_vertexSize;
	GLint		_tcontrolSize;
	GLint		_tevalSize;
	GLint		_geometrySize;
	GLint		_fragmentSize;
	};
	GLint		_stageSize [shader_NStages];
	};
	static const GLushort c_ShaderType [shader_NStages];
    private:
	void		LoadFromPak (const CDatapak& pak);
    public:
			// Initializers for all valid combinations of shaders
	inline		Sources (const char* v, const char* f) :_vertex(v),_tcontrol(nullptr),_teval(nullptr),_geometry(nullptr),_fragment(f) { fill_n (_stageSize, shader_NStages, NoObject); }
	inline		Sources (const char* v, const char* g, const char* f) :_vertex(v),_tcontrol(nullptr),_teval(nullptr),_geometry(g),_fragment(f) { fill_n (_stageSize, shader_NStages, NoObject); }
	inline		Sources (const char* v, const char* tc, const char* te, const char* f) :_vertex(v),_tcontrol(tc),_teval(te),_geometry(nullptr),_fragment(f) { fill_n (_stageSize, shader_NStages, NoObject); }
	inline		Sources (const char* v, const char* tc, const char* te, const char* g, const char* f) :_vertex(v),_tcontrol(tc),_teval(te),_geometry(g),_fragment(f) { fill_n (_stageSize, shader_NStages, NoObject); }
			// Same, but loading from a pak, with parameters being filenames
	inline		Sources (const CDatapak& pak, const char* v, const char* f) :_vertex(v),_tcontrol(nullptr),_teval(nullptr),_geometry(nullptr),_fragment(f) { LoadFromPak (pak); }
	inline		Sources (const CDatapak& pak, const char* v, const char* g, const char* f) :_vertex(v),_tcontrol(nullptr),_teval(nullptr),_geometry(g),_fragment(f) { LoadFromPak (pak); }
	inline		Sources (const CDatapak& pak, const char* v, const char* tc, const char* te, const char* f) :_vertex(v),_tcontrol(tc),_teval(te),_geometry(nullptr),_fragment(f) { LoadFromPak (pak); }
	inline		Sources (const CDatapak& pak, const char* v, const char* tc, const char* te, const char* g, const char* f) :_vertex(v),_tcontrol(tc),_teval(te),_geometry(g),_fragment(f) { LoadFromPak (pak); }
	inline bool	HaveStage (GLuint s) const	{ return (_stage[s]); }
	inline GLushort	ShaderType (GLuint s) const	{ return (c_ShaderType[s]); }
	inline void	ShaderSource (GLuint id, GLuint s) const noexcept;
    };
    //}}}
public:
    inline		CShader (GLXContext ctx, const Sources& src)
			    : CGObject(ctx,glCreateProgram())	{ Load(src); }
    inline		CShader (CShader&& v)			: CGObject(move(v)) {}
    inline CShader&	operator= (CShader&& v)			{ CGObject::operator= (move(v)); return (*this); }
			~CShader (void) noexcept;
private:
    void		Load (const Sources& src);
};

//----------------------------------------------------------------------

class CTexture : public CGObject {
public:
			CTexture (GLXContext ctx, const GLubyte* p, GLuint psz) noexcept;
    inline explicit	CTexture (CTexture&& v)	: CGObject(move(v)),_width(v._width),_height(v._height) {}
			~CTexture (void) noexcept;
    inline CTexture&	operator= (CTexture&& v)	{ CGObject::operator= (move(v)); _width = v._width; _height = v._height; return (*this); }
    inline GLushort	Width (void) const	{ return (_width); }
    inline GLushort	Height (void) const	{ return (_height); }
private:
    inline GLuint	GenId (void) const	{ GLuint id; glGenTextures (1, &id); return (id); }
    inline GLubyte*	LoadPNG (const GLubyte* p, GLuint psz) noexcept;
private:
    GLushort		_width;
    GLushort		_height;
};

//----------------------------------------------------------------------

class CFont : public CGObject {
public:
			CFont (GLXContext ctx, const GLubyte* p, GLuint psz) noexcept;
    inline explicit	CFont (CFont&& v)		: CGObject(move(v)),_width(v._width),_height(v._height),_rowwidth(v._rowwidth) {}
			~CFont (void) noexcept;
    inline CFont&	operator= (CFont&& v)		{ CGObject::operator= (move(v)); _width = v._width; _height = v._height; _rowwidth = v._rowwidth; return (*this); }
    inline GLubyte	Width (void) const		{ return (_width); }
    inline GLubyte	Height (void) const		{ return (_height); }
    inline GLushort	LetterX (GLubyte c) const	{ return ((c%_rowwidth)*_width); }
    inline GLushort	LetterY (GLubyte c) const	{ return ((c/_rowwidth)*_height); }
private:
    inline GLuint	GenId (void) const		{ GLuint id; glGenTextures (1, &id); return (id); }
private:
    GLubyte		_width;
    GLubyte		_height;
    GLushort		_rowwidth;
};

//----------------------------------------------------------------------

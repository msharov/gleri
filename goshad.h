#pragma once
#include "gob.h"

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

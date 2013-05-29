// This file is part of the GLERI project
//
// Copyright (c) 2012 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "goshad.h"

//----------------------------------------------------------------------

/*static*/ const GLushort CShader::Sources::c_ShaderType [shader_NStages] = {
    GL_VERTEX_SHADER,
    GL_TESS_CONTROL_SHADER,
    GL_TESS_EVALUATION_SHADER,
    GL_GEOMETRY_SHADER,
    GL_FRAGMENT_SHADER
};

//----------------------------------------------------------------------

CShader::~CShader (void) noexcept
{
    if (Id() != NoObject)
	glDeleteProgram (Id());
}

inline void CShader::Sources::ShaderSource (GLuint id, GLuint idx) const noexcept
{
    glShaderSource (id, 1, const_cast<const char**>(&_stage[idx]), &_stageSize[idx]);
}

void CShader::Sources::LoadFromPak (const CDatapak& pak)
{
    GLuint sz;
    for (GLuint i = 0; i < shader_NStages; ++i) {
	if (!_stage[i]) continue;
	_stage[i] = (const char*) pak.File (_stage[i], sz);
	_stageSize[i] = sz;
    }
}

void CShader::Load (const Sources& src)
{
    GLint result = False;
    int infoLogLength = 0;

    GLuint stages [Sources::shader_NStages];
    for (GLuint i = 0; i < Sources::shader_NStages; ++i) {
	if (!src.HaveStage(i)) {
	    stages[i] = NoObject;
	    continue;
	}
	stages[i] = glCreateShader (src.ShaderType(i));
	src.ShaderSource (stages[i], i);
	glCompileShader (stages[i]);
	glGetShaderiv (stages[i], GL_COMPILE_STATUS, &result);
	if (!result) {
	    glGetShaderiv (stages[i], GL_INFO_LOG_LENGTH, &infoLogLength);
	    char infoLog [infoLogLength];
	    glGetShaderInfoLog (stages[i], infoLogLength, nullptr, infoLog);
	    throw XError ("shader error:\n%s", infoLog);
	}
    }

    for (GLuint i = 0; i < Sources::shader_NStages; ++i)
	if (stages[i] != NoObject)
	    glAttachShader (Id(), stages[i]);
    glLinkProgram (Id());

    glGetProgramiv (Id(), GL_LINK_STATUS, &result);
    if (!result) {
	glGetProgramiv (Id(), GL_INFO_LOG_LENGTH, &infoLogLength);
	char infoLog [infoLogLength];
	glGetProgramInfoLog (Id(), infoLogLength, nullptr, infoLog);
	throw XError ("shader error:\n%s", infoLog);
    }

    for (GLuint i = 0; i < Sources::shader_NStages; ++i)
	if (stages[i] != NoObject)
	    glDeleteShader (stages[i]);
}

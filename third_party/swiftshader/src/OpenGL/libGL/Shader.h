// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Shader.h: Defines the abstract Shader class and its concrete derived
// classes VertexShader and FragmentShader. Implements GL shader objects and
// related functionality.


#ifndef LIBGL_SHADER_H_
#define LIBGL_SHADER_H_

#include "ResourceManager.h"

#include "compiler/TranslatorASM.h"

#define _GDI32_
#include <windows.h>
#include <GL/GL.h>
#include <GL/glext.h>

#include <string>
#include <list>
#include <vector>

namespace glsl
{
	class OutputASM;
}

namespace gl
{

class Shader : public glsl::Shader
{
	friend class Program;

public:
	Shader(ResourceManager *manager, GLuint handle);

	virtual ~Shader();

	virtual GLenum getType() = 0;
	GLuint getName() const;

	void deleteSource();
	void setSource(GLsizei count, const char *const *string, const GLint *length);
	int getInfoLogLength() const;
	void getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog);
	int getSourceLength() const;
	void getSource(GLsizei bufSize, GLsizei *length, char *source);

	void compile();
	bool isCompiled();

	void addRef();
	void release();
	unsigned int getRefCount() const;
	bool isFlaggedForDeletion() const;
	void flagForDeletion();

	static void releaseCompiler();

protected:
	static bool compilerInitialized;
	TranslatorASM *createCompiler(GLenum shaderType);
	void clear();

	static bool compareVarying(const glsl::Varying &x, const glsl::Varying &y);

	char *mSource;
	std::string infoLog;

private:
	virtual void createShader() = 0;
	virtual void deleteShader() = 0;

	const GLuint mHandle;
	unsigned int mRefCount;     // Number of program objects this shader is attached to
	bool mDeleteStatus;         // Flag to indicate that the shader can be deleted when no longer in use

	ResourceManager *mResourceManager;
};

class VertexShader : public Shader
{
	friend class Program;

public:
	VertexShader(ResourceManager *manager, GLuint handle);

	~VertexShader();

	virtual GLenum getType();
	int getSemanticIndex(const std::string &attributeName);

	virtual sw::Shader *getShader() const;
	virtual sw::VertexShader *getVertexShader() const;

private:
	virtual void createShader();
	virtual void deleteShader();

	sw::VertexShader *vertexShader;
};

class FragmentShader : public Shader
{
public:
	FragmentShader(ResourceManager *manager, GLuint handle);

	~FragmentShader();

	virtual GLenum getType();

	virtual sw::Shader *getShader() const;
	virtual sw::PixelShader *getPixelShader() const;

private:
	virtual void createShader();
	virtual void deleteShader();

	sw::PixelShader *pixelShader;
};
}

#endif   // LIBGL_SHADER_H_

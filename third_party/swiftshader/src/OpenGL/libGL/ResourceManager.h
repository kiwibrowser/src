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

// ResourceManager.h : Defines the ResourceManager class, which tracks objects
// shared by multiple GL contexts.

#ifndef LIBGL_RESOURCEMANAGER_H_
#define LIBGL_RESOURCEMANAGER_H_

#include "common/NameSpace.hpp"

#define _GDI32_
#include <windows.h>
#include <GL/GL.h>
#include <GL/glext.h>

#include <map>

namespace gl
{
class Buffer;
class Shader;
class Program;
class Texture;
class Renderbuffer;

enum TextureType
{
	TEXTURE_2D,
	PROXY_TEXTURE_2D,
	TEXTURE_CUBE,

	TEXTURE_TYPE_COUNT,
	TEXTURE_UNKNOWN
};

class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();

	void addRef();
	void release();

	GLuint createBuffer();
	GLuint createShader(GLenum type);
	GLuint createProgram();
	GLuint createTexture();
	GLuint createRenderbuffer();

	void deleteBuffer(GLuint buffer);
	void deleteShader(GLuint shader);
	void deleteProgram(GLuint program);
	void deleteTexture(GLuint texture);
	void deleteRenderbuffer(GLuint renderbuffer);

	Buffer *getBuffer(GLuint handle);
	Shader *getShader(GLuint handle);
	Program *getProgram(GLuint handle);
	Texture *getTexture(GLuint handle);
	Renderbuffer *getRenderbuffer(GLuint handle);

	void setRenderbuffer(GLuint handle, Renderbuffer *renderbuffer);

	void checkBufferAllocation(unsigned int buffer);
	void checkTextureAllocation(GLuint texture, TextureType type);
	void checkRenderbufferAllocation(GLuint renderbuffer);

private:
	std::size_t mRefCount;

	typedef std::map<GLint, Buffer*> BufferMap;
	BufferMap mBufferMap;
	//NameSpace mBufferNameSpace;

	typedef std::map<GLint, Shader*> ShaderMap;
	ShaderMap mShaderMap;

	typedef std::map<GLint, Program*> ProgramMap;
	ProgramMap mProgramMap;
	//NameSpace mProgramShaderNameSpace;

	typedef std::map<GLint, Texture*> TextureMap;
	TextureMap mTextureMap;
	//NameSpace mTextureNameSpace;

	typedef std::map<GLint, Renderbuffer*> RenderbufferMap;
	RenderbufferMap mRenderbufferMap;
	//NameSpace mRenderbufferNameSpace;
};

}

#endif // LIBGL_RESOURCEMANAGER_H_
